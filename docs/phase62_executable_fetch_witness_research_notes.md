# Phase 62: executable instruction-fetch witness M0 — research notes

## Objective

Phase 62 will be the first gate to claim a deliberately tiny amount of target instruction execution, but not general process execution. It will reuse the Phase 61 canonical `rip=0x60000000` and ABI-aligned `rsp=0x70002ff8` provenance topology. Root will initialize exactly three bytes in a dedicated ordinary entry frame: `0x90` (`nop`) followed by `0x0f 0x0b` (`ud2`). The target entry mapping alone will lose the x86 execute-disable attribute; the stack remains NX and no ELF, loader, arguments, return sentinel, or stack image exists.

`nop` has no operand, memory, capability, or stack side effect. `ud2` explicitly raises the x86 invalid-opcode exception. Its faulting instruction pointer should be `0x60000001`, which is one byte after the initial `nop`. Root will receive one `seL4_Fault_UserException`, require the saved fault IP and stack pointer to match the fixed values, require exception vector `6`, and deliberately withhold the reply. Therefore the target remains blocked with no continuation.

> A `nop; ud2` evidence record proves the one-byte `nop` was fetched and completed before the deliberate invalid-opcode trap. It does not prove C runtime startup, arbitrary user-code safety, ELF compatibility, a process lifecycle, Linux ABI behavior, or package-manager functionality.

## Pinned local ABI

The x86_64 seL4 user-exception fault message contains, in order, `FaultIP`, `SP`, `FLAGS`, `Number`, and `Code`. The Phase 61 target PML4/VSpace construction and fixed frame mapping helper pattern remain the source baseline. The existing Phase 43 NX proof provides separately verified execute-disable and executable-control mapping behavior.

## References

[1]: https://docs.sel4.systems/Tutorials/fault-handlers.html "seL4 Fault handling tutorial"
[2]: https://www.felixcloutier.com/x86/ud "UD — Undefined Instruction"
[3]: `src/kernel/libsel4/sel4_arch_include/x86_64/sel4/sel4_arch/constants.h` — pinned x86_64 user-exception message indices
[4]: `src/projects/helixos/src/x86_nx_mapping_probe.c` — verified local x86 NX/executable mapping pattern
