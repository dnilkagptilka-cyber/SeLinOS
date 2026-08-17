# SeLinOS VT-d / IOSpace source notes — 2026-08-13

This note records source material gathered before a future physical-platform DMA-containment implementation. It does not change the status of the current QEMU TCG edu profile.

| Source | Finding used by SeLinOS | Engineering consequence |
|---|---|---|
| [seL4 PC99 platform documentation][1] | seL4 documents execution on x86/x64 systems, both QEMU and hardware. | QEMU is appropriate for functional lifecycle regressions; hardware remains a separately tested deployment target. |
| [libsel4vm guest IOSpace interface][2] | An IOSpace capability is an explicit object used for x86 IOMMU and ARM SMMU VM features. | A future SeLinOS native broker must use capability/IOSpace policy for DMA containment rather than treating CPU VSpace unmap as a replacement for IOMMU policy. |
| [seL4 mapping tutorial][3] | A frame capability tracks one mapping; copies are needed for multiple mappings, and pages can be unmapped by page unmap invocation. | `dmad` must retain exact frame-cap derivations and mapping ownership; M2’s Page_Unmap plus CNode revoke/delete cleans the single CPU mapping only. |

> **Current boundary:** QEMU edu TCG evidence demonstrates mediated functional DMA and CPU-side capability/mapping teardown. It is not evidence of VT-d/IOSpace DMA containment, interrupt-remapping policy, or protection against malicious bus-master DMA.

A future VT-d closure test requires a supported physical x86 system with firmware/IOMMU enablement, explicit device-to-IOSpace attachment, a per-device IOVA allow-list, DMA map/unmap ordering, negative out-of-range DMA evidence, and fault/recovery behaviour. It must be reported as a separate hardware profile rather than merged into QEMU TCG success.

## References

[1]: https://docs.sel4.systems/Hardware/IA32.html "seL4 PC99 platform documentation"
[2]: https://docs.sel4.systems/projects/virtualization/docs/api/libsel4vm_guest_iospace.html "libsel4vm IOSpace interface"
[3]: https://github.com/seL4/sel4-tutorials/blob/master/tutorials/mapping/mapping.md "seL4 mapping tutorial"
