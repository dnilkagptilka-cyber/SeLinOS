# SeLinOS KAPI-device M2: mediated QEMU edu PCI probe

## Статус

Эта веха реализует **узкий, воспроизводимо проверяемый Linux-shaped PCI driver-model profile** для одного QEMU `edu` устройства. Она не добавляет Linux kernel, Linux VM или LKL: `seL4` остаётся единственным привилегированным ядром, а `deviced`, `irqd`, `dmad` и driver domain выполняются как отдельные user-space domains.

> Успешный `pci_register_driver()` здесь означает, что локальный KAPI shim вызвал callback `probe()` в VSpace драйвера после synchronous match в изолированном `deviced`. Он не означает наличия общего Linux kernel object graph или универсальной совместимости Linux driver model.

## Проверенный путь

| Шаг | Владелец authority | Наблюдаемый результат |
|---|---|---|
| PCI discovery и command policy | `rootd` с retained scoped IOPort capability | Находит QEMU edu `1234:11e8`, BAR0 1 MiB и INTx line; driver domain не получает config-space I/O. |
| Device registry | `selinos-deviced` | Принимает capability-backed synchronous запрос с PCI vendor/device, выполняет match и возвращает immutable resource projection. |
| Driver registration | KAPI shim в driver domain | Итерирует local `pci_device_id` table, вызывает `deviced`, создаёт local shadow `struct pci_dev` и вызывает local `probe()`. |
| Enable/master | KAPI shim | `pci_enable_device()` и `pci_set_master()` принимаются только внутри active/probing shadow-device lifecycle. |
| IRQ/DMA | `selinos-irqd` и `selinos-dmad` | Single-device runtime остаётся capability-scoped: driver не получает `IRQHandler`, `IRQControl` или allocator authority. `dma_free_coherent()` получает accepted reply только после CPU Page_Unmap и exact child-cap revoke/delete. |
| Remove | KAPI shim и `irqd` в driver domain/service boundary | `pci_unregister_driver()` вызывает local `remove()`; `free_irq()` получает accepted reply только после `irqd` CSpace revoke/delete точной driver notification capability и выключает shadow device. |

## IPC contract

`include/selinos_deviced_protocol.h` определяет contract только для QEMU edu profile. Rootd выдаёт driver domain единственную endpoint capability; получение этой capability позволяет запросить match, но не даёт raw access к PCI configuration space, BAR allocator или physical-memory allocator.

| Направление | Сообщение | Содержание |
|---|---|---|
| KAPI driver → `deviced` | `SELINOS_DEVICED_REGISTER_MAGIC` | `vendor`, `device` из текущей записи driver-local `pci_device_id`. |
| `deviced` → KAPI driver | `SELINOS_DEVICED_REGISTER_ACCEPTED` | Non-forgeable profile token, QEMU edu identity, IRQ line, BAR0 start и BAR0 length. |
| `deviced` → KAPI driver | `SELINOS_DEVICED_REGISTER_NO_MATCH` | Никакого ресурса, capability или object ownership не выдаётся. |

Reply используется только для построения driver-local shadow `pci_dev`; `probe()` не исполняется в address space `deviced`. Это необходимо, поскольку callback pointer принадлежит коду изолированного driver domain, а не server domain. KAPI передаёт returned QEMU edu token также в M2 `request_irq()`/`free_irq()` и `dma_free_coherent()` IPC; `irqd` и `dmad` отклоняют значение, отличное от current profile token. В M2 token root-issued при обнаружении QEMU edu и передаётся startup records только в `deviced`, `irqd` и `dmad`; KAPI получает его только в accepted reply. Он различает driver generation within a boot и служит linkage guard, но M2 всё ещё **не** dynamic token allocator или multi-device registry.

## Runtime evidence

Проверенный запуск QEMU с `-device edu` должен содержать следующую ordered causal chain:

```text
SeLinOS deviced: mediated QEMU edu PCI registry online.
SeLinOS deviced: QEMU edu PCI ID matched; driver-local probe authorised.
SeLinOS irqd: driver IRQ registration accepted.
SeLinOS edu driver: KAPI request_irq and dma_alloc_coherent passed.
SeLinOS edu driver: KAPI guard checks passed.
SeLinOS edu driver: device-token guard checks passed.
SeLinOS edu driver: BAR0 MMIO liveness passed.
SeLinOS edu driver: mediated 100-byte coherent DMA roundtrip passed.
SeLinOS edu driver: mediated IRQ raise/wait/device-ack passed.
SeLinOS dmad: DMA CPU mapping unmapped and child capability revoked.
SeLinOS KAPI 6.18: deviced matched QEMU edu; local probe completed.
SeLinOS irqd: driver notification revoked and registration released.
SeLinOS KAPI 6.18: deviced-driven QEMU edu remove completed.
```

Baseline без `-device edu` запускает тот же persistent `deviced` domain без endpoint/resource record и явно фиксирует dormant state. Полные hashes и marker set хранятся в [`tests/artifacts/selinos_driver_runtime_m1.verification.json`](../tests/artifacts/selinos_driver_runtime_m1.verification.json), а независимая проверка выполняется `tools/verify_driver_runtime_m1.py`.

## Текущий scope и ограничения

Веха доказывает source-KAPI behaviour только для self-authored QEMU edu fixture и конкретной последовательности `pci_register_driver()` → `probe()` → `pci_unregister_driver()`/`remove()`. Active negative coverage sends forged device-token requests directly to the driver-granted `irqd` and `dmad` endpoints; both return rejection while the subsequently valid lifecycle continues. Это доказывает token validation within one service generation, не simultaneous multi-device ownership. BAR frame, IRQ notification и DMA frame по-прежнему initial-granted из root bootstrap: M2 не передаёт их динамически от `deviced` и не создаёт general resource allocator. Однако для единственной DMA lease `dmad` теперь перед accepted reply выполняет `seL4_X86_Page_Unmap`, `seL4_CNode_Revoke` и `seL4_CNode_Delete` точного driver-domain frame capability; KAPI после этого отвергает reuse через `dma_alloc_coherent()` и `dma_set_mask_and_coherent()`.

| Реализовано и проверено | Явно не реализовано |
|---|---|
| Один QEMU edu PCI ID match; vendor/device wildcard match в server contract | Dynamic multi-device registry, hotplug, bus scanning ownership в `pcid`, MSI/MSI-X, SR-IOV, AER и PCIe power management |
| Local immutable `pci_dev` view: vendor, device, IRQ, BAR0 start/length, SeLinOS token | General Linux `struct device`/`kobject` graph, sysfs, reference counting и uevents |
| `pci_enable_device`, `pci_set_master`, `pci_disable_device` lifecycle gates для shadow object | Real per-driver PCI configuration writes after registration |
| Single mediated IRQ/DMA QEMU edu path, terminal DMA CPU unmap plus exact child-cap revoke/delete on `dma_free_coherent`, and notification-cap revoke/delete on `free_irq` | Dynamic multi-lease `dmad`, frame reuse/zeroisation, dynamic multi-device `irqd`, IOMMU/VT-d containment |
| Linux 6.18.44 module artifact parse/version verification remains green | ELF relocation execution, native `.ko` loader, general Linux KABI execution |

Следующий correctness-critical шаг — заменить root-static single-device grants dynamic token-indexed `deviced`/`irqd`/`dmad` ownership, добавить frame reuse/zeroisation, multi-device IRQ state and TCB-lifecycle cleanup при `remove()` или fault driver domain. Только после этого можно считать resource delivery and teardown general service-owned, а не QEMU fixture-specific.
