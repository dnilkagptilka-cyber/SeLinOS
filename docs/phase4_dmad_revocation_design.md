# SeLinOS Phase 4: dmad lease revocation design

## Status and purpose

Current M1 has a **single pre-granted QEMU edu lease**. `dma_free_coherent()` now performs a scoped synchronous release IPC with `selinos-dmad`, but the service only records acceptance. It does **not** unmap the driver CPU mapping, delete the delegated frame capability, revoke capability descendants, or protect device DMA with an IOMMU. This document defines the next implementation target without overstating the current evidence.

## Required state machine

| State | Owner | Permitted operation | Required transition condition |
|---|---|---|---|
| `UNBOUND` | `dmad` | Bind only an authenticated device token and a root-created frame lease | Device policy and physical address mask validated |
| `ACTIVE` | `dmad` plus driver domain | Return CPU mapping and `dma_addr_t`; permit device DMA only within live IOVA policy | Lease record retains exact frame cap lineage, mapping addresses and byte range |
| `RELEASING` | `dmad` | Reject new use of token; quiesce device and drain completion path | Correct `dma_free_coherent()` tuple authenticated |
| `REVOKED` | `dmad` | Deny stale token, stale cap, CPU mapping and IOVA access | CPU/device unmaps complete; delegated capability descendants deleted/revoked |
| `REUSABLE` | trusted allocator | Frame can be offered to a later lease | IOMMU and CSpace/VSpace cleanup evidence recorded |

## Implementation requirements

The dynamic service must allocate a non-forgeable lease ID and validate it together with device identity, CPU address, DMA address and requested size. The driver must never receive `vka`, `IRQControl`, PCI configuration I/O, untyped memory authority, an IOSpace root capability or a generic frame allocator capability.

Release must be ordered: first block new requests, then quiesce/ack outstanding device activity, then unmap CPU mapping and device IOVA mapping, then delete or revoke the exact delegated capability derivation, and only then mark the backing frame reusable. A successful IPC reply before these steps is not an acceptable revocation guarantee. On x86_64, a frame capability tracks only one mapping, so any policy that needs multiple mappings must create and track distinct capability derivations before it can safely unmap/revoke them.[4]

The current generated `libsel4` interface exposes `seL4_X86_Page_Unmap`, `seL4_CNode_Delete` and `seL4_CNode_Revoke`. Their future use needs explicit ownership of the driver VSpace capability and delegated CSpace derivation; the current release-only M1 does not grant this authority to `selinos-dmad`. `sel4utils_process_t` records the process VSpace, CSpace object and allocation ownership after `sel4utils_configure_process()`, so trusted lifecycle code must retain an authoritative process/lease record rather than accept VSpace or CSpace pointers from the driver.[5]

> `CNode_Revoke` and VSpace/IOSpace unmapping address different parts of the lifecycle. Neither CPU mapping cleanup nor capability cleanup alone proves bus-master DMA containment without an IOMMU policy.[1] [2]

## Completion evidence

| Test | Required observable result |
|---|---|
| Invalid device token | Allocation and free requests are rejected without changing an active lease |
| Incorrect free tuple | `dmad` rejects request and original lease remains active |
| Release success | Driver loses CPU mapping/capability access; subsequent stale use faults or is denied |
| Frame reuse | A new lease cannot observe contents or authority from the old driver domain |
| VT-d profile | DMA outside the lease IOVA allow-list faults or is denied on a real DMAR-enabled platform |
| Regression | Existing QEMU edu 100-byte DMA, mediated IRQ registration/release, boot baseline and KABI fixture remain green |

## References

[1]: https://docs.sel4.systems/projects/sel4/api-doc.html "seL4 API Reference"
[2]: https://docs.sel4.systems/projects/capdl/lang-spec.html "seL4 capDL language specification"
[3]: https://docs.sel4.systems/projects/virtualization/docs/api/libsel4vm_guest_iospace.html "seL4 IOSpace API documentation"
[4]: https://docs.sel4.systems/Tutorials/mapping.html "seL4 mapping tutorial"
[5]: https://github.com/seL4/sel4_libs/blob/master/libsel4utils/include/sel4utils/process.h "sel4utils process interface"
