# SeLinOS KABI M2: non-executing ELF relocation stage

## Verified result

SeLinOS now parses and relocates the real Linux 6.18.44 `selinos_kabi_probe-6.18.44.ko` fixture as an x86_64 `ET_REL` object into caller-owned bounded memory. The stage is freestanding and uses no Linux runtime, Linux kernel, VM or LKL. An isolated permanent `selinos-modld` domain embeds the fixture as data and reproduces pinned SHA-256 checking, `__versions` CRC verification and relocation during QEMU boot. The independent check is:

```sh
cd /home/ubuntu/helixos
./tools/verify_kabi_relocation_stage.py
```

The verification compiles the host regression, validates the fixture SHA-256 and existing KABI report, runs the relocation stage and checks semantic output. The fixture has nine relocation sections and is relocated with **17 applied relocations** across 17 allocatable sections. The stage discovers non-zero relocated addresses for `init_module` and `cleanup_module`, but does not call either function.

| Element | Implemented and tested | Boundary |
|---|---|---|
| ELF acceptance | ELF64, little-endian, x86_64, `ET_REL`, metadata/vermagic parse | No general ELF executable loader |
| Memory layout | Copies/zeros `SHF_ALLOC` sections into caller-owned bounded memory inside `selinos-modld` | No executable page permission; no separate execution-domain module VSpace is created |
| Relocations | `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32`, `R_X86_64_32S` | Other x86_64 relocation types are rejected |
| Integrity | Pinned SHA-256 accepts the exact real fixture and rejects a one-byte modified expected digest | Exact-artifact integrity only; no PKCS#7, certificate-chain, key rotation or distribution-signature policy |
| Imports and CRC | Curated table resolves `_printk` and `__x86_return_thunk`, validates the three fixture `__versions` CRC records and rejects an unlisted name or altered CRC | Three-symbol test table only; no complete SeLinOS kernel-export resolver |
| Entrypoints | Reads relocated `init_module` / `cleanup_module` addresses | No execution, unload invocation, scheduler integration or fault containment |

The host fixture uses `R_X86_64_32S`; the regression therefore reserves a low signed-32-bit test mapping. This is an ELF relocation range requirement, not evidence of an executable SeLinOS module VSpace layout.

## Evidence

Machine-readable evidence is stored in [`tests/artifacts/selinos_kabi_relocation_stage.verification.json`](../tests/artifacts/selinos_kabi_relocation_stage.verification.json). It deliberately records `module_code_executed: false`, absence of a Linux PKCS#7 signature policy and the test-only curated resolver. The verifier checks pinned-digest acceptance/rejection, curated-export allow/deny behaviour and runtime `__versions` CRC allow/deny behaviour before relocation. Those exclusions are part of the verification contract.

> **Compatibility statement:** M2 establishes an isolated, non-executing integrity/CRC/relocation preparation path for one version-pinned real Linux module artifact. It does **not** establish native Linux `.ko` loading or execution.

## Next closure requirements

The next native-loader milestone must add Linux-compatible signed module package policy, a complete SeLinOS KABI export resolver beyond the current three-symbol/CRC fixture table, a dedicated module VSpace/CSpace, W^X transition after relocation, capability manifest enforcement, controlled `module_init()` / `module_exit()` calls and fault-driven unload/revoke. Each condition needs independent negative tests before SeLinOS can claim real `.ko` execution.
