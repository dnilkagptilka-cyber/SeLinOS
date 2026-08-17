# SeLinOS Phase 13 network and repository checkpoint

The current default x86_64/PC99 QEMU-TCG image is:

```text
00ac1ebb621bed84cf9f04d2e441bea6afe58b7ce53ae098c2ec9def82d77197
```

## Verified N0 result

A separate default-OFF build, `build-e1000-discovery-probe/`, enables `SeLinRootE1000DiscoveryProbe=ON` and boots QEMU’s standard `e1000` PCI device. `tools/verify_network_e1000_discovery_n0.py` binds that image, the root PCI implementation, its public scalar contract, the CMake gate and serial evidence. It verifies the root-only marker:

```text
SeLinOS network N0: quarantined e1000 PCI identity found; BAR/IRQ/DMA/network authority withheld.
```

The N0 helper reads PCI configuration identity `8086:100e` and returns scalar BDF metadata only. Its I/O-port capability is released on every return path. No BAR frame, MMIO mapping, command-register mutation, IRQ cap, DMA lease or capability is exposed to a driver domain.

## Hard stop rule

The current QEMU TCG profile still reports `ACPI: 0 IOMMUs detected`. Therefore N1+ cannot delegate NIC MMIO, IRQ or DMA authority. Any future BAR-range validation or uncached root register observation must remain a separate default-OFF root-only experiment and must not transmit or receive a frame. A NIC driver domain, Ethernet, IP, DNS, TCP, HTTP(S), repository transport and repository-trust policy are all unimplemented.

## Package-manager consequence

The N0 proof does not reduce the `dpkg`/`apt` gate to networking alone. Durable filesystem state, broad process/TLS/dynamic-linker support, repository metadata trust, time/replay policy and a pinned end-to-end package profile remain independently required.

## N1 decision

The existing e1000 allocator gate confirms that a normal QEMU e1000 BAR0 is firmware-assigned (`0xfeb80000`) and that the allocation primitive rejects it rather than relocating it. A future N1 may only inspect or bounds-validate that preassigned resource with exact register restoration and root-only scalar output. The destructive force-unassigned allocator fixture is not an N1 network path and cannot be combined with a NIC driver or packet experiment. In particular, N1 does not bypass the IOMMU/IOSpace stop rule for BAR frame, IRQ or DMA delegation.

## Production-profile separation check

The current production build cache records `SeLinRootE1000DiscoveryProbe:BOOL=OFF`. N0 PCI discovery executes only in `build-e1000-discovery-probe/`; it is not present in the normal production boot path.

The detailed N1 contract is recorded separately in `docs/phase13_e1000_bar_n1_gate.md`. It requires default-OFF root-only scalar validation with exact restoration and does not authorize a NIC mapping, driver, IRQ, DMA, bus mastering or transport.
