# Phase 35: root construction-request dispatch M0 gate

## Status

**Status: verified.** On the x86_64/PC99 QEMU TCG production image `8093b0889573961fa634cf5825c7a67ee789bd60af956553e4c1f3af16cd1816`, `verify_root_dispatch_m0.py` independently binds the root source, protocol header, probe source, image and QEMU evidence. Persistent root context M0 is also independently verified. Before the root idle loop, M0 accepts and replies to one exact request/reply transaction on a dedicated endpoint. It does not construct a child.

## Exact topology

Root allocates one endpoint after its persistent context is initialized. Root retains the receive cap. A single isolated `root-construct-dispatch-probe` domain receives only the endpoint send cap in its fixed CSpace slot 8. No taskd, objectd, memd, ABI probe, ROMFS, driver or device domain receives this cap. The probe calls root exactly once with `(REQUEST, slot=1, generation=1)`. Root validates a normal IPC tag with three words and replies `(DISPATCH_READY, slot=1, generation=1)`; the probe validates and prints success.

| Party | Allowed M0 operation | Forbidden operation |
|---|---|---|
| Root | One `seL4_Recv` and one exact `seL4_Reply` on its construction endpoint. | VKA/VSpace/process-loader call after receiving the request; child construction, cap transfer, CNode mutation or dispatch of fault/VFS/device messages. |
| Probe | One `seL4_Call`, validates three status words. | Obtaining root allocator/loader authority, child handle, endpoint beyond its one send cap or any client parameter. |
| Main root flow | Lowers priority, invokes bounded `selinos_root_dispatch_m0_once()`, then enters existing idle loop. | Unbounded multiplexing, polling arbitrary endpoints, badge reinterpretation or replacement of unknown-syscall mediation. |

## Required evidence

The verifier must bind root source, protocol header, probe source, image and QEMU log. It must show that root creates exactly one endpoint, copies it only to the probe, retains the root CPtr in a private static field, checks exact request shape and sends no extra cap. The new root dispatch function must contain no VKA, VSpace, `sel4utils_configure_process`, `seL4_TCB_`, untyped retype, device, IRQ, DMA, PCI, I/O, IOSpace or Linux clone path. The QEMU log must order root endpoint ready, root exact request acceptance and root fixed reply before the root idle marker. The unblocked probe success marker must appear after the fixed reply; it may occur after root enters its yield loop because scheduling resumes the caller after the reply.

## Promotion rule

M0 may claim only that root handled one exact, isolated construction-request **status transaction** before idle. It cannot claim child construction, dynamic allocation, capability delivery, taskd ownership, child start, teardown, reuse, Linux clone/fork/pthread, W^X, ELF loading, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_root_dispatch_m0.verification.json`, verified by `./tools/verify_root_dispatch_m0.py` within the final 41-verifier regression suite.
