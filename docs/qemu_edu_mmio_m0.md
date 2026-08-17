# SeLinOS M0: изолированный QEMU edu MMIO driver path

**Статус:** успешно собран и проверен в QEMU 8.2.2/TCG 13 августа 2026.  
**Compatibility profile:** `SELINOS_LINUX_BASELINE_6_18_44`, x86_64/PC99.  
**Устройство:** QEMU `edu` (`-device edu`), PCI ID `1234:11e8`.[1]

## Проверенный маршрут

SeLinOS запускает единый seL4 root domain, который создаёт отдельные TCB/CSpace/VSpace для служебных доменов. Для `edu` root domain выполняет PCI config-space discovery через seL4 capability, выданную ровно на x86 I/O ports `0xcf8..0xcff`. После обнаружения `1234:11e8` root domain выделяет device frame capabilities только на страницы BAR0, копирует их в новый driver CSpace и маппит их в driver VSpace по адресу `0x600000000000`. PCI configuration control, IRQ, DMA и остальные устройства этому драйверу **не** передаются.

| Шаг | Результат | Доказательство |
|---|---|---|
| PCI discovery | Обнаружен `edu` PCI function | `SeLinOS M0: QEMU edu PCI function detected.` |
| Capability scope | В driver VSpace mapped только BAR0 1 MiB | `SeLinOS M0: BAR0 mapped only into edu driver domain.` |
| MMIO identity | Прочитан `0x010000ed`; low-byte marker `0xed` | Эталонный лiveness driver проверил device response |
| MMIO liveness | Запись `0x13579bdf` в offset `0x04` возвращает `~value` | `SeLinOS edu driver: BAR0 MMIO liveness passed.` |
| Scoped INTx IRQ | root выдаёт только notification/IRQHandler на IOAPIC pin EDU; driver raises, waits и подтверждает interrupt | `SeLinOS edu driver: scoped IRQ raise/wait/ack passed.` |
| Linux-shaped source profile | Отдельный KAPI probe компилируется и исполняется | `SeLinOS KAPI 6.18: edu driver compiles; deviced/irqd/dmad runtime is pending.` |

Полный runtime log находится в [`../selinos_edu_irq.log`](../selinos_edu_irq.log).

## Строгая граница утверждения

Этот результат означает **изолированный PCI/MMIO capability path к QEMU edu**, а не абсолютную Linux driver compatibility. Сам MMIO driver пока self-authored; отдельный KAPI probe использует самостоятельно написанные ограниченные headers `linux/pci.h`, `linux/interrupt.h`, `linux/dma-mapping.h`, `linux/device.h` и возвращает `-ENOSYS` для ещё не подключённых runtime services.

| Область Linux driver model | Статус M0 |
|---|---|
| `struct pci_dev`, PCI ID table и driver-shaped compilation | Частичный source profile; probe компилируется |
| PCI enumeration и BAR0 resource | Выполнено для QEMU edu на bus 0/function 0 profile |
| MMIO 32-bit liveness | Выполнено в отдельном driver domain |
| `request_irq`, INTx, hardware ack и scoped seL4 IRQHandler | Проверен self-authored driver path; Linux KAPI `request_irq()` runtime ещё не связан с `irqd` |
| `dma_alloc_coherent`, `dma_set_mask_and_coherent`, 28-bit DMA grant | KAPI declarations есть; runtime не реализован |
| `struct device` lifecycle/probe/remove | Declarations есть; `deviced` runtime не реализован |
| Linux source driver without adaptation | Не доказано |
| Linux `.ko` module binary loader/KABI | Не реализовано |

## Следующая проверяемая веха

Следующий минимальный end-to-end test должен связать уже проверенный notification/IRQHandler path с KAPI-shaped `request_irq()` через `irqd`. После этого `dmad` должен выделить coherent buffer с address mask 28 bits и выполнить documented 100-byte DMA roundtrip to/from device offset `0x40000`.[1] [2]

## References

[1] [QEMU EDU device specification](https://www.qemu.org/docs/master/specs/edu.html).

[2] [Dynamic DMA mapping using the generic device — Linux Kernel Documentation](https://docs.kernel.org/core-api/dma-api.html).
