# SeLinOS Linux syscall ABI M1: mediated `getpid`

**Статус:** завершён и воспроизводимо проверен на x86_64/PC99 в QEMU TCG.  
**Граница совместимости:** это узкий development proof для одного Linux x86_64 syscall, а не общий Linux userspace ABI.

## Реализованный маршрут

Изолированный ELF `selinos-linux-syscall-probe` исполняет нативную инструкцию `syscall` с Linux syscall number **39** (`getpid`) в `RAX`. Поскольку этот номер не является syscall микроядра seL4, seL4 доставляет `seL4_Fault_UnknownSyscall` на root-held fault endpoint процесса. Gateway аутентифицирует номер из `seL4_UnknownSyscall_RAX` и возвращает детерминированное тестовое значение `4242`.

| Элемент | Реализованный M1 contract |
|---|---|
| Caller | Отдельный seL4 process `selinos-linux-syscall-probe` с собственными TCB, CSpace и VSpace |
| Linux ABI input | `RAX = 39` перед `syscall` |
| Fault boundary | `seL4_Fault_UnknownSyscall` на endpoint, созданный при `sel4utils_configure_process` |
| Return value | `RAX = 4242` только для test contract |
| Resume control | Reply изменяет `RAX` и `FaultIP`; `FaultIP` сдвигается на два байта, пропуская x86_64 opcode `syscall` |
| User-mode proof | Probe сравнивает `RAX` с `4242` и посылает отдельный acknowledgement syscall только при равенстве |

> **Доказательство end-to-end:** root получает continuation acknowledgement только после выполнения сравнения в изолированном userspace. Поэтому один лишь факт receipt первого fault не считается успехом.

## Важная x86_64 деталь

Для `UnknownSyscall` ответ длиной **16 registers** копирует `RAX` через `FaultIP`. Это позволяет менять Linux return register и restart address, не переписывая `RSP`, `RFLAGS`, `FS_BASE` или `GS_BASE`. Более широкий перенос context оказался неподходящим для M1: он ненужно затрагивает state, не принадлежащий минимальному return contract, включая TLS-related registers.

Gateway не выполняет других IPC операций между получением fault frame и long fault reply: эти операции могут изменять IPC buffer, который содержит non-fast message registers. Сначала аутентифицируется `RAX`, затем единственным reply выполняются изменение RAX и переход после `syscall`.

## Воспроизводимая проверка

```bash
cd /home/ubuntu/helixos
./tools/verify_linux_syscall_abi_m1.py
```

Verifier пересобирает image и запускает QEMU через PTY-backed serial capture. Он требует два ordered markers:

```text
SeLinOS ABI gateway: Linux getpid syscall fault mediated, replied and user continuation acknowledged.
SeLinOS M0: eleven isolated service domains started.
```

Machine-readable record: [`tests/artifacts/selinos_linux_syscall_abi_m1.verification.json`](../tests/artifacts/selinos_linux_syscall_abi_m1.verification.json). Последний boot trace verifier сохраняет в [`tests/artifacts/selinos_linux_syscall_abi_m1.boot.log`](../tests/artifacts/selinos_linux_syscall_abi_m1.boot.log).

## Что это не доказывает

M1 не реализует `exit`, `write`, `read`, `brk`, `mmap`, file descriptors, VFS, ELF dynamic linker, glibc ABI, сеть, `dpkg` или `apt`. Gateway пока исполняется в root task как узкий bootstrap proof; постоянный изолированный `selinos-abi-gated` server ещё не принимает fault endpoint и не является general dispatcher. Также M1 не утверждает выполнение Linux programs или Linux packages.

Следующая работа должна расширить dispatcher из M1 в persistent ABI domain и добавить конкретные syscall contracts с отдельными userspace fixtures и independent verifiers.
