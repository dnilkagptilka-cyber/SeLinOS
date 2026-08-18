# Phase 64: sealed static-image format and W^X loader design M0 gate

## Status

**Status: VERIFIED.** Phase 64 defines a deliberately small, non-ELF input format for a future SeLinOS static-image mapper. It is a parser and policy decision gate only. It neither allocates frames nor maps pages, changes permissions, resumes a task, transfers control, reads ELF, applies relocations, creates a process, or exposes a Linux ABI. The deterministic host parser-negative suite, SHA-bound QEMU TCG transcript, independent verifier, 36-profile rebuild and 70-standalone-verifier regression passed.

The format is necessary because the existing x86 runtime work can verify an ELF metadata parser but cannot claim safe execution merely from parser acceptance. The historic loader checkpoint records that normal x86 mapping attributes were not, at that point, an auditable executable-permission interface; later Phase 43 and Phase 62/63 proofs establish bounded NX and executable-fetch primitives but not a general loader lifecycle. [1] [2]

> A Phase 64 acceptance means only that a fixed byte sequence satisfies the sealed-image grammar and all policy checks. It does **not** mean that the sequence is mapped or executable.

## Format grammar

The canonical wire format is little-endian, fixed-width and bounded to a single 4 KiB page. It is named **SeLin Sealed Image version 1** (`SSIM-v1`). All integer arithmetic must be overflow-checked before ranges are evaluated.

| Field | Offset | Width | Required value or policy |
|---|---:|---:|---|
| Magic | `0x00` | 8 bytes | ASCII `SELINS64` |
| Version | `0x08` | 2 bytes | `1` |
| Header bytes | `0x0a` | 2 bytes | exactly `64` |
| Segment count | `0x0c` | 2 bytes | exactly `1` for M0 |
| Reserved | `0x0e` | 2 bytes | exactly `0` |
| Table offset | `0x10` | 4 bytes | exactly `64` |
| Source byte length | `0x14` | 4 bytes | in `[1, 4096]` |
| Entry virtual address | `0x18` | 8 bytes | low-canonical page-aligned `0x60000000` |
| Source SHA-256 | `0x20` | 32 bytes | digest of exactly the declared source bytes |
| Segment record | `0x40` | 32 bytes | one record as below |

The sole segment record contains `virtual_address` (`u64`), `file_offset` (`u32`), `file_bytes` (`u32`), `memory_bytes` (`u32`), `permissions` (`u16`), `reserved` (`u16`) and `reserved_tail` (`u64`). The record is valid only when its virtual address equals the header entry, its file offset is after the fixed header plus table, `file_bytes` is non-zero, `file_bytes <= memory_bytes`, all source ranges remain within `source_byte_length`, all reserved fields are zero, and the declared source SHA-256 matches. M0 requires `memory_bytes == file_bytes` and a single executable/read-only segment.

| Permission bit | Name | M0 policy |
|---:|---|---|
| `0x1` | read | required |
| `0x2` | write | forbidden |
| `0x4` | execute | required |
| other | reserved | forbidden |

The parser accepts only the permission mask `0x5` (`R|X`). Therefore every valid Phase 64 source image is statically W^X at the grammar boundary: a writable executable segment is rejected before any mapping exists. The M0 address is fixed to the previously evidenced low-canonical entry page, but that reuse does not extend the prior executable-fetch proof.

## Source-hash ledger

The source hash is a SHA-256 digest of bytes `[0, source_byte_length)`, with the 32 digest bytes inside the header treated as zero for the digest calculation. This avoids a self-referential digest while binding the header, segment table and payload. The fixture generator records the expected final wire image digest and the deterministic zeroed-digest source digest. The independent verifier will recompute both from the checked-in fixture and check the QEMU transcript’s acceptance/rejection ledger.

| Ledger item | Bound object | Purpose |
|---|---|---|
| `wire_sha256` | exact fixture byte stream | Binds the bytes presented to the parser. |
| `source_sha256` | image with digest field zeroed | Binds all protected grammar and payload bytes without a circular dependency. |
| `payload_sha256` | declared segment source range | Separates program-byte provenance from container metadata. |
| evidence SHA-256 bindings | source, protocol, server, probe, image and transcript | Binds the proof implementation and QEMU artifact. |

## Required M0 transaction

A default-OFF `SeLinSealedStaticImageProbe=ON` profile uses a status-only parser service. A fixed service-local fixture buffer is parsed only as bytes; no parser capability, TCB, CNode, PML4, frame, mapping or task is created by this transaction. One exact valid request produces a one-record accepted ledger. A duplicate request receives a distinct, fixed rejection code. The service retains only the accepted record count and no parsed pointer after reply.

The verified QEMU TCG proof covers one positive image plus independent negative inputs. It rejects an altered payload digest, a W+X permission mask, a non-canonical entry address, a non-zero reserved field, a table range outside declared source bytes, and a duplicate request. Every parser rejection occurs before any loader mapping or target-task action; the transcript, source guard checks and independent verifier bind this boundary.

## Security invariants

| Invariant | Enforcement in M0 |
|---|---|
| Bounded parsing | Fixed 4 KiB maximum, exact header/table sizes, checked addition and fixed single-record count. |
| Canonical provenance | Fixed low-canonical, page-aligned entry VA matching the only segment VA. |
| Static W^X | `R|X` exact mask; write and unknown bits are rejected. |
| Seal integrity | Deterministic SHA-256 ledger over zeroed-digest source bytes and separately checked payload range. |
| Fail closed | Any malformed field returns a rejection code; parsing constructs no kernel object and retains no source pointer. |
| Replay boundary | A second request is refused independently of payload validity. |

## Explicit non-claims

Phase 64 does not prove a generic image format, object storage, loader allocation, frame ownership, page mapping, permission transition, entry transfer, stack construction, C runtime, ELF, static linking, dynamic linking, relocation, syscall ABI, task lifecycle, Linux application execution, `dpkg`, `apt`, or Linux driver/KABI compatibility. It also does not change the Phase 49 DMA-containment blocker.

## References

[1]: `docs/phase21_elfrt_m2_wx_blocker_checkpoint_2026-08-14.md` — prior normal x86 W^X loader blocker and non-executing boundary.
[2]: `docs/phase63_reply_terminal_fault_m0_gate.md` — verified one VM-fault repair/restart to terminal user exception.
