# Phase 80 — static ELF64 RX plus RW/BSS load M0 gate

**Status: VERIFIED**

## Narrow contract

This isolated x86_64/PC99 QEMU TCG proof accepts exactly one embedded, byte-pinned ELF64 little-endian `ET_EXEC` fixture. Its two page-aligned `PT_LOAD` records are deliberately fixed: a 52-byte `PF_R|PF_X` text segment at `0x60000000`, and a separate `PF_R|PF_W`, non-executable data segment at `0x60001000` with `p_filesz=4` and `p_memsz=64`. The latter therefore has exactly 60 bytes of bounded BSS tail in this proof.

The root task parses the fixture with interpreter loading disabled, requires the existing initial no-W+X policy, writes text and data only through distinct root-private writable aliases, zeros each page before copying its declared initialized payload, confirms the initialized four-byte data word and every declared BSS byte, and unmaps both aliases before configuring the target mappings. The target receives text as read/execute and data plus stack as read/write with x86 execute-disable. The target has one resume only.

The fixed text payload reads the initialized data word, confirms the first BSS byte is zero, writes the fixed byte `0xa5` to that BSS byte, rereads it, and then executes the one terminal `UD2`. The root accepts only the corresponding unreplied `UserException` at the pinned text address plus offset `50`, with the configured badge, stack pointer and x86 invalid-opcode vector. A failed payload branch reaches a distinct `INT3` path and therefore cannot satisfy the success terminal check.

## Evidence environment

| Property | Fixed value |
|---|---|
| Platform | x86_64 / PC99 |
| Emulator | QEMU 8.2.2 TCG with `-cpu max` |
| CMake selector | `SeLinStaticElfRxRwBssM0=ON` |
| Default value | `OFF` |
| Kernel | seL4 pinned revision `1326364bc9135d9445d936ebc01e38a402c1f4c6` |
| Runtime bound | 12 seconds; expected timeout exit `124` after unreplied terminal fault |

## Explicit non-claims

This M0 proof does **not** implement arbitrary `PT_LOAD` processing, file I/O, page-sized or multi-page data segments, `ET_DYN`, ASLR, relocations, `PT_INTERP`, a dynamic linker, TLS, argv/envp/auxv construction, a C runtime, system calls, process lifecycle, threads, `fork`, signals, VFS execution, package installation, `dpkg`, `apt`, Linux ABI compatibility, Debian package compatibility, Linux kernel, Linux VM, LKL, DMA containment, IOMMU functionality, or hardware validation outside the named QEMU TCG fixture.
