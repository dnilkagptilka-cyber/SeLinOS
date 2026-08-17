# SeLinOS Phase 13 e1000 BAR N1 gate

**Status:** verified, separate default-OFF root-only experiment. N0 proves only root PCI identity discovery for QEMU e1000 `8086:100e`; verified N1 adds only a temporarily quiesced size probe of its firmware-assigned 32-bit BAR0, exact BAR/PCI-command restoration with read-back verification, and scalar range metadata. It does not establish MMIO ownership or a usable NIC resource.

N1 exact-matches the supplied N0 identity before any configuration write, snapshots PCI command and BAR0, temporarily quiesces memory decoding and bus mastering for the sizing transaction, restores all changed registers and reads back the original values before reporting success. The result contains only scalar physical range metadata. The implementation and serial evidence are bound by `verify_network_e1000_bar_range_n1.py`; the recorded profile and continuing stop rule are in `phase13_e1000_bar_n1_checkpoint_2026-08-14.md`.

| Allowed N1 action | Required bound | Prohibited consequence |
|---|---|---|
| Read firmware BAR0 value and BAR type. | Reject I/O, malformed and unsupported 64-bit resource forms. | No device frame cap or VSpace mapping. |
| Validate a scalar range inside the 32-bit physical-address space. | Overflow-safe base/size arithmetic. | No BAR relocation or command-register enable. |
| If sizing is used, write all ones only under an exact rollback transaction. | Restore verification on every exit path. | No driver-domain cap copy, IRQ cap, DMA lease or bus mastering. |

> **Authority stop rule:** N1 cannot begin a NIC driver path. Even a successfully restored scalar BAR range does not override the current QEMU TCG finding of zero IOMMUs; therefore frame mapping, IRQ and DMA delegation remain blocked pending platform-specific IOSpace containment evidence.

## Implementation finding

The existing `selinos_pci_assign_unconfigured_bar32()` is not an N1 validator: after quiescing command bits, it rejects any BAR whose decoded address is neither zero nor all ones. That behavior correctly preserves the normal firmware-assigned e1000 BAR and must remain intact. N1 therefore requires a separate read/validate/restore helper; it must not repurpose the allocator’s force-unassigned fixture or program a new address.
