# План следующих фаз SeLinOS: bounded NUL-поиск в `argv`/`envp`

**Базовая точка:** verified Phase 92 M0 и Phase 93 M1.
**Цель:** построить проверяемый слой чтения и валидации строк `argv`/`envp`, не выдавая его за общий `execve`, Linux ABI или совместимость с Debian до закрытия существенно более широкого набора контрактов.

## 1. Исходная граница и принцип продвижения

Текущий M0 доказывает чтение первого байта self-authored строки через фиксированный указатель из `[rsp+8]`. M1 добавляет ровно один фиксированный NUL-sentinel на `argv[0]+7`, сохраняя указатель в `RAX`, первый байт в `RCX`, sentinel в `RDX` и используя тот же изолированный seL4 context bridge. Это хороший фундамент, но он ещё не доказывает поиск NUL: один заранее известный offset не заменяет bounded scan.

Следующие фазы должны идти вертикально. Каждая фаза получает отдельный default-OFF профиль, отдельный источник runtime evidence, независимый verifier и явный список non-claims. Нельзя объединять все расширения в один большой witness: иначе будет невозможно установить, какой именно invariant нарушился.

| Область | Уже доказано в M0/M1 | Что требуется доказать дальше |
|---|---|---|
| Указатель | Один self-authored `argv[0]` pointer из фиксированного stack slot | Валидация диапазона каждого pointer и проверка переполнений при вычислении адресов |
| Строка | Первый байт и фиксированный NUL на offset `7` | Поиск NUL в bounded диапазоне с корректным результатом для `found`, `not found` и fault |
| Массив | Один заранее подготовленный slot | `argc` указателей, обязательный `argv[argc] == NULL`, отсутствие выхода за лимит |
| `envp` | Не доказан | Отдельная таблица указателей, отдельный NULL sentinel и отсутствие смешения ownership с `argv` |
| Fault path | Один 16-word reply через `FaultIP`; normal 18-word frame не заявляется | Fail-closed классификация bad pointer, unmapped page, non-canonical address и unterminated input |
| ABI | Узкие самописные witnesses | Никаких claims о полном `execve`, ELF loader, Linux ABI или Debian до отдельной программы совместимости |

## 2. Целевой формат входного initial stack

Перед реализацией scan нужно зафиксировать внутренний контракт представления, не полагаясь на неявное сходство с Linux. Целевой тестовый layout следует формировать root-owned fixture-строителем и валидировать до запуска target domain:

```text
stack_top
    argc                         // bounded unsigned word
    argv[0]                      // pointer to bytes
    ...
    argv[argc - 1]               // pointer to bytes
    NULL                         // mandatory argv terminator
    envp[0]                      // pointer to bytes
    ...
    NULL                         // mandatory envp terminator
    auxv[0].type, auxv[0].value
    ...
    AT_NULL, 0
```

На первом этапе этот layout является **внутренним SeLinOS test contract**, а не заявлением о том, что SeLinOS уже реализует Linux process startup. Каждый pointer и каждая таблица должны иметь независимые fixture constants: `stack_base`, `stack_bytes`, `argv_table_base`, `envp_table_base`, `string_region_base`, `string_region_bytes`, `max_argc`, `max_envc` и `max_string_bytes`.

### Непреложные safety invariants

1. **Проверка арифметики до доступа.** Для `base + offset`, `count * sizeof(pointer)` и `pointer + length` сначала должна проверяться возможность переполнения `seL4_Word`; только затем разрешается dereference.
2. **Проверка каноничности.** Для x86_64 адрес должен удовлетворять зафиксированному canonical-address predicate проекта. Адреса вне разрешённого user range должны завершаться контролируемым `invalid-pointer`, а не случайным VMFault.
3. **Проверка диапазона.** Каждый read должен попадать в заранее разрешённый `[region_base, region_base + region_bytes)`. Нельзя считать сам факт наличия capability или одной mapped page доказательством корректности произвольного user pointer.
4. **Boundedness.** Должны существовать независимые лимиты количества указателей и байтов одной строки. При достижении лимита без NUL возвращается `unterminated`, а не продолжается чтение.
5. **Page-boundary policy.** Сначала нужно доказать single-page строку. Межстраничное чтение следует вынести в отдельную фазу, где проверяются обе страницы и отсутствие TOCTOU между проверкой и чтением.
6. **Fail-closed результат.** Допустимые результаты должны быть перечислены в enum/contract: `FOUND_NUL`, `LIMIT_EXCEEDED`, `INVALID_POINTER`, `UNMAPPED`, `TABLE_NOT_TERMINATED`, `ARITHMETIC_OVERFLOW`, `FAULT_UNCLASSIFIED`. Любой неожиданный fault — failure evidence, а не успех.

## 3. Фаза 94 / M2: bounded scan одной строки `argv[0]`

**Цель.** Превратить фиксированный M1 sentinel в настоящий, но ограниченный поиск NUL для одной self-authored строки.

### Реализация

