# SeLinOS Linux syscall ABI M2: bounded `getpid`, `write` and `exit`

**Статус:** завершён и воспроизводимо проверен на x86_64/PC99 в QEMU TCG.  
**Граница совместимости:** development slice, а не общий Linux userspace ABI.

M2 расширяет M1 fault-mediated `getpid` до одной ограниченной последовательности изолированного userspace process: `getpid()` → `write(1, payload, length)` → `exit(0)`. Последний `exit(0)` принимается root gateway только после того, как probe в userspace проверил оба возвращённых значения. Поэтому этот test не сводится к приёму syscall faults со стороны root.

| Linux syscall | Number | M2 contract | Результат теста |
|---|---:|---|---|
| `getpid` | 39 | Принимается только как первый fault; возвращается deterministic test PID `4242` | Probe продолжает только при `RAX == 4242` |
| `write` | 1 | Разрешён только `fd=1` или `fd=2`, 1–127 bytes, полностью в одной userspace page | Root печатает точный user payload и возвращает requested byte count |
| `exit` | 60 | Разрешён только `exit(0)` после успешного `getpid` и `write` | Probe остаётся остановленным; reply намеренно не посылается |

## Безопасность доступа к user buffer

Root не интерпретирует pointer изолированного process как свой виртуальный адрес. После получения `write` fault gateway сначала сохраняет первые 16 message registers (`RAX`…`FaultIP`) в локальный frame snapshot. Это обязательно, потому что последующий mapping использует seL4 IPC buffer.

Затем gateway проверяет три границы: non-zero size, maximum **127 bytes**, отсутствие arithmetic overflow и принадлежность range одной 4 KiB page. Только после этого он получает cap backing frame через metadata VSpace probe, копирует этот cap в root CSpace и временно map-ит duplicate в root VSpace. Ровно `length` bytes копируются в local console buffer; duplicate mapping unmap-ится и его cap удаляется до reply. Никакая постоянная shared mapping, raw process address-space authority или multi-page copy не создаётся.

> **Непосредственно доказанный результат:** serial trace содержит payload, созданный в изолированном probe: `SeLinOS ABI M2: mediated Linux write(1) userspace payload passed.` Затем gateway получает `exit(0)`, что возможно только после userspace comparisons обоих return values.

## x86_64 reply contract

На x86_64 `UnknownSyscall` reply длиной 16 переносит `RAX` до `FaultIP`. M2 восстанавливает эти registers из snapshot, заменяет `RAX` на syscall result и повышает `FaultIP` на два байта, чтобы пропустить opcode `syscall`. `RSP`, `RFLAGS`, `FS_BASE`, `GS_BASE` и прочий state изолированного process не переписываются.

## Воспроизводимая проверка

```bash
cd /home/ubuntu/helixos
./tools/verify_linux_syscall_abi_m2.py
```

Verifier пересобирает image, запускает QEMU через PTY-backed serial capture и требует ordered markers:

```text
SeLinOS ABI M2: mediated Linux write(1) userspace payload passed.
SeLinOS ABI gateway: getpid, bounded write and exit(0) mediated.
SeLinOS M0: eleven isolated service domains started.
```

Machine-readable record: [`tests/artifacts/selinos_linux_syscall_abi_m2.verification.json`](../tests/artifacts/selinos_linux_syscall_abi_m2.verification.json). Последний trace: [`tests/artifacts/selinos_linux_syscall_abi_m2.boot.log`](../tests/artifacts/selinos_linux_syscall_abi_m2.boot.log).

## Не реализовано

M2 не предоставляет general file-descriptor table, arbitrary pointer validation, multi-page buffers, `read`, `brk`, `mmap`, process creation, signals, VFS, dynamic linking, glibc, network, `dpkg` или `apt`. Gateway по-прежнему находится в root task: выделенный persistent `selinos-abi-gated` server пока не владеет fault endpoints. Соответственно M2 не является доказательством запуска существующих Linux ELF или Linux packages.
