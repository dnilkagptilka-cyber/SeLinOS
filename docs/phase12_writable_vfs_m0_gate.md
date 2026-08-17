# SeLinOS Phase 12 writable VFS M0 gate

**Status:** design gate. The completed virtio M2 experiment does not supply a block driver, DMA containment, or block I/O. Consequently, SeLinOS may not label its first writable-file milestone as persistent storage, POSIX filesystem support, or a `dpkg`/`apt` prerequisite completion.

The first admissible slice is a **volatile, bounded, isolated VFS transaction**. It extends the existing ROMFS endpoint without forwarding a user pointer, mapping client data, or granting raw memory authority. The server owns the mutable bytes and receives a fixed-size IPC payload. This can prove request validation, descriptor lifecycle, atomic in-server replacement and isolation, but its contents are intentionally lost at reboot.

| M0 action | Required bounds | Required evidence | Explicit exclusions |
|---|---|---|---|
| Open mutable state object | Fixed server-defined pathname and fixed descriptor; one authorized test client. | Reject unknown/malformed path and invalid FD. | Arbitrary path lookup, directory tree, permissions, concurrent descriptor tables. |
| Write one state record | Fixed exactly-one-word IPC payload; no client pointer, no shared frame and no partial update. | Read-after-write round trip plus failed malformed-write non-mutation proof. | Streaming writes, partial writes, append, mmap, file offsets beyond zero. |
| Close mutable descriptor | Exact fixed FD validation. | Closed/invalid-FD rejection after lifecycle transition. | Process cleanup, fork/exec inheritance and general POSIX FD semantics. |
| Reboot behavior | State is explicitly volatile. | Boot log identifies volatile backing and does not claim recovery. | Block I/O, persistence, journaling, crash recovery, durability barriers. |

> **Stop rule:** this phase must create neither a virtqueue nor a block request. It must not copy a device frame into a VFS domain, depend on an unproven IOMMU, or state compatibility with `dpkg`, `apt`, package extraction, or a general writable filesystem.

## Entry and exit conditions

The implementation may start only with a separate evidence record and independent verifier binding its image, the endpoint protocol, server, client probe and serial log. The verifier must require the successful volatile round trip and reject markers for persistence, device DMA, block reads/writes, arbitrary pathname support, or package-manager execution.

The next phase may begin only after a separately proven block backend and durability model replace the in-server byte storage. At that point, failure ordering, atomic replace, flush semantics, recovery and multi-client ownership must be tested before any Debian package claim.

## Relationship to package management

This M0 gate addresses a small fragment of the missing writable-filesystem row in `docs/dpkg_apt_prerequisite_gate.md`. It does not alter the conclusion of that gate: `dpkg` and `apt` remain unsupported because persistent storage, a directory/metadata model, archive extraction, a state database, normal process execution and repository trust are all absent.
