# Phase 65: sealed static-image mapping and terminal witness M0 gate

## Status

**Status: VERIFIED.** The QEMU TCG transcript, independent verifier, 37-profile rebuild and 71-standalone-verifier regression passed. Phase 65 follows the verified Phase 64 parser gate. It may materialize exactly one already-accepted `SSIM-v1` payload into exactly one newly allocated target entry frame, map that frame executable at the validated canonical entry address, perform exactly one target resume, and receive the existing terminal `ud2` UserException. It is not a general loader and not an ELF execution claim.

> The Phase 64 container is **not** mapped as code. The verified segment payload begins at container offset `96`, while the fixed target entry is at page offset zero. Phase 65 therefore copies only the authenticated three-byte payload (`nop; ud2`) into a distinct zeroed entry frame at offset zero, unmaps the root’s writable alias, then creates the sole target mapping as executable. This prevents treating format metadata as executable instructions.

## Required transaction

| Step | Actor | Required action | Ledger condition |
|---:|---|---|---|
| 1 | Root | Construct deterministic Phase 64 fixture and parse it exactly once. | Parser accepts one RX segment, entry `0x60000000`, file/memory length 3, payload digest. |
| 2 | Root | Allocate one target entry frame and map a temporary root-private writable alias. | Root-only materialization alias exists; target entry mapping does not yet exist. |
| 3 | Root | Copy exactly the parser-validated payload bytes from fixture source offset 96 to entry-frame offset 0. | Payload digest from parser summary equals independently recomputed source payload digest. |
| 4 | Root | Remove the root-private writable alias. | No root writable alias remains before target mapping. |
| 5 | Root | Map that same frame once into the existing isolated target PML4 at `0x60000000` with x86 executable/default attributes. | One entry frame, one target executable mapping; stack remains NX. |
| 6 | Root | Write/read canonical entry and ABI-aligned stack context, then resume exactly once. | `rip=0x60000000`, `rsp=0x70002ff8`, x86 normalized `rflags=0x202`. |
| 7 | Root | Receive terminal UserException for `ud2` at `0x60000001`; withhold reply. | One terminal vector-6 witness; no reply or second resume. |

## W^X ledger

The Phase 65 proof must make the mapping lifecycle inspectable in source and transcript. The only writable access to the entry-frame bytes is through the root-private temporary alias before any target executable mapping. Root must explicitly unmap that alias before mapping the frame into the target VSpace. The target entry mapping uses the existing x86 executable/default mapping primitive; the separately allocated stack frame uses `ExecuteDisable`.

| Resource | Writable state | Executable state | Permitted lifecycle |
|---|---|---|---|
| SSIM container buffer | ordinary root-local parser bytes | never mapped to target | Phase 64 parse input only. |
| Entry frame, root alias | root-private initialization write | not target-mapped | exists only before step 4. |
| Entry frame, target mapping | no root alias | target executable | exactly one mapping at validated entry. |
| Stack frame | no intended writes in M0 | target NX | fixed `0x70002000` mapping only. |

## Required negative boundaries

The root must fail closed before materialization/mapping if the sealed parser rejects the fixture, if the parsed entry differs from the fixed canonical target entry, if the payload length differs from three, if the parsed permission mask differs from `R|X`, or if the recomputed payload digest differs from parser output. The verifier must reject any implementation which maps the raw SSIM container as target code, retains a root alias after target mapping, permits W+X source permissions, performs two resumes, replies to the terminal UserException, or introduces ELF/relocation/process APIs.

## Explicit non-claims

Phase 65 will not prove a general static binary loader, arbitrary code execution, entry/stack image construction, return/exit behavior, lifecycle/reclamation, ELF, relocation, C runtime, dynamic linker, Linux `execve`, a general Linux process, Linux ABI compatibility, persistent storage, networking, `dpkg`, `apt`, or Linux driver/KABI compatibility. The Phase 49 DMA-containment blocker remains unchanged.

## References

[1]: `docs/phase64_sealed_static_image_m0_gate.md` — verified SSIM-v1 grammar and parser-only SHA-256 ledger.
[2]: `docs/phase63_reply_terminal_fault_m0_gate.md` — verified one executable entry repair/restart to terminal UserException.
