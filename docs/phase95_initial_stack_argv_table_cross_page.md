# Phase 95 — Initial-stack `argv` Table and Cross-page NUL Parsing

**Status:** IMPLEMENTED and build-verified. This phase extends the Phase 94 one-string checker with a bounded `argv` pointer-table parser and an explicit cross-page reader policy.

## Contract

The parser receives a table base, an authorized table-region size, `argc`, a validated policy, and a reader callback. It reads exactly `argc + 1` pointer entries and requires `argv[argc] == NULL`. Every pointer word and every string byte is obtained through the callback; the parser does not perform unchecked direct dereferences for table or string data.

```c
enum selinos_initial_stack_argv_status
selinos_initial_stack_argv_parse_table(
    uintptr_t table_base,
    size_t table_region_bytes,
    size_t argc,
    const struct selinos_initial_stack_policy *policy,
    selinos_initial_stack_argv_read_byte_fn reader,
    void *reader_context,
    struct selinos_initial_stack_argv_table_result *result);
```

The policy contains independent `max_argc`, `max_envc`, `max_string_bytes`, `page_bytes`, and `allow_cross_page_strings` fields. The parser checks `argc <= max_argc`, checked `argc + 1`, checked multiplication by `sizeof(uintptr_t)`, table-region bounds, and every address addition before reading.

| Policy | Behavior |
|---|---|
| `allow_cross_page_strings=true` | The reader may resolve consecutive bytes on different pages; the reader remains responsible for mapping and authority checks. |
| `allow_cross_page_strings=false` | The parser stops before the first page boundary and returns `CROSS_PAGE_DISABLED`. |
| `argv[argc] != NULL` | Returns `TABLE_NOT_TERMINATED` and records the failing index. |
| A reader failure | Returns `READER_FAULT` and records the failing string/table index. |
| No NUL before `max_string_bytes` | Returns `LIMIT_EXCEEDED`. |

The result record reports the validated `argc`, number of strings checked, total bytes read including each NUL, and first failure index. The parser does not remap pages, recover faults, or modify capabilities.

## Cross-page fixture

The unit test uses a virtual byte-map reader. The first string starts at virtual address `0x1026` with an eight-byte page size, so `selinos\0` crosses a page boundary. With cross-page support enabled, two strings and the NULL table sentinel validate successfully. With support disabled, the first string returns `CROSS_PAGE_DISABLED`. Replacing the final table entry with a nonzero pointer returns `TABLE_NOT_TERMINATED`.

## Verification

```bash
python3 tools/verify_initial_stack_argv_contract.py
gcc -std=c11 -Wall -Wextra -Werror -pedantic \
  -Isrc/projects/helixos/include \
  src/projects/helixos/src/selinos_initial_stack_argv.c \
  tests/initial_stack_argv_test.c \
  -o /tmp/selinos_initial_stack_argv_test
/tmp/selinos_initial_stack_argv_test
cmake -S src -B build-phase95-argv-table -GNinja \
  -DSeLinInitialStackArgvTableM3=ON
cmake --build build-phase95-argv-table --parallel 2
```

The independent verifier checks the contract statuses, reader API, checked arithmetic, page policy, table terminator cases, cross-page unit fixture, and default-OFF CMake gate. The build completed with the existing non-fatal linker warnings about `.note.GNU-stack` in the toolchain objects.

## Explicit non-claims

This phase does not validate an externally supplied Linux initial stack, provide hardware page-fault recovery, prove atomicity against concurrent mutation, implement `envp`, parse `auxv`, copy arguments, perform path lookup, replace a process, implement general `execve`, or establish Linux ABI or Debian 13.6 compatibility. Cross-page support is safe only when the supplied reader enforces the actual mapping and authority boundary.
