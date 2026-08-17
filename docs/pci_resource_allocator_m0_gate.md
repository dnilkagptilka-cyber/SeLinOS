# SeLinOS root PCI resource allocator M0 gate

**Status:** a **quarantined, root-only 32-bit memory-BAR primitive** is source-implemented and has a narrow QEMU fixture proof; a generic PCI allocator and NIC delegation are not implemented.

Direct SeLinOS boot does not make any assumption that firmware BAR state is safe to replace. QEMU monitor inspection reports an identifiable `8086:100e` e1000 and its BAR layout, while a normal SeLinOS QEMU `-device e1000` boot observed BAR0 already set to `0xfeb80000`. The root primitive rejects such a preassigned BAR rather than relocating it. A separate destructive QEMU fixture build was required to clear BAR0 to zero and exercise the sizing/programming transaction. Therefore PCI ID discovery alone still cannot lead to MMIO mapping, IRQ setup, DMA enablement or driver-capability delegation.

| Required allocator property | Minimum evidence |
|---|---|
| Enumerate configuration space under root-only I/O-port authority | BDF, header type and BAR-kind bounds checks; no driver sees PCI config cap |
| BAR sizing | Standard write-all-ones/read/restore sequence validated per accepted BAR type |
| Address allocation | Non-overlapping root-owned physical-MMIO range policy; reject 64-bit/I/O BARs until explicitly supported |
| BAR programming | Read-back match, rollback on failure and no bus-master enable before driver bundle acceptance |
| Interrupt routing | A validated INTx/MSI policy and independently scoped IRQ capability lifecycle |
| Delegation | Copy only final BAR frame caps, endpoint caps and explicit DMA leases into driver bundle after policy validation |

The existing QEMU edu profile remains unrelated and cannot be used to infer general PCI allocation. The new primitive is insufficient for networking: until resource collision checking, frame capability acquisition, interrupt policy and capability delegation are independently verified, e1000/virtio-net remain research only and SeLinOS has no networking capability.

## Local device-frame capability finding

The pinned `sel4platsupport_alloc_frame_at()` wrapper delegates to `vka_alloc_frame_at()` for a caller-supplied physical address. It is therefore useful only after root has established a valid BAR address; it does not size, assign or validate a PCI BAR. This confirms the required sequence: configuration-space allocation first, device-frame capability acquisition second, and process delegation only after both have passed policy checks.

## BAR sizing reference

The Linux `pci_regs.h` register definitions state that decoded BAR size is determined by writing `0xffffffff`, reading the value back, and observing decoded one bits. They also distinguish memory versus I/O BARs, 32-bit versus 64-bit memory BARs, and the command bits that separately enable memory response and bus mastering.[1] SeLinOS M0 must snapshot and restore each BAR during sizing, allocate an aligned non-overlapping range, program and read back the assigned address, then enable only required memory/bus-master bits after the resource projection is accepted.

[1]: https://sites.uclouvain.be/SystInfo/usr/include/linux/pci_regs.h.html "Linux PCI configuration register definitions"

## Quarantined QEMU allocator experiment — 2026-08-13

The experiment was compiled only in `/home/ubuntu/helixos/build-pci-bar-probe` with `SeLinRootPciBarAllocatorProbe=ON` and `SeLinRootPciBarAllocatorForceUnassignedE1000=ON`. Both options default to `OFF`; they are not enabled in the normal reproducible image. Root held temporary PCI configuration-space I/O authority only. The experiment created **no** NIC MMIO frame capability, NIC VSpace mapping, NIC driver domain, IRQ capability, DMA lease or network stack.

| Evidence item | Result | Interpretation |
|---|---|---|
| Normal QEMU `-device e1000` probe | BAR0 read as `0xfeb80000`; transaction rejected it | The ownership guard preserves a firmware-assigned BAR rather than relocating it. |
| Forced-unassigned fixture | A QEMU-only test switch writes e1000 BAR0 to zero before invoking the allocator | Deliberately destructive preparation; it is not a production resource-management policy. |
| BAR sizing and program/read-back | The log records `BAR probe assigned BAR0=0xc0000000` and `... assignment read-back passed` | The constrained root transaction sized, aligned, programmed and read back one e1000 32-bit memory BAR inside the policy window. |
| Authority after proof | No frame mapping, NIC driver domain, IRQ capability or DMA capability exists | This is allocator-only evidence. NIC delegation and networking remain blocked. |

> The fixture proves only a narrow QEMU 32-bit memory-BAR transaction. It does not prove arbitrary topology allocation, bridge-window management, collision avoidance against other assigned BARs, 64-bit BAR support, firmware handoff policy, device-frame capability acquisition or safe NIC capability delegation.
