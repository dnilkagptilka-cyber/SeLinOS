# Phase 84 Gate — static ELF64 two-RX plus `PT_NOTE` M0

**Status:** VERIFIED

The isolated `x86_64/PC99` QEMU TCG profile built successfully and reached the bounded terminal second-RX `UD2` witness after parser validation of the fixed four-byte file-resident note. The root transaction did not map or copy note metadata. Existing ELFRT M1 host coverage also passed after validation was made compatible with individually bounded ordinary multi-note host ELF metadata.

## Objective

Phase 84 extends the published fixed Phase 83 ELF64 fixture by adding exactly one `PT_NOTE` header. The note remains file-resident metadata only: it is never mapped, interpreted as code, copied into a target address space, or supplied to a runtime ABI. The fixed executable contract remains two page-aligned `RX` load segments, fixed `RO+NX` and `RW+BSS+NX` load segments, a separate root-owned `RW+NX` stack, one resume and terminal second-RX `UD2`.

The parser extension must validate each recognised `PT_NOTE` as `PF_R`, `p_memsz == p_filesz`, an in-file bounded range and power-of-two/zero alignment. The Phase 84 exact fixture additionally requires exactly one record with zero virtual and physical addresses and a four-byte payload at fixed non-load file offset `0x200`; this keeps ordinary readable multi-note host ELF files parseable while making the proof fixture deterministic. A parser summary field must record observed note presence and byte size. The root transaction must still ignore the note for all mappings.

| Contract element | Required fixed value | Rejected state |
|---|---|---|
| Program-header count | 6 | Any other fixture header count |
| `PT_NOTE` | Exact fixture: one `PF_R` file-resident record | Fixture duplicate, writable/executable, out-of-file or memory-bearing mismatch |
| Exact fixture note | Four fixed bytes at non-load offset | Any payload or offset deviation |
| Mappings | Two RX, RO+NX, RW+BSS+NX, stack RW+NX | Note mapped, W+X, executable stack or metadata-driven mapping |
| Terminal witness | Second RX `UD2` | Reply, repair, retry, second resume or alternate fault |

## Non-claims

This milestone will not claim general ELF notes, GNU property interpretation, build-id semantics, ELF mapping, a general ELF loader, auxiliary vectors, dynamic linking, C runtime startup, Linux ABI compatibility, Debian package execution, `dpkg`, `apt`, Linux kernel execution, a Linux VM, a container, chroot or compatibility mode.
