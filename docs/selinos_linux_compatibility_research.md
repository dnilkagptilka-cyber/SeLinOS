# SeLinOS: исходная рамка Linux internal API и driver model

**Дата:** 13 августа 2026, GMT+3  
**Статус:** исследовательская спецификация, не реализованная совместимость

## 1. Непременная версия-специфичность

Цель «полная совместимость с Linux-драйверами» не может быть определена без конкретной версии Linux, конфигурации и архитектуры. Официальная документация Linux прямо указывает, что у Linux **нет стабильного binary kernel interface и нет стабильного kernel internal interface**. Внутренние имена функций, структуры и параметры могут изменяться; при изменении основное дерево драйверов обновляется совместно с ядром.[2]

> Следовательно, SeLinOS не может честно заявлять совместимость «со всеми драйверами Linux вообще». Корректная цель: воспроизведение **одного закреплённого Linux kernel-internal contract** для определённой версии, architecture, `.config` и выбранного набора driver subsystems; дальнейшие ветви получают отдельные compatibility profiles.

## 2. Рекомендованная первоначальная база

На 13 августа 2026 официальный сайт kernel.org перечисляет 6.18.44 как текущую ветвь longterm; вместе с этим активны 6.12.103, 6.6.151, 6.1.182 и более старые longterm branches.[1] Для нового исследования SeLinOS рекомендуется применить **Linux 6.18.44 x86_64** в качестве `SELINOS_LINUX_BASELINE_6_18_44`.

| Свойство | Зафиксированное решение |
|---|---|
| Базовая kernel линия | Linux 6.18.44 longterm |
| ISA | x86_64 / LP64 |
| Целевой компьютер в M0 | QEMU PC99, один CPU |
| Модель конфигурации | Явно закреплённый `selinos-linux-6.18-x86_64.config` |
| Модель модулей | Source-level compatibility сначала; binary `.ko` compatibility не объявляется до создания versioned symbol/relocation loader |
| Лицензионная граница | Не копировать реализацию Linux в SeLinOS; использовать документацию/API-contract и самостоятельно написанную реализацию. Любой перенос исходников драйверов требует отдельной проверки лицензионной совместимости и происхождения кода. |

Фиксация на LTS не делает API стабильным сама по себе, но останавливает дрейф target contract и создаёт возможность воспроизводимых compile/runtime test suites.

## 3. Три уровня совместимости, которые нельзя смешивать

| Уровень | Смысл | Начальная цель SeLinOS |
|---|---|---|
| UAPI | syscall ABI, ELF, glibc/POSIX, ioctl и userspace programs | Поздняя независимая подсистема `linux-uapi` |
| KAPI | Исходные headers, функции, macros и semantics, которыми пользуется драйвер | Главная цель первой driver-совместимости |
| KABI | Готовый `.ko` binary module, relocation, layout, exported symbol CRC и config-dependent binary contract | Не входит в первый driver milestone; нужен отдельный loader и exact target build metadata |

Linux подчёркивает различие между стабильным kernel-to-userspace interface и изменяемыми internal interfaces.[2] SeLinOS должен поддерживать обе оси независимо и не выдавать покрытие UAPI за совместимость Linux KAPI/KABI.

## 4. Минимальный профиль driver model v0

Первый проверяемый driver profile сознательно ограничен виртуальными и хорошо наблюдаемыми устройствами QEMU. Он должен включать следующее:

| Подсистема Linux 6.18 profile | SeLinOS owner-domain | Проверяемая функция |
|---|---|---|
| `struct device`, `struct driver`, bus/device registration | `deviced` | registration, probe/remove lifecycle, reference counting |
| PCI enumeration + BAR/resource model | `pcid` / `deviced` | обнаружение QEMU virtio/educational PCI устройства |
| IRQ/request_irq model | `irqd` | delivery только в domain драйвера по notification/endpoint |
| DMA mapping API | `dmad` | IOMMU-aware DMA grants, sync/mapping bookkeeping |
| memory allocation subset | `kmemd` | `kmalloc/kfree`, `kzalloc`, page/frame-backed allocation, GFP profile |
| workqueues/completion/wait queues | `workd` | sleep/wakeup without embedding kernel scheduler in driver domain |
| logging + error semantics | `printkd` | `pr_*`, `dev_*`, errno-compatible results |
| virtio transport | isolated `virtio-*d` | первый reference driver path |

