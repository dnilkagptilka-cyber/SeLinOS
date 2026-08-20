# Phase 81 — Static ELF64 RX/RO/RW+BSS Load M0 Gate

**Status: VERIFIED — isolated QEMU witness and independent SHA-bound verification gate prepared.**

Phase 81 is a deliberately bounded x86_64/PC99 QEMU TCG proof. It accepts exactly one embedded ELF64 `ET_EXEC` fixture with three page-aligned, non-overlapping `PT_LOAD` records. The transaction maps a fixed RX text page, a separate RO+NX constant page, and a separate RW+NX initialized-data/BSS page into one target, then performs a single resume and accepts only the fixed terminal `UD2` user exception.

| Segment | File offset | Virtual address | `p_filesz` / `p_memsz` | ELF flags | Target mapping |
|---|---:|---:|---:|---|---|
| Text | `0x1000` | `0x60000000` | `72 / 72` | `PF_R | PF_X` | Read/execute, non-writable |
| Read-only data | `0x2000` | `0x60002000` | `4 / 4` | `PF_R` | Read-only plus execute-disable |
| Writable data/BSS | `0x3000` | `0x60001000` | `4 / 64` | `PF_R | PF_W` | Read/write plus execute-disable |

The fixed text checks the initialized RW dword, checks one zero-filled BSS byte, checks the independently mapped RO dword, writes and rereads one BSS byte, and then executes `UD2` at fixed text offset `70`. Any check failure branches to the fixed `INT3` path. Root-private writable aliases are unmapped before the target receives text, RO, or RW mappings. The terminal `UserException` must have badge `0x81`, instruction pointer `0x60000046`, stack pointer `0x70002ff8`, and invalid-opcode vector `6`.

> The evidence goal is **not** general ELF support. It is an isolated proof that one byte-pinned three-segment static fixture can be validated, initialized, mapped with the stated fixed permissions, and driven through one bounded terminal control-flow path.

| Required condition | Rejection condition |
|---|---|
| `ET_EXEC`, `EM_X86_64`, three exact `PT_LOAD` records | Any fixture byte or ELF geometry difference |
| One writable and one executable segment, zero writable/executable segments | Writable target text or executable target RO/RW data |
| Exact RO initialized value, RW initialized value, and 60-byte BSS zero region | Nonzero BSS, data mismatch, `INT3`, VM fault, reply, retry, or second resume |
| Terminal `UD2` `UserException` only | Any terminal label, badge, IP, SP, or vector mismatch |

This phase does **not** claim acceptance of arbitrary `PT_LOAD` layouts, `ET_DYN`, relocations, ELF interpreter support, dynamic linking, C runtime initialization, system calls, process lifecycle, Linux ABI compatibility, Debian package execution, `dpkg`, or `apt`.
