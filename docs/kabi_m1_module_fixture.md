# SeLinOS KABI M1: проверенный Linux 6.18.44 module artifact

**Статус:** завершён как тест артефакта, но не как загрузчик или runtime драйвера.

## Назначение

Эта веха проверяет, что SeLinOS умеет анализировать **реальный** ELF64 модуль Linux, созданный для единственного закреплённого compatibility profile — `SELINOS_LINUX_BASELINE_6_18_44`. Linux-ядро здесь использовалось исключительно для формирования reference-артефакта; оно не включено в образ SeLinOS, не загружается и не является его runtime-компонентом.

Linux internal module ABI не является стабильным между релизами или конфигурациями, поэтому проверка опирается на exact source release, `.config`, `vermagic`, таблицу symbol-version CRC и manifest, а не на название семейства Linux в целом.[1]

| Компонент | Проверенный результат |
|---|---|
| Baseline | Linux `6.18.44`, `x86_64`, `SELINOS_LINUX_BASELINE_6_18_44` |
| Artifact | `tests/artifacts/selinos_kabi_probe-6.18.44.ko` |
| SHA-256 artifact | `a4c3a6614e39191af7071ba320b2aac4ae303a9ac815c2cd4d86d2e1575a723d` |
| Format | ELF64 little-endian, `ET_REL`, `EM_X86_64` |
| `vermagic` | `6.18.44 SMP preempt mod_unload modversions` |
| Undefined symbols | `_printk`, `__x86_return_thunk` |
| `__versions` records | `_printk`, `__x86_return_thunk`, `module_layout` |
| Relocation sections | 9 |
| Manifest verification | Passed: no unknown undefined symbol and no version-CRC mismatch |

## Воспроизводимый путь

Тестовый модуль расположен в отдельном каталоге `samples/selinos_kabi_probe` внутри локальной pinned-копии исходников Linux. Его включение — это `CONFIG_SAMPLE_SELINOS_KABI_PROBE=m`; такая интеграция заставляет штатный Kbuild сформировать файл `.ko` с нормальными `.modinfo`, `__versions`, symbol relocations и module metadata.

```bash
# Linux source and output trees prepared as described in the baseline profile.
./scripts/config --file ../linux-6.18.44-kabi-config/.config \
  --enable SAMPLES --module SAMPLE_SELINOS_KABI_PROBE
make O=../linux-6.18.44-kabi-config olddefconfig
make O=../linux-6.18.44-kabi-config samples/selinos_kabi_probe/selinos_kabi_probe.ko

cd /path/to/selinos
python3 tools/verify_kabi_module.py \
  tests/artifacts/selinos_kabi_probe-6.18.44.ko \
  docs/profiles/selinos-linux-6.18.44-kabi-manifest.json \
  tests/artifacts/selinos_kabi_probe-6.18.44.verification.json
```

## Что именно проверяет SeLinOS

`src/projects/selinos/kabi/src/selinos_kabi_module.c` — freestanding buffer parser, предназначенный для будущего user-space `selmod` service. Он принимает полный byte buffer; проверяет ELF magic, разрядность, endianness, `ET_REL`, `EM_X86_64`, таблицу секций, `.modinfo`, наличие `license`, соответствие `vermagic`, символы и секции перемещений. Он **не исполняет** module code и не выдаёт никакие device capabilities.

`tools/verify_kabi_module.py` — независимая test-side проверка. Она сопоставляет undefined symbols и записи `__versions` с полным KABI manifest SeLinOS, созданным из фактического Linux `vmlinux.symvers`. У fixture отсутствуют неизвестные imports и CRC mismatches. Это подтверждает корректность первой цепочки:

> Linux 6.18.44 source/config → Kbuild `.ko` → SeLinOS ELF parser → `vermagic`/symbol/CRC verifier.

## Строгая граница результата

Положительный результат не означает, что файл `.ko` уже можно загрузить или выполнить в SeLinOS. Для этого понадобятся signature verifier, ELF relocation application, memory allocator с W^X, разрешение каждого KABI symbol на реализованную SeLinOS service ABI, managed device lifecycle, IRQ, DMA и crash containment. Следующая реализуемая подцель — сделать mediated coherent DMA runtime и затем загрузочный сервис, который может **принять и отклонить** module artifact по политике до появления исполнения.

## References

[1]: https://www.kernel.org/doc/html/latest/process/stable-api-nonsense.html "The Linux Kernel: Stable API Nonsense"
[2]: https://docs.kernel.org/kbuild/modules.html "Building External Modules"
[3]: https://docs.kernel.org/admin-guide/module-signing.html "Kernel module signing facility"