Полноценные block, net, USB, DRM/GPU, Wi-Fi, audio, ACPI hotplug, eBPF, kernel TLS и filesystem drivers **не** входят в profile v0. Они будут отдельными closure targets с собственными tests.

## 5. Требование к структуре SeLinOS

Чтобы сохранить смысл микроядерности, каждому перенесённому драйверу не передаётся власть Linux ring-0. Вместо этого драйвер выполняется в отдельном seL4 VSpace/CSpace и видит портированную KAPI library plus capability-backed services. `deviced` выдаёт только конкретные MMIO/PIO/PCI resources; `irqd` — только notification конкретного IRQ; `dmad` — только scoped DMA buffers. Это несовместимо с неявным global authority типичного Linux driver, поэтому source compatibility сначала является **semantic compatibility profile**, а не буквальной идентичностью реализации.

## 6. Матрица приёмки

Нельзя объявить драйвер «совместимым» по одной успешной компиляции. Для каждого driver profile необходимы все критерии:

| Проверка | Условие успеха |
|---|---|
| Header/build | Эталонный driver source компилируется против `selinos-kapi-6.18` без изменения driver logic; допускаются manifest-defined transport adapters |
| Probe | driver получает device/resource, проходит registration и `probe()` |
| IRQ | реальное device event доставляется только выделенному driver domain |
| DMA | device видит только явно выделенный буфер; out-of-range DMA отклоняется/изоляционно не доступна |
| Remove/fault | driver domain можно остановить и перезапустить, не разрушая ядро/остальные сервисы |
| Functional I/O | эталонный I/O roundtrip успешен на QEMU device |
| Regression | trace сравнивается с заранее закреплённым reference behavior Linux 6.18.44 |

## 7. Нерешённые решения перед кодом

1. Утвердить, что `6.18.44` — baseline, либо пользователь выбирает другую конкретную Linux release/config.
2. Выбрать первый reference driver: QEMU `edu` device проще всего показывает MMIO, IRQ и DMA; virtio-blk полезнее для VFS, но значительно шире по transport API.
3. Определить отношение к Linux source code: заново написанная implementation и отдельно переносимые/адаптированные GPL-compatible drivers должны быть юридически и технически разделены.
4. Выбрать KAPI adaptation mode: механический compatibility headers + service shim или driver-specific source adaptation. Первый выгоднее для measurable compatibility, второй может быстрее привести одно устройство в работу.
5. Зафиксировать target `.config`, exported symbol profile и filesystem layout для будущей KABI phase.

## References