Новый default-OFF профиль должен переиспользовать изолированный TCB, VSpace, NXE-enabled seL4 fork и whole-context bridge. В target witness следует задать `RAX = argv0`, `RCX = 0`, `RDX = max_bytes`, затем в bounded loop:

```text
if index == max_bytes: return LIMIT_EXCEEDED
load byte [RAX + index]
if byte == 0: return FOUND_NUL with length=index
index++
```

Для первой версии строка должна находиться в одной mapped RW+NX page, а `max_bytes` должен быть меньше page size. Результат лучше вернуть в заранее выбранных регистрах через terminal witness, например `RAX=FOUND_NUL`, `RCX=length`, `RDX=bytes_read`; это не должно использовать normal syscall reply-frame semantics.

### Gate M2

Успехом считается только следующий набор: строка `selinos` даёт `FOUND_NUL`, длина равна `7`, количество прочитанных байтов равно `8` вместе с NUL, terminal IP соответствует последней инструкции witness, и root проверяет точное значение результата. Отдельно нужно выполнить отрицательную fixture с `max_bytes=7`, где NUL находится сразу за лимитом и ожидается `LIMIT_EXCEEDED`; эта fixture не должна быть смешана с положительным образом.

**M2 не заявляет** обход `argv`-таблицы, `argc`, `envp`, межстраничные строки, копирование или общий `execve`.

## 4. Фаза 95 / M3: bounded массив `argv`

**Цель.** Проверить `argc` указателей и обязательный NULL sentinel, вызывая M2 primitive для каждой строки с независимым budget.

### Последовательность

1. Прочитать `argc` из fixture stack только после проверки, что stack word находится в разрешённом диапазоне.
2. Проверить `argc <= max_argc` без арифметического переполнения.
3. Вычислить размер таблицы как `argc * pointer_size` с checked multiplication.
4. Для каждого `argv[i]` проверить pointer range, single-page policy и bounded NUL scan.
5. Проверить слово `argv[argc] == NULL`; ненулевое значение даёт `TABLE_NOT_TERMINATED`.
6. Зафиксировать aggregate result: number of entries, total bytes включая/исключая NUL по явной конвенции, first failure index и failure code.

Первая M3 fixture должна содержать два статических аргумента, например `selinos` и `--m0`, и не должна иметь alias между pointer tables и строками. Лимиты нужно проверить отдельно: `argc=0`, `argc=max_argc`, `argc=max_argc+1`, NULL на неверном индексе, pointer в разрешённую страницу без NUL и pointer за пределами.

**M3 не заявляет** user-controlled input, arbitrary memory safety, POSIX quoting, shell parsing, environment expansion или изменение address space.

## 5. Фаза 96 / M4: негативная матрица и fault taxonomy

До добавления `envp` нужен отдельный negative-test gate. Положительный runtime marker без негативных тестов недостаточен: он доказывает только happy path.

| Negative fixture | Ожидаемый результат | Что запрещено считать успехом |
|---|---|---|
| NUL отсутствует до `max_string_bytes` | `LIMIT_EXCEEDED` | Чтение следующей страницы или бесконечный loop |
| Pointer неканонический | `INVALID_POINTER` | Неопределённый VMFault без классификации |
| Pointer canonical, но вне разрешённого region | `INVALID_POINTER` | Проверка только верхних битов |
| Последний byte page без NUL | `LIMIT_EXCEEDED` или `UNMAPPED` по policy | Молчаливый cross-page read |
| `argv[argc] != NULL` | `TABLE_NOT_TERMINATED` | Продолжение до случайного NULL |
| `argc * sizeof(pointer)` overflow | `ARITHMETIC_OVERFLOW` | Обёрнутый маленький размер таблицы |
| Pointer table unmapped | `UNMAPPED` | Root remap, исправление или повтор без evidence |
| Fault badge/label/vector mismatch | `FAULT_UNCLASSIFIED` | Признание любого fault terminal witness |
| `max_string_bytes == 0` | deterministic `LIMIT_EXCEEDED` | Хотя бы один implicit read |

Каждый negative run должен иметь отдельный transcript и forbidden-success markers. Верификатор обязан проверять, что ожидаемый failure code действительно получен, а не только отсутствие общего `FAILED` текста.

## 6. Фаза 97 / M5: bounded поиск в `envp`

`envp` следует реализовывать только после стабилизации M3/M4. Технически он похож на `argv`, но ownership и aggregate budget должны быть разделены. Нельзя переиспользовать один счётчик или один pointer-table bound так, чтобы ошибка в `argv` могла скрыть ошибку в `envp`.

### Минимальный M5 contract

Fixture должна содержать `argv` с двумя строками и `envp` с двумя строками, затем отдельный NULL sentinel. Target обязан сначала доказать, что `argv` table завершена, затем перейти к `envp`; при любой ошибке второй массив не читается. Runtime result должен различать `argv_count`, `envp_count`, `argv_bytes`, `envp_bytes`, `argv_status` и `envp_status`.

