# Phase 97 (M5): bounded initial-stack `auxv` validation gate

## Status

**Phase 97 M5 is complete as an isolated build and contract gate.** The implementation validates a Linux-style auxiliary-vector sequence through the authorized bounded reader interface. The profile remains default-OFF and is enabled only with `SeLinInitialStackAuxvM5=ON`.

This gate does **not** claim Debian 13.6 compatibility. It proves only the bounded `auxv` parsing contract and its integration into the SeLinOS build profile.

## Scope

The M5 parser consumes `(type, value)` pairs through the existing authorized word-reader callback. It stops only at an entry whose `type` is zero, enforces an independent `max_auxv` entry bound, checks pair-size arithmetic before address formation, rejects zero or unreadable entry addresses, and propagates reader faults without dereferencing untrusted initial-stack memory directly.

The contract distinguishes the following outcomes:

| Outcome | Meaning |
|---|---|
| `SELINOS_INITIAL_STACK_ARGV_AUXV_VALIDATED` | A bounded sequence reached `AT_NULL` (`type == 0`) successfully. |
| `SELINOS_INITIAL_STACK_ARGV_AUXV_LIMIT_EXCEEDED` | The independent auxiliary-vector entry bound was exhausted before `AT_NULL`. |
| `SELINOS_INITIAL_STACK_ARGV_AUXV_READER_FAULT` | The authorized reader rejected an entry read. |
| `SELINOS_INITIAL_STACK_ARGV_AUXV_NOT_TERMINATED` | The sequence did not satisfy the required termination contract. |

## Evidence

The following checks passed in a fresh build directory, after bootstrapping the pinned seL4/seL4_libs sources:

| Check | Result |
|---|---|
| CMake configure | Passed for `SeLinInitialStackAuxvM5=ON`. |
| Full Ninja build | Passed: all 284 build steps completed and `images/selinos-root-image-x86_64-pc99` was generated. |
| Generated M5 selector | `CONFIG_SELINOS_INITIAL_STACK_AUXV_M5_PROBE 1`. |
| Generated M2 selector | Disabled in the M5-only profile. |
| Generated M3 selector | Disabled in the M5-only profile. |
| Generated M4 selector | Disabled in the M5-only profile. |
| Independent static verifier | `python3 tools/verify_initial_stack_argv_contract.py` passed. |
| Host unit tests | Compiled with `-std=c11 -Wall -Wextra -Werror` and exited successfully. |

The clean build directory was `build-phase97-auxv`. The build output ended with the rootserver link, image generation, and bootable ELF conversion. The linker emitted only the existing `.note.GNU-stack` deprecation warning; it did not fail the build.

## Selector interpretation

M2, M3, M4, and M5 are intentionally separate CMake options. The M5 profile is an isolated default-OFF probe, so the generated configuration contains only the M5 selector when configured with `-DSeLinInitialStackAuxvM5=ON`. The earlier selectors remain present in the generated configuration as disabled entries; this demonstrates that the options were not removed or silently renamed.

## Security boundary

The parser is a contract-level validator, not an ABI-complete process loader. It does not establish that the complete initial stack belongs to a Debian process, does not authorize arbitrary memory, and does not prove kernel/userland interoperability. The reader callback remains responsible for capability- or mapping-based authorization and for fault containment. All parser limits and pointer arithmetic remain bounded before reads are attempted.

## Reproduction

```sh
cd SeLinOS-git
cmake -S src -B build-phase97-auxv -GNinja \
  -DSeLinInitialStackAuxvM5=ON
cmake --build build-phase97-auxv --parallel 2
python3 tools/verify_initial_stack_argv_contract.py
gcc -std=c11 -Wall -Wextra -Werror \
  -I src/projects/helixos/include \
  tests/initial_stack_argv_test.c \
  src/projects/helixos/src/selinos_initial_stack_argv.c \
  -o /tmp/selinos_initial_stack_argv_test
/tmp/selinos_initial_stack_argv_test
```

## Commit gate

The implementation, tests, verifier, CMake profile, and this document are ready to be committed together. The next evidence-gated increment should add runtime execution evidence for the `auxv` witness; it must remain separate from any claim of full Debian compatibility.

## Source files

- `src/projects/helixos/include/selinos_initial_stack_argv.h`
- `src/projects/helixos/src/selinos_initial_stack_argv.c`
- `tests/initial_stack_argv_test.c`
- `tools/verify_initial_stack_argv_contract.py`
- `src/projects/helixos/CMakeLists.txt`
- `docs/phase97_initial_stack_auxv_m5_gate.md`

## Hashes at gate creation

```text
src/projects/helixos/CMakeLists.txt
b35fde1b5680b2ba32e66647fdba34d483b02b05aa3a7ddac1b9bd333dfc171c

src/projects/helixos/src/domain_manager.c
df870c0c0654f69e890cca8462dffb09d2babab6f2678e2f03c89c9603429c6a
```

The `domain_manager.c` hash is recorded for traceability; this M5 increment does not modify that file.
