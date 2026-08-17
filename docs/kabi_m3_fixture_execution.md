# SeLinOS KABI M3: controlled pinned-fixture execution

## Verified result

SeLinOS now executes the real, pinned Linux 6.18.44 `selinos_kabi_probe-6.18.44.ko` fixture in the separately spawned **`selinos-modexec`** seL4 domain. Before `init_module()` is entered, the domain verifies the fixture SHA-256, the module's three `__versions` CRC records and its curated import set, then applies the supported x86_64 `ET_REL` relocations. The fixture returns zero from `init_module()` and invokes the curated `_printk` stub once from each of `init_module()` and `cleanup_module()`.

| Aspect | Evidence | Explicit limit |
|---|---|---|
| Artifact admission | Pinned SHA-256, `vermagic`, runtime `__versions` CRC and curated export table | No PKCS#7/X.509 Linux module-signing policy |
| Relocation | Real fixture uses the supported x86_64 relocation subset | No general relocation-type support |
| Entry lifecycle | `init_module()` returns zero; `cleanup_module()` is invoked; two `_printk` calls are observed | One self-authored fixture only |
| Domain isolation | `selinos-modexec` is a dedicated TCB/CSpace/VSpace with no PCI, raw IRQ or DMA capability | There is not yet a fault-driven module-manager/unload protocol |
| Memory safety claim | No device authority is granted | The pinned x86 mapping profile does **not** prove NX-enforced W^X; production/general native module execution remains blocked |

The reproducible host-side entrypoint regression and artifact checks are run through:

```sh
cd /home/ubuntu/helixos
./tools/verify_kabi_fixture_execution.py
```

The QEMU baseline record additionally requires the `selinos-modexec` init/cleanup completion and explicit no-NX-proof markers. The independent evidence record is [`tests/artifacts/selinos_kabi_fixture_execution.verification.json`](../tests/artifacts/selinos_kabi_fixture_execution.verification.json).

> **Compatibility statement:** this is controlled execution of one exact Linux 6.18.44 fixture under a SeLinOS development profile. It is **not** proof of general `.ko` compatibility, third-party driver loading, Linux module-signature compatibility, or production W^X module confinement.

## Remaining closure work

A general native module-loader claim still needs a complete version-pinned export/KAPI implementation, third-party signing/trust policy, a tested hardware-enforceable W^X mechanism, explicit per-module capability manifests, a module manager, fault endpoint handling, safe unload/reload and test matrices covering real driver classes. The current x86_64 execute-control blocker is recorded in [`research_module_wx_x86_64_2026-08-13.md`](research_module_wx_x86_64_2026-08-13.md).
