# Phase 94 — Initial-stack `argv[0]` NUL Contract

**Status:** IMPLEMENTED and build-verified. This phase introduces the first reusable pure-C validation primitive for one already-authorized `argv[0]` region. It is an internal SeLinOS contract and is not a Linux ABI declaration.

## Contract

The caller provides a pointer to an authorized mapped byte region, the size of that region, and a maximum number of bytes to inspect. The checker reads only the intersection of those two bounds and stops at the first NUL. It returns the byte length excluding NUL and the number of bytes read including NUL.

```c
enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_find_nul(const uint8_t *argv0,
                                    size_t region_bytes,
                                    size_t max_string_bytes,
                                    struct selinos_initial_stack_argv_result *result);
```

The initial-stack policy additionally records `stack_base`, `stack_bytes`, `max_argc`, `max_envc`, and `max_string_bytes`. Policy validation rejects zero-sized regions or limits, an overflowing `stack_base + stack_bytes`, and pointer-table limits that would overflow `count * sizeof(uintptr_t)`.

| Status | Meaning |
|---|---|
| `FOUND_NUL` | A NUL byte was found within both bounds; `length` excludes it. |
| `LIMIT_EXCEEDED` | The configured string budget ended before NUL, including a zero-byte budget. |
| `INVALID_POINTER` | `argv0` is NULL. |
| `ARITHMETIC_OVERFLOW` | The authorized base plus region size cannot be represented. |
| `REGION_EXHAUSTED` | The authorized region ended before NUL while the configured budget was larger. |
| `INVALID_ARGUMENT` | Result pointer is NULL or the authorized region size is zero. |

## Safety properties

The implementation checks pointer and size preconditions before dereferencing. It performs checked address-range arithmetic, uses a bounded loop, never performs a string-library call, and has no retry, remap, fault recovery, or arbitrary capability behavior. The function does not catch hardware page faults; the caller must provide a genuinely authorized mapped region. The distinction between `LIMIT_EXCEEDED` and `REGION_EXHAUSTED` is intentional: the former indicates policy exhaustion and the latter indicates exhaustion of the caller-authorized region.

## Verification

The following checks pass:

```bash
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Isrc/projects/helixos/include \
  src/projects/helixos/src/selinos_initial_stack_argv.c \
  tests/initial_stack_argv_test.c \
  -o /tmp/selinos_initial_stack_argv_test
/tmp/selinos_initial_stack_argv_test
python3 tools/verify_initial_stack_argv_contract.py
cmake -S src -B build-phase94-stack -GNinja \
  -DSeLinInitialStackArgvNulM2=ON
cmake --build build-phase94-stack --parallel 2
```

The host test covers a terminated string, a limit that ends immediately before NUL, an authorized region without NUL, NULL pointer, zero budget, zero region, a NULL result pointer, valid policy and overflowing stack range. The SeLinOS build gate is `SeLinInitialStackArgvNulM2`, and it remains default-OFF.

## Explicit non-claims

This phase does not parse the initial stack, read `argc`, walk the `argv` pointer table, validate `argv[argc]`, inspect `envp`, support cross-page strings, catch VM faults, copy strings, modify a target address space, implement `execve`, provide a Linux ABI, execute Debian binaries, or establish Debian 13.6 compatibility. It validates one already-authorized byte region only.
