# Phase 98: Debian 13.6 compatibility boundary and runtime `auxv` witness

## Gate status

**Phase 98 M6 passed its bounded runtime witness gate.** SeLinOS now has an isolated, default-OFF runtime profile that exercises the pure-C `auxv` parser in the root task under QEMU. The profile validates a bounded `(type, value)` sequence through an authorized reader callback and emits a deterministic serial transcript.

This is **not** a claim of Debian 13.6 compatibility. It is evidence for one process-initialization sub-contract that must precede a real Debian userland execution path.

## What was implemented

The new `SeLinInitialStackAuxvRuntimeM6` option adds `selinos_initial_stack_auxv_runtime_m6.c` only when explicitly enabled. The witness uses a local fixed table containing `AT_PHDR`, `AT_ENTRY`, and `AT_NULL` pairs. The parser receives the table through a reader that authorizes each byte only when the address falls inside the declared bounded region. The witness then checks that three entries were examined and that the final pair is `(0, 0)`.

The runtime fixture is intentionally synthetic. It does not pretend that the root task has already received a Linux process initial stack, and it does not bypass the authority boundary by directly dereferencing an untrusted address.

| Layer | Phase 98 evidence |
|---|---|
| Parser contract | Existing M5 bounded `auxv` parser remains the only parser used. |
| Authority boundary | M6 callback checks base and length before each byte read. |
| Runtime integration | Root main calls the witness only under `CONFIG_SELINOS_INITIAL_STACK_AUXV_RUNTIME_M6_PROBE`. |
| Isolation | M6 is `DEFAULT OFF`; a default configure generates the M5 and M6 selectors as disabled. |
| Runtime output | QEMU serial transcript records validated entry count and `AT_NULL` values. |

## Build and runtime evidence

A clean M5+M6 profile was configured and built in `build-phase98-m6` with seL4/seL4_libs dependencies already pinned by the project bootstrap. The build completed all 312 Ninja steps and generated the PC99 root image. The generated configuration contained both `CONFIG_SELINOS_INITIAL_STACK_AUXV_M5_PROBE 1` and `CONFIG_SELINOS_INITIAL_STACK_AUXV_RUNTIME_M6_PROBE 1`.

The QEMU command used `qemu-system-x86_64 -cpu max -nographic -serial mon:stdio -m size=1G` with the generated kernel and root image. QEMU remained live until the controlled 12-second timeout (`rc=124`), and the transcript contained the following success markers:

```text
SeLinOS Phase 98 M6: auxv runtime witness begin.
SeLinOS Phase 98 M6: auxv runtime witness validated. entries=0x3 at_null_type=0x0 at_null_value=0x0
SeLinOS M0: root task entering idle/yield loop.
```

The committed transcript is `tests/artifacts/selinos_initial_stack_auxv_runtime_m6.qemu.log` with SHA-256:

```text
8f0a3a1e290d6a42d6865d2f245f39f6b6c939fa6cd1d8d5a08d25ab35190214
```

The first run exposed a valid contract failure rather than a hidden fault: the shared policy validator requires nonzero `max_argc` and `max_envc` limits even for auxv-only parsing. The witness was corrected to provide those independent limits, rebuilt, and rerun. The successful transcript above is the only runtime evidence retained for the gate.

## Regression checks

The independent M6 verifier passed, the existing M2–M5 initial-stack verifier passed, and the host unit tests compiled with `-std=c11 -Wall -Wextra -Werror` and exited successfully. A default CMake configure produced disabled selectors for both M5 and M6, confirming that the new runtime experiment does not activate implicitly.

## Debian 13.6 compatibility boundary

Debian’s official release information identifies Debian 13.6 as the sixth update of Debian 13 (trixie), released on July 11, 2026. The official architecture list includes amd64.[1] The Debian point-release announcement also states that a point release updates packages and is not a new major Debian version.[2]

Those facts define the target, but they do not imply that a seL4-based system can execute Debian binaries. For SeLinOS, compatibility remains a staged engineering claim requiring at least an amd64 ELF loader, correct process entry and initial-stack construction, the required memory and file services, a sufficiently compatible syscall surface, signal/thread behavior, dynamic linking support, and runtime validation against actual Debian 13.6 userland artifacts.

The Phase 98 result covers only the parser-side `auxv` termination and reader-boundary sub-contract. It does not yet prove that `AT_PHDR`, `AT_ENTRY`, `AT_PHNUM`, `AT_BASE`, `AT_RANDOM`, `AT_EXECFN`, or hardware capability values are produced correctly for a loaded Debian process. It also does not prove `getauxval()` behavior, dynamic linker startup, libc initialization, or any Debian package execution.

## Next evidence-gated increment

The next increment should construct the first authorized initial-stack image for a controlled loaded ELF fixture and compare its emitted `auxv` entries against the loader’s sealed ELF metadata. Only after that witness passes should the project attempt a minimal dynamically linked Debian-compatible process. The compatibility claim should remain limited to each demonstrated ABI and service surface until a complete Debian 13.6 boot-and-userland test exists.

## References

[1]: https://www.debian.org/releases/trixie/ "Debian ‘trixie’ Release Information"
[2]: https://www.debian.org/News/2026/20260711 "Updated Debian 13: 13.6 released"
