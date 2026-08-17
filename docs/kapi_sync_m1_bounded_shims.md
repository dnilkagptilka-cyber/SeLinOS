# SeLinOS KAPI synchronization M1: bounded driver-domain shims

**Status:** verified for one isolated QEMU edu driver domain under x86_64/PC99 QEMU TCG. This milestone adds a deliberately narrow source-compatible surface for selected Linux-style completion, workqueue, timer and spinlock calls. It is **not** a Linux scheduler, timer subsystem, worker-pool implementation or SMP locking model.

## Purpose and boundary

Linux workqueues normally enqueue work for execution by independent worker threads; Linux documentation describes workers as the asynchronous execution context that processes queued work items.[1] Linux lock semantics also distinguish sleeping, CPU-local and spinning locks, and describe `spin_lock_irqsave()`-style operations as saving/disabling and later restoring interrupt state in conventional non-RT configurations.[2] SeLinOS Phase 6 does not yet have the scheduler, clock service, interrupt-state model or SMP runtime necessary to reproduce those semantics.

Accordingly, Phase 6 exposes a constrained API that permits a self-authored driver fixture to compile and execute predictable local control flow without acquiring new kernel or hardware authority. The implementation resides only in the already isolated driver domain and is separate from the root-held PCI configuration authority, `irqd` IRQHandler authority and `dmad` DMA-frame lifecycle.

| Linux-shaped interface | SeLinOS M1 behavior | Explicitly absent |
|---|---|---|
| `init_completion`, `complete`, `wait_for_completion_timeout` | A local counter is incremented by `complete()` and consumed by a successful wait. A wait is non-blocking and returns `0` when not already complete. | Sleeping, scheduler wakeups, timeout passage, signals and cross-domain waiter queues. |
| `INIT_WORK`, `schedule_work`, `cancel_work_sync` | A non-pending work item executes its callback synchronously in the submitting driver domain and then clears pending state. | Worker threads, asynchronous queueing, flushing, CPU affinity, ordering across work items and forward-progress guarantees. |
| `timer_setup`, `mod_timer`, `del_timer_sync` | One timer object records pending state and its requested deadline; cancellation returns whether it was pending. | Tick source, timekeeping, deadline expiry, callback dispatch and cross-domain cancellation races. |
| `spin_lock_init`, `spin_lock_irqsave`, `spin_unlock_irqrestore` | A local lock field transitions to/from a held state and returns neutral flags. | Interrupt masking, preemption/migration control, atomic hardware instructions, recursion handling, lockdep and SMP exclusion. |

> **Compatibility statement:** the milestone demonstrates only the listed bounded operations in one test driver. It does not establish behavioral compatibility with drivers that depend on deferred work, actual timeout expiration, interrupt-safe critical sections, contention, multiprocessor execution or scheduler-mediated wakeups.

## Implementation and isolation

The curated public interfaces are `linux/completion.h`, `linux/workqueue.h`, `linux/timer.h` and `linux/spinlock.h`. Their implementation is contained in `kapi_core.c`, alongside the pre-existing narrow PCI, IRQ and DMA shims. No Phase 6 call maps a page, issues a capability, changes a device token, enables DMA or exposes root PCI configuration space.

The isolated `selinos-edu-kapi-probe` executes the following ordered assertion sequence inside the already verified QEMU edu `probe()` callback. It validates local spin bookkeeping, an initially incomplete completion, synchronous work callback execution plus completion consumption, non-pending work cancellation, timer pending/deadline replacement and timer cancellation. The timer callback counter remains zero by design, proving that the test does **not** claim an implemented timer dispatcher.

| Test element | Runtime assertion |
|---|---|
| Completion | Empty wait returns `0`; synchronous work callback calls `complete()`; following wait returns `1`; next wait returns `0`. |
| Work item | `schedule_work()` invokes the callback exactly once; the work item is no longer pending and `cancel_work_sync()` returns false. |
| Timer | First `mod_timer()` returns `0`; reschedule returns `1`; deadline updates from 5 to 9; no callback runs; first `del_timer_sync()` returns `1`, second returns `0`. |
| Spin bookkeeping | `spin_lock_irqsave()` marks the local object held; `spin_unlock_irqrestore()` clears it and restores only neutral local flags. |

## Reproducibility evidence

The dedicated boot record is [`tests/artifacts/selinos_kapi_sync_m1.boot.log`](../tests/artifacts/selinos_kapi_sync_m1.boot.log), generated using the existing QEMU edu configuration in instruction-count mode. It contains the marker:

```text
SeLinOS edu driver: Phase 6 completion/work/timer/spinlock shim checks passed.
```

The independent verifier [`tools/verify_kapi_sync_m1.py`](../tools/verify_kapi_sync_m1.py) pins the SHA-256 values of the production image, implementation, four headers, type shim, EDU driver fixture and boot record. It also rejects the Phase 6 failure marker and retains the earlier DMA/IRQ lifecycle markers. Run it after building with:

```bash
cd /home/ubuntu/helixos
./tools/verify_kapi_sync_m1.py
```

## Remaining work

A later scheduler/timer phase must replace these stubs with service-domain protocols and independently verify their blocking, cancellation, ordering and lifetime behavior. That work requires task lifecycle/TID management, wakeup routing, monotonic time, timer ownership, preemption/interrupt policy and cross-domain synchronization design. Native Linux `.ko` execution remains blocked by the independently documented W^X/NX issue; the Phase 6 API surface does not change that blocker.

## References

[1]: https://docs.kernel.org/core-api/workqueue.html "Linux kernel documentation: Workqueue"
[2]: https://docs.kernel.org/locking/locktypes.html "Linux kernel documentation: Lock types and their rules"
