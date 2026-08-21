#ifndef SELINOS_EXECVE_REPLY_CONTEXT_BRIDGE_M0_H
#define SELINOS_EXECVE_REPLY_CONTEXT_BRIDGE_M0_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/* Candidate bounded bridge: root changes only faulted target RIP/RSP through
 * TCB context, then performs one 16-word reply through FaultIP. It is not a
 * normal 18-word reply-frame or a general execve implementation. */
bool selinos_execve_reply_context_bridge_m0_start(vka_t *vka, vspace_t *vspace);

#endif
