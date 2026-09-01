# Phase 93 — Execve Reply `argv[0]` Fixed-NUL M1 Gate

**Status:** VERIFIED. Phase 93 extends the verified Phase 92 M0 by one additional bounded byte read. The isolated x86_64/PC99 target preserves the `argv[0]` pointer in `RAX`, reads its first byte into `RCX`, reads exactly one fixed NUL sentinel at `argv[0] + 7` into `RDX`, and terminates at `ud2`.

## M1 contract

The self-authored stack contains the seven-byte string `selinos` at `0x70002f00`; the next byte remains zero because the complete page is cleared before the seven characters are written. The M1 witness is:

```text
mov    rax, [rsp + 8]
movzx  ecx, byte ptr [rax]
movzx  edx, byte ptr [rax + 7]
ud2
```

The terminal invalid-opcode fault is accepted only at interpreter offset `12`, with the fixed fault badge and vector. Root reads the complete faulted context and requires `RAX=0x70002f00`, `RCX=0x73`, and `RDX=0x00`. The existing whole-context bridge still changes only fixed `RIP` and `RSP`; the normal 18-word syscall reply frame is not used to claim stack restoration.

| Verified boundary | Explicitly not established |
|---|---|
| One fixed pointer load from `[rsp+8]`. | Caller-controlled `argv`, `argc` validation, pointer range validation, or multi-element arrays. |
| One fixed first-byte read and one fixed NUL-sentinel read at offset `7`. | A loop, NUL search, string traversal, arbitrary dereference, length calculation, `envp`, or `auxv` parsing. |
| Preservation of pointer in `RAX`, first byte in `RCX`, and sentinel in `RDX`. | General register ABI, normal syscall reply-frame restoration, or a complete initial process stack. |
| One self-authored RX witness under the pinned x86 NXE-enabled seL4 fork. | General W^X, fault recovery, ELF loading, `execve`, process replacement, Linux ABI, or Debian compatibility. |

## Reproducible profile

```bash
./bootstrap_sources.sh
cmake -S src -B build-phase93-m1 -GNinja \
  -DSeLinExecveReplyArgv0StringNulM1=ON
cmake --build build-phase93-m1 --parallel 2
cd build-phase93-m1
./simulate --mem-size=128M --cpu=max --cpu-opt='' \
  --extra-qemu-args='-no-reboot'
```

The runtime record uses QEMU 8.2.2 TCG, `cpu=max`, 128 MiB memory, no KVM, and a bounded 150-second run. The process remains in the existing root idle/yield loop after the witness, so external exit code `124` is expected and is not a test failure. Success is defined by the M1 marker and absence of the fail-closed marker.

## Independent verification

```bash
python3 tools/verify_phase91_reproducible_nx_dependencies.py
python3 tools/verify_execve_reply_argv0_string_nul_m1.py
python3 tools/verify_execve_reply_argv0_string_byte_m0.py
```

The M1 manifest binds the exact source files, generated configuration, kernel and root images, canonical QEMU transcript, gate document, and SHA-256 values. The verifier rejects missing markers, failure markers, changed byte offsets, a second target resume, or an attempt to claim normal reply-frame `SP` restoration.

## Explicit non-claims

Phase 93 does **not** prove a normal 18-word reply-frame restoration, arbitrary `argv` traversal, NUL scanning, bounded-copy implementation, `argc`/`envp`/`auxv` semantics, path lookup, argument copying, general `execve`, process replacement, ELF loading, `PT_INTERP`, a dynamic linker, libc, POSIX or Linux ABI compatibility, Debian binary execution, `dpkg`, `apt`, or Debian 13.6 package compatibility.
