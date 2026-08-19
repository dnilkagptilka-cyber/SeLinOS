# Phase 78: fresh target stack VMFault FSR M2 gate

## Status

**Status: VERIFIED HISTORICAL DIAGNOSTIC; SUPERSEDED AS CURRENT RUNTIME BEHAVIOR BY M3.** The default-OFF M2 experiment inherited the M1 payload and mapping ledger, performed one fresh-target read and recorded raw `FSR=0x000000000000000c` before fail-closed return. It issued no reply, repair or second resume. That trace remains a SHA-bound diagnostic of the pre-NXE boot path; it is not a current compatibility claim.

After the verified M3 correction enables EFER.NXE before paging, the same bounded `mov rax,[rsp]; ud2` transaction reaches the terminal post-read `UD2` `UserException` branch. This proves one initial stack read only. It does not convert M2 into a PTE reader, mapping repair, or general stack-access result.

> M2 is a fault-classification gate, not a page-table reader. The generated seL4 client interface exposes frame address and map/unmap operations, not a general user-level fresh-PTE read invocation. The experiment does not invent one, map page tables into root, alter kernel debug policy, or grant new authority.

## Historical M2 transaction

M2 used the exact Phase 78 M1 root-only `mov rax,[rsp]; ud2` payload, temporary entry-alias unmap, M0 NX-stack/RX-entry ledger, one `seL4_TCB_Resume`, one badged fault receive, and no reply, repair, second resume, or continuation. It added no second target access.

| Historical branch | Required IPC fields | Historical result | Strict interpretation |
|---|---|---|---|
| Reserved-bit user data fault | `label=VMFault`, badge `0x74`, `IP=0x60000000`, `Addr=0x70002ff8`, `PrefetchFault=0`, `FSR=0x0c` | Captured before NXE correction. | The access was classified with user and reserved-bit flags. It did not prove the exact hierarchy state. |
| Non-present user read | Same fields with `FSR=0x4` | Not captured in pre-NXE M2. | Would only identify that error-code class, not a missing mapping level. |
| Protection-class user read | Same fields with `FSR=0x5` | Not captured in pre-NXE M2. | Would only identify that error-code class, not permission intent or stack correctness. |

## Root-cause correction and revalidation

The pre-NXE `0x0c` value is consistent with an XD leaf being treated as reserved when EFER.NXE is inactive. The original long-mode path set EFER.LME but neither validated CPUID NX support nor enabled EFER.NXE. M3 introduced a CPUID NX gate and sets EFER.LME plus EFER.NXE before paging.

M3 independently established the post-correction terminal `UD2` branch with no reserved-bit VMFault, reply, repair, retry, or second resume. The Phase 43 execute-disable witness was also revalidated with exact raw instruction-protection `FSR=0x15`, excluding the former reserved-bit ambiguity for that bounded witness.[1]

## Non-claims

M2 and M3 together do not prove page-table presence, hierarchy ownership, general target stack readability or writability, stack initialization, return, a runnable process, ELF loading, Linux ABI compatibility, `dpkg`, `apt`, Debian package compatibility, or a Debian primary environment.

[1]: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html "Intel® 64 and IA-32 Architectures Software Developer’s Manual"
