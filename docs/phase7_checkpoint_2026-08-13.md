# SeLinOS Phase 7 checkpoint — 2026-08-13

The current production image is `build/images/selinos-root-image-x86_64-pc99` with SHA-256:

```text
10eb7f9d6156fd1e9e3570c0b49127049b5c48ddfa9037336a782a6148ee52c2
```

Sixteen independent evidence verifiers pass for this image generation, including one separate opt-in virtio discovery verifier. The latest bounded addition is `ROMFS packed-path M1`: the direct isolated ROMFS client sends `/selinos-banner` through a four-word IPC message containing a byte count plus two big-endian packed 64-bit words. The server accepts only one to sixteen printable, non-NUL bytes and resolves the path in its immutable embedded `newc` CPIO archive. It does not map client memory, accept arbitrary pointer strings, traverse directories or provide a general VFS.

The current verified stack also includes CPIO multi-file lookup, Linux syscall identity/TID M5, bounded KAPI synchronization M1, root-only PCI BAR allocator proof, and the prior driver/KABI/ELF metadata milestones. A separate default-OFF QEMU proof additionally found modern virtio block PCI identity `1af4:1042` using only temporary root configuration-I/O authority; it did not read/map/program a BAR or touch feature negotiation, virtqueues, IRQ, DMA, I/O or a storage domain. No native dynamic ELF or Linux `.ko` execution is enabled: the pinned x86_64 seL4 mapping attribute API remains cache-only and exposes no audited NX/execute control.

The next package-manager prerequisite implementation must be a bounded writable storage/VFS design. `dpkg` and `apt` must remain unclaimed until persistent atomic updates, package status database lifecycle, `.deb` handling, general process/TLS/dynamic execution, network/TLS/DNS and repository trust verification are independently evidenced.
