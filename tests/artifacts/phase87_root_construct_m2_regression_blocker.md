# Phase 87 Regression Blocker — Root Construction M2 Transcript

**Status:** RESOLVED — the publication blocker was cleared by a successful single-vCPU ordered replay before the 96-verifier full-suite pass.

The clean Phase 87 controlled `execve` replacement M0 isolated QEMU witness, SHA-bound manifest, and independent verifier passed. The all-profile rebuild completed with `ALL_PROFILE_REBUILD_PASS count=63`, and the binding audit reached `ALL_BINDING_MISMATCHES=0` before the final standalone suite.

The complete standalone verifier regression subsequently reached `verify_root_construct_m2.py` and failed because its stored runtime transcript lacked the exact required marker:

> `SeLinOS root construction M2: malformed, unavailable or duplicate lease rejected; no cap delivered.`

Three bounded replay strategies were attempted without replacing the tracked M2 transcript because none met the verifier’s full strict ordered seven-marker predicate: eight QEMU attempts with `-serial mon:stdio`, eight with `-monitor none -serial stdio`, and eight with QEMU instruction-count mode. Candidate logs showed the later M1/M2 success, rejection, and client markers, but suffered serial text interleaving around the early M1 endpoint-ready marker.

The blocker was resolved by an explicit single-vCPU QEMU replay. Its candidate transcript contained all existing required markers in declared order, was SHA-bound into the M2 manifest, and passed `verify_root_construct_m2.py`. The criterion was not weakened.

## Integrity boundary

This transient blocker did **not** invalidate the limited Phase 87 isolated control-flow witness. It was resolved before reporting the mandatory whole-project regression as successful or publishing the milestone.
