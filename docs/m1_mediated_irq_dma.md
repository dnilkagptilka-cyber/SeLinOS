# SeLinOS Driver-runtime M1: mediated IRQ и test-only coherent DMA для QEMU edu

**Статус:** проверенный QEMU TCG integration slice; это не завершённая реализация Linux `request_irq()` / `dma_alloc_coherent()` и не доказательство изоляции DMA на реальном оборудовании. Цель M1 — зафиксировать минимальный работающий capability flow для одного QEMU edu device без передачи driver domain глобального `IRQControl`, PCI configuration I/O или allocator authority.

QEMU edu имеет PCI ID `1234:11e8`, BAR0 объёмом 1 MiB и DMA protocol с source, destination, count и command регистрами. По умолчанию устройство принимает только 28-bit DMA адреса; его внутренний 4 KiB buffer доступен по device offset `0x40000`.[1] SeLinOS применяет это как ограниченный воспроизводимый тестовый профиль, а не как общий hardware-DMA ABI.

## Реализованный capability flow

| Компонент | Выданные права | Намеренно не выдаются |
|---|---|---|
| `rootd` | PCI config-space IOPort capability, `IRQControl`, root RAM-frame allocator | Ничего из этого не копируется в driver domain |
| `selinos-irqd` | Одна hardware notification, один scoped `IRQHandler`, completion endpoint и driver notification | `IRQControl`, PCI config I/O, BAR0 и DMA allocator |
| `selinos-edu-mmio-driver` | Только BAR0 frame capabilities, одна mapped RAM page, driver notification и completion endpoint | `IRQHandler`, PCI config I/O, `IRQControl`, произвольная RAM allocation authority |

Перед запуском driver domain `rootd` обнаруживает QEMU edu через собственный узко выданный IOPort capability и устанавливает в его PCI Command register биты **Memory Space** и **Bus Master**. Именно последняя операция делает функциональный bus-master DMA возможным, но она остаётся в trusted PCI path; самому драйверному домену PCI configuration space не доступен.

### Linux-shaped KAPI IRQ registration

Self-authored QEMU edu driver теперь вызывает стандартную Linux-shaped пару `request_irq()` / `free_irq()` из SeLinOS KAPI. Перед этим domain runtime binds only its three pre-granted transport capabilities through private `selinos_kapi_bind_irq()`. Вызов `request_irq()` выполняет synchronous `seL4_Call` в отдельный irqd request endpoint с фиксированным registration message и IRQ line; `selinos-irqd` validates request и возвращает accepted reply, после чего KAPI сохраняет handler и `dev_id`. При notification `selinos_kapi_dispatch_one_irq()` вызывает зарегистрированный handler, а после его device-level BAR acknowledgement отправляет completion в `selinos-irqd`. Таким образом, transport completion не остаётся в коде driver, а KAPI handler получает привычную форму `irq_handler_t(int, void *)`.

Это всё ещё **одна статическая binding на domain**, не general multi-device IRQ allocator: registration IPC существует, но привязан к единственному заранее выданному endpoint и не является динамическим irqd registry. `free_irq()` посылает distinct badged control notification, получает synchronous unregister reply от `selinos-irqd` и лишь затем снимает локальный handler registration; QEMU evidence подтверждает `driver IRQ registration released`. KAPI по-прежнему не поддерживает shared IRQ, mask/unmask semantics или capability revoke.

### Linux-shaped KAPI coherent DMA lease

Тот же self-authored driver вызывает `dma_set_mask_and_coherent()` с QEMU edu mask `0x0fffffff`, затем `dma_alloc_coherent()` для 256-byte sub-lease и получает CPU pointer вместе с `dma_addr_t`. Private runtime bind связывает эти значения с одним предварительно выданным `struct device`, одной CPU mapping и одним physical DMA address; API отклоняет иной device, mask шире allowed mask, второй concurrent allocation, нулевой/слишком большой request и несовпадающий `dma_free_coherent()` tuple. После 100-byte roundtrip driver вызывает `dma_free_coherent()`.

Это теперь отдельный, но **узкий `dmad` service**, а не dynamic allocator: `dma_free_coherent()` перед local state release выполняет synchronous IPC в scoped dmad endpoint, который подтверждает release единственной QEMU edu lease. Physical frame всё ещё создаёт `rootd`, mapping не отзывается в момент `dma_free_coherent()`, а private bind не является Linux KAPI. Проверка доказывает только корректную форму и data flow `dma_set_mask_and_coherent()` / `dma_alloc_coherent()` / `dma_free_coherent()` для одного pre-granted QEMU lease.

Self-authored QEMU driver также выполняет отрицательные policy checks: duplicate `request_irq()`, запрос mask шире granted mask, использование иного `struct device`, вторая concurrent DMA allocation и `dma_free_coherent()` с неверным size должны быть отклонены. Успешное прохождение выводит `KAPI guard checks passed`; это проверка данных guard conditions текущего single-lease runtime, а не доказательство общей Linux DMA semantics.

