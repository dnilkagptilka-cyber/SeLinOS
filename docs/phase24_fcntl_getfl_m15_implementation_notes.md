# M15 implementation notes

Insert a future `fcntl` gate after the verified `/selinos-version` `openat → FD 6` reply and before M10 `fstat`. Accept only `syscall=72`, `RDI=6`, `RSI=3`, `RDX=0`; return `0` for fixed immutable read-only status. The corresponding isolated probe must issue the same call immediately after open and compare zero before fstat/read/close. The root branch must not call ROMFS, map a user buffer or mutate state.

This note is not implementation, evidence or a compatibility claim.

Recovery rule: the first M15 attempt accidentally altered an earlier volatile-state read-back (`cmp $8` / `read`) while locating the version path. Before future M15 QEMU runs, compare the probe object with the verified baseline and confirm that only the intended post-version-open instruction window changed.

## 2026-08-14 sequencing correction before second attempt

The recovered M14 probe establishes that `fstat` requires `RDI == 6`. A successful `fcntl(F_GETFL)` returns `RAX == 0`; therefore a following `mov %rax, %rdi` would pass descriptor `0` to the M10 fstat gate and deterministically invalidate the proof. The M15 window must restore the immutable descriptor explicitly after the return check:

```asm
"mov $72, %%rax\n"
"mov $6, %%rdi\n"
"mov $3, %%rsi\n"
"xor %%rdx, %%rdx\n"
"syscall\n"
"test %%rax, %%rax\n"
"jnz 1f\n"
"mov $6, %%rdi\n"
```

The subsequent existing `mov %%r12, %%rsi` and `mov %[fstat], %%rax` instructions are then unchanged. This correction is a pre-implementation design constraint, not M15 evidence or a compatibility claim.