[1] [The Linux Kernel Archives — текущие release ветви](https://www.kernel.org/). Проверено 13 августа 2026: longterm 6.18.44, 6.12.103, 6.6.151, 6.1.182 и другие.

[2] [The Linux Kernel Driver Interface — Linux Kernel Documentation](https://www.kernel.org/doc/html/latest/process/stable-api-nonsense.html). Официальное объяснение отсутствия стабильного binary и source internal interface для Linux kernel drivers.

[3] [Linux Device Model — Linux Kernel Documentation](https://docs.kernel.org/driver-api/driver-model/overview.html).

[4] [Device drivers infrastructure — Linux Kernel Documentation](https://docs.kernel.org/driver-api/infrastructure.html).

[5] [DMA-API-HOWTO — Linux Kernel Documentation](https://docs.kernel.org/core-api/dma-api-howto.html).

[6] [Generic IRQ handling — Linux Kernel Documentation](https://docs.kernel.org/core-api/genericirq.html).

## Дополнение: QEMU edu acceptance contract

Официальная QEMU-спецификация определяет `edu` как учебное PCI-устройство, специально предназначенное для написания kernel drivers с I/O, IRQ и DMA. Устройство имеет PCI ID `1234:11e8`, BAR0 размером 1 MiB и по умолчанию ограничивает DMA 28 битами (256 MiB); QEMU требует, чтобы OS driver правильно установил `dma_mask`.[7]

| Фаза теста `edu` | Регистр/механизм | Критерий SeLinOS |
|---|---|---|
| PCI probe | `1234:11e8`, BAR0 1 MiB | `deviced` создаёт `struct pci_dev`-совместимое описание и выдаёт driver domain только BAR capability |
| MMIO liveness | offset `0x04` | запись `x` возвращает `~x`; только допустимые access size проходит shim |
| Async computation | `0x08`, `0x20` | completion/workqueue profile наблюдает завершение factorial операции |
| IRQ request/ack | status `0x24`, raise `0x60`, ack `0x64` | `request_irq`-profile доставляет interrupt изолированному driver domain; handler очищает cause через ack |
| DMA H2D | src `0x80`, dst `0x88=0x40000`, count `0x90`, cmd `0x98=1` | `dma_alloc_coherent` возвращает capability-backed `dma_addr_t`; test buffer попадает в device buffer |
| DMA D2H + IRQ | src `0x40000`, dst `dma+offset`, count, cmd `0x98=0x7` | data roundtrip и interrupt status `0x100`; driver ack, mapping освобождается |
| Fault isolation | driver crash/restart during active request | `dmad` отзываёт DMA grant, `irqd` маскирует IRQ, остальные domains продолжают работать |

### Минимальный coherent DMA shim

Linux DMA documentation уточняет, что `dma_addr_t` может быть выдан устройству как DMA source/target, но CPU не обязан иметь возможность разыменовывать его напрямую, поскольку CPU и DMA address spaces могут различаться. `dma_alloc_coherent()` возвращает CPU virtual address и DMA handle; соответствующий `dma_free_coherent()` должен получать тот же `dev`, size и handle.[8]

Отсюда профиль `selinos-kapi-6.18/dma-mapping.h` v0 обязан включить `dma_addr_t`, `dma_alloc_coherent`, `dma_free_coherent`, ограниченный `dma_set_mask_and_coherent`, а также lifecycle-таблицу `{driver_domain, device_cap, CPU frame capability, dma_addr, size, direction, active}` в `dmad`. Для QEMU edu v0 допустима только coherent allocation до 256 MiB; streaming mappings, scatter-gather и non-coherent callbacks — последующие шаги.

### Принятое ограничение реализации

QEMU documentation описывает **современную master документацию QEMU**, в то время как локально установлен QEMU 8.2.2 может отличаться от указанной версии 11.1.50. Поэтому SeLinOS должен сначала выполнить runtime detection PCI ID/BAR и проверку liveness register, а затем отмечать test как unsupported, если устройство отсутствует или contract отличается. Нельзя переносить claim совместимости из документации master на локальный binary без такой проверки.

[7] [QEMU EDU device specification](https://www.qemu.org/docs/master/specs/edu.html).

[8] [Dynamic DMA mapping using the generic device — Linux Kernel Documentation](https://docs.kernel.org/core-api/dma-api.html).

## Дополнение: device model и IRQ architecture

Linux device model унифицирует bus-specific frameworks вокруг общих `struct device` и `struct driver`; PCI-устройство включает generic `struct device` как поле, при этом bus layer знает детали общего и PCI object layout, а device-specific driver обычно не должен напрямую зависеть от этих внутренних полей.[9] В SeLinOS literal globally-accessible Linux object graph нельзя выдавать каждому driver domain: он заменяется owner-held metadata в `deviced` и ABI-visible shadow object в driver VSpace.

| Linux semantic object | SeLinOS domain owner | Представление в driver domain | Правило authority |
|---|---|---|---|
| `struct device` | `deviced` | KAPI shadow с immutable identity + capability token | Драйвер не может менять resource ownership |
| `struct pci_dev` | `pcid` + `deviced` | profile-limited PCI view (vendor/device/BAR/IRQ) | Только `pcid` выполняет config-space enumeration |
| `struct device_driver` | `kmodd` | registered callbacks and module manifest | `probe/remove` исполняется в driver domain, lifecycle управляется `deviced` |
| Device resource | `deviced` | MMIO mapping or PIO capability; no raw global physical address | Grant ограничен конкретным BAR/region |
| sysfs object | будущий `sysfsd` | read-only `kobject` projection для M0 | Не участвует в EDU functional path M0 |

Linux generic IRQ layer даёт driver-facing функции request, enable, disable и free IRQ, абстрагируя controller-specific distinctions.[10] В SeLinOS profile v0 `irqd` удерживает IRQControl/IRQHandler authority, создаёт per-device notification и передаёт драйверу только endpoint/notification cap. KAPI `request_irq()` регистрирует callback в domain-local dispatcher; `irqd` доставляет signal, dispatcher вызывает handler, а driver через MMIO ack завершает hardware-specific часть. `free_irq()` отзывает notification и synchronises domain-local dispatch before unregister.

> Такое отображение сохраняет наблюдаемую семантику probe/request_irq/handler/ack для выбранного профиля, но намеренно не воспроизводит shared ring-0 address space Linux. Поэтому статус должен называться **SeLinOS Linux 6.18 source KAPI profile**, а не universal KABI.

[9] [The Linux Kernel Device Model — Linux Kernel Documentation](https://docs.kernel.org/driver-api/driver-model/overview.html).

[10] [Linux generic IRQ handling — Linux Kernel Documentation](https://docs.kernel.org/core-api/genericirq.html).

## Дополнение: module loader и PCI profile

Linux module signing facility подписывает modules при установке и проверяет подпись при загрузке. В restrictive mode (`CONFIG_MODULE_SIG_FORCE`) unsigned или signed unknown key modules не загружаются; ключи представляются X.509 certificates, а документация перечисляет RSA, NIST P-384 ECDSA и ML-DSA как поддерживаемые public-key варианты.[11] SeLinOS переносит **policy**, а не формат Linux `.ko` в M0: `kmodd` принимает подписанный `selmod` manifest+ELF, проверяет embedded trust anchor и только после проверки создаёт driver domain. Linux `.ko` relocation/KABI format остаётся отдельной поздней фазой, поскольку он требует exact ABI version, symbol versions и relocations выбранного Linux build.

| Элемент | M0 решение SeLinOS | Отложено |
|---|---|---|
| Artifact | ELF `ET_DYN` plus signed `selmod` CBOR/manifest | Native Linux `.ko` parsing |
| Signing | X.509 trust anchor; `ed25519` profile сначала, PKCS#7-compatible parsing позднее | Exact Linux module-signature trailer compatibility |
| Loader | `kmodd` verifies hash/signature, creates VSpace/CSpace, requests grants | Relocations against Linux KABI global kernel symbols |
| Device binding | manifest PCI IDs + `deviced` policy | uevent/modalias/module autoload ecosystem |
| Unload | revoke IRQ/MMIO/DMA grants, stop TCB, reclaim resources | Linux RCU/module refcount quiescence equivalence |

Linux PCI documentation организует driver-facing material вокруг PCI Support Library, PCI hotplug и peer-to-peer DMA support.[12] QEMU edu profile M0 включает лишь PCI discovery, ID matching, BAR0 memory resource mapping, bus mastering policy, one IRQ, device enable/disable and DMA mask validation. Hotplug, SR-IOV, peer-to-peer DMA, AER, MSI-X и PCIe power management объявляются unsupported до отдельных profiles.

[11] [Kernel module signing facility — Linux Kernel Documentation](https://docs.kernel.org/admin-guide/module-signing.html).

[12] [The Linux PCI driver implementer’s API guide — Linux Kernel Documentation](https://docs.kernel.org/driver-api/pci/index.html).
