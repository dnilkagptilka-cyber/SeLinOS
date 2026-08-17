# SeLinOS Phase 13 network and repository M0 gate

**Status:** planning gate. SeLinOS has no production NIC service, no IP stack and no repository client. The existing QEMU e1000 identity and BAR allocator are quarantined root-side evidence only; they are not a network-device capability, a driver grant or proof of packet transmission.

| Ordered step | Permitted evidence target | Required safety property | Not yet permitted |
|---|---|---|---|
| N0 | Root-only e1000 PCI identity scan in a separate default-OFF image. | Scalar BDF/vendor/device metadata only; no BAR, IRQ, DMA or driver-domain cap. | NIC driver start or packet I/O. |
| N1 | Root-only e1000 BAR range validation with exact restore. | PCI command quiesced during probe; scalar bounds only. | MMIO mapping, descriptor setup or device status writes. |
| N2 | Root-only read-only NIC register observation. | One uncached temporary root mapping with mandatory unmap/delete/free teardown. | Device reset, transmit/receive enable, interrupt acknowledgement or frame delegation. |
| N3 | Driver-domain MMIO/IRQ/DMA readiness. | Hardware IOMMU/IOSpace containment must be evidenced first. Current QEMU TCG profile reports zero IOMMUs, so this step is blocked. | Any NIC authority transfer. |
| N4 | Bounded link-layer and IP transport. | Isolated network service requires bounded packet buffers, completion/timeout policy and receive-path authority review. | DNS, TCP, HTTP(S) or package acquisition. |
| N5 | Repository trust substrate. | Pinned trust roots, monotonic/time policy, replay/downgrade policy and audit evidence. | APT or repository compatibility claims. |

> **Stop rule:** Until N3 has a platform-specific IOMMU/IOSpace proof, no SeLinOS build may give a NIC domain a BAR frame, IRQ cap, DMA memory, bus-master enable or packet descriptor ring. QEMU-only root observations do not weaken this rule.

## Relationship to package management

APT acquisition additionally requires HTTP(S), DNS or a pinned transport alternative, repository metadata integrity/authentication, keyring policy, replay protection and durable package-state storage. Phase 13 therefore cannot create a `dpkg`, `apt` or Linux package compatibility claim. Its first acceptable result is a separately gated root-only NIC metadata or register observation, comparable in scope to the completed virtio M0–M2 proof sequence.
