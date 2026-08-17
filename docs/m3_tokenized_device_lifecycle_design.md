# SeLinOS M3: bounded token-indexed dual-device lifecycle

## Цель

M2 доказала один QEMU edu lifecycle с fixed root grants и terminal `dma_free_coherent()` / `free_irq()` capability teardown. M3 реализует и проверяет **bounded two-device token-indexed service-owned lifecycle**: для каждой из максимум двух QEMU edu PCI functions root создаёт отдельные `deviced`, `irqd`, `dmad` и driver-domain bundle. Это не общий dynamic registry.

## Token

Root создаёт opaque 64-bit device token из trusted per-device resource record; `deviced` возвращает его только после successful PCI match. Token связан с `{domain instance, PCI function, generation, driver registration}`; KAPI clears its driver-local binding after remove. Service-wide fault detection and universal token invalidation remain unimplemented. Сервисный token не является raw physical address, PCI BDF authority, CSpace pointer или capability itself.

| API class | Required token relation | Authority that stays outside driver domain |
|---|---|---|
| `pci_register_driver` / probe | `deviced` returns root-issued token only after policy match | PCI enumeration/configuration and BAR allocation |
| `request_irq` / `free_irq` | `irqd` validates token, IRQ line and generation | `IRQControl`, `IRQHandler`, notification allocation |
| `dma_*` | `dmad` validates token, exact active lease and DMA mask | frame/untyped allocator, VSpace/CSpace lifecycle record, IOSpace/IOMMU authority |
| `remove` / fault | Target: token moves atomically to RELEASING then REVOKED | Current bounded profile only tears down the active driver grants on `remove`; fault transition remains future work |

## State model

| State | Valid operations | Required invariant |
|---|---|---|
| `DISCOVERED` | registration matching | No driver cap grants exist. |
| `BOUND` | profile-approved resource grant setup | One trusted owner record identifies domain CNode, VSpace and granted child caps. |
| `ACTIVE` | KAPI IRQ/DMA operations with matching token | Every live resource record carries same token and generation. |
| `RELEASING` | only completion / teardown | New IRQ/DMA grants are rejected. |
| `REVOKED` | no driver KAPI resource operation | CPU mappings and delegated caps have been removed; stale token fails. |
| `REUSABLE` | later new generation may bind | Reused frame is zeroed and no old CSpace/VSpace/IOSpace mapping remains. |

## Topology observation before M3

QEMU 8.2.2 accepts two `edu` devices in the current test command. M3 enumerates up to two matching functions and, for `-device edu -device edu`, starts two independent bundles. The machine-readable record [`tests/artifacts/selinos_m3_two_edu.verification.json`](../tests/artifacts/selinos_m3_two_edu.verification.json) requires two occurrences each of successful PCI match, wrong-token rejection fixture, 100-byte DMA roundtrip, DMA capability teardown, IRQ notification teardown and remove completion; `tools/verify_m3_two_edu.py` verifies those multiplicities and hashes.

## Required M3 evidence

The completed bounded M3 profile shows two independent concurrent registered instances, rejects wrong tokens in both `irqd` and `dmad` without changing each active lease, and shows `free_irq` and `dma_free_coherent` teardown before accepted replies. Future unbounded M3 closure additionally requires sequential-generation reuse with zeroisation and proof that a new generation cannot observe old frame contents or re-use old notification/frame authority.

> M3 still does not establish bus-master DMA containment. Only a configured IOSpace/IOMMU path with a device-specific IOVA allow-list can support that claim.

## Non-goals

This bounded M3 profile does not supply unbounded dynamic registry, hotplug, sysfs/uevents, full PCI topology, shared IRQ semantics, MSI-X, coherent allocator reuse/zeroisation, or universal driver KAPI. These remain separate closure targets on the route to a testable Linux compatibility profile.
