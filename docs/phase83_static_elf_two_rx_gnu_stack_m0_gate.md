# Phase 83 Gate — static ELF64 two-RX plus `PT_GNU_STACK` M0

**Status:** VERIFIED — isolated parser-backed fixture, QEMU TCG runtime proof and SHA-bound evidence completed; full regression and publication remain pending.

## Objective

Phase 83 extends the published Phase 82 fixed four-`PT_LOAD` ELF64 proof by adding exactly one fixed fifth program header: `PT_GNU_STACK` (`p_type=0x6474e551`) with `PF_R|PF_W`, no `PF_X`, zero file/memory extent and zero alignment. The fixture retains the exact Phase 82 entry `RX` jump, second `RX` execution page, `RO+NX` constant page, `RW+NX` data/BSS page, one resume and terminal `UD2` on the second RX page.

The ELF parser must no longer silently ignore this metadata. It must recognise no more than one `PT_GNU_STACK`, reject malformed or duplicate records, record its flags in a bounded summary, and preserve existing rejection of writable-plus-executable `PT_LOAD` segments, interpreter, dynamic linkage and relocation evidence. The Phase 83 fixture validator must require the recorded non-executable `PF_R|PF_W` stack request. The root transaction must continue to map its actual target stack `RW+NX`; it must not infer executable stack permission from untrusted metadata.

| Contract element | Required fixed value | Rejected state |
|---|---|---|
| Program-header count | 5 | Any count other than the fixture’s five headers |
| Stack metadata type | `0x6474e551` | Unknown substitution, duplicate or malformed `PT_GNU_STACK` |
| Stack metadata flags | `PF_R|PF_W` | `PF_X`, absent read/write combination or broad flag policy |
| Stack metadata extent | `p_offset=p_vaddr=p_paddr=p_filesz=p_memsz=0` | File-backed, memory-bearing or address-bearing stack metadata |
| Target stack mapping | `RW+NX` | Executable target stack, W+X mapping or metadata-driven permission escalation |
| Terminal execution | Second RX `UD2` at `0x60003046` | Fault reply, repair, retry, additional resume or alternate terminal path |

## Evidence threshold

Verification requires an isolated default-OFF CMake profile, a QEMU TCG serial transcript containing explicit `PT_GNU_STACK`/NX-stack markers and the Phase 82 terminal witness, a SHA-bound manifest, an independent verifier, zero mismatches in the comprehensive binding audit, and a complete standalone verifier regression.

## Non-claims

Phase 83 does **not** prove arbitrary program-header support, arbitrary `PT_GNU_STACK` values, stack allocation policy, stack growth, guard pages, process ABI initial stacks, auxiliary vectors, `ET_DYN`, ASLR, relocations, `PT_INTERP`, dynamic linking, C runtime startup, Linux system-call ABI compatibility, Linux driver/KABI compatibility, Debian package execution, `dpkg`, `apt`, filesystem loading, or any Linux-kernel component. It is a single embedded metadata and mapping-policy proof running on seL4 as the only privileged kernel.
