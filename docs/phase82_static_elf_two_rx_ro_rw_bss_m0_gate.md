# Phase 82 Gate — static ELF64 two-RX plus RO/RW+BSS load M0

**Status:** VERIFIED — isolated build, QEMU TCG runtime proof and SHA-bound independent verification completed.

## Objective

Phase 82 is a deliberately narrow extension of the published Phase 81 static ELF64 materialisation proof. It must accept and execute **one exact, embedded, little-endian x86_64 `ET_EXEC` ELF64 fixture** with four page-aligned `PT_LOAD` records: a five-byte entry `RX` page at `0x60000000`, a `RO+NX` constant page at `0x60002000`, a `RW+NX` initialised-data-plus-BSS page at `0x60001000`, and a second `RX` page at `0x60003000`.

The entry page must contain exactly one direct relative jump to the second `RX` page. The second `RX` page must read the fixed `RW` and `RO` values, confirm one zero BSS byte, write and reread one BSS byte, and terminate at a fixed `UD2`. The only accepted runtime outcome is one unreplied `UserException` at `0x60003046` with the Phase 82 badge. The entry jump and terminal `UD2` make successful instruction fetch from **both separately mapped executable `PT_LOAD` pages** necessary for acceptance.

## Required transaction

| Step | Required evidence-bound action | Forbidden outcome |
|---|---|---|
| 1 | Exact fixture bytes are compared before ELF metadata parsing. | Variable path, external file, `ET_DYN`, interpreter, dynamic tags, relocations, or any mutable fixture input. |
| 2 | Root allocates separate frames for entry RX, second RX, RO, RW+BSS, IPC and stack. | Frame sharing between distinct segment roles. |
| 3 | Root privately maps each payload frame writable, zeroes it, copies the exact payload, verifies copied bytes and verifies the BSS tail is zero. | Mapping target payload writable or retaining a root-private writable alias after materialisation. |
| 4 | Root unmaps all writable aliases before mapping the target: both RX frames read/execute, RO read-only NX, RW read/write NX, stack read/write NX. | W+X target mapping, executable data/RO/stack, or target execution before all mappings are established. |
| 5 | Root writes and reads back the fixed x86_64 entry/stack context, performs exactly one resume, then accepts only the fixed terminal `UserException`. | Fault reply, repair, retry, additional resume, secondary execution or teardown claim. |

## Exact fixture policy

The validator must reject every byte sequence except the Phase 82 fixture and must additionally require the following parsed facts:

| Property | Required value |
|---|---|
| ELF identity | ELF64, little-endian, x86_64, `ET_EXEC` |
| Program-header count | Exactly 4 |
| Load-segment counts | Exactly 4 `PT_LOAD`; exactly 2 executable; exactly 1 writable; exactly 0 writable-and-executable |
| Entry segment | `p_offset=0x1000`, `p_vaddr=0x60000000`, `PF_R|PF_X`, 5 bytes, direct jump to `0x60003000` |
| RO segment | `p_offset=0x2000`, `p_vaddr=0x60002000`, `PF_R`, 4 bytes |
| RW+BSS segment | `p_offset=0x3000`, `p_vaddr=0x60001000`, `PF_R|PF_W`, `p_filesz=4`, `p_memsz=64` |
| Second RX segment | `p_offset=0x4000`, `p_vaddr=0x60003000`, `PF_R|PF_X`, 72 bytes, terminal `UD2` at relative offset 70 |
| Unsupported ELF features | No interpreter, dynamic table, needed entries, RELA, text relocations, relocation, loader or runtime support |

## Evidence required for verification

The milestone may be marked **VERIFIED** only after an isolated default-OFF CMake profile builds, a QEMU TCG transcript includes the Phase 82 transaction markers and terminal fault at `0x60003046`, an independent verifier validates source invariants and transcript markers, and a SHA-bound manifest binds source, protocol, fixture, image and transcript.

## Non-claims

Phase 82 does **not** prove arbitrary multi-segment ELF loading, arbitrary program-header ordering, page-sized or multi-page segment support, `ET_DYN`, ASLR, relocations, `PT_INTERP`, dynamic linking, an ELF C runtime, Unix process semantics, Linux system-call ABI compatibility, Linux driver/KABI compatibility, Debian package execution, `dpkg`, `apt`, filesystem loading, or any Linux-kernel component. It is a single embedded four-segment executable proof running on seL4 as the only privileged kernel.
