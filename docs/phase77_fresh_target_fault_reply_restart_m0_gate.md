# Phase 77: fresh target fault-reply/restart M0 gate

## Status

**Status: VERIFIED.** The default-OFF `SeLinFreshTargetFaultReplyRestartProbe=ON` isolated profile completed the Phase 76 first `UserException` witness at `0x60000001`, then sent exactly one zero-label reply of length one with only reply word 0 (`FaultIP`) changed to `0x60000003`. The fresh target completed that second NOP and produced the required second badged invalid-opcode `UserException` at `0x60000004`, vector `6`, while preserving `rsp=0x70002ff8`; root withheld a second reply and contains no second fresh-TCB resume. The claim is bound to the QEMU TCG transcript and implementation hashes in `tests/artifacts/selinos_fresh_target_fault_reply_restart_m0.verification.json`; the full 49-profile rebuild and standalone verifier regression passed.

> Phase 77 is a test of one architecture-constrained control transfer, not a restartable process model. The root must retain the second fault as terminal and must not send a second reply.

| Step | Required action | Required boundary |
|---:|---|---|
| 1 | Complete all Phase 69–76 prerequisites, including the Phase 76 first terminal fault at `0x60000001`, vector `6`. | No alternate construction, entry mapping, initial context, or first-resume path. |
| 2 | Before target mapping, materialize exactly `NOP; UD2; NOP; UD2` at fresh entry `0x60000000`. | One root-private write alias, removed before executable target mapping; no ELF or writable-executable target mapping. |
| 3 | Send exactly one zero-label `UserException` reply of length one with only reply word 0, `FaultIP`, set to `0x60000003`. | No register write syscall after the first fault; no `RSP`/`FLAGS` mutation; no second `seL4_TCB_Resume`. |
| 4 | Receive and validate exactly one subsequent badged `UserException` at `0x60000004`, vector `6`, with the fixed stack pointer. | Withhold reply permanently; no third instruction, recovery, exit, teardown, reuse, or successor target. |

## Required implementation controls

The proposed profile must be default-OFF and isolated. Its verifier must bind the gate, protocol header, root source, CMake profile, Phase 77 images and QEMU TCG transcript by SHA-256. It must also structurally isolate its own `#if CONFIG_SELINOS_FRESH_TARGET_FAULT_REPLY_RESTART_PROBE` block and require one `seL4_Reply` with label zero and message length one, one `seL4_TCB_Resume(fresh_tcb.cptr)` inherited only from the first-fetch transaction, exactly two `seL4_Recv(fresh_fault_endpoint.cptr, ...)` operations across the Phase 76/77 combined scope, and no reply after the second fault.

The verifier must inspect the x86_64 payload and fault addresses rather than accepting generic success text. The required runtime sequence is a first UserException at `0x60000001`, one reply that changes only `FaultIP` to `0x60000003`, then a second UserException at `0x60000004`; both must have exception number `6`, carry badge `0x74`, and preserve `rsp=0x70002ff8`.

## Explicit non-claims

Phase 77 would not prove a general fault policy, signal delivery, `sigreturn`, exception recovery, arbitrary register mutation, a scheduler, a process lifecycle, `execve`, ELF loading, dynamic linking, Linux ABI compatibility, `dpkg`, or `apt`.
