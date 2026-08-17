# SeLinOS VT-d / IOSpace containment M0 gate

**Status:** no hardware-DMA containment is implemented or claimed. Current driver evidence demonstrates CPU-side page unmap and capability revoke/delete for a QEMU EDU DMA lease. That is not equivalent to an IOMMU policy constraining a DMA-capable device.

The seL4 supported-platform matrix lists PC99 x64 with VT-D support, while also cautioning that simulator hardware support is limited and QEMU generally does not implement enough hardware devices for complex systems.[1] A direct audit of the current generated SeLinOS build finds `CONFIG_IOMMU=1` and generated x86 `seL4_X86_IOPageTable_Map`/`seL4_X86_Page_MapIO` interfaces. However, the current QEMU TCG boot log reports `ACPI: 0 IOMMUs detected`, and SeLinOS source contains no verified IOSpace capability flow, IOMMU page table, device-to-domain attachment or DMA-remapping test. The configuration and callable API are thus **prerequisites**, not evidence of containment; this QEMU development profile cannot establish a hardware-DMA-isolation proof.

| Required condition | Current state | Acceptance evidence required |
|---|---|---|
| Confirm hardware/IOMMU configuration | Build prerequisite confirmed; QEMU runtime blocker observed | `CONFIG_IOMMU=1`, generated IOSpace API audit, and a target with ACPI/VT-d detection (current QEMU boot says `ACPI: 0 IOMMUs detected`). |
| Create device-specific IOSpace authority | Not implemented | Root-only mint record tied to PCI BDF and a valid nonzero IOMMU domain ID; generated API requires the IOSpace to be assigned to a PCI device. |
| Map only approved DMA buffers | Not implemented | IO page-table map plus `seL4_X86_Page_MapIO` trace for 4 KiB frames, negative out-of-range DMA test and device-visible address audit. |
| Revoke DMA mapping on teardown | CPU-side cap teardown only | IOMMU unmap/invalidation proof before backing memory reuse. |
| Delegate driver authority safely | Not implemented | Driver receives only device-scoped MMIO/IRQ/DMA capabilities, with independent containment verifier. |

> **Generated-API boundary:** the pinned kernel creates a device IOSpace only by minting the global IOSpace capability with encoded PCI bus/device/function and a valid nonzero IOMMU domain ID. `seL4_X86_IOPageTable_Map` rejects an IOSpace not assigned to a PCI device; `seL4_X86_Page_MapIO` requires a 4 KiB page, valid requested rights and sufficient IO page tables. These documented error conditions define the required positive and negative test cases for SeLinOS M0.

> **Claim boundary:** without the above evidence, SeLinOS treats DMA-capable hardware as trusted by the current QEMU development profile. Token validation and CPU-side revocation are useful lifecycle controls, but they do not establish hardware-enforced DMA isolation.

## References

[1]: https://docs.sel4.systems/Hardware/ "seL4 supported platforms"