Отдельные обязательные cases: пустой `envp`, `envp` из одной пустой строки, отсутствие `envp` NULL, pointer alias между `argv` и `envp`, общий aggregate limit и индивидуальный per-string limit. Alias вначале должен быть либо запрещён fixture validator-ом, либо явно разрешён с отдельным доказательством ownership; нельзя оставлять его неопределённым.

**M5 не заявляет** libc environment semantics, `getenv`, mutation, `putenv`, inheritance, secure-exec rules или Linux ABI.

## 7. Фаза 98 / M6: отдельный initial-stack validation layer

Когда bounded primitives и negative matrix доказаны, их следует вынести из экспериментального root transaction в отдельный модуль, например `selinos_initial_stack_validate`. Модуль должен принимать описательную структуру policy, а не доступ к произвольному capability space:

```c
struct selinos_initial_stack_policy {
    seL4_Word stack_base;
    seL4_Word stack_bytes;
    seL4_Word max_argc;
    seL4_Word max_envc;
    seL4_Word max_string_bytes;
    bool allow_cross_page_strings;
};
```

API должен возвращать immutable result record с кодом ошибки и счётчиками. Он не должен сам выдавать capabilities, remap-ить страницы, исправлять fault или менять target context. Управление памятью, mapping и lifecycle остаются снаружи. Это разделение позволит отдельно тестировать parser policy и seL4 authority boundary.

### M6 acceptance

Нужны host-side unit tests для арифметики и bounds, target-side tests для реальных mapped pages, и QEMU integration test, который запускает тот же result record через существующий rootserver. Все три уровня должны ссылаться на одну версию policy schema; расхождение host и target трактуется как блокер.

## 8. Фаза 99 / M7: интеграция с execve-подобным state transition

Только после M6 можно проектировать state transition, и сначала только как отдельную test transaction:

```text
OLD_STATE -> VALIDATING_STACK -> STACK_VALIDATED -> READY_TO_TRANSFER
                                      \-> REJECTED(reason)
```

`STACK_VALIDATED` не должен автоматически означать `EXECUTED`. Следующий transition обязан отдельно описать ownership нового VSpace, lifetime старого VSpace, register initialization, teardown on failure и capability revocation. До появления этих правил нельзя называть результат `execve`.

Для первой интеграции следует оставить существующий syscall-number-59 fault shape и отдельный default-OFF switch. Не менять общую Linux syscall gateway, не добавлять normal 18-word reply-frame claim и не подключать dynamic linker или libc в ту же фазу.

## 9. Evidence и критерии продвижения

Каждая фаза должна публиковать следующие артефакты:

| Артефакт | Требование |
|---|---|
| Контракт | Markdown gate с точной машинной последовательностью, входом, выходом и non-claims |
| Код | Отдельный header/source и default-OFF CMake option |
| Positive runtime | Канонический QEMU transcript с командой, средой, marker и bounded exit |
| Negative runtime | Минимум один failure transcript на каждый новый class ошибки |
| Manifest | SHA-256 для source, generated config, kernel/root images, transcript и gate |
| Independent verifier | Не зависит от rootserver success text; проверяет байты, offsets, counts, markers и forbidden markers |
| Regression | M0, M1, Phase 91 и все ранее verified verifiers должны продолжать проходить |
| Git | Отдельный commit и опубликованная ветка; main не менять автоматически |

Продвижение разрешается только если одновременно выполнены следующие условия: clean build с pinned seL4/seL4_libs, default-OFF isolation, positive runtime, negative runtime, manifest verification, отсутствие forbidden claims и regression всех предыдущих gates. Timeout после достижения success marker допустим для idle-loop профиля; timeout до marker всегда считается inconclusive.

## 10. Рекомендуемый порядок реализации

Практический порядок должен быть таким: сначала Phase 94/M2 для одной строки; затем Phase 95/M3 для массива `argv`; затем Phase 96/M4 с полной negative matrix; затем Phase 97/M5 для `envp`; затем Phase 98/M6 с выделением validation layer; и только после этого Phase 99/M7 для отдельного state-transition эксперимента. Если на любой фазе появляется VMFault, register-clobber или page-boundary ambiguity, фазу нужно остановить, добавить диагностический evidence и не продвигать положительный marker.

Наиболее важный технический риск — не сам цикл поиска NUL, а корректная граница authority между root и target: root может подготовить self-authored fixture, но это не доказывает безопасность произвольных user pointers. Второй риск — преждевременное смешение parser proof с process replacement. Поэтому каждая фаза должна отвечать на один вопрос и иметь отдельный fail-closed результат.

## References

[1]: https://docs.sel4.systems/Tutorials/fault-handlers.html "seL4 fault handler tutorial"

[2]: https://docs.sel4.systems/projects/sel4/api-doc.html "seL4 API documentation"

[3]: https://man7.org/linux/man-pages/man2/execve.2.html "Linux execve(2) manual page"

[4]: https://man7.org/linux/man-pages/man3/exec.3.html "Linux exec(3) manual page"
