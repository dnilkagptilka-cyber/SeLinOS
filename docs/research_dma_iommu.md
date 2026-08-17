# DMA и IOMMU: проектные выводы SeLinOS M1

QEMU edu — тестовое PCI-устройство с BAR0 размером 1 MiB и PCI ID `1234:11e8`. Его DMA interface программируется MMIO-регистрами source (`0x80`), destination (`0x88`), transfer count (`0x90`) и command (`0x98`). Устройство предоставляет собственный 4 KiB DMA buffer по offset `0x40000`; command bit `0x01` запускает transfer, bit `0x02` определяет направление, а bit `0x04` запрашивает completion interrupt. По умолчанию device принимает только 28-bit DMA addresses (256 MiB), если QEMU не запущен с подходящим `dma_mask`.[1]

На PC99 seL4 документирует x86/x64 execution и VT-d IOMMU support.[2] Однако корректность и формальные предположения seL4 не означают автоматическую защиту от bus-master DMA: DMA-capable device может обходить CPU/MMU mappings, пока не ограничен IOMMU policy.[3] Поэтому SeLinOS разделяет следующие режимы.

| Режим | Допустимое утверждение | Нельзя утверждать |
|---|---|---|
| QEMU edu M1 без IOMMU | Controlled functional DMA test через root-owned frame и exact device capability | Strong DMA fault containment между driver domains |
| Реальное x86/VT-d | Device IOVA domain, allow-list pinned DMA frames, revoke/unmap before reuse | Поддержка без platform-specific ACPI/VT-d discovery and tests |
| Полная driver compatibility | Per-device policy, mapping lifecycle, cache/coherency semantics, recovery | Что M1 уже загружает или исполняет arbitrary Linux `.ko` |

Следующая реализация должна держать PCI config authority, frame allocation и DMA-map lifecycle в trusted broker domain. Driver domain получает только scoped RPC interface или single-use DMA frame grant, а не allocator authority, PCI config I/O или глобальный physical-memory capability. До IOMMU backend M1 маркируется test-only.

## VT-d/IOMMU follow-up

Официальная документация seL4 для PC99 указывает поддержку x86/x64 execution, QEMU/hardware deployment и VT-d IOMMU support.[2] API документация `libsel4vm_guest_iospace` описывает IOSpace capability как отдельное пространство I/O, присоединяемое к guest/VM для IOMMU (x86) и SMMU (ARM) features.[4] SeLinOS не использует VM runtime, однако этот API подтверждает, что будущий x86 backend должен быть capability- and IOSpace-based, а не строиться на CPU VSpace mapping как замене DMA policy.

Следующая VT-d подзадача должна начинаться только на проверяемой physical x86 platform с ACPI DMAR discovery: root-owned `pcid` создаёт scoped IOSpace policy, `dmad` maps only live DMA leases into device IOVA range, и unmap/revoke предшествует reuse frame. QEMU edu M1 без IOMMU остаётся functional test-only path и не может быть использован как доказательство этого свойства.

## Capability and mapping lifecycle follow-up

Официальные материалы seL4 разделяют CSpace lifecycle и mapping lifecycle: `CNode_Revoke` удаляет descendants capability, а frame mapping/unmapping относится к VSpace mapping operations.[5] Frame capability даёт authority map/unmap physical memory с конкретными rights.[6] Следовательно, будущий `dmad` не должен трактовать собственный accepted free IPC как revoke: он обязан вести lease record, unmap CPU/device mappings where applicable, delete/revoke delegated caps по correct derivation path и только затем позволять frame reuse. Даже эта CPU-side cleanup последовательность не заменяет IOMMU IOSpace unmap для device DMA containment.

## References

[1]: https://www.qemu.org/docs/master/specs/edu.html "QEMU EDU device specification"
[2]: https://docs.sel4.systems/Hardware/IA32.html "seL4 PC99 platform documentation"
[3]: https://sel4.systems/Verification/assumptions.html "What the seL4 Proofs Assume"
[4]: https://docs.sel4.systems/projects/virtualization/docs/api/libsel4vm_guest_iospace.html "seL4 IOSpace API documentation"
[5]: https://docs.sel4.systems/projects/sel4/api-doc.html "seL4 API Reference"
[6]: https://docs.sel4.systems/projects/capdl/lang-spec.html "seL4 capDL language specification"
