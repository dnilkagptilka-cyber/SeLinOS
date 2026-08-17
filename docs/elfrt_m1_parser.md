# SeLinOS ELF runtime parser M1

**Статус:** завершён и воспроизводимо проверен как freestanding parser component.  
**Граница совместимости:** validates ELF64/x86_64 ET_EXEC and ET_DYN metadata; it does not load, map, relocate, interpret or execute an ELF image.

ELFRT M1 begins Phase 5 with a component that accepts an explicit byte buffer only. It validates ELF identity, little-endian 64-bit format, `EM_X86_64`, `ET_EXEC`/`ET_DYN`, a bounded program-header table, `PT_LOAD` file and memory ranges, page-alignment congruence, a policy-controlled single `PT_INTERP`, and bounded `PT_DYNAMIC` metadata. It discovers `DT_STRTAB`, `DT_STRSZ`, ordered `DT_NEEDED`, `DT_RELA`, and text-relocation indicators but performs no resolution or relocation.

| Validated metadata | M1 behaviour | Deliberately absent |
|---|---|---|
| `PT_LOAD` | Bounds, `filesz ≤ memsz`, max 8 segments, alignment and flag counts | Page allocation/mapping and BSS creation |
| `PT_INTERP` | At most one bounded null-terminated path; policy may reject it | Opening/interpreting loader path |
| `PT_DYNAMIC` | Bounded `DT_*` scan and dependency names from string table | Dependency graph resolution, symbol lookup, relocation, constructors |
| Text relocations / W+X segments | Detects `DT_TEXTREL` / `DF_TEXTREL` and `PF_W|PF_X`; initial-load policy rejects either | Any permission transition or executable mapping |

A real host-built PIC ET_DYN fixture references `puts`, yielding dynamic metadata and one or more `DT_NEEDED` entries. A second real PIE executable fixture carries `PT_INTERP`: the harness confirms parser rejection when interpreter policy is disabled, and metadata-only acceptance when that policy is enabled. It also rejects a truncated ELF buffer. Finally, the harness proves that the initial-load policy rejects detected text-relocation and writable-executable summary states. Neither fixture is injected into a SeLinOS process, interpreted, relocated, loaded or executed in QEMU.

> The System V ABI describes dynamic linking as creating executable and shared-object memory images, relocating them, and then transferring control. `PT_INTERP` names the program interpreter and `PT_DYNAMIC` carries dynamic metadata; therefore parser acceptance alone is not dynamic-linker compatibility.[1]

## Reproducible verification

```bash
cd /home/ubuntu/helixos
./tools/verify_elfrt_m1.py
```

The verifier rebuilds the target freestanding library, recompiles fixture and host harness from source, verifies every source/artifact hash in [`tests/artifacts/selinos_elfrt_m1.verification.json`](../tests/artifacts/selinos_elfrt_m1.verification.json), and requires the parser’s acceptance/rejection marker.

## References

[1]: https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html "System V ABI: Dynamic Linking"
[2]: https://man7.org/linux/man-pages/man5/elf.5.html "Linux elf(5) manual page"