`rootd` retypes одну pinned normal-RAM страницу по guest physical address `0x02000000` (32 MiB). Адрес выровнен на page boundary, находится выше размещённого boot image и ниже QEMU edu 28-bit mask `0x0fffffff`. В driver domain появляется только копия capability этой одной frame, mapped на fixed virtual address `0x600000100000`; это explicit lease, а не право создавать новые DMA mappings.

> Для QEMU edu теста направление `RAM → EDU` программируется command `0x05`, а `EDU → RAM` — `0x07`: биты включают start, direction и completion interrupt. Документация QEMU определяет command bit `0x04` как запрос interrupt `0x100` после завершения DMA.[1]

После hardware interrupt `selinos-irqd` ожидает driver completion. Драйвер сначала очищает device interrupt status через BAR0 `0x64`, затем синхронно отправляет фиксированное completion сообщение. Лишь после этого `selinos-irqd` делает `seL4_IRQHandler_Ack()`. Следовательно, драйверный домен не владеет kernel IRQ acknowledgement capability, а unacknowledged/malformed completion не приводит к автоматическому повторному разрешению IRQ.

## Проверка

Проверка была выполнена с QEMU TCG и `-device edu`. QEMU log подтверждает BAR0 grant, 100-byte RAM-to-device-to-RAM roundtrip и mediated IRQ flow; точный вывод сохранён в [`../selinos_dma_irq_m1.log`](../selinos_dma_irq_m1.log).

| Проверка | Команда | Наблюдаемое доказательство |
|---|---|---|
| Полная сборка | `cd build && ninja` | Образ `images/selinos-root-image-x86_64-pc99` с `selinos-irqd` и обновлённым driver ELF |
| QEMU edu runtime | `timeout --signal=TERM 15s ./simulate -c max -o "" --extra-qemu-args='-device edu' --reset-terminal` | `dmad ... service online`; `irqd: driver IRQ registration accepted`; `KAPI request_irq and dma_alloc_coherent passed`; `KAPI guard checks passed`; MMIO liveness passed; `mediated 100-byte coherent DMA roundtrip passed`; `mediated IRQ ... passed`; `dmad ... lease release accepted`; `irqd: driver IRQ registration released` |
| Базовый boot без edu | `timeout --signal=TERM 10s ./simulate -c max -o "" --reset-terminal` | Device не обнаружен, семь изолированных service domains запущены; лог: [`../selinos_m1_baseline.log`](../selinos_m1_baseline.log) |
| KABI M1 regression | `./tests/kabi_module_parser_test …` и `python3 tools/verify_kabi_module.py …` | Fixture Linux 6.18.44 остаётся accepted; 0 CRC version mismatches |

## Безопасностная и compatibility граница

Этот тест не использует IOMMU. Фактический QEMU boot log сообщает `ACPI: 0 IOMMUs detected`; поэтому bus-master device технически способен обращаться к guest physical memory по допустимым ему адресам. Возможность назначить capability на CPU mapping **не** ограничивает DMA сама по себе. Платформенная документация seL4 указывает поддержку VT-d для PC99, но изоляция требует отдельного IOMMU policy/backend и platform discovery.[2] Формальные предположения seL4 также не следует интерпретировать как автоматическую защиту от DMA.[3]

Соответственно, в M1 корректно заявлять только **functional QEMU edu DMA через явную low-memory lease**, **single-binding KAPI `request_irq()` dispatch** и **single-lease KAPI coherent-DMA API**, подтверждённые self-authored driver. Нельзя заявлять strong DMA containment, dynamic coherent allocator, hot-unplug-safe revocation, general Linux IRQ/DMA semantics или поддержку произвольных Linux PCI drivers. Dynamic multi-device KAPI-to-irqd/dmad registry, mapping revocation и module-loader integration остаются отдельной следующей работой; реальный Linux `.ko` пока не получает этот runtime через module loader.

## Следующая работа

| Приоритет | Результат | Критерий готовности |
|---|---|---|
| `irqd` RPC | General multi-binding `request_irq()` / `free_irq()` protocol с ownership token, shared IRQ, mask/unmask и capability cleanup | KAPI driver получает delivery без raw IRQHandler cap, а `free_irq()` revokes transport authority |
| `dmad` RPC | Dynamic lease registry, `dma_set_mask_and_coherent()`, alloc/free, mapping revocation | Нельзя использовать DMA address после `free`; invalid device token отклоняется |
| VT-d backend | ACPI DMAR discovery и per-device IOVA allow-list | DMA от test device за пределы mapped IOVA вызывает fault/denial в проверяемой платформенной конфигурации |
| Device lifecycle | `pcid`/`deviced` probe, enable/master, remove and teardown | Remove прекращает IRQ, unmaps lease и делает stale capabilities unusable |

## References

[1]: https://www.qemu.org/docs/master/specs/edu.html "QEMU EDU device specification"
[2]: https://docs.sel4.systems/Hardware/IA32.html "seL4 PC99 platform documentation"
[3]: https://sel4.systems/Verification/assumptions.html "What the seL4 proofs assume"
