# Phase 96 — Initial-stack `envp` Table Parser

**Status:** IMPLEMENTED and unit/build-verified. This phase adds a separate bounded parser for an `envp` pointer table, using the same authorized byte reader as the `argv` parser but applying the independent `max_envc` policy limit.

## Contract

The parser reads exactly `envc + 1` pointer words and requires `envp[envc] == NULL`. It then scans each environment string until NUL, subject to `max_string_bytes` and `allow_cross_page_strings`. It returns a separate `selinos_initial_stack_envp_table_result` with `envc`, `strings_checked`, `total_bytes_read`, and the first failure index.

```c
enum selinos_initial_stack_argv_status
selinos_initial_stack_envp_parse_table(
    uintptr_t table_base,
    size_t table_region_bytes,
    size_t envc,
    const struct selinos_initial_stack_policy *policy,
    selinos_initial_stack_argv_read_byte_fn reader,
    void *reader_context,
    struct selinos_initial_stack_envp_table_result *result);
```

The parser is intentionally implemented through a shared internal bounded table routine. The public `argv` wrapper selects `policy->max_argc`; the public `envp` wrapper selects `policy->max_envc`. Therefore, an oversized `envc` cannot be accepted because `max_argc` happens to be larger.

| Case | Result |
|---|---|
| `envp[envc] == NULL` and all strings terminate | `TABLE_VALIDATED`. |
| `envc > max_envc` | `COUNT_LIMIT_EXCEEDED` before any table read. |
| Nonzero `envp[envc]` | `TABLE_NOT_TERMINATED` with `failure_index == envc`. |
| Environment string crosses a page and policy allows it | Reader resolves bytes across the boundary. |
| Environment string crosses a page and policy disallows it | `CROSS_PAGE_DISABLED` before the cross-page byte. |
| Reader cannot authorize a pointer/table byte | `READER_FAULT`. |

## Tests and build

The host-side virtual memory fixture contains an independent `envp` table with one `K=V\0` entry and its own NULL sentinel. The test confirms successful parsing, four bytes read including NUL, and rejection of `envc=2` when `max_envc=1`. The existing argv cross-page and sentinel tests remain active.

```bash
python3 tools/verify_initial_stack_argv_contract.py
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Isrc/projects/helixos/include \
  src/projects/helixos/src/selinos_initial_stack_argv.c \
  tests/initial_stack_argv_test.c \
  -o /tmp/selinos_initial_stack_argv_test
/tmp/selinos_initial_stack_argv_test
cmake -S src -B build-phase96-envp -GNinja \
  -DSeLinInitialStackEnvpM4=ON
cmake --build build-phase96-envp --parallel 2
```

## Explicit non-claims

This is an internal bounded `envp` table parser. It does not implement Linux environment inheritance, `getenv`, mutation, secure-exec rules, environment expansion, `auxv`, process replacement, general `execve`, libc, POSIX, Linux ABI, or Debian 13.6 compatibility. The callback must enforce the actual mapping and capability authority boundary; the parser does not catch or repair unsafe hardware faults.
