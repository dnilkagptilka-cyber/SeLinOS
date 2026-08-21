#ifndef SELINOS_EXECVE_REPLY_STACK_READ_M0_H
#define SELINOS_EXECVE_REPLY_STACK_READ_M0_H

#include <stdbool.h>

#include <allocman/vka.h>
#include <sel4utils/vspace.h>

/* Performs one bounded execve-number unknown-syscall reply into one fixed RX
 * witness which reads exactly the first word at the supplied RSP and reaches a
 * terminal UD2. This is not a general initial-stack or execve implementation. */
bool selinos_execve_reply_stack_read_m0_start(vka_t *vka, vspace_t *vspace);

#endif
