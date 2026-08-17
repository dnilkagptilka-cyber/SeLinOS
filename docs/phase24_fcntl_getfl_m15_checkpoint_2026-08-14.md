# SeLinOS Phase 24 M15 checkpoint

## Verified M14 recovery baseline

Before the second M15 attempt, the M14 recovery baseline was independently reconfirmed. `verify_linux_uname_m14.py` passed; the restored production source SHA-256 values were `f2ac1a6cd48f13a9771b8205fc2c07a4dbc70e880cb3430d0d51877275636f55` for `linux_syscall_probe.c` and `927cb8735cc2fbbbe88b12920ff393ed6ee95bed0c521e59ce27c82d2db308bb` for `domain_manager.c`.

## Completed second attempt

M15 was implemented only after that baseline was confirmed. The pre-implementation handoff note was corrected: a successful `F_GETFL` returns `RAX=0`, so forwarding `RAX` to M10 `fstat` would erroneously pass FD 0. The isolated probe now checks the zero result and explicitly restores FD 6 before the existing M10 `fstat` arguments.

The root admits exactly x86_64 `fcntl` syscall 72 with `RDI=6`, `RSI=3` (`F_GETFL`) and `RDX=0`; it replies `0`. Its branch makes no ROMFS IPC, client-memory mapping or mutable state change. It is placed only between the verified `/selinos-version` open reply and M10 `fstat` gate.

## Binary guard and QEMU proof

The candidate probe passed `verify_m15_probe_object_window.py` against an exact reconstructed M14 source whose SHA-256 is `f2ac1a6cd48f13a9771b8205fc2c07a4dbc70e880cb3430d0d51877275636f55`. The preserved baseline object is `tests/artifacts/selinos_linux_syscall_probe_m14_baseline.o`, SHA-256 `e52bb32c090211403f10906832e04b1f84dbfa799a2d68bf9644c42df2716cf3`. This stable artifact replaced the earlier mutable opt-in build object, which became unsuitable once all profiles were rebuilt.

QEMU TCG emitted the M15 success marker, `SeLinOS ABI M15: fixed immutable FD F_GETFL mediated without VFS IPC.`, and the prior M14 marker in the same run. `verify_linux_fcntl_getfl_m15.py` passed against SHA-bound runtime evidence.

## Final regression state

All six default-OFF opt-in profiles were rebuilt before controlled evidence SHA rebinding. The full default-plus-opt-in independent regression suite passed: **32 verifiers**. The current default production image is `build/images/selinos-root-image-x86_64-pc99` with SHA-256 `2ab45cdd0de7f54672f8b68b49af2bf40e7a3dbef6639caa721d17c297dfbde4`.

M15 claims only the one fixed immutable `fcntl(F_GETFL)` request shape. It does **not** establish general `fcntl`, mutable descriptor or file-status flags, general VFS semantics, glibc, `dpkg`, `apt`, or package compatibility.
