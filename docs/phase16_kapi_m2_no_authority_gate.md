# SeLinOS Phase 16 Linux KAPI M2 no-authority gate

**Status:** design gate only. The existing KAPI M1 `request_irq`/`free_irq` path is already bound to the QEMU EDU device-token, `irqd` control endpoints and a real notification/completion sequence. It must not be weakened, silently redirected to a stub or generalized to another PCI device.

The next KAPI milestone may add a **separate no-authority I/O lifecycle proof**, but only under the following contract.

| Candidate API | Permitted M2 behavior | Prohibited behavior |
|---|---|---|
| `ioremap`/`iounmap` | Validate a single bounded fixture range, return only an opaque non-device test handle or fail closed, and prove matching cleanup. | No seL4 device-frame allocation, VSpace mapping of device physical memory, cache-attribute claim, register access or cap transfer. |
| Existing `request_irq`/`free_irq` | Preserve current token-bound EDU semantics unchanged; exercise only existing independently verified M1 authority path if required for regression. | No fake success without endpoint/token validation, no additional IRQ number/device, no NIC IRQ or IRQ capability issuance. |
| New no-authority IRQ probe | If needed, expose a clearly private `selinos_*` test hook returning a documented rejection before any seL4 operation. | No overload of the Linux `request_irq` symbol, no handler registration, notification wait, device interrupt, or completion signal. |

> **Decision:** a canonical Linux `request_irq` stub that reports success while issuing no interrupt authority would be misleading and would collide with the current, token-bound EDU M1 implementation. Therefore it is not an acceptable M2 route. A future no-authority test must be private, fail-closed and separately named.

## Required proof obligations

A future implementation must have an isolated probe, an independent verifier and a default production profile that remains unchanged. Its evidence must demonstrate all of the following: no `vka_alloc_frame_at`; no `vspace_map_pages_at_vaddr`; no `seL4_X86_IRQControl_*`; no `seL4_Signal`/`seL4_Wait` in the no-authority path; no bus-master enable; no DMA lease; no driver-domain capability copy. It must also retain the current QEMU TCG zero-IOMMU stop rule for e1000/virtio resources.

This gate does not close the `apt`/`dpkg` prerequisite gap. It provides at most an auditable KAPI shape for a future loader test and is not device, storage, network or package-manager functionality.
