# SeLinOS Phase 13 e1000 BAR N1 checkpoint

**Status:** verified, quarantined and **default OFF**. This checkpoint records only a root-only QEMU TCG proof for the firmware-assigned BAR0 of the legacy e1000 PCI function `8086:100e`. It is not a NIC driver, a device-capability handoff, a transport proof or a package-management prerequisite completion.

| Item | Evidence-bound result |
|---|---|
| Build profile | `build-e1000-bar-range-probe/` configured with `SeLinRootE1000BarRangeProbe=ON`. |
| Fixture | x86_64/PC99 QEMU TCG instruction-count run with `-device e1000`; no Linux kernel is present. |
| Starting identity | Root N0-style PCI lookup returns scalar BDF plus vendor/device identity `8086:100e`. |
| N1 input guard | Before any configuration-space write, N1 re-reads the exact BDF and verifies that the supplied identity still matches. |
| BAR policy | N1 accepts only a nonzero 32-bit memory BAR0. It rejects I/O BARs, unsupported memory forms and malformed ranges. |
| Sizing transaction | Root temporarily quiesces PCI memory decoding and bus mastering, writes all ones only to BAR0, reads the size mask, restores BAR0 with read-back verification, then restores the original PCI command word with read-back verification. |
| Output | The helper returns only scalar `paddr`, `size` and `bar_index` metadata after overflow-safe 32-bit range validation. |
| Runtime result | The serial log records both the successful exact-restore marker and the explicit absence of BAR frame, mapping, IRQ, DMA, bus mastering and packet I/O. |

> **Authority boundary:** a scalar physical range is not a seL4 device-frame capability. N1 does not map MMIO, issue an IRQ capability, lease/map DMA, retain bus mastering, copy a capability to a NIC domain, reset the device, configure descriptors, transmit or receive a frame.

## Reproduction

```bash
cd /home/ubuntu/helixos
cmake -S src -B build-e1000-bar-range-probe -G Ninja \
  -DSeLinRootE1000BarRangeProbe=ON
cmake --build build-e1000-bar-range-probe
qemu-system-x86_64 -accel tcg,thread=single -icount 1 -cpu max \
  -nographic -serial mon:stdio -m size=1G \
  -kernel build-e1000-bar-range-probe/images/kernel-x86_64-pc99 \
  -initrd build-e1000-bar-range-probe/images/selinos-root-image-x86_64-pc99 \
  -device e1000
./tools/verify_network_e1000_bar_range_n1.py
```

The resulting log must include the following success records.

```text
SeLinOS network N1: e1000 firmware BAR range validated and restored; no MMIO/IRQ/DMA/NIC authority.
SeLinOS network N1: no BAR frame, mapping, IRQ, DMA, bus mastering or packet I/O.
```

## Current stop rule

The exact same QEMU TCG log reports `ACPI: 0 IOMMUs detected`. Therefore N1 does not permit N2 read-only MMIO observation, and it does not permit any BAR-frame issuance, VSpace mapping, IRQ delegation, DMA authority, bus mastering, NIC-domain startup or packet path. A later N3 must first provide platform-specific, independently verified IOSpace/IOMMU containment before a NIC resource can leave root ownership.

| Remains unimplemented | Why N1 does not establish it |
|---|---|
| e1000 driver or Linux KAPI binding | N1 invokes no driver code and presents no device frame or interrupt capability. |
| Link-layer, IP or repository transport | N1 has neither registers, descriptors, packet buffers nor scheduling/timeout semantics. |
| Package retrieval, `dpkg` or `apt` | There is no network authority, package filesystem, process/TLS runtime or repository trust chain. |
| Hardware isolation claim | The active test profile explicitly reports zero detected IOMMUs. |

The default production build retains `SeLinRootE1000BarRangeProbe=OFF`; it does not execute this experiment.
