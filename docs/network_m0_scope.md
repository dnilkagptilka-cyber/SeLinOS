# SeLinOS network M0 scope

**Status:** planned; no network device, socket ABI, packet transport or network stack is implemented.

Dynamic ELF execution is blocked from production use by the pinned x86_64 execute-permission limitation, but network services can be developed independently. The first network milestone must remain capability-scoped: one emulated NIC function, an isolated `netd`, a narrowly granted MMIO/IRQ/DMA capability set, and a packet-loopback proof before socket syscalls or DNS are discussed.

| Increment | Required proof | Explicit non-claim |
|---|---|---|
| Network M0a | PCI match for one supported NIC with BAR/IRQ/DMA authority only in driver bundle | General hardware support or Linux NIC driver compatibility |
| Network M0b | `netd` endpoint accepts a bounded packet descriptor and validates lifetime | IP, ARP, DHCP, TCP, UDP or sockets |
| Network M0c | Isolated loopback or QEMU packet path returns a bounded payload | Internet reachability, TLS, DNS or package download |
| Network M1 | Linux `socket`/`connect`/`sendto`/`recvfrom` subset routes to `netd` with pointer-safe transfer | POSIX networking or apt transport support |

No implementation should grant a network service authority over unrelated device caps, user VSpaces or arbitrary DMA frames. Every packet buffer must have a clear map/unmap/revoke lifecycle analogous to existing QEMU edu DMA evidence.

## Research basis

QEMU documentation explains that an emulated e1000 is discovered through PCI and controlled through its memory-mapped register bank, while virtio-net is a paravirtual PCI device that uses guest-memory command queues and a defined device protocol. Either profile requires isolated MMIO, IRQ and DMA lifecycle handling; M0 must choose one explicit device profile rather than treating a generic QEMU NIC as compatible.[1]

[1]: https://www.qemu.org/2018/02/09/understanding-qemu-devices/ "Understanding QEMU devices"

## Pre-implementation baseline

A QEMU boot with `-device e1000` completed the normal twelve-domain SeLinOS bootstrap while outputting only the existing absent-edu path. No NIC discovery marker, MMIO mapping, IRQ grant, DMA frame grant or network endpoint exists in that trace. The corresponding artifact is [`tests/artifacts/selinos_network_m0_preimplementation.log`](../tests/artifacts/selinos_network_m0_preimplementation.log). This is a baseline observation, not networking evidence.

## Direct-boot PCI allocation prerequisite

QEMU monitor inspection with `-device e1000` reports Intel `8086:100e` functions, but their BAR0 values remain unassigned (`0xffffffffffffffff`) in the direct kernel boot configuration. The existing SeLinOS edu profile is configured differently and cannot be used as proof that a generic NIC has an assigned BAR. A read-only e1000 scanner was therefore not retained: it would either reject the zero/unassigned BAR or risk pretending an unassigned resource was safe to map. Network M0a requires a separately verified root PCI resource allocator that sizes, assigns and validates BARs before any MMIO/IRQ/DMA capability can be issued.

The raw monitor record is [`tests/artifacts/qemu_e1000_info_pci.log`](../tests/artifacts/qemu_e1000_info_pci.log). This finding is an environment prerequisite, not evidence of NIC support.
