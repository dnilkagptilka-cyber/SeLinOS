// SPDX-License-Identifier: MIT
#include <allocman/bootstrap.h>
#include <selinos-root/gen_config.h>
#include <allocman/vka.h>
#include <sel4/sel4.h>
#include <sel4runtime.h>
#include <simple-default/simple-default.h>
#include <sel4utils/mapping.h>
#include <sel4utils/process.h>
#include <sel4utils/vspace.h>

#include "selinos_bootstrap.h"
#include "selinos_romfs_protocol.h"
#include "selinos_pci.h"
#include "selinos_edu_domain.h"
#include "selinos_taskd_m1_protocol.h"
#include "selinos_objectd_m1_protocol.h"
#include "selinos_memd_m1_protocol.h"
#include "selinos_taskd_reservation_m1_protocol.h"
#include "selinos_taskd_dynamic_alloc_m0_protocol.h"
#include "selinos_taskd_dynamic_tcb_m0_protocol.h"
#include "selinos_taskd_dynamic_cspace_m0_protocol.h"
#include "selinos_taskd_dynamic_vspace_m0_protocol.h"
#include "selinos_taskd_inert_bundle_m0_protocol.h"
#include "selinos_taskd_suspended_linkage_m0_protocol.h"
#include "selinos_taskd_populated_cspace_m0_protocol.h"
#include "selinos_taskd_self_rooted_cspace_m0_protocol.h"
#include "selinos_taskd_ipc_buffer_m0_protocol.h"
#include "selinos_taskd_zeroed_context_m0_protocol.h"
#include "selinos_taskd_fault_witness_m0_protocol.h"
#include "selinos_taskd_entry_stack_m0_protocol.h"
#include "selinos_taskd_exec_fetch_m0_protocol.h"
#include "selinos_taskd_vm_restart_m0_protocol.h"
#include "selinos_sealed_static_image_m0_protocol.h"
#include "selinos_sealed_static_image_m0.h"
#include "selinos_kabi_policy.h"
#include "selinos_sealed_static_image_mapping_m0_protocol.h"
#include "selinos_static_image_terminal_lifecycle_m0_protocol.h"
#include "selinos_static_image_terminal_lifecycle_m0_record.h"
#include "selinos_static_image_teardown_authorization_m0_protocol.h"
#include "selinos_static_image_mapping_revocation_authorization_m0_protocol.h"
#include "selinos_static_image_mapping_revocation_m0_protocol.h"
#include "selinos_static_image_target_capability_deletion_m0_protocol.h"
#include "selinos_static_image_object_reclamation_m0_protocol.h"
#include "selinos_static_image_fresh_bundle_authorization_m0_protocol.h"
#include "selinos_fresh_target_bundle_construction_m0_protocol.h"
#include "selinos_fresh_target_bundle_configuration_m0_protocol.h"
#include "selinos_opaque_lease_m1_protocol.h"
#include "selinos_tcb_lease_m1_protocol.h"
#include "selinos_tcb_resume_m2_protocol.h"
#include "selinos_root_dispatch_m0_protocol.h"
#include "selinos_root_construct_m1.h"
#if CONFIG_SELINOS_X86_NX_MAPPING_PROBE
#include "selinos_x86_nx_probe.h"
#endif

#define SELINOS_ALLOCATOR_STATIC_POOL_SIZE (1u << 17)
#define SELINOS_ALLOCATOR_VIRTUAL_POOL_SIZE (1u << 20)
#define SELINOS_INITIAL_SERVICE_COUNT 9u
#define SELINOS_LINUX_READ 0u
#define SELINOS_LINUX_WRITE 1u
#define SELINOS_LINUX_MMAP 9u
#define SELINOS_LINUX_MPROTECT 10u
#define SELINOS_LINUX_BRK 12u
#define SELINOS_LINUX_PREAD64 17u
#define SELINOS_LINUX_LSEEK 8u
#define SELINOS_LINUX_GETDENTS64 217u
#define SELINOS_LINUX_FCNTL 72u
#define SELINOS_LINUX_F_GETFL 3u
#define SELINOS_LINUX_UNAME 63u
#define SELINOS_LINUX_ARCH_PRCTL 158u
#define SELINOS_LINUX_ARCH_SET_FS 0x1002u
#define SELINOS_LINUX_ARCH_GET_FS 0x1003u
#define SELINOS_LINUX_CLOSE 3u
#define SELINOS_LINUX_FSTAT 5u
#define SELINOS_LINUX_OPENAT 257u
#define SELINOS_LINUX_GETPID 39u
#define SELINOS_LINUX_EXIT 60u
#define SELINOS_LINUX_GETUID 102u
#define SELINOS_LINUX_GETGID 104u
#define SELINOS_LINUX_GETEUID 107u
#define SELINOS_LINUX_GETEGID 108u
#define SELINOS_LINUX_GETTID 186u
#define SELINOS_LINUX_SET_TID_ADDRESS 218u
#define SELINOS_LINUX_GETRLIMIT 97u
#define SELINOS_LINUX_FUTEX 202u
#define SELINOS_LINUX_CLOCK_GETTIME 228u
#define SELINOS_LINUX_TEST_PID 4242u
#define SELINOS_LINUX_TEST_TID 4243u
#define SELINOS_LINUX_CLOCK_MONOTONIC 1u
#define SELINOS_LINUX_TEST_TIME_SECONDS 1234u
#define SELINOS_LINUX_TEST_TIME_NANOSECONDS 0u
#define SELINOS_LINUX_RLIMIT_NOFILE 7u
#define SELINOS_LINUX_TEST_RLIMIT_NOFILE 64u
#define SELINOS_LINUX_FUTEX_WAIT 0u
#define SELINOS_LINUX_FUTEX_WAKE 1u
#define SELINOS_LINUX_EAGAIN 11u
#define SELINOS_LINUX_ENOTDIR 20u
#define SELINOS_LINUX_MMAP_ADDRESS 0x70000000u
#define SELINOS_LINUX_BRK_BASE 0x71000000u
#define SELINOS_LINUX_PAGE_SIZE (1u << seL4_PageBits)
#define SELINOS_LINUX_PROT_READ_WRITE 3u
#define SELINOS_LINUX_MAP_PRIVATE_ANONYMOUS 0x22u
#define SELINOS_LINUX_AT_FDCWD ((seL4_Word)-100)
#define SELINOS_LINUX_SEEK_SET 0u
#define SELINOS_X86_SYSCALL_REPLY_REGISTERS (seL4_UnknownSyscall_FaultIP + 1u)
#define SELINOS_ABI_MAX_WRITE_BYTES 127u
#define SELINOS_LINUX_M10_STAT_MAGIC 0x53454c5354415430ull /* "SELSTAT0" */
#define SELINOS_LINUX_M10_STAT_SIZE 8u
#define SELINOS_LINUX_TLS_TEST_WORD 0x544c5353454c494eull /* "TLSSELIN" */
#define SELINOS_LINUX_UTSNAME_FIELD_BYTES 65u
#define SELINOS_LINUX_UTSNAME_BYTES (SELINOS_LINUX_UTSNAME_FIELD_BYTES * 6u)

static const unsigned char selinos_linux_utsname[SELINOS_LINUX_UTSNAME_BYTES] = {
    'L','i','n','u','x',0,
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES] = 's',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES + 1u] = 'e',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES + 2u] = 'l',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES + 3u] = 'i',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES + 4u] = 'n',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES + 5u] = 'o',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES + 6u] = 's',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u] = '6',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 1u] = '.',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 2u] = '1',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 3u] = '8',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 4u] = '.',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 5u] = '4',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 6u] = '4',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 7u] = '-',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 8u] = 's',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 9u] = 'e',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 10u] = 'l',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 11u] = 'i',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 12u] = 'n',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 13u] = 'o',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 2u + 14u] = 's',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u] = 'S',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 1u] = 'e',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 2u] = 'L',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 3u] = 'i',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 4u] = 'n',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 5u] = 'O',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 6u] = 'S',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 7u] = ' ',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 8u] = 'A',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 9u] = 'B',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 10u] = 'I',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 11u] = ' ',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 12u] = 'M',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 13u] = '1',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 3u + 14u] = '4',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 4u] = 'x',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 4u + 1u] = '8',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 4u + 2u] = '6',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 4u + 3u] = '_',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 4u + 4u] = '6',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u] = 'l',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 1u] = 'o',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 2u] = 'c',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 3u] = 'a',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 4u] = 'l',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 5u] = 'd',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 6u] = 'o',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 7u] = 'm',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 8u] = 'a',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 9u] = 'i',
    [SELINOS_LINUX_UTSNAME_FIELD_BYTES * 5u + 10u] = 'n',
};

static char allocator_static_pool[SELINOS_ALLOCATOR_STATIC_POOL_SIZE];
static sel4utils_alloc_data_t root_vspace_data;
/* Phase 34 prerequisite: these root-private descriptors have static lifetime.
 * No non-root CSpace receives an accessor or a construction endpoint here. */
static simple_t root_simple_context;
static vka_t root_vka_context;
static vspace_t root_vspace_context;
static bool root_construction_context_initialized;
#if !CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER
static seL4_CPtr root_dispatch_m0_endpoint = seL4_CapNull;
#endif
static seL4_CPtr selinos_romfs_root_endpoint = seL4_CapNull;

static void debug_puts(const char *text)
{
    seL4_DebugPutString((char *)text);
}

struct selinos_x86_syscall_frame {
    seL4_Word values[SELINOS_X86_SYSCALL_REPLY_REGISTERS];
};

/* The frame is captured before a handler invokes any seL4 operation. Mapping
 * a user page consumes the root IPC buffer, so reply state is restored only
 * from this local snapshot. */
static void capture_linux_x86_64_syscall(struct selinos_x86_syscall_frame *frame)
{
    for (size_t index = 0u; index < SELINOS_X86_SYSCALL_REPLY_REGISTERS; ++index) {
        frame->values[index] = seL4_GetMR(index);
    }
}

/* Reply with RAX through FaultIP only. RSP/RFLAGS/TLS retain the faulted
 * process state, while FaultIP advances over x86_64's two-byte SYSCALL. */
static void reply_linux_x86_64_syscall(const struct selinos_x86_syscall_frame *frame,
                                       seL4_Word result)
{
    for (size_t index = 0u; index < SELINOS_X86_SYSCALL_REPLY_REGISTERS; ++index) {
        seL4_SetMR(index, frame->values[index]);
    }
    seL4_SetMR(seL4_UnknownSyscall_RAX, result);
    seL4_SetMR(seL4_UnknownSyscall_FaultIP,
               frame->values[seL4_UnknownSyscall_FaultIP] + 2u);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u,
                                    SELINOS_X86_SYSCALL_REPLY_REGISTERS));
}

/* M2 explicitly permits one user-buffer page per write request. The root
 * temporarily maps only the page that backs the pointer, copies at most 127
 * bytes, then unmaps and deletes its duplicate capability before replying. */
/* A set_tid_address M1 request may identify only a mapped user page. The
 * pointer is not retained or written: clear-child-tid / clone lifecycle needs
 * taskd and is deliberately outside this bounded ABI gate. */
static bool has_linux_user_word_page(sel4utils_process_t *probe, seL4_Word user_address)
{
    const seL4_Word page_size = 1u << seL4_PageBits;
    const seL4_Word page_mask = ~(page_size - 1u);

    return probe != NULL && user_address != 0u &&
           user_address + sizeof(seL4_Word) >= user_address &&
           ((user_address + sizeof(seL4_Word) - 1u) & page_mask) ==
               (user_address & page_mask) &&
           vspace_get_cap(&probe->vspace, (void *)(user_address & page_mask)) != seL4_CapNull;
}

static bool copy_linux_write_buffer(vka_t *vka, vspace_t *root_vspace,
                                    sel4utils_process_t *probe,
                                    seL4_Word user_address, seL4_Word length,
                                    char output[SELINOS_ABI_MAX_WRITE_BYTES + 1u])
{
    const seL4_Word page_size = 1u << seL4_PageBits;
    const seL4_Word page_mask = ~(page_size - 1u);
    const seL4_Word page_base = user_address & page_mask;
    seL4_CPtr frame;
    void *mapping;

    if (length == 0u || length > SELINOS_ABI_MAX_WRITE_BYTES ||
        user_address + length < user_address ||
        ((user_address + length - 1u) & page_mask) != page_base) {
        return false;
    }
    frame = vspace_get_cap(&probe->vspace, (void *)page_base);
    if (frame == seL4_CapNull) {
        return false;
    }
    mapping = sel4utils_dup_and_map(vka, root_vspace, frame, seL4_PageBits);
    if (mapping == NULL) {
        return false;
    }
    const char *source = (const char *)mapping + (user_address - page_base);
    for (size_t index = 0u; index < length; ++index) {
        output[index] = source[index];
    }
    output[length] = '\0';
    sel4utils_unmap_dup(vka, root_vspace, mapping, seL4_PageBits);
    return true;
}

/* Copy a small NUL-terminated pathname into root-local storage and compare it
 * byte-for-byte with one declared literal. It deliberately cannot provide a
 * general pathname interface: one mapped page, one bounded literal and no
 * retained client pointer are permitted. */
static bool copy_linux_bytes_to_user(vka_t *vka, vspace_t *root_vspace,
                                     sel4utils_process_t *probe,
                                     seL4_Word user_address,
                                     const unsigned char *source, seL4_Word length)
{
    const seL4_Word page_size = 1u << seL4_PageBits;
    const seL4_Word page_mask = ~(page_size - 1u);
    const seL4_Word page_base = user_address & page_mask;
    seL4_CPtr frame;
    void *mapping;

    if (source == NULL || length == 0u || length > page_size ||
        user_address + length < user_address ||
        ((user_address + length - 1u) & page_mask) != page_base) {
        return false;
    }
    frame = vspace_get_cap(&probe->vspace, (void *)page_base);
    if (frame == seL4_CapNull) {
        return false;
    }
    mapping = sel4utils_dup_and_map(vka, root_vspace, frame, seL4_PageBits);
    if (mapping == NULL) {
        return false;
    }
    unsigned char *destination = (unsigned char *)mapping + (user_address - page_base);
    for (seL4_Word index = 0u; index < length; ++index) {
        destination[index] = source[index];
    }
    sel4utils_unmap_dup(vka, root_vspace, mapping, seL4_PageBits);
    return true;
}

static bool copy_linux_exact_string(vka_t *vka, vspace_t *root_vspace,
                                    sel4utils_process_t *probe,
                                    seL4_Word user_address, const char *expected)
{
    char copied[SELINOS_ABI_MAX_WRITE_BYTES + 1u];
    size_t length = 0u;

    if (expected == NULL) {
        return false;
    }
    while (length < SELINOS_ABI_MAX_WRITE_BYTES && expected[length] != '\0') {
        ++length;
    }
    if (length == SELINOS_ABI_MAX_WRITE_BYTES ||
        !copy_linux_write_buffer(vka, root_vspace, probe, user_address,
                                 (seL4_Word)(length + 1u), copied)) {
        return false;
    }
    for (size_t index = 0u; index <= length; ++index) {
        if (copied[index] != expected[index]) {
            return false;
        }
    }
    return true;
}

static bool copy_linux_word_to_user(vka_t *vka, vspace_t *root_vspace,
                                    sel4utils_process_t *probe,
                                    seL4_Word user_address, seL4_Word value)
{
    const seL4_Word page_size = 1u << seL4_PageBits;
    const seL4_Word page_mask = ~(page_size - 1u);
    const seL4_Word page_base = user_address & page_mask;
    seL4_CPtr frame;
    void *mapping;

    if (user_address + sizeof(value) < user_address ||
        ((user_address + sizeof(value) - 1u) & page_mask) != page_base) {
        return false;
    }
    frame = vspace_get_cap(&probe->vspace, (void *)page_base);
    if (frame == seL4_CapNull) {
        return false;
    }
    mapping = sel4utils_dup_and_map(vka, root_vspace, frame, seL4_PageBits);
    if (mapping == NULL) {
        return false;
    }
    seL4_Word *destination = (seL4_Word *)((char *)mapping + (user_address - page_base));
    *destination = value;
    sel4utils_unmap_dup(vka, root_vspace, mapping, seL4_PageBits);
    return true;
}

static bool call_romfs_root(seL4_Word operation, seL4_Word first, seL4_Word second,
                            seL4_Word length, seL4_Word *status,
                            seL4_Word *value, seL4_Word *payload)
{
    if (selinos_romfs_root_endpoint == seL4_CapNull) {
        return false;
    }
    seL4_SetMR(0, operation);
    if (length > 1u) {
        seL4_SetMR(1, first);
    }
    if (length > 2u) {
        seL4_SetMR(2, second);
    }
    seL4_MessageInfo_t reply = seL4_Call(selinos_romfs_root_endpoint,
                                         seL4_MessageInfo_new(0u, 0u, 0u, length));
    if (seL4_MessageInfo_get_label(reply) != 0u ||
        seL4_MessageInfo_get_length(reply) < 2u) {
        return false;
    }
    *status = seL4_GetMR(0);
    *value = seL4_GetMR(1);
    *payload = seL4_MessageInfo_get_length(reply) > 2u ? seL4_GetMR(2) : 0u;
    return true;
}

static bool call_romfs_root_open_path(seL4_Word byte_length, seL4_Word first_word,
                                      seL4_Word second_word, seL4_Word *status,
                                      seL4_Word *value, seL4_Word *payload)
{
    seL4_MessageInfo_t reply;

    if (selinos_romfs_root_endpoint == seL4_CapNull) {
        return false;
    }
    seL4_SetMR(0, SELINOS_ROMFS_OP_OPEN_PATH);
    seL4_SetMR(1, byte_length);
    seL4_SetMR(2, first_word);
    seL4_SetMR(3, second_word);
    reply = seL4_Call(selinos_romfs_root_endpoint,
                      seL4_MessageInfo_new(0u, 0u, 0u,
                                           SELINOS_ROMFS_OPEN_PATH_REQUEST_WORDS));
    if (seL4_MessageInfo_get_label(reply) != 0u ||
        seL4_MessageInfo_get_length(reply) < 2u) {
        return false;
    }
    *status = seL4_GetMR(0);
    *value = seL4_GetMR(1);
    *payload = seL4_MessageInfo_get_length(reply) > 2u ? seL4_GetMR(2) : 0u;
    return true;
}

/* M3/M4 allocate a single anonymous read/write 4 KiB page at a fixed test
 * address. The mapping remains process-local; root obtains no standing cap
 * beyond the process VSpace metadata it already owns during bootstrap. */
static bool provision_linux_anonymous_page(sel4utils_process_t *probe,
                                           seL4_Word fixed_address)
{
    const void *address = (void *)(uintptr_t)fixed_address;
    reservation_t reservation = vspace_reserve_range_at(&probe->vspace, (void *)address,
                                                         SELINOS_LINUX_PAGE_SIZE,
                                                         seL4_AllRights, 1);
    if (reservation.res == NULL) {
        return false;
    }
    if (vspace_new_pages_at_vaddr(&probe->vspace, (void *)address, 1u,
                                  seL4_PageBits, reservation) != seL4_NoError) {
        vspace_free_reservation(&probe->vspace, reservation);
        return false;
    }
    return true;
}

static bool initialise_root_environment(simple_t *simple, vka_t *vka, vspace_t *vspace)
{
    allocman_t *allocman;
    void *virtual_pool;
    reservation_t reservation;
    int error;

    seL4_BootInfo *bootinfo = sel4runtime_bootinfo();
    if (bootinfo == NULL) {
        return false;
    }

    simple_default_init_bootinfo(simple, bootinfo);
    allocman = bootstrap_use_current_simple(simple, SELINOS_ALLOCATOR_STATIC_POOL_SIZE,
                                            allocator_static_pool);
    if (allocman == NULL) {
        return false;
    }

    allocman_make_vka(vka, allocman);
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(
        vspace, &root_vspace_data, simple_get_pd(simple), vka, bootinfo);
    if (error != 0) {
        return false;
    }

    reservation = vspace_reserve_range(vspace, SELINOS_ALLOCATOR_VIRTUAL_POOL_SIZE,
                                       seL4_AllRights, 1, &virtual_pool);
    if (reservation.res == NULL) {
        return false;
    }

        bootstrap_configure_virtual_pool(allocman, virtual_pool,
                                      SELINOS_ALLOCATOR_VIRTUAL_POOL_SIZE,
                                      simple_get_pd(simple));

    return true;
}

static bool start_one_service(vka_t *vka, vspace_t *vspace, const char *image_name,
                              const char *thread_name)
{
    sel4utils_process_t service;
    char *argv[] = {(char *)image_name, NULL};
    int error = sel4utils_configure_process(&service, vka, vspace, image_name);
    if (error != 0) {
        return false;
    }

#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(service.thread.tcb.cptr, (char *)thread_name);
#endif

    error = sel4utils_spawn_process_v(&service, vka, vspace, 1, argv, 1);
    return error == 0;
}

/* ROMFS M1 grants one endpoint only to a server and its single test client.
 * The root retains no data-path role after startup; neither process gets a
 * device, IRQ, DMA, PCI or raw VSpace capability. */
/* Phase 28 M1 wires an objectd-local reservation-state fixture. Root gives
 * only one endpoint to objectd and its isolated probe; no allocator, object
 * cap or device authority is delegated. */
static bool start_objectd_fixed_inventory_m1_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t objectd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    char *objectd_argv[] = {"selinos-objectd", NULL};
    char *probe_argv[] = {"selinos-objectd-m1-probe", NULL};
    seL4_CPtr objectd_slot;
    seL4_CPtr probe_slot;

    if (sel4utils_configure_process(&objectd, vka, vspace, "selinos-objectd") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-objectd-m1-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError) {
        return false;
    }
    objectd_slot = sel4utils_copy_cap_to_process(&objectd, vka, endpoint.cptr);
    probe_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    if (objectd_slot != SELINOS_OBJECTD_M1_ENDPOINT_SLOT ||
        probe_slot != SELINOS_OBJECTD_M1_PROBE_ENDPOINT_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(objectd.thread.tcb.cptr, "selinos-objectd");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-objectd-m1-probe");
#endif
    if (sel4utils_spawn_process_v(&objectd, vka, vspace, 1, objectd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    return true;
}

#if !CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER
/* Phase 35 M0 grants the dedicated probe only one send cap; root retains the
 * endpoint receive cap for a fixed status reply and does not construct a child. */
static bool start_root_dispatch_m0_probe_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t probe;
    vka_object_t endpoint;
    char *probe_argv[] = {"selinos-root-dispatch-m0-probe", NULL};
    seL4_CPtr probe_slot;

    if (root_dispatch_m0_endpoint != seL4_CapNull ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-root-dispatch-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError) {
        return false;
    }
    probe_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    if (probe_slot != SELINOS_ROOT_DISPATCH_M0_PROBE_ENDPOINT_SLOT) {
        return false;
    }
    root_dispatch_m0_endpoint = endpoint.cptr;
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-root-dispatch-m0-probe");
#endif
    if (sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        root_dispatch_m0_endpoint = seL4_CapNull;
        return false;
    }
    seL4_DebugPutString("SeLinOS root dispatch M0: isolated request endpoint ready.\n");
    return true;
}

bool selinos_root_dispatch_m0_once(void)
{
    seL4_Word badge = 0u;
    if (!root_construction_context_initialized || root_dispatch_m0_endpoint == seL4_CapNull) {
        return false;
    }
    seL4_MessageInfo_t message = seL4_Recv(root_dispatch_m0_endpoint, &badge);
    if (seL4_MessageInfo_get_label(message) != 0u ||
        seL4_MessageInfo_get_length(message) != SELINOS_ROOT_DISPATCH_M0_WORDS ||
        seL4_MessageInfo_get_extraCaps(message) != 0u ||
        seL4_GetMR(0) != SELINOS_ROOT_DISPATCH_M0_REQUEST ||
        seL4_GetMR(1) != SELINOS_ROOT_DISPATCH_M0_SLOT ||
        seL4_GetMR(2) != SELINOS_ROOT_DISPATCH_M0_GENERATION) {
        seL4_SetMR(0, SELINOS_ROOT_DISPATCH_M0_REQUEST);
        seL4_SetMR(1, SELINOS_ROOT_DISPATCH_M0_SLOT);
        seL4_SetMR(2, SELINOS_ROOT_DISPATCH_M0_GENERATION);
        seL4_Reply(seL4_MessageInfo_new((seL4_Word)seL4_InvalidArgument, 0u, 0u,
                                        SELINOS_ROOT_DISPATCH_M0_WORDS));
        return false;
    }
    seL4_DebugPutString("SeLinOS root dispatch M0: exact fixed request accepted.\n");
    seL4_SetMR(0, SELINOS_ROOT_DISPATCH_M0_READY);
    seL4_SetMR(1, SELINOS_ROOT_DISPATCH_M0_SLOT);
    seL4_SetMR(2, SELINOS_ROOT_DISPATCH_M0_GENERATION);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, SELINOS_ROOT_DISPATCH_M0_WORDS));
    seL4_DebugPutString("SeLinOS root dispatch M0: fixed status reply issued; no child constructed.\n");
    return true;
}
#endif

/* Phase 33 M2 transfers one fresh child TCB cap and permits one subsequent
 * taskd resume. Root configures the child and spawns it suspended. */
static bool start_transferred_tcb_single_resume_m2_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t objectd;
    sel4utils_process_t child;
    sel4utils_process_t probe;
    vka_object_t client_endpoint;
    vka_object_t object_endpoint;
    vka_object_t child_completion_endpoint;
    vka_object_t success_notification;
    char *taskd_argv[] = {"selinos-taskd-tcb-resume-m2", NULL};
    char *objectd_argv[] = {"selinos-objectd-tcb-resume-m2", NULL};
    char *child_argv[] = {"selinos-tcb-resume-m2-child", NULL};
    char *probe_argv[] = {"selinos-taskd-tcb-resume-m2-probe", NULL};
    seL4_CPtr taskd_client_slot;
    seL4_CPtr taskd_object_slot;
    seL4_CPtr taskd_completion_slot;
    seL4_CPtr taskd_success_slot;
    seL4_CPtr objectd_endpoint_slot;
    seL4_CPtr objectd_tcb_slot;
    seL4_CPtr child_completion_slot;
    seL4_CPtr probe_client_slot;
    seL4_CPtr probe_success_slot;

    if (sel4utils_configure_process(&taskd, vka, vspace, "selinos-taskd-tcb-resume-m2") != 0 ||
        sel4utils_configure_process(&objectd, vka, vspace, "selinos-objectd-tcb-resume-m2") != 0 ||
        sel4utils_configure_process(&child, vka, vspace, "selinos-tcb-resume-m2-child") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-taskd-tcb-resume-m2-probe") != 0 ||
        vka_alloc_endpoint(vka, &client_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &object_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &child_completion_endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success_notification) != seL4_NoError) {
        return false;
    }
    taskd_client_slot = sel4utils_copy_cap_to_process(&taskd, vka, client_endpoint.cptr);
    taskd_object_slot = sel4utils_copy_cap_to_process(&taskd, vka, object_endpoint.cptr);
    taskd_completion_slot = sel4utils_copy_cap_to_process(&taskd, vka, child_completion_endpoint.cptr);
    taskd_success_slot = sel4utils_copy_cap_to_process(&taskd, vka, success_notification.cptr);
    objectd_endpoint_slot = sel4utils_copy_cap_to_process(&objectd, vka, object_endpoint.cptr);
    objectd_tcb_slot = sel4utils_copy_cap_to_process(&objectd, vka, child.thread.tcb.cptr);
    child_completion_slot = sel4utils_copy_cap_to_process(&child, vka, child_completion_endpoint.cptr);
    probe_client_slot = sel4utils_copy_cap_to_process(&probe, vka, client_endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success_notification.cptr);
    if (taskd_client_slot != SELINOS_TCB_RESUME_M2_TASKD_CLIENT_ENDPOINT_SLOT ||
        taskd_object_slot != SELINOS_TCB_RESUME_M2_TASKD_OBJECT_ENDPOINT_SLOT ||
        taskd_completion_slot != SELINOS_TCB_RESUME_M2_TASKD_CHILD_COMPLETION_SLOT ||
        taskd_success_slot != SELINOS_TCB_RESUME_M2_TASKD_SUCCESS_NOTIFY_SLOT ||
        objectd_endpoint_slot != SELINOS_TCB_RESUME_M2_OBJECT_ENDPOINT_SLOT ||
        objectd_tcb_slot != SELINOS_TCB_RESUME_M2_OBJECT_TCB_SOURCE_SLOT ||
        child_completion_slot != SELINOS_TCB_RESUME_M2_CHILD_COMPLETION_SEND_SLOT ||
        probe_client_slot != SELINOS_TCB_RESUME_M2_PROBE_CLIENT_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TCB_RESUME_M2_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(taskd.thread.tcb.cptr, "selinos-taskd-tcb-resume-m2");
    seL4_DebugNameThread(objectd.thread.tcb.cptr, "selinos-objectd-tcb-resume-m2");
    seL4_DebugNameThread(child.thread.tcb.cptr, "selinos-tcb-resume-m2-child");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-taskd-tcb-resume-m2-probe");
#endif
    if (sel4utils_spawn_process_v(&child, vka, vspace, 1, child_argv, 0) != 0 ||
        sel4utils_spawn_process_v(&objectd, vka, vspace, 1, objectd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    seL4_DebugPutString("SeLinOS TCB resume M2: fresh child spawned suspended for one taskd resume.\n");
    return true;
}

/* Phase 32 M1 transfers one cap for a root-preprovisioned child TCB. The
 * child remains configured but unspawned, so taskd cannot exercise the cap. */
static bool start_tcb_control_lease_m1_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t objectd;
    sel4utils_process_t child;
    sel4utils_process_t probe;
    vka_object_t client_endpoint;
    vka_object_t object_endpoint;
    vka_object_t success_notification;
    char *taskd_argv[] = {"selinos-taskd-tcb-lease-m1", NULL};
    char *objectd_argv[] = {"selinos-objectd-tcb-lease-m1", NULL};
    char *probe_argv[] = {"selinos-taskd-tcb-lease-m1-probe", NULL};
    seL4_CPtr taskd_client_slot;
    seL4_CPtr taskd_object_slot;
    seL4_CPtr taskd_success_slot;
    seL4_CPtr objectd_endpoint_slot;
    seL4_CPtr objectd_tcb_slot;
    seL4_CPtr probe_client_slot;
    seL4_CPtr probe_success_slot;

    if (sel4utils_configure_process(&taskd, vka, vspace, "selinos-taskd-tcb-lease-m1") != 0 ||
        sel4utils_configure_process(&objectd, vka, vspace, "selinos-objectd-tcb-lease-m1") != 0 ||
        sel4utils_configure_process(&child, vka, vspace, "selinos-tcb-lease-suspended-child") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-taskd-tcb-lease-m1-probe") != 0 ||
        vka_alloc_endpoint(vka, &client_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &object_endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success_notification) != seL4_NoError) {
        return false;
    }
    taskd_client_slot = sel4utils_copy_cap_to_process(&taskd, vka, client_endpoint.cptr);
    taskd_object_slot = sel4utils_copy_cap_to_process(&taskd, vka, object_endpoint.cptr);
    taskd_success_slot = sel4utils_copy_cap_to_process(&taskd, vka, success_notification.cptr);
    objectd_endpoint_slot = sel4utils_copy_cap_to_process(&objectd, vka, object_endpoint.cptr);
    objectd_tcb_slot = sel4utils_copy_cap_to_process(&objectd, vka, child.thread.tcb.cptr);
    probe_client_slot = sel4utils_copy_cap_to_process(&probe, vka, client_endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success_notification.cptr);
    if (taskd_client_slot != SELINOS_TCB_LEASE_M1_TASKD_CLIENT_ENDPOINT_SLOT ||
        taskd_object_slot != SELINOS_TCB_LEASE_M1_TASKD_OBJECT_ENDPOINT_SLOT ||
        taskd_success_slot != SELINOS_TCB_LEASE_M1_TASKD_SUCCESS_NOTIFY_SLOT ||
        objectd_endpoint_slot != SELINOS_TCB_LEASE_M1_OBJECT_ENDPOINT_SLOT ||
        objectd_tcb_slot != SELINOS_TCB_LEASE_M1_OBJECT_TCB_SOURCE_SLOT ||
        probe_client_slot != SELINOS_TCB_LEASE_M1_PROBE_CLIENT_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TCB_LEASE_M1_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(taskd.thread.tcb.cptr, "selinos-taskd-tcb-lease-m1");
    seL4_DebugNameThread(objectd.thread.tcb.cptr, "selinos-objectd-tcb-lease-m1");
    seL4_DebugNameThread(child.thread.tcb.cptr, "selinos-tcb-lease-suspended-child");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-taskd-tcb-lease-m1-probe");
#endif
    seL4_DebugPutString("SeLinOS TCB lease M1: dedicated child fixture configured and deliberately suspended.\n");
    if (sel4utils_spawn_process_v(&objectd, vka, vspace, 1, objectd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    return true;
}

/* Phase 31 M1 transfers one root-preprovisioned notification token through
 * objectd to taskd. The token is opaque and provides no task-resource claim. */
static bool start_opaque_lease_m1_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t objectd;
    sel4utils_process_t probe;
    vka_object_t client_endpoint;
    vka_object_t object_endpoint;
    vka_object_t success_notification;
    vka_object_t opaque_token;
    char *taskd_argv[] = {"selinos-taskd-opaque-lease-m1", NULL};
    char *objectd_argv[] = {"selinos-objectd-opaque-lease-m1", NULL};
    char *probe_argv[] = {"selinos-taskd-opaque-lease-m1-probe", NULL};
    seL4_CPtr taskd_client_slot;
    seL4_CPtr taskd_object_slot;
    seL4_CPtr taskd_success_slot;
    seL4_CPtr objectd_endpoint_slot;
    seL4_CPtr objectd_token_slot;
    seL4_CPtr probe_client_slot;
    seL4_CPtr probe_success_slot;

    if (sel4utils_configure_process(&taskd, vka, vspace, "selinos-taskd-opaque-lease-m1") != 0 ||
        sel4utils_configure_process(&objectd, vka, vspace, "selinos-objectd-opaque-lease-m1") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-taskd-opaque-lease-m1-probe") != 0 ||
        vka_alloc_endpoint(vka, &client_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &object_endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success_notification) != seL4_NoError ||
        vka_alloc_notification(vka, &opaque_token) != seL4_NoError) {
        return false;
    }
    taskd_client_slot = sel4utils_copy_cap_to_process(&taskd, vka, client_endpoint.cptr);
    taskd_object_slot = sel4utils_copy_cap_to_process(&taskd, vka, object_endpoint.cptr);
    taskd_success_slot = sel4utils_copy_cap_to_process(&taskd, vka, success_notification.cptr);
    objectd_endpoint_slot = sel4utils_copy_cap_to_process(&objectd, vka, object_endpoint.cptr);
    objectd_token_slot = sel4utils_copy_cap_to_process(&objectd, vka, opaque_token.cptr);
    probe_client_slot = sel4utils_copy_cap_to_process(&probe, vka, client_endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success_notification.cptr);
    if (taskd_client_slot != SELINOS_OPAQUE_LEASE_M1_TASKD_CLIENT_ENDPOINT_SLOT ||
        taskd_object_slot != SELINOS_OPAQUE_LEASE_M1_TASKD_OBJECT_ENDPOINT_SLOT ||
        taskd_success_slot != SELINOS_OPAQUE_LEASE_M1_TASKD_SUCCESS_NOTIFY_SLOT ||
        objectd_endpoint_slot != SELINOS_OPAQUE_LEASE_M1_OBJECT_ENDPOINT_SLOT ||
        objectd_token_slot != SELINOS_OPAQUE_LEASE_M1_OBJECT_TOKEN_SOURCE_SLOT ||
        probe_client_slot != SELINOS_OPAQUE_LEASE_M1_PROBE_CLIENT_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_OPAQUE_LEASE_M1_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(taskd.thread.tcb.cptr, "selinos-taskd-opaque-lease-m1");
    seL4_DebugNameThread(objectd.thread.tcb.cptr, "selinos-objectd-opaque-lease-m1");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-taskd-opaque-lease-m1-probe");
#endif
    if (sel4utils_spawn_process_v(&objectd, vka, vspace, 1, objectd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    return true;
}

/* Phase 30 M1.5 coordinates two status-only reservation services through a
 * dedicated taskd. No lease cap, object, mapping or child control cap exists
 * in this bundle. */
static bool start_taskd_combined_reservation_m1_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t objectd;
    sel4utils_process_t memd;
    sel4utils_process_t probe;
    vka_object_t client_endpoint;
    vka_object_t object_endpoint;
    vka_object_t mem_endpoint;
    vka_object_t success_notification;
    char *taskd_argv[] = {"selinos-taskd-reservation-m1", NULL};
    char *objectd_argv[] = {"selinos-objectd-reservation-m2", NULL};
    char *memd_argv[] = {"selinos-memd-reservation-m2", NULL};
    char *probe_argv[] = {"selinos-taskd-reservation-m1-probe", NULL};
    seL4_CPtr taskd_client_slot;
    seL4_CPtr taskd_object_slot;
    seL4_CPtr taskd_mem_slot;
    seL4_CPtr taskd_success_slot;
    seL4_CPtr objectd_slot;
    seL4_CPtr memd_slot;
    seL4_CPtr probe_client_slot;
    seL4_CPtr probe_success_slot;

    if (sel4utils_configure_process(&taskd, vka, vspace, "selinos-taskd-reservation-m1") != 0 ||
        sel4utils_configure_process(&objectd, vka, vspace, "selinos-objectd-reservation-m2") != 0 ||
        sel4utils_configure_process(&memd, vka, vspace, "selinos-memd-reservation-m2") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-taskd-reservation-m1-probe") != 0 ||
        vka_alloc_endpoint(vka, &client_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &object_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &mem_endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success_notification) != seL4_NoError) {
        return false;
    }

    taskd_client_slot = sel4utils_copy_cap_to_process(&taskd, vka, client_endpoint.cptr);
    taskd_object_slot = sel4utils_copy_cap_to_process(&taskd, vka, object_endpoint.cptr);
    taskd_mem_slot = sel4utils_copy_cap_to_process(&taskd, vka, mem_endpoint.cptr);
    taskd_success_slot = sel4utils_copy_cap_to_process(&taskd, vka, success_notification.cptr);
    objectd_slot = sel4utils_copy_cap_to_process(&objectd, vka, object_endpoint.cptr);
    memd_slot = sel4utils_copy_cap_to_process(&memd, vka, mem_endpoint.cptr);
    probe_client_slot = sel4utils_copy_cap_to_process(&probe, vka, client_endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success_notification.cptr);
    if (taskd_client_slot != SELINOS_TASKD_RES_M1_CLIENT_ENDPOINT_SLOT ||
        taskd_object_slot != SELINOS_TASKD_RES_M1_OBJECT_ENDPOINT_SLOT ||
        taskd_mem_slot != SELINOS_TASKD_RES_M1_MEM_ENDPOINT_SLOT ||
        taskd_success_slot != SELINOS_TASKD_RES_M1_SUCCESS_NOTIFY_SLOT ||
        objectd_slot != SELINOS_TASKD_RES_M1_OBJECT_SERVER_ENDPOINT_SLOT ||
        memd_slot != SELINOS_TASKD_RES_M1_MEM_SERVER_ENDPOINT_SLOT ||
        probe_client_slot != SELINOS_TASKD_RES_M1_PROBE_CLIENT_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_RES_M1_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(taskd.thread.tcb.cptr, "selinos-taskd-reservation-m1");
    seL4_DebugNameThread(objectd.thread.tcb.cptr, "selinos-objectd-reservation-m2");
    seL4_DebugNameThread(memd.thread.tcb.cptr, "selinos-memd-reservation-m2");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-taskd-reservation-m1-probe");
#endif
    if (sel4utils_spawn_process_v(&objectd, vka, vspace, 1, objectd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&memd, vka, vspace, 1, memd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    return true;
}

/* Phase 29 M1 wires a memd-local fixed-plan reservation fixture. Root gives
 * only one endpoint to memd and its isolated probe; no VSpace, frame, mapping
 * or device authority is delegated. */
static bool start_memd_fixed_mapping_inventory_m1_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t memd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    char *memd_argv[] = {"selinos-memd", NULL};
    char *probe_argv[] = {"selinos-memd-m1-probe", NULL};
    seL4_CPtr memd_slot;
    seL4_CPtr probe_slot;

    if (sel4utils_configure_process(&memd, vka, vspace, "selinos-memd") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-memd-m1-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError) {
        return false;
    }
    memd_slot = sel4utils_copy_cap_to_process(&memd, vka, endpoint.cptr);
    probe_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    if (memd_slot != SELINOS_MEMD_M1_ENDPOINT_SLOT ||
        probe_slot != SELINOS_MEMD_M1_PROBE_ENDPOINT_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(memd.thread.tcb.cptr, "selinos-memd");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-memd-m1-probe");
#endif
    if (sel4utils_spawn_process_v(&memd, vka, vspace, 1, memd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    return true;
}

/* Phase 25 M1 wires one static child lifecycle fixture. Root constructs the
 * processes and IPC objects once, then delegates exactly the child TCB and
 * protocol caps to taskd; it does not activate or control the child after the
 * three images are spawned. This is not dynamic task construction or clone. */
static bool start_taskd_fixed_child_lifecycle_m1_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t child;
    sel4utils_process_t probe;
    vka_object_t client_endpoint;
    vka_object_t child_completion_endpoint;
    vka_object_t child_start_notification;
    vka_object_t probe_success_notification;
    char *taskd_argv[] = {"selinos-taskd", NULL};
    char *child_argv[] = {"selinos-task-lifecycle-child", NULL};
    char *probe_argv[] = {"selinos-task-lifecycle-probe", NULL};
    seL4_CPtr taskd_client_slot;
    seL4_CPtr taskd_completion_slot;
    seL4_CPtr taskd_start_slot;
    seL4_CPtr taskd_child_tcb_slot;
    seL4_CPtr taskd_probe_success_slot;
    seL4_CPtr child_start_slot;
    seL4_CPtr child_completion_slot;
    seL4_CPtr probe_client_slot;
    seL4_CPtr probe_success_slot;

    if (sel4utils_configure_process(&taskd, vka, vspace, "selinos-taskd") != 0 ||
        sel4utils_configure_process(&child, vka, vspace,
                                    "selinos-task-lifecycle-child") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-task-lifecycle-probe") != 0 ||
        vka_alloc_endpoint(vka, &client_endpoint) != seL4_NoError ||
        vka_alloc_endpoint(vka, &child_completion_endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &child_start_notification) != seL4_NoError ||
        vka_alloc_notification(vka, &probe_success_notification) != seL4_NoError) {
        return false;
    }

    taskd_client_slot = sel4utils_copy_cap_to_process(&taskd, vka, client_endpoint.cptr);
    taskd_completion_slot = sel4utils_copy_cap_to_process(&taskd, vka,
                                                           child_completion_endpoint.cptr);
    taskd_start_slot = sel4utils_copy_cap_to_process(&taskd, vka,
                                                      child_start_notification.cptr);
    taskd_child_tcb_slot = sel4utils_copy_cap_to_process(&taskd, vka,
                                                          child.thread.tcb.cptr);
    taskd_probe_success_slot = sel4utils_copy_cap_to_process(&taskd, vka,
                                                              probe_success_notification.cptr);
    child_start_slot = sel4utils_copy_cap_to_process(&child, vka,
                                                      child_start_notification.cptr);
    child_completion_slot = sel4utils_copy_cap_to_process(&child, vka,
                                                           child_completion_endpoint.cptr);
    probe_client_slot = sel4utils_copy_cap_to_process(&probe, vka, client_endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka,
                                                        probe_success_notification.cptr);
    if (taskd_client_slot != SELINOS_TASKD_M1_CLIENT_ENDPOINT_SLOT ||
        taskd_completion_slot != SELINOS_TASKD_M1_CHILD_COMPLETION_SLOT ||
        taskd_start_slot != SELINOS_TASKD_M1_CHILD_START_SLOT ||
        taskd_child_tcb_slot != SELINOS_TASKD_M1_CHILD_TCB_SLOT ||
        taskd_probe_success_slot != SELINOS_TASKD_M1_PROBE_SUCCESS_SLOT ||
        child_start_slot != SELINOS_TASKD_M1_CHILD_START_WAIT_SLOT ||
        child_completion_slot != SELINOS_TASKD_M1_CHILD_COMPLETION_SEND_SLOT ||
        probe_client_slot != SELINOS_TASKD_M1_PROBE_CLIENT_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_M1_PROBE_SUCCESS_WAIT_SLOT) {
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(taskd.thread.tcb.cptr, "selinos-taskd");
    seL4_DebugNameThread(child.thread.tcb.cptr, "selinos-task-child");
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-task-probe");
#endif
    if (sel4utils_spawn_process_v(&child, vka, vspace, 1, child_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    return true;
}

static bool start_romfs_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t server;
    sel4utils_process_t client;
    vka_object_t endpoint;
    char *server_argv[] = {"selinos-romfsd", NULL};
    char *client_argv[] = {"selinos-romfs-probe", NULL};
    seL4_CPtr server_slot;
    seL4_CPtr client_slot;

    if (sel4utils_configure_process(&server, vka, vspace, "selinos-romfsd") != 0 ||
        sel4utils_configure_process(&client, vka, vspace, "selinos-romfs-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError) {
        return false;
    }
    server_slot = sel4utils_copy_cap_to_process(&server, vka, endpoint.cptr);
    client_slot = sel4utils_copy_cap_to_process(&client, vka, endpoint.cptr);
    if (server_slot != SELINOS_ROMFS_ENDPOINT_SLOT ||
        client_slot != SELINOS_ROMFS_ENDPOINT_SLOT) {
        return false;
    }
    selinos_romfs_root_endpoint = endpoint.cptr;
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(server.thread.tcb.cptr, "selinos-romfsd");
    seL4_DebugNameThread(client.thread.tcb.cptr, "selinos-romfs-probe");
#endif
    if (sel4utils_spawn_process_v(&server, vka, vspace, 1, server_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&client, vka, vspace, 1, client_argv, 1) != 0) {
        return false;
    }
    return true;
}

/* Development ABI M5: root-held dispatch for an isolated probe. It permits
 * bounded identity/TID returns, a mapped-page-only set_tid_address acknowledgement,
 * bounded stdout/stderr write, one anonymous mmap, fixed brk, a ROMFS lifecycle,
 * zero-length stdin EOF read and exit(0). No clone, TLS or scheduler semantics. */
static bool run_linux_syscall_abi_probe(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t probe;
    char *argv[] = {"selinos-linux-syscall-probe", NULL};
    seL4_Word badge = 0u;
    seL4_Word probe_tls_base = 0u;
    seL4_MessageInfo_t fault;
    struct selinos_x86_syscall_frame frame;
    char console_message[SELINOS_ABI_MAX_WRITE_BYTES + 1u];

    if (sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-linux-syscall-probe") != 0) {
        debug_puts("SeLinOS ABI gateway: probe process configuration failed.\n");
        return false;
    }
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(probe.thread.tcb.cptr, "selinos-linux-syscall-probe");
#endif
    if (sel4utils_spawn_process_v(&probe, vka, vspace, 1, argv, 1) != 0 ||
        probe.fault_endpoint.cptr == seL4_CapNull) {
        debug_puts("SeLinOS ABI gateway: probe spawn or fault endpoint failed.\n");
        return false;
    }

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected getpid UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_GETPID) {
        debug_puts("SeLinOS ABI gateway: expected Linux getpid number in RAX.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_LINUX_TEST_PID);

    const seL4_Word credential_syscalls[] = {
        SELINOS_LINUX_GETUID,
        SELINOS_LINUX_GETGID,
        SELINOS_LINUX_GETEUID,
        SELINOS_LINUX_GETEGID,
    };
    for (size_t index = 0u;
         index < sizeof(credential_syscalls) / sizeof(credential_syscalls[0]); ++index) {
        fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
        if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
            debug_puts("SeLinOS ABI gateway: expected credential UnknownSyscall fault.\n");
            return false;
        }
        capture_linux_x86_64_syscall(&frame);
        if (frame.values[seL4_UnknownSyscall_RAX] != credential_syscalls[index]) {
            debug_puts("SeLinOS ABI gateway: unexpected credential syscall number.\n");
            return false;
        }
        reply_linux_x86_64_syscall(&frame, 0u);
    }

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected gettid UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_GETTID) {
        debug_puts("SeLinOS ABI gateway: expected Linux gettid number in RAX.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_LINUX_TEST_TID);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected set_tid_address UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_SET_TID_ADDRESS ||
        !has_linux_user_word_page(&probe, frame.values[seL4_UnknownSyscall_RDI])) {
        debug_puts("SeLinOS ABI gateway: restricted set_tid_address rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_LINUX_TEST_TID);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected write UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_WRITE ||
        (frame.values[seL4_UnknownSyscall_RDI] != 1u &&
         frame.values[seL4_UnknownSyscall_RDI] != 2u) ||
        !copy_linux_write_buffer(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 frame.values[seL4_UnknownSyscall_RDX],
                                 console_message)) {
        debug_puts("SeLinOS ABI gateway: restricted Linux write rejected.\n");
        return false;
    }
    const seL4_Word write_length = frame.values[seL4_UnknownSyscall_RDX];
    reply_linux_x86_64_syscall(&frame, write_length);
    debug_puts(console_message);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected mmap UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_MMAP ||
        frame.values[seL4_UnknownSyscall_RDI] != 0u ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_PAGE_SIZE ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_LINUX_PROT_READ_WRITE ||
        frame.values[seL4_UnknownSyscall_R10] != SELINOS_LINUX_MAP_PRIVATE_ANONYMOUS ||
        frame.values[seL4_UnknownSyscall_R8] != (seL4_Word)-1 ||
        frame.values[seL4_UnknownSyscall_R9] != 0u ||
        !provision_linux_anonymous_page(&probe, SELINOS_LINUX_MMAP_ADDRESS)) {
        debug_puts("SeLinOS ABI gateway: restricted anonymous mmap rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_LINUX_MMAP_ADDRESS);

    /* M14 returns one fully pinned x86_64 utsname-layout record. It has no
     * hostname namespace or host-identity input. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected uname UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_UNAME ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_MMAP_ADDRESS ||
        !copy_linux_bytes_to_user(vka, vspace, &probe,
                                  frame.values[seL4_UnknownSyscall_RDI],
                                  selinos_linux_utsname, SELINOS_LINUX_UTSNAME_BYTES)) {
        debug_puts("SeLinOS ABI gateway: restricted uname rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    /* M12 does not depend on a nonexistent FS-base fault register: root holds
     * the probe TCB cap and invokes the explicit seL4 TLS-base operation only
     * for the single anonymous page it just provisioned. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected arch_prctl UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_ARCH_PRCTL ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_ARCH_SET_FS ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_MMAP_ADDRESS ||
        !has_linux_user_word_page(&probe, frame.values[seL4_UnknownSyscall_RSI]) ||
        seL4_TCB_SetTLSBase(probe.thread.tcb.cptr,
                            frame.values[seL4_UnknownSyscall_RSI]) != seL4_NoError) {
        debug_puts("SeLinOS ABI gateway: restricted ARCH_SET_FS rejected.\n");
        return false;
    }
    probe_tls_base = frame.values[seL4_UnknownSyscall_RSI];
    reply_linux_x86_64_syscall(&frame, 0u);

    /* M13 returns only the M12 base root itself accepted. It does not inspect
     * a remote TCB state or expose a general TLS query primitive. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected ARCH_GET_FS UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_ARCH_PRCTL ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_ARCH_GET_FS ||
        frame.values[seL4_UnknownSyscall_RSI] == 0u ||
        probe_tls_base != SELINOS_LINUX_MMAP_ADDRESS ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI], probe_tls_base)) {
        debug_puts("SeLinOS ABI gateway: restricted ARCH_GET_FS rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected clock_gettime UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_CLOCK_GETTIME ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_CLOCK_MONOTONIC ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_MMAP_ADDRESS ||
        !copy_linux_word_to_user(vka, vspace, &probe, SELINOS_LINUX_MMAP_ADDRESS,
                                 SELINOS_LINUX_TEST_TIME_SECONDS) ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 SELINOS_LINUX_MMAP_ADDRESS + sizeof(seL4_Word),
                                 SELINOS_LINUX_TEST_TIME_NANOSECONDS)) {
        debug_puts("SeLinOS ABI gateway: deterministic clock_gettime rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected getrlimit UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_GETRLIMIT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_RLIMIT_NOFILE ||
        frame.values[seL4_UnknownSyscall_RSI] !=
            SELINOS_LINUX_MMAP_ADDRESS + 2u * sizeof(seL4_Word) ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 SELINOS_LINUX_MMAP_ADDRESS + 2u * sizeof(seL4_Word),
                                 SELINOS_LINUX_TEST_RLIMIT_NOFILE) ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 SELINOS_LINUX_MMAP_ADDRESS + 3u * sizeof(seL4_Word),
                                 SELINOS_LINUX_TEST_RLIMIT_NOFILE)) {
        debug_puts("SeLinOS ABI gateway: deterministic getrlimit rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected futex wait UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_FUTEX ||
        !has_linux_user_word_page(&probe, frame.values[seL4_UnknownSyscall_RDI]) ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_FUTEX_WAIT ||
        frame.values[seL4_UnknownSyscall_RDX] != 1u ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        frame.values[seL4_UnknownSyscall_R8] != 0u ||
        frame.values[seL4_UnknownSyscall_R9] != 0u) {
        debug_puts("SeLinOS ABI gateway: bounded futex wait rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, (seL4_Word)-SELINOS_LINUX_EAGAIN);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected futex wake UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_FUTEX ||
        !has_linux_user_word_page(&probe, frame.values[seL4_UnknownSyscall_RDI]) ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_FUTEX_WAKE ||
        frame.values[seL4_UnknownSyscall_RDX] != 1u ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        frame.values[seL4_UnknownSyscall_R8] != 0u ||
        frame.values[seL4_UnknownSyscall_R9] != 0u) {
        debug_puts("SeLinOS ABI gateway: bounded futex wake rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected brk query UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_BRK ||
        frame.values[seL4_UnknownSyscall_RDI] != 0u ||
        !provision_linux_anonymous_page(&probe, SELINOS_LINUX_BRK_BASE)) {
        debug_puts("SeLinOS ABI gateway: restricted brk query rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_LINUX_BRK_BASE);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected brk grow UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_BRK ||
        frame.values[seL4_UnknownSyscall_RDI] !=
            SELINOS_LINUX_BRK_BASE + SELINOS_LINUX_PAGE_SIZE) {
        debug_puts("SeLinOS ABI gateway: restricted brk grow rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame,
                               SELINOS_LINUX_BRK_BASE + SELINOS_LINUX_PAGE_SIZE);

    seL4_Word romfs_status;
    seL4_Word romfs_value;
    seL4_Word romfs_payload;
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected openat UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_OPENAT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_AT_FDCWD ||
        !copy_linux_exact_string(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 "/selinos-release") ||
        frame.values[seL4_UnknownSyscall_RDX] != 0u ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        !call_romfs_root(SELINOS_ROMFS_OP_OPEN, SELINOS_ROMFS_FILE_RELEASE, 0u, 2u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_FD_RELEASE) {
        debug_puts("SeLinOS ABI gateway: restricted ROMFS openat rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, romfs_value);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected ROMFS read UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_READ ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_RELEASE ||
        frame.values[seL4_UnknownSyscall_RSI] == 0u ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_RELEASE_MAGIC_LENGTH ||
        !call_romfs_root(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_RELEASE, 0u, 3u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_RELEASE_LENGTH ||
        romfs_payload != SELINOS_ROMFS_RELEASE_MAGIC ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI], romfs_payload)) {
        debug_puts("SeLinOS ABI gateway: restricted ROMFS read rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_ROMFS_RELEASE_MAGIC_LENGTH);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected close UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_CLOSE ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_RELEASE ||
        !call_romfs_root(SELINOS_ROMFS_OP_CLOSE, SELINOS_ROMFS_FD_RELEASE, 0u, 2u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK || romfs_value != 0u) {
        debug_puts("SeLinOS ABI gateway: restricted ROMFS close rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    /* M9 exposes one additional immutable CPIO record through the same exact
     * pathname and fixed-FD mediation pattern; it is not a general open path. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected version openat UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_OPENAT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_AT_FDCWD ||
        !copy_linux_exact_string(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 "/selinos-version") ||
        frame.values[seL4_UnknownSyscall_RDX] != 0u ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        !call_romfs_root(SELINOS_ROMFS_OP_OPEN, SELINOS_ROMFS_FILE_VERSION, 0u, 2u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_FD_VERSION) {
        debug_puts("SeLinOS ABI gateway: restricted version openat rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, romfs_value);

    /* M15 observes the fixed read-only status of the one immutable version
     * descriptor. It deliberately makes no ROMFS IPC, user-memory mapping or
     * mutable state change. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected bounded F_GETFL fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_FCNTL ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VERSION ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_F_GETFL ||
        frame.values[seL4_UnknownSyscall_RDX] != 0u) {
        debug_puts("SeLinOS ABI gateway: bounded F_GETFL rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    /* M10 returns a SeLinOS-declared two-word probe record rather than a
     * Linux struct stat. It has no VFS side effect and admits only FD 6. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected bounded version fstat fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_FSTAT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VERSION ||
        frame.values[seL4_UnknownSyscall_RSI] == 0u ||
        frame.values[seL4_UnknownSyscall_RSI] > UINT64_MAX - sizeof(seL4_Word) ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 SELINOS_LINUX_M10_STAT_MAGIC) ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI] + sizeof(seL4_Word),
                                 SELINOS_LINUX_M10_STAT_SIZE)) {
        debug_puts("SeLinOS ABI gateway: bounded version fstat rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected version read UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_READ ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VERSION ||
        frame.values[seL4_UnknownSyscall_RSI] == 0u ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_VERSION_LENGTH ||
        !call_romfs_root(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VERSION, 0u, 3u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_VERSION_LENGTH ||
        romfs_payload != SELINOS_ROMFS_VERSION_MAGIC ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI], romfs_payload)) {
        debug_puts("SeLinOS ABI gateway: restricted version read rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_ROMFS_VERSION_LENGTH);

    /* M11 proves only that fixed immutable records are not directories. The
     * rejection intentionally neither maps the supplied user buffer nor sends
     * any VFS IPC. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected bounded getdents64 fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_GETDENTS64 ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VERSION ||
        frame.values[seL4_UnknownSyscall_RSI] == 0u ||
        frame.values[seL4_UnknownSyscall_RDX] != 32u) {
        debug_puts("SeLinOS ABI gateway: bounded getdents64 rejected request shape.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, (seL4_Word)-SELINOS_LINUX_ENOTDIR);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected version close UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_CLOSE ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VERSION ||
        !call_romfs_root(SELINOS_ROMFS_OP_CLOSE, SELINOS_ROMFS_FD_VERSION, 0u, 2u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK || romfs_value != 0u) {
        debug_puts("SeLinOS ABI gateway: restricted version close rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected invalid volatile openat fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_OPENAT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_AT_FDCWD ||
        frame.values[seL4_UnknownSyscall_RDX] != 0u ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        copy_linux_exact_string(vka, vspace, &probe,
                                frame.values[seL4_UnknownSyscall_RSI],
                                "/selinos-state")) {
        debug_puts("SeLinOS ABI gateway: malformed volatile openat guard failed.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, (seL4_Word)-SELINOS_ROMFS_STATUS_ENOENT);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected volatile openat fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_OPENAT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_AT_FDCWD ||
        frame.values[seL4_UnknownSyscall_RDX] != 0u ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        !copy_linux_exact_string(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 "/selinos-state") ||
        !call_romfs_root_open_path(14u, 0x2f73656c696e6f73ull,
                                   0x2d73746174650000ull,
                                   &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_FD_VOLATILE_STATE) {
        debug_puts("SeLinOS ABI gateway: volatile openat rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, romfs_value);

    /* M8 is deliberately an acknowledgement of the existing one-page RW test
     * mapping, not a general protection-changing VM operation. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected bounded mprotect fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_MPROTECT ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_LINUX_MMAP_ADDRESS ||
        frame.values[seL4_UnknownSyscall_RSI] != SELINOS_LINUX_PAGE_SIZE ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_LINUX_PROT_READ_WRITE ||
        !has_linux_user_word_page(&probe, SELINOS_LINUX_MMAP_ADDRESS)) {
        debug_puts("SeLinOS ABI gateway: bounded mprotect rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    /* romfsd has no cursor state. M8 accepts only the no-op fixed origin so
     * userspace can prove a seek-like ABI shape without claiming seekability. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected bounded lseek fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_LSEEK ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        frame.values[seL4_UnknownSyscall_RSI] != 0u ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_LINUX_SEEK_SET) {
        debug_puts("SeLinOS ABI gateway: bounded lseek rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    /* Fixed-offset pread64 reuses the existing server read at offset zero;
     * no cursor is stored and no partial, arbitrary-offset or immutable-FD
     * pread behavior is introduced by this gate. */
    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected bounded volatile pread64 fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_PREAD64 ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        frame.values[seL4_UnknownSyscall_RSI] == 0u ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        frame.values[seL4_UnknownSyscall_R10] != 0u ||
        !call_romfs_root(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 3u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        romfs_payload != SELINOS_ROMFS_VOLATILE_STATE_INITIAL ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI], romfs_payload)) {
        debug_puts("SeLinOS ABI gateway: bounded volatile pread64 rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_ROMFS_VOLATILE_STATE_LENGTH);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected volatile initial read fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_READ ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        !call_romfs_root(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 3u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        romfs_payload != SELINOS_ROMFS_VOLATILE_STATE_INITIAL ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI], romfs_payload)) {
        debug_puts("SeLinOS ABI gateway: volatile initial read rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_ROMFS_VOLATILE_STATE_LENGTH);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected volatile write fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    char volatile_bytes[SELINOS_ABI_MAX_WRITE_BYTES + 1u];
    seL4_Word volatile_word = 0u;
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_WRITE ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        !copy_linux_write_buffer(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 SELINOS_ROMFS_VOLATILE_STATE_LENGTH, volatile_bytes)) {
        debug_puts("SeLinOS ABI gateway: volatile write payload rejected.\n");
        return false;
    }
    for (size_t index = 0u; index < SELINOS_ROMFS_VOLATILE_STATE_LENGTH; ++index) {
        volatile_word = (volatile_word << 8u) | (unsigned char)volatile_bytes[index];
    }
    if (volatile_word != SELINOS_ROMFS_VOLATILE_STATE_TEST ||
        !call_romfs_root(SELINOS_ROMFS_OP_WRITE, SELINOS_ROMFS_FD_VOLATILE_STATE,
                         volatile_word, 3u, &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_VOLATILE_STATE_LENGTH) {
        debug_puts("SeLinOS ABI gateway: volatile write rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_ROMFS_VOLATILE_STATE_LENGTH);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected volatile read-back fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_READ ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        !call_romfs_root(SELINOS_ROMFS_OP_READ, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 3u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK ||
        romfs_value != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        romfs_payload != SELINOS_ROMFS_VOLATILE_STATE_TEST ||
        !copy_linux_word_to_user(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI], romfs_payload)) {
        debug_puts("SeLinOS ABI gateway: volatile read-back rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, SELINOS_ROMFS_VOLATILE_STATE_LENGTH);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected volatile close fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_CLOSE ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        !call_romfs_root(SELINOS_ROMFS_OP_CLOSE, SELINOS_ROMFS_FD_VOLATILE_STATE, 0u, 2u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_OK || romfs_value != 0u) {
        debug_puts("SeLinOS ABI gateway: volatile close rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected closed volatile write fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_WRITE ||
        frame.values[seL4_UnknownSyscall_RDI] != SELINOS_ROMFS_FD_VOLATILE_STATE ||
        frame.values[seL4_UnknownSyscall_RDX] != SELINOS_ROMFS_VOLATILE_STATE_LENGTH ||
        !copy_linux_write_buffer(vka, vspace, &probe,
                                 frame.values[seL4_UnknownSyscall_RSI],
                                 SELINOS_ROMFS_VOLATILE_STATE_LENGTH, volatile_bytes) ||
        !call_romfs_root(SELINOS_ROMFS_OP_WRITE, SELINOS_ROMFS_FD_VOLATILE_STATE,
                         SELINOS_ROMFS_VOLATILE_STATE_TEST, 3u,
                         &romfs_status, &romfs_value, &romfs_payload) ||
        romfs_status != SELINOS_ROMFS_STATUS_EBADF) {
        debug_puts("SeLinOS ABI gateway: closed volatile write guard failed.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, (seL4_Word)-SELINOS_ROMFS_STATUS_EBADF);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected EOF read UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_READ ||
        frame.values[seL4_UnknownSyscall_RDI] != 0u ||
        frame.values[seL4_UnknownSyscall_RSI] != 0u ||
        frame.values[seL4_UnknownSyscall_RDX] != 0u) {
        debug_puts("SeLinOS ABI gateway: restricted EOF read rejected.\n");
        return false;
    }
    reply_linux_x86_64_syscall(&frame, 0u);

    fault = seL4_Recv(probe.fault_endpoint.cptr, &badge);
    if (seL4_MessageInfo_get_label(fault) != seL4_Fault_UnknownSyscall) {
        debug_puts("SeLinOS ABI gateway: expected exit UnknownSyscall fault.\n");
        return false;
    }
    capture_linux_x86_64_syscall(&frame);
    if (frame.values[seL4_UnknownSyscall_RAX] != SELINOS_LINUX_EXIT ||
        frame.values[seL4_UnknownSyscall_RDI] != 0u) {
        debug_puts("SeLinOS ABI gateway: expected Linux exit(0).\n");
        return false;
    }
    /* Do not reply: test process remains stopped after its verified exit(0). */
    debug_puts("SeLinOS ABI M15: fixed immutable FD F_GETFL mediated without VFS IPC.\n");
    debug_puts("SeLinOS ABI M14: fixed 390-byte x86_64 utsname-layout record mediated.\n");
    debug_puts("SeLinOS ABI M13: root-tracked ARCH_GET_FS mapped TLS base returned.\n");
    debug_puts("SeLinOS ABI M12: ARCH_SET_FS mapped-page TLS base and fs:0 load/store mediated.\n");
    debug_puts("SeLinOS ABI M11: getdents64(FD 6) rejected with ENOTDIR; no directory service.\n");
    debug_puts("SeLinOS ABI M10: FD 6 probe-local two-word fstat record mediated; not Linux struct stat.\n");
    debug_puts("SeLinOS ABI M9: exact immutable /selinos-version open/read/close bridge mediated.\n");
    debug_puts("SeLinOS ABI M8: mapped-page mprotect acknowledgement, zero-origin lseek and fixed-offset volatile pread64 mediated.\n");
    debug_puts("SeLinOS ABI M7: fixed volatile VFS open/read/write/read-back/close bridge mediated.\n");
    debug_puts("SeLinOS ABI M6: deterministic clock_gettime/getrlimit and non-blocking futex mismatch/wake mediated.\n");
    debug_puts("SeLinOS ABI M5: getuid/getgid/geteuid/getegid/gettid and mapped set_tid_address mediated.\n");
    debug_puts("SeLinOS ABI gateway: getpid, write, mmap, brk, ROMFS open/read/close, EOF read and exit(0) mediated.\n");
    return true;
}

#if CONFIG_SELINOS_TASKD_DYNAMIC_ALLOCATION_PROBE
static bool start_taskd_dynamic_alloc_m0_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    char *taskd_argv[] = {"selinos-taskd-dynamic-alloc-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-dynamic-alloc-m0-probe", NULL};
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    if (sel4utils_configure_process(&taskd, vka, vspace, "selinos-taskd-dynamic-alloc-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace, "selinos-taskd-dynamic-alloc-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError) return false;
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    if (taskd_endpoint_slot != SELINOS_TASKD_DYN_M0_SERVER_ENDPOINT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_DYN_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_DYN_M0_PROBE_SUCCESS_NOTIFY_SLOT) return false;
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) return false;
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_DYNAMIC_TCB_OWNERSHIP_PROBE
static bool start_taskd_dynamic_tcb_m0_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t target_tcb;
    cspacepath_t root_target_path;
    seL4_CPtr root_target_slot;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_target_tcb_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-dynamic-tcb-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-dynamic-tcb-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-dynamic-tcb-m0") != 0) {
        debug_puts("SeLinOS taskd dynamic TCB M0: taskd scaffolding configuration rejected.\n");
        return false;
    }
    if (sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-dynamic-tcb-m0-probe") != 0) {
        debug_puts("SeLinOS taskd dynamic TCB M0: probe scaffolding configuration rejected.\n");
        return false;
    }
    if (vka_alloc_endpoint(vka, &endpoint) != seL4_NoError) {
        debug_puts("SeLinOS taskd dynamic TCB M0: endpoint allocation rejected.\n");
        return false;
    }
    if (vka_alloc_notification(vka, &success) != seL4_NoError) {
        debug_puts("SeLinOS taskd dynamic TCB M0: notification allocation rejected.\n");
        return false;
    }
    if (vka_alloc_tcb(vka, &target_tcb) != seL4_NoError) {
        debug_puts("SeLinOS taskd dynamic TCB M0: target TCB allocation rejected.\n");
        return false;
    }

    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    root_target_slot = target_tcb.cptr;
    vka_cspace_make_path(vka, root_target_slot, &root_target_path);
    taskd_target_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_target_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_DYNAMIC_TCB_M0_SERVER_ENDPOINT_SLOT ||
        taskd_target_tcb_slot != SELINOS_TASKD_DYNAMIC_TCB_M0_TARGET_TCB_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_DYNAMIC_TCB_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_DYNAMIC_TCB_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        debug_puts("SeLinOS taskd dynamic TCB M0: ownership-cap move or slot validation rejected.\n");
        return false;
    }

    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0) {
        debug_puts("SeLinOS taskd dynamic TCB M0: taskd scaffolding spawn rejected.\n");
        return false;
    }
    if (sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        debug_puts("SeLinOS taskd dynamic TCB M0: probe scaffolding spawn rejected.\n");
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_DYNAMIC_CSPACE_ROLLBACK_PROBE
static bool start_taskd_dynamic_cspace_m0_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_cnode;
    vka_object_t owned_cnode;
    cspacepath_t root_owned_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_owned_cnode_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-dynamic-cspace-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-dynamic-cspace-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-dynamic-cspace-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-dynamic-cspace-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_DYNAMIC_CSPACE_M0_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError) {
        return false;
    }
    /* The injected policy rejection happens before any cap transfer. */
    vka_free_object(vka, &rollback_cnode);
    if (vka_alloc_cnode_object(vka, SELINOS_TASKD_DYNAMIC_CSPACE_M0_SLOT_BITS,
                               &owned_cnode) != seL4_NoError) {
        return false;
    }

    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_owned_path);
    taskd_owned_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_owned_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_DYNAMIC_CSPACE_M0_SERVER_ENDPOINT_SLOT ||
        taskd_owned_cnode_slot != SELINOS_TASKD_DYNAMIC_CSPACE_M0_OWNED_CNODE_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_DYNAMIC_CSPACE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_DYNAMIC_CSPACE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }

    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_DYNAMIC_VSPACE_ROLLBACK_PROBE
static bool start_taskd_dynamic_vspace_m0_bundle(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_vspace_root;
    vka_object_t owned_vspace_root;
    cspacepath_t root_owned_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_owned_root_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-dynamic-vspace-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-dynamic-vspace-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-dynamic-vspace-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-dynamic-vspace-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError) {
        return false;
    }
    /* The injected policy rejection occurs before any PML4 cap transfer. */
    vka_free_object(vka, &rollback_vspace_root);
    if (vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError) {
        return false;
    }

    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_owned_path);
    taskd_owned_root_slot = sel4utils_move_cap_to_process(&taskd, root_owned_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_DYNAMIC_VSPACE_M0_SERVER_ENDPOINT_SLOT ||
        taskd_owned_root_slot != SELINOS_TASKD_DYNAMIC_VSPACE_M0_OWNED_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_DYNAMIC_VSPACE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_DYNAMIC_VSPACE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }

    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_INERT_BUNDLE_ROLLBACK_PROBE
static bool start_taskd_inert_bundle_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-inert-bundle-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-inert-bundle-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-inert-bundle-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-inert-bundle-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_INERT_BUNDLE_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError) {
        return false;
    }
    /* Generation 1 is rejected before delegation and freed in reverse order. */
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_INERT_BUNDLE_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError) {
        return false;
    }

    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_INERT_BUNDLE_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_INERT_BUNDLE_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_INERT_BUNDLE_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_INERT_BUNDLE_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_INERT_BUNDLE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_INERT_BUNDLE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }

    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_SUSPENDED_LINKAGE_PROBE
static bool start_taskd_suspended_linkage_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-suspended-linkage-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-suspended-linkage-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-suspended-linkage-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-suspended-linkage-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_SUSPENDED_LINKAGE_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError) {
        return false;
    }
    /* Generation 1 is rejected before ASID assignment, configuration or transfer. */
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_SUSPENDED_LINKAGE_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,
                            0u, owned_vspace_root.cptr, 0u, 0u,
                            seL4_CapNull) != seL4_NoError) {
        return false;
    }

    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_SUSPENDED_LINKAGE_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_SUSPENDED_LINKAGE_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_SUSPENDED_LINKAGE_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_SUSPENDED_LINKAGE_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_SUSPENDED_LINKAGE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_SUSPENDED_LINKAGE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }

    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_POPULATED_CSPACE_PROBE
static bool start_taskd_populated_cspace_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-populated-cspace-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-populated-cspace-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-populated-cspace-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-populated-cspace-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_POPULATED_CSPACE_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError) {
        return false;
    }
    /* Generation 1 is rejected before target CNode population or configuration. */
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_POPULATED_CSPACE_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_POPULATED_CSPACE_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_POPULATED_CSPACE_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode,
                        target_notification.cptr, seL4_WordBits,
                        seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,
                            0u, owned_vspace_root.cptr, 0u, 0u,
                            seL4_CapNull) != seL4_NoError) {
        return false;
    }

    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_POPULATED_CSPACE_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_POPULATED_CSPACE_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_POPULATED_CSPACE_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_POPULATED_CSPACE_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_POPULATED_CSPACE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_POPULATED_CSPACE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }

    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_SELF_ROOTED_CSPACE_PROBE
static bool start_taskd_self_rooted_cspace_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-self-rooted-cspace-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-self-rooted-cspace-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-self-rooted-cspace-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-self-rooted-cspace-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,
                            0u, owned_vspace_root.cptr, 0u, 0u,
                            seL4_CapNull) != seL4_NoError) {
        return false;
    }
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_SELF_ROOTED_CSPACE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_IPC_BUFFER_PROBE
static bool start_taskd_ipc_buffer_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t rollback_frame;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_frame;
    vka_object_t paging_objects[3];
    int paging_object_count = 0;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *taskd_argv[] = {"selinos-taskd-ipc-buffer-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-ipc-buffer-m0-probe", NULL};

    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-ipc-buffer-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-ipc-buffer-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_IPC_BUFFER_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &rollback_frame) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_frame);
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_IPC_BUFFER_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_frame) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_IPC_BUFFER_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_IPC_BUFFER_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_IPC_BUFFER_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_IPC_BUFFER_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        sel4utils_map_page(vka, owned_vspace_root.cptr, target_frame.cptr,
                           (void *)SELINOS_TASKD_IPC_BUFFER_M0_FIXED_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_IPC_BUFFER_M0_TARGET_FRAME_SLOT,
                        SELINOS_TASKD_IPC_BUFFER_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,
                            0u, owned_vspace_root.cptr, 0u,
                            SELINOS_TASKD_IPC_BUFFER_M0_FIXED_VADDR,
                            target_frame.cptr) != seL4_NoError) {
        return false;
    }
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_IPC_BUFFER_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_IPC_BUFFER_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_IPC_BUFFER_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_IPC_BUFFER_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_IPC_BUFFER_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_IPC_BUFFER_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_ZEROED_CONTEXT_PROBE
static bool start_taskd_zeroed_context_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t rollback_frame;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_frame;
    vka_object_t paging_objects[3];
    seL4_UserContext zero_context = {0};
    seL4_UserContext observed_context = {0};
    int paging_object_count = 0;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    seL4_Word register_index;
    char *taskd_argv[] = {"selinos-taskd-zeroed-context-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-zeroed-context-m0-probe", NULL};

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_TASKD_ZEROED_CONTEXT_M0_X86_64_CONTEXT_WORDS,
                   "Phase 59 requires the complete pinned x86_64 register context");
    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-zeroed-context-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-zeroed-context-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_ZEROED_CONTEXT_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &rollback_frame) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_frame);
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_ZEROED_CONTEXT_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_frame) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ZEROED_CONTEXT_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_ZEROED_CONTEXT_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ZEROED_CONTEXT_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_ZEROED_CONTEXT_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        sel4utils_map_page(vka, owned_vspace_root.cptr, target_frame.cptr,
                           (void *)SELINOS_TASKD_ZEROED_CONTEXT_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ZEROED_CONTEXT_M0_TARGET_FRAME_SLOT,
                        SELINOS_TASKD_ZEROED_CONTEXT_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr, seL4_CapNull, owned_cnode.cptr,
                            0u, owned_vspace_root.cptr, 0u,
                            SELINOS_TASKD_ZEROED_CONTEXT_M0_FIXED_IPC_BUFFER_VADDR,
                            target_frame.cptr) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd zeroed context M0: pre-register configuration failed.\n");
        return false;
    }
    if (seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,
                                SELINOS_TASKD_ZEROED_CONTEXT_M0_X86_64_CONTEXT_WORDS,
                                &zero_context) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd zeroed context M0: zero-context write failed.\n");
        return false;
    }
    if (seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,
                               SELINOS_TASKD_ZEROED_CONTEXT_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd zeroed context M0: zero-context read-back failed.\n");
        return false;
    }
    for (register_index = 0u;
         register_index < SELINOS_TASKD_ZEROED_CONTEXT_M0_X86_64_CONTEXT_WORDS;
         register_index++) {
        seL4_Word expected_word =
            register_index == SELINOS_TASKD_ZEROED_CONTEXT_M0_X86_64_RFLAGS_WORD
                ? SELINOS_TASKD_ZEROED_CONTEXT_M0_X86_64_NORMALIZED_RFLAGS
                : 0u;
        if (((const seL4_Word *)&observed_context)[register_index] != expected_word) {
            seL4_DebugPutString("SeLinOS taskd zeroed context M0: read-back violates the zero-plus-normalized-RFLAGS contract.\n");
            return false;
        }
    }
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_ZEROED_CONTEXT_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_ZEROED_CONTEXT_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_ZEROED_CONTEXT_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_ZEROED_CONTEXT_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_ZEROED_CONTEXT_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_ZEROED_CONTEXT_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_FAULT_WITNESS_PROBE
static bool start_taskd_fault_witness_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t fault_endpoint;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t rollback_frame;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_frame;
    vka_object_t paging_objects[3];
    seL4_UserContext zero_context = {0};
    seL4_UserContext observed_context = {0};
    seL4_MessageInfo_t fault_message;
    int paging_object_count = 0;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word success_badge = 0u;
    seL4_Word fault_badge = 0u;
    seL4_Word register_index;
    char *taskd_argv[] = {"selinos-taskd-fault-witness-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-fault-witness-m0-probe", NULL};

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_TASKD_FAULT_WITNESS_M0_X86_64_CONTEXT_WORDS,
                   "Phase 60 requires the complete pinned x86_64 register context");
    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-fault-witness-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-fault-witness-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &rollback_frame) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_frame);
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_frame) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_FAULT_WITNESS_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_FAULT_WITNESS_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_FAULT_WITNESS_M0_TARGET_FRAME_SLOT,
                        SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(owned_cnode.cptr,
                        SELINOS_TASKD_FAULT_WITNESS_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_TASKD_FAULT_WITNESS_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        sel4utils_map_page(vka, owned_vspace_root.cptr, target_frame.cptr,
                           (void *)SELINOS_TASKD_FAULT_WITNESS_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr,
                            SELINOS_TASKD_FAULT_WITNESS_M0_TARGET_FAULT_ENDPOINT_SLOT,
                            owned_cnode.cptr, 0u, owned_vspace_root.cptr, 0u,
                            SELINOS_TASKD_FAULT_WITNESS_M0_FIXED_IPC_BUFFER_VADDR,
                            target_frame.cptr) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd fault witness M0: pre-resume construction failed.\n");
        return false;
    }
    if (seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,
                                SELINOS_TASKD_FAULT_WITNESS_M0_X86_64_CONTEXT_WORDS,
                                &zero_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,
                               SELINOS_TASKD_FAULT_WITNESS_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd fault witness M0: zero-context setup failed.\n");
        return false;
    }
    for (register_index = 0u;
         register_index < SELINOS_TASKD_FAULT_WITNESS_M0_X86_64_CONTEXT_WORDS;
         register_index++) {
        seL4_Word expected_word =
            register_index == SELINOS_TASKD_FAULT_WITNESS_M0_X86_64_RFLAGS_WORD
                ? SELINOS_TASKD_FAULT_WITNESS_M0_X86_64_NORMALIZED_RFLAGS
                : 0u;
        if (((const seL4_Word *)&observed_context)[register_index] != expected_word) {
            seL4_DebugPutString("SeLinOS taskd fault witness M0: zero-context read-back contract failed.\n");
            return false;
        }
    }
    if (seL4_TCB_Resume(owned_tcb.cptr) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd fault witness M0: sole target resume failed.\n");
        return false;
    }
    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_VMFault ||
        fault_badge != SELINOS_TASKD_FAULT_WITNESS_M0_FAULT_BADGE ||
        seL4_GetMR(seL4_VMFault_IP) != 0u ||
        seL4_GetMR(seL4_VMFault_Addr) != 0u ||
        seL4_GetMR(seL4_VMFault_PrefetchFault) == 0u) {
        seL4_DebugPutString("SeLinOS taskd fault witness M0: unexpected first fault; target remains blocked.\n");
        return false;
    }
    /* Deliberately do not seL4_Reply(): this leaves the target fault-blocked. */
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_FAULT_WITNESS_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_FAULT_WITNESS_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_FAULT_WITNESS_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_FAULT_WITNESS_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_FAULT_WITNESS_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_FAULT_WITNESS_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &success_badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_ENTRY_STACK_PROBE
static bool entry_stack_m0_is_low_canonical(seL4_Word address)
{
    return address < ((seL4_Word)1u << 47u);
}

static bool entry_stack_m0_is_abi_entry_stack(seL4_Word address)
{
    return address >= SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_VADDR &&
           address < SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_VADDR +
                         ((seL4_Word)1u << seL4_PageBits) &&
           (address & 0xfu) ==
               SELINOS_TASKD_ENTRY_STACK_M0_X86_64_STACK_ENTRY_MODULO;
}

static bool start_taskd_entry_stack_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t fault_endpoint;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t rollback_frame;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_ipc_frame;
    vka_object_t target_entry_frame;
    vka_object_t target_stack_frame;
    vka_object_t paging_objects[4];
    seL4_UserContext requested_context = {0};
    seL4_UserContext observed_context = {0};
    const seL4_X86_VMAttributes nx_attributes =
        (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                 seL4_X86_ExecuteDisable);
    int paging_object_count = 0;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word success_badge = 0u;
    seL4_Word register_index;
    char *taskd_argv[] = {"selinos-taskd-entry-stack-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-entry-stack-m0-probe", NULL};

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_TASKD_ENTRY_STACK_M0_X86_64_CONTEXT_WORDS,
                   "Phase 61 requires the complete pinned x86_64 register context");
    if (!entry_stack_m0_is_low_canonical(
            SELINOS_TASKD_ENTRY_STACK_M0_FIXED_ENTRY_VADDR) ||
        !entry_stack_m0_is_low_canonical(
            SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_POINTER) ||
        !entry_stack_m0_is_abi_entry_stack(
            SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_POINTER) ||
        entry_stack_m0_is_low_canonical(0xffff800000000000ull) ||
        entry_stack_m0_is_abi_entry_stack(
            SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_POINTER - 1u)) {
        seL4_DebugPutString("SeLinOS taskd entry-stack M0: canonical or ABI-alignment negative guard failed.\n");
        return false;
    }
    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-entry-stack-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-entry-stack-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &rollback_frame) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_frame);
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_stack_frame) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ENTRY_STACK_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ENTRY_STACK_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ENTRY_STACK_M0_TARGET_IPC_FRAME_SLOT,
                        SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(owned_cnode.cptr,
                        SELINOS_TASKD_ENTRY_STACK_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_TASKD_ENTRY_STACK_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ENTRY_STACK_M0_TARGET_ENTRY_FRAME_SLOT,
                        SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_entry_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_ENTRY_STACK_M0_TARGET_STACK_FRAME_SLOT,
                        SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_stack_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError ||
        sel4utils_map_page(vka, owned_vspace_root.cptr, target_ipc_frame.cptr,
                           (void *)SELINOS_TASKD_ENTRY_STACK_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,
                           target_entry_frame.cptr,
                           (void *)SELINOS_TASKD_ENTRY_STACK_M0_FIXED_ENTRY_VADDR,
                           seL4_AllRights, nx_attributes, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,
                           target_stack_frame.cptr,
                           (void *)SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_VADDR,
                           seL4_AllRights, nx_attributes, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr,
                            SELINOS_TASKD_ENTRY_STACK_M0_TARGET_FAULT_ENDPOINT_SLOT,
                            owned_cnode.cptr, 0u, owned_vspace_root.cptr, 0u,
                            SELINOS_TASKD_ENTRY_STACK_M0_FIXED_IPC_BUFFER_VADDR,
                            target_ipc_frame.cptr) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd entry-stack M0: NX provenance construction failed.\n");
        return false;
    }
    requested_context.rip = SELINOS_TASKD_ENTRY_STACK_M0_FIXED_ENTRY_VADDR;
    requested_context.rsp = SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_POINTER;
    if (seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,
                                SELINOS_TASKD_ENTRY_STACK_M0_X86_64_CONTEXT_WORDS,
                                &requested_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,
                               SELINOS_TASKD_ENTRY_STACK_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd entry-stack M0: provenance context write/read failed.\n");
        return false;
    }
    for (register_index = 0u;
         register_index < SELINOS_TASKD_ENTRY_STACK_M0_X86_64_CONTEXT_WORDS;
         register_index++) {
        seL4_Word expected_word =
            register_index == 0u
                ? SELINOS_TASKD_ENTRY_STACK_M0_FIXED_ENTRY_VADDR
                : register_index == 1u
                    ? SELINOS_TASKD_ENTRY_STACK_M0_FIXED_STACK_POINTER
                    : register_index == SELINOS_TASKD_ENTRY_STACK_M0_X86_64_RFLAGS_WORD
                        ? SELINOS_TASKD_ENTRY_STACK_M0_X86_64_NORMALIZED_RFLAGS
                        : 0u;
        if (((const seL4_Word *)&observed_context)[register_index] != expected_word) {
            seL4_DebugPutString("SeLinOS taskd entry-stack M0: context read-back violates provenance contract.\n");
            return false;
        }
    }
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_ENTRY_STACK_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_ENTRY_STACK_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_ENTRY_STACK_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_ENTRY_STACK_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_ENTRY_STACK_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_ENTRY_STACK_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &success_badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_EXEC_FETCH_PROBE
static bool start_taskd_exec_fetch_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t fault_endpoint;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t rollback_frame;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_ipc_frame;
    vka_object_t target_entry_frame;
    vka_object_t target_stack_frame;
    vka_object_t paging_objects[4];
    seL4_UserContext requested_context = {0};
    seL4_UserContext observed_context = {0};
    seL4_MessageInfo_t fault_message;
    int paging_object_count = 0;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word success_badge = 0u;
    seL4_Word fault_badge = 0u;
    seL4_Word register_index;
    void *root_entry_mapping = NULL;
    char *taskd_argv[] = {"selinos-taskd-exec-fetch-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-exec-fetch-m0-probe", NULL};

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_TASKD_EXEC_FETCH_M0_X86_64_CONTEXT_WORDS,
                   "Phase 62 requires the complete pinned x86_64 register context");
    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-exec-fetch-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-exec-fetch-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &rollback_frame) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_frame);
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_stack_frame) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_EXEC_FETCH_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_EXEC_FETCH_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_EXEC_FETCH_M0_TARGET_IPC_FRAME_SLOT,
                        SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(owned_cnode.cptr,
                        SELINOS_TASKD_EXEC_FETCH_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_TASKD_EXEC_FETCH_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_EXEC_FETCH_M0_TARGET_ENTRY_FRAME_SLOT,
                        SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_entry_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_EXEC_FETCH_M0_TARGET_STACK_FRAME_SLOT,
                        SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_stack_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError) {
        return false;
    }
    root_entry_mapping = vspace_map_pages(vspace, &target_entry_frame.cptr, NULL,
                                          seL4_AllRights, 1u, seL4_PageBits, 1);
    if (root_entry_mapping == NULL) {
        return false;
    }
    ((volatile uint8_t *)root_entry_mapping)[0] = SELINOS_TASKD_EXEC_FETCH_M0_NOP_OPCODE;
    ((volatile uint8_t *)root_entry_mapping)[1] = SELINOS_TASKD_EXEC_FETCH_M0_UD2_OPCODE_0;
    ((volatile uint8_t *)root_entry_mapping)[2] = SELINOS_TASKD_EXEC_FETCH_M0_UD2_OPCODE_1;
    vspace_unmap_pages(vspace, root_entry_mapping, 1u, seL4_PageBits,
                       VSPACE_PRESERVE);
    root_entry_mapping = NULL;
    if (sel4utils_map_page(vka, owned_vspace_root.cptr, target_ipc_frame.cptr,
                           (void *)SELINOS_TASKD_EXEC_FETCH_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,
                           target_entry_frame.cptr,
                           (void *)SELINOS_TASKD_EXEC_FETCH_M0_FIXED_ENTRY_VADDR,
                           seL4_AllRights, seL4_X86_Default_VMAttributes,
                           paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,
                           target_stack_frame.cptr,
                           (void *)SELINOS_TASKD_EXEC_FETCH_M0_FIXED_STACK_VADDR,
                           seL4_AllRights,
                           (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                                   seL4_X86_ExecuteDisable),
                           paging_objects, &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr,
                            SELINOS_TASKD_EXEC_FETCH_M0_TARGET_FAULT_ENDPOINT_SLOT,
                            owned_cnode.cptr, 0u, owned_vspace_root.cptr, 0u,
                            SELINOS_TASKD_EXEC_FETCH_M0_FIXED_IPC_BUFFER_VADDR,
                            target_ipc_frame.cptr) != seL4_NoError) {
        return false;
    }
    requested_context.rip = SELINOS_TASKD_EXEC_FETCH_M0_FIXED_ENTRY_VADDR;
    requested_context.rsp = SELINOS_TASKD_EXEC_FETCH_M0_FIXED_STACK_POINTER;
    if (seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,
                                SELINOS_TASKD_EXEC_FETCH_M0_X86_64_CONTEXT_WORDS,
                                &requested_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,
                               SELINOS_TASKD_EXEC_FETCH_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError) {
        return false;
    }
    for (register_index = 0u;
         register_index < SELINOS_TASKD_EXEC_FETCH_M0_X86_64_CONTEXT_WORDS;
         register_index++) {
        seL4_Word expected_word =
            register_index == 0u
                ? SELINOS_TASKD_EXEC_FETCH_M0_FIXED_ENTRY_VADDR
                : register_index == 1u
                    ? SELINOS_TASKD_EXEC_FETCH_M0_FIXED_STACK_POINTER
                    : register_index == SELINOS_TASKD_EXEC_FETCH_M0_X86_64_RFLAGS_WORD
                        ? SELINOS_TASKD_EXEC_FETCH_M0_X86_64_NORMALIZED_RFLAGS
                        : 0u;
        if (((const seL4_Word *)&observed_context)[register_index] != expected_word) {
            return false;
        }
    }
    if (seL4_TCB_Resume(owned_tcb.cptr) != seL4_NoError) {
        return false;
    }
    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_UserException ||
        fault_badge != SELINOS_TASKD_EXEC_FETCH_M0_FAULT_BADGE ||
        seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_TASKD_EXEC_FETCH_M0_FIXED_POST_NOP_FAULT_VADDR ||
        seL4_GetMR(seL4_UserException_SP) !=
            SELINOS_TASKD_EXEC_FETCH_M0_FIXED_STACK_POINTER ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_TASKD_EXEC_FETCH_M0_X86_INVALID_OPCODE_VECTOR) {
        seL4_DebugPutString("SeLinOS taskd exec-fetch M0: unexpected post-fetch fault; target remains blocked.\n");
        return false;
    }
    /* Deliberately do not reply: target remains blocked at the `ud2` fault. */
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_EXEC_FETCH_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_EXEC_FETCH_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_EXEC_FETCH_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_EXEC_FETCH_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_EXEC_FETCH_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_EXEC_FETCH_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &success_badge);
    return true;
}
#endif

#if CONFIG_SELINOS_TASKD_VM_RESTART_PROBE
static bool start_taskd_vm_restart_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t taskd;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    vka_object_t fault_endpoint;
    vka_object_t rollback_tcb;
    vka_object_t rollback_cnode;
    vka_object_t rollback_vspace_root;
    vka_object_t rollback_frame;
    vka_object_t owned_tcb;
    vka_object_t owned_cnode;
    vka_object_t owned_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_ipc_frame;
    vka_object_t target_entry_frame;
    vka_object_t target_stack_frame;
    vka_object_t paging_objects[4];
    seL4_UserContext requested_context = {0};
    seL4_UserContext observed_context = {0};
    seL4_MessageInfo_t fault_message;
    int paging_object_count = 0;
    cspacepath_t root_tcb_path;
    cspacepath_t root_cnode_path;
    cspacepath_t root_vspace_path;
    seL4_CPtr taskd_endpoint_slot;
    seL4_CPtr taskd_tcb_slot;
    seL4_CPtr taskd_cnode_slot;
    seL4_CPtr taskd_vspace_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word success_badge = 0u;
    seL4_Word fault_badge = 0u;
    seL4_Word register_index;
    void *root_entry_mapping = NULL;
    char *taskd_argv[] = {"selinos-taskd-vm-restart-m0", NULL};
    char *probe_argv[] = {"selinos-taskd-vm-restart-m0-probe", NULL};

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_TASKD_VM_RESTART_M0_X86_64_CONTEXT_WORDS,
                   "Phase 63 requires the complete pinned x86_64 register context");
    if (sel4utils_configure_process(&taskd, vka, vspace,
                                    "selinos-taskd-vm-restart-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-taskd-vm-restart-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError ||
        vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &rollback_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                               &rollback_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &rollback_vspace_root) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &rollback_frame) != seL4_NoError) {
        return false;
    }
    vka_free_object(vka, &rollback_frame);
    vka_free_object(vka, &rollback_vspace_root);
    vka_free_object(vka, &rollback_cnode);
    vka_free_object(vka, &rollback_tcb);
    if (vka_alloc_tcb(vka, &owned_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                               &owned_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &owned_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_stack_frame) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_VM_RESTART_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, owned_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_VM_RESTART_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_VM_RESTART_M0_TARGET_IPC_FRAME_SLOT,
                        SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(owned_cnode.cptr,
                        SELINOS_TASKD_VM_RESTART_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_TASKD_VM_RESTART_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_VM_RESTART_M0_TARGET_ENTRY_FRAME_SLOT,
                        SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_entry_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(owned_cnode.cptr,
                        SELINOS_TASKD_VM_RESTART_M0_TARGET_STACK_FRAME_SLOT,
                        SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_stack_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 owned_vspace_root.cptr) != seL4_NoError) {
        return false;
    }
    root_entry_mapping = vspace_map_pages(vspace, &target_entry_frame.cptr, NULL,
                                          seL4_AllRights, 1u, seL4_PageBits, 1u);
    if (root_entry_mapping == NULL) {
        return false;
    }
    ((volatile uint8_t *)root_entry_mapping)[0] = SELINOS_TASKD_VM_RESTART_M0_NOP_OPCODE;
    ((volatile uint8_t *)root_entry_mapping)[1] = SELINOS_TASKD_VM_RESTART_M0_UD2_OPCODE_0;
    ((volatile uint8_t *)root_entry_mapping)[2] = SELINOS_TASKD_VM_RESTART_M0_UD2_OPCODE_1;
    vspace_unmap_pages(vspace, root_entry_mapping, 1u, seL4_PageBits,
                       VSPACE_PRESERVE);
    root_entry_mapping = NULL;
    if (sel4utils_map_page(vka, owned_vspace_root.cptr, target_ipc_frame.cptr,
                           (void *)SELINOS_TASKD_VM_RESTART_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,
                           target_stack_frame.cptr,
                           (void *)SELINOS_TASKD_VM_RESTART_M0_FIXED_STACK_VADDR,
                           seL4_AllRights,
                           (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                                   seL4_X86_ExecuteDisable),
                           paging_objects, &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(owned_tcb.cptr,
                           SELINOS_TASKD_VM_RESTART_M0_TARGET_FAULT_ENDPOINT_SLOT,
                           owned_cnode.cptr, 0u, owned_vspace_root.cptr, 0u,
                           SELINOS_TASKD_VM_RESTART_M0_FIXED_IPC_BUFFER_VADDR,
                           target_ipc_frame.cptr) != seL4_NoError) {
        return false;
    }
    requested_context.rip = SELINOS_TASKD_VM_RESTART_M0_FIXED_ENTRY_VADDR;
    requested_context.rsp = SELINOS_TASKD_VM_RESTART_M0_FIXED_STACK_POINTER;
    if (seL4_TCB_WriteRegisters(owned_tcb.cptr, 0u, 0u,
                                SELINOS_TASKD_VM_RESTART_M0_X86_64_CONTEXT_WORDS,
                                &requested_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(owned_tcb.cptr, 0u, 0u,
                               SELINOS_TASKD_VM_RESTART_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError) {
        return false;
    }
    for (register_index = 0u;
         register_index < SELINOS_TASKD_VM_RESTART_M0_X86_64_CONTEXT_WORDS;
         register_index++) {
        seL4_Word expected_word =
            register_index == 0u
                ? SELINOS_TASKD_VM_RESTART_M0_FIXED_ENTRY_VADDR
                : register_index == 1u
                    ? SELINOS_TASKD_VM_RESTART_M0_FIXED_STACK_POINTER
                    : register_index == SELINOS_TASKD_VM_RESTART_M0_X86_64_RFLAGS_WORD
                        ? SELINOS_TASKD_VM_RESTART_M0_X86_64_NORMALIZED_RFLAGS
                        : 0u;
        if (((const seL4_Word *)&observed_context)[register_index] != expected_word) {
            return false;
        }
    }
    if (seL4_TCB_Resume(owned_tcb.cptr) != seL4_NoError) {
        return false;
    }
    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_VMFault ||
        fault_badge != SELINOS_TASKD_VM_RESTART_M0_FAULT_BADGE ||
        seL4_GetMR(seL4_VMFault_IP) != SELINOS_TASKD_VM_RESTART_M0_FIXED_ENTRY_VADDR ||
        seL4_GetMR(seL4_VMFault_Addr) != SELINOS_TASKD_VM_RESTART_M0_FIXED_ENTRY_VADDR ||
        seL4_GetMR(seL4_VMFault_PrefetchFault) == 0u) {
        seL4_DebugPutString("SeLinOS taskd VM-restart M0: unexpected initial VM fault.\n");
        return false;
    }
    if (sel4utils_map_page_with_attributes(vka, owned_vspace_root.cptr,
                           target_entry_frame.cptr,
                           (void *)SELINOS_TASKD_VM_RESTART_M0_FIXED_ENTRY_VADDR,
                           seL4_AllRights, seL4_X86_Default_VMAttributes,
                           paging_objects, &paging_object_count) != seL4_NoError) {
        seL4_DebugPutString("SeLinOS taskd VM-restart M0: entry repair mapping failed.\n");
        return false;
    }
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 0u));
    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_UserException ||
        fault_badge != SELINOS_TASKD_VM_RESTART_M0_FAULT_BADGE ||
        seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_TASKD_VM_RESTART_M0_FIXED_POST_NOP_FAULT_VADDR ||
        seL4_GetMR(seL4_UserException_SP) !=
            SELINOS_TASKD_VM_RESTART_M0_FIXED_STACK_POINTER ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_TASKD_VM_RESTART_M0_X86_INVALID_OPCODE_VECTOR) {
        seL4_DebugPutString("SeLinOS taskd VM-restart M0: terminal user exception mismatch.\n");
        return false;
    }
    taskd_endpoint_slot = sel4utils_copy_cap_to_process(&taskd, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    vka_cspace_make_path(vka, owned_tcb.cptr, &root_tcb_path);
    vka_cspace_make_path(vka, owned_cnode.cptr, &root_cnode_path);
    vka_cspace_make_path(vka, owned_vspace_root.cptr, &root_vspace_path);
    taskd_tcb_slot = sel4utils_move_cap_to_process(&taskd, root_tcb_path, vka);
    taskd_cnode_slot = sel4utils_move_cap_to_process(&taskd, root_cnode_path, vka);
    taskd_vspace_slot = sel4utils_move_cap_to_process(&taskd, root_vspace_path, vka);
    if (taskd_endpoint_slot != SELINOS_TASKD_VM_RESTART_M0_SERVER_ENDPOINT_SLOT ||
        taskd_tcb_slot != SELINOS_TASKD_VM_RESTART_M0_TCB_SLOT ||
        taskd_cnode_slot != SELINOS_TASKD_VM_RESTART_M0_CNODE_SLOT ||
        taskd_vspace_slot != SELINOS_TASKD_VM_RESTART_M0_VSPACE_ROOT_SLOT ||
        probe_endpoint_slot != SELINOS_TASKD_VM_RESTART_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_TASKD_VM_RESTART_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&taskd, vka, vspace, 1, taskd_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &success_badge);
    return true;
}
#endif

#if CONFIG_SELINOS_SEALED_STATIC_IMAGE_PROBE
static bool start_sealed_static_image_m0(vka_t *vka, vspace_t *vspace)
{
    sel4utils_process_t service;
    sel4utils_process_t probe;
    vka_object_t endpoint;
    vka_object_t success;
    seL4_CPtr service_endpoint_slot;
    seL4_CPtr probe_endpoint_slot;
    seL4_CPtr probe_success_slot;
    seL4_Word badge = 0u;
    char *service_argv[] = {"selinos-sealed-static-image-m0", NULL};
    char *probe_argv[] = {"selinos-sealed-static-image-m0-probe", NULL};

    if (sel4utils_configure_process(&service, vka, vspace,
                                    "selinos-sealed-static-image-m0") != 0 ||
        sel4utils_configure_process(&probe, vka, vspace,
                                    "selinos-sealed-static-image-m0-probe") != 0 ||
        vka_alloc_endpoint(vka, &endpoint) != seL4_NoError ||
        vka_alloc_notification(vka, &success) != seL4_NoError) {
        return false;
    }
    service_endpoint_slot = sel4utils_copy_cap_to_process(&service, vka, endpoint.cptr);
    probe_endpoint_slot = sel4utils_copy_cap_to_process(&probe, vka, endpoint.cptr);
    probe_success_slot = sel4utils_copy_cap_to_process(&probe, vka, success.cptr);
    if (service_endpoint_slot != SELINOS_SEALED_STATIC_IMAGE_M0_SERVER_ENDPOINT_SLOT ||
        probe_endpoint_slot != SELINOS_SEALED_STATIC_IMAGE_M0_PROBE_ENDPOINT_SLOT ||
        probe_success_slot != SELINOS_SEALED_STATIC_IMAGE_M0_PROBE_SUCCESS_NOTIFY_SLOT) {
        return false;
    }
    if (sel4utils_spawn_process_v(&service, vka, vspace, 1, service_argv, 1) != 0 ||
        sel4utils_spawn_process_v(&probe, vka, vspace, 1, probe_argv, 1) != 0) {
        return false;
    }
    (void)seL4_Wait(success.cptr, &badge);
    return true;
}
#endif

#if CONFIG_SELINOS_SEALED_STATIC_IMAGE_MAPPING_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
static bool start_sealed_static_image_mapping_m0(vka_t *vka, vspace_t *vspace)
{
    vka_object_t fault_endpoint;
    vka_object_t target_tcb;
    vka_object_t target_cnode;
    vka_object_t target_vspace_root;
    vka_object_t target_notification;
    vka_object_t target_ipc_frame;
    vka_object_t target_entry_frame;
    vka_object_t target_stack_frame;
    vka_object_t paging_objects[4];
    seL4_UserContext requested_context = {0};
    seL4_UserContext observed_context = {0};
    seL4_MessageInfo_t fault_message;
    struct selinos_sealed_static_image_m0_summary summary;
    uint8_t sealed_image[SELINOS_SEALED_STATIC_IMAGE_M0_BYTES];
    uint8_t expected_payload_digest[SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES];
    int paging_object_count = 0;
    seL4_Word fault_badge = 0u;
    seL4_Word register_index;
    void *root_entry_mapping = NULL;
#if CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    vka_object_t fresh_tcb;
    vka_object_t fresh_cnode;
    vka_object_t fresh_vspace_root;
    vka_object_t fresh_notification;
    vka_object_t fresh_ipc_frame;
    vka_object_t fresh_entry_frame;
    vka_object_t fresh_stack_frame;
#endif
#if CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    vka_object_t fresh_fault_endpoint;
    vka_object_t fresh_paging_objects[3];
    int fresh_paging_object_count = 0;
#endif

    _Static_assert(sizeof(seL4_UserContext) / sizeof(seL4_Word) ==
                       SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_64_CONTEXT_WORDS,
                   "Phase 65 requires complete pinned x86_64 context");
    selinos_sealed_static_image_m0_make_fixture(sealed_image);
    if (selinos_sealed_static_image_m0_parse(sealed_image, sizeof(sealed_image),
                                             &summary) != SELINOS_SEALED_STATIC_IMAGE_M0_OK ||
        summary.entry_vaddr != SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_ENTRY_VADDR ||
        summary.segment_file_bytes != SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_PAYLOAD_BYTES ||
        summary.segment_memory_bytes != SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_PAYLOAD_BYTES ||
        summary.permissions != SELINOS_SEALED_STATIC_IMAGE_M0_PERMISSION_RX) {
        return false;
    }
    selinos_kabi_sha256(sealed_image + summary.segment_file_offset,
                        summary.segment_file_bytes, expected_payload_digest);
    for (register_index = 0u; register_index < SELINOS_SEALED_STATIC_IMAGE_M0_DIGEST_BYTES;
         register_index++) {
        if (expected_payload_digest[register_index] != summary.payload_sha256[register_index]) {
            return false;
        }
    }
    if (vka_alloc_endpoint(vka, &fault_endpoint) != seL4_NoError ||
        vka_alloc_tcb(vka, &target_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka, SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                               &target_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &target_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &target_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &target_stack_frame) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_CNODE_SELF_SLOT,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_NOTIFICATION_SLOT,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_notification.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_IPC_FRAME_SLOT,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(target_cnode.cptr,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_FAULT_ENDPOINT_SLOT,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FAULT_BADGE) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_ENTRY_FRAME_SLOT,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_entry_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(target_cnode.cptr,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_STACK_FRAME_SLOT,
                        SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, target_stack_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 target_vspace_root.cptr) != seL4_NoError) {
        return false;
    }
    root_entry_mapping = vspace_map_pages(vspace, &target_entry_frame.cptr, NULL,
                                          seL4_AllRights, 1u, seL4_PageBits, 1u);
    if (root_entry_mapping == NULL) {
        return false;
    }
    for (register_index = 0u; register_index < summary.segment_file_bytes; register_index++) {
        ((volatile uint8_t *)root_entry_mapping)[register_index] =
            sealed_image[summary.segment_file_offset + register_index];
    }
    vspace_unmap_pages(vspace, root_entry_mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    root_entry_mapping = NULL;
    if (sel4utils_map_page(vka, target_vspace_root.cptr, target_ipc_frame.cptr,
                           (void *)SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_IPC_BUFFER_VADDR,
                           seL4_AllRights, 1, paging_objects,
                           &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, target_vspace_root.cptr,
                           target_stack_frame.cptr,
                           (void *)SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_STACK_VADDR,
                           seL4_AllRights,
                           (seL4_X86_VMAttributes)(seL4_X86_Default_VMAttributes |
                                                   seL4_X86_ExecuteDisable),
                           paging_objects, &paging_object_count) != seL4_NoError ||
        sel4utils_map_page_with_attributes(vka, target_vspace_root.cptr,
                           target_entry_frame.cptr,
                           (void *)SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_ENTRY_VADDR,
                           seL4_AllRights, seL4_X86_Default_VMAttributes,
                           paging_objects, &paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(target_tcb.cptr,
                           SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_TARGET_FAULT_ENDPOINT_SLOT,
                           target_cnode.cptr, 0u, target_vspace_root.cptr, 0u,
                           SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_IPC_BUFFER_VADDR,
                           target_ipc_frame.cptr) != seL4_NoError) {
        return false;
    }
    requested_context.rip = SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_ENTRY_VADDR;
    requested_context.rsp = SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_STACK_POINTER;
    if (seL4_TCB_WriteRegisters(target_tcb.cptr, 0u, 0u,
                                SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_64_CONTEXT_WORDS,
                                &requested_context) != seL4_NoError ||
        seL4_TCB_ReadRegisters(target_tcb.cptr, 0u, 0u,
                               SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_64_CONTEXT_WORDS,
                               &observed_context) != seL4_NoError) {
        return false;
    }
    for (register_index = 0u;
         register_index < SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_64_CONTEXT_WORDS;
         register_index++) {
        seL4_Word expected_word =
            register_index == 0u
                ? SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_ENTRY_VADDR
                : register_index == 1u
                    ? SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_STACK_POINTER
                    : register_index == SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_64_RFLAGS_WORD
                        ? SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_64_NORMALIZED_RFLAGS
                        : 0u;
        if (((const seL4_Word *)&observed_context)[register_index] != expected_word) {
            return false;
        }
    }
    if (seL4_TCB_Resume(target_tcb.cptr) != seL4_NoError) {
        return false;
    }
    fault_message = seL4_Recv(fault_endpoint.cptr, &fault_badge);
    if (seL4_MessageInfo_get_label(fault_message) != seL4_Fault_UserException ||
        fault_badge != SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FAULT_BADGE ||
        seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_POST_NOP_FAULT_VADDR ||
        seL4_GetMR(seL4_UserException_SP) !=
            SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_FIXED_STACK_POINTER ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_SEALED_STATIC_IMAGE_MAPPING_M0_X86_INVALID_OPCODE_VECTOR) {
        return false;
    }
#if CONFIG_SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_PROBE
    {
        const struct selinos_static_image_terminal_lifecycle_m0_record record = {
            .terminal_ip = seL4_GetMR(seL4_UserException_FaultIP),
            .terminal_vector = seL4_GetMR(seL4_UserException_Number),
            .tcb_observed = target_tcb.cptr,
            .cnode_observed = target_cnode.cptr,
            .vspace_observed = target_vspace_root.cptr,
            .entry_frame_observed = target_entry_frame.cptr,
            .stack_frame_observed = target_stack_frame.cptr,
            .ipc_frame_observed = target_ipc_frame.cptr,
        };
        if (record.terminal_ip != SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_M0_TERMINAL_IP ||
            record.terminal_vector !=
                SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_M0_INVALID_OPCODE_VECTOR ||
            record.tcb_observed == seL4_CapNull ||
            record.cnode_observed == seL4_CapNull ||
            record.vspace_observed == seL4_CapNull ||
            record.entry_frame_observed == seL4_CapNull ||
            record.stack_frame_observed == seL4_CapNull ||
            record.ipc_frame_observed == seL4_CapNull) {
            return false;
        }
        debug_puts("SeLinOS static-image terminal lifecycle M0: one terminal ownership record observed; no reply, resume, unmap, delete, reclaim or successor task.\n");
    }
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_PROBE
    {
        bool authorization_used = false;
        seL4_Word authorization = SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_REJECTED;
        if (!authorization_used &&
            seL4_GetMR(seL4_UserException_FaultIP) ==
                SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_TERMINAL_IP &&
            seL4_GetMR(seL4_UserException_Number) ==
                SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_INVALID_OPCODE_VECTOR &&
            target_tcb.cptr != seL4_CapNull && target_cnode.cptr != seL4_CapNull &&
            target_vspace_root.cptr != seL4_CapNull && target_entry_frame.cptr != seL4_CapNull &&
            target_stack_frame.cptr != seL4_CapNull && target_ipc_frame.cptr != seL4_CapNull) {
            authorization = SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_APPROVED;
            authorization_used = true;
        }
        if (authorization != SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_APPROVED ||
            !authorization_used) {
            return false;
        }
        authorization = authorization_used
                            ? SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_REJECTED
                            : SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_APPROVED;
        if (authorization != SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_M0_REJECTED) {
            return false;
        }
        debug_puts("SeLinOS static-image teardown authorization M0: one terminal ledger authorized then duplicate rejected; no reply, resume, unmap, delete, free or successor task.\n");
    }
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_PROBE
    {
        bool authorization_used = false;
        seL4_Word authorization = SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_REJECTED;
        seL4_Word next_order = SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_ENTRY_ORDER;
        if (!authorization_used &&
            seL4_GetMR(seL4_UserException_FaultIP) ==
                SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_TERMINAL_IP &&
            seL4_GetMR(seL4_UserException_Number) ==
                SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_INVALID_OPCODE_VECTOR &&
            target_entry_frame.cptr != seL4_CapNull &&
            next_order++ == SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_ENTRY_ORDER &&
            target_stack_frame.cptr != seL4_CapNull &&
            next_order++ == SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_STACK_ORDER &&
            target_ipc_frame.cptr != seL4_CapNull &&
            next_order++ == SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_IPC_ORDER) {
            authorization = SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_APPROVED;
            authorization_used = true;
        }
        if (authorization != SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_APPROVED ||
            !authorization_used) {
            return false;
        }
        authorization = authorization_used
                            ? SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_REJECTED
                            : SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_APPROVED;
        if (authorization != SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_M0_REJECTED) {
            return false;
        }
        debug_puts("SeLinOS static-image mapping revocation authorization M0: entry-stack-IPC ledger authorized then duplicate rejected; no unmap, reply, resume, delete, free or successor task.\n");
    }
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    if (seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_M0_TERMINAL_IP ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_M0_INVALID_OPCODE_VECTOR ||
        target_entry_frame.cptr == seL4_CapNull ||
        target_stack_frame.cptr == seL4_CapNull ||
        target_ipc_frame.cptr == seL4_CapNull ||
        seL4_X86_Page_Unmap(target_entry_frame.cptr) != seL4_NoError ||
        seL4_X86_Page_Unmap(target_stack_frame.cptr) != seL4_NoError ||
        seL4_X86_Page_Unmap(target_ipc_frame.cptr) != seL4_NoError) {
        return false;
    }
    debug_puts("SeLinOS static-image mapping revocation M0: entry-stack-IPC target mappings unmapped in order; frames, caps, PML4, TCB and terminal fault retained.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    if (seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_TERMINAL_IP ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_INVALID_OPCODE_VECTOR ||
        target_tcb.cptr == seL4_CapNull ||
        target_cnode.cptr == seL4_CapNull ||
        target_vspace_root.cptr == seL4_CapNull ||
        target_notification.cptr == seL4_CapNull ||
        target_entry_frame.cptr == seL4_CapNull ||
        target_stack_frame.cptr == seL4_CapNull ||
        target_ipc_frame.cptr == seL4_CapNull ||
        seL4_CNode_Delete(target_cnode.cptr,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_NOTIFICATION_SLOT,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_CNODE_SLOT_BITS) != seL4_NoError ||
        seL4_CNode_Delete(target_cnode.cptr,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_IPC_FRAME_SLOT,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_CNODE_SLOT_BITS) != seL4_NoError ||
        seL4_CNode_Delete(target_cnode.cptr,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_FAULT_ENDPOINT_SLOT,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_CNODE_SLOT_BITS) != seL4_NoError ||
        seL4_CNode_Delete(target_cnode.cptr,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_ENTRY_FRAME_SLOT,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_CNODE_SLOT_BITS) != seL4_NoError ||
        seL4_CNode_Delete(target_cnode.cptr,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_STACK_FRAME_SLOT,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_CNODE_SLOT_BITS) != seL4_NoError ||
        seL4_CNode_Delete(target_cnode.cptr,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_SELF_SLOT,
                           SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_M0_CNODE_SLOT_BITS) != seL4_NoError) {
        return false;
    }
    debug_puts("SeLinOS static-image target capability deletion M0: target notification-IPC-fault-entry-stack-self cap copies deleted in order; root descriptors and all objects retained.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    if (seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_M0_TERMINAL_IP ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_M0_INVALID_OPCODE_VECTOR ||
        target_tcb.cptr == seL4_CapNull ||
        target_cnode.cptr == seL4_CapNull ||
        target_vspace_root.cptr == seL4_CapNull ||
        fault_endpoint.cptr == seL4_CapNull ||
        target_notification.cptr == seL4_CapNull ||
        target_entry_frame.cptr == seL4_CapNull ||
        target_stack_frame.cptr == seL4_CapNull ||
        target_ipc_frame.cptr == seL4_CapNull ||
        paging_object_count <= 0 ||
        paging_object_count > (int)SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_M0_MAX_PAGING_OBJECTS) {
        return false;
    }
    vka_free_object(vka, &target_tcb);
    vka_free_object(vka, &target_entry_frame);
    vka_free_object(vka, &target_stack_frame);
    vka_free_object(vka, &target_ipc_frame);
    for (register_index = (seL4_Word)paging_object_count; register_index > 0u; register_index--) {
        vka_free_object(vka, &paging_objects[register_index - 1u]);
    }
    vka_free_object(vka, &target_cnode);
    vka_free_object(vka, &target_vspace_root);
    vka_free_object(vka, &fault_endpoint);
    vka_free_object(vka, &target_notification);
    debug_puts("SeLinOS static-image object reclamation M0: terminal TCB, unmapped frames, reverse paging objects, CNode, PML4, endpoint and notification disposed; no reuse performed.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE
    {
        bool authorization_used = false;
        const seL4_Word retired_generation =
            SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_RETIRED_GENERATION;
        const seL4_Word fresh_generation =
            SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_FRESH_GENERATION;
        seL4_Word authorization =
            SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_INVALID;
        if (!authorization_used &&
            seL4_GetMR(seL4_UserException_FaultIP) ==
                SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_TERMINAL_IP &&
            seL4_GetMR(seL4_UserException_Number) ==
                SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_INVALID_OPCODE_VECTOR &&
            fresh_generation > retired_generation) {
            authorization = SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_APPROVED;
            authorization_used = true;
        }
        if (authorization != SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_APPROVED ||
            !authorization_used) {
            return false;
        }
        authorization = authorization_used
                            ? SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_STALE
                            : SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_APPROVED;
        if (authorization != SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_STALE) {
            return false;
        }
        authorization = retired_generation != fresh_generation
                            ? SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_STALE
                            : SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_INVALID;
        if (authorization != SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_REJECTED_STALE) {
            return false;
        }
        debug_puts("SeLinOS static-image fresh-bundle authorization M0: generation 2 authorized once; duplicate and retired generation 1 rejected; no allocation or reuse performed.\n");
    }
#endif
#if CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    if (seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_M0_TERMINAL_IP ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_M0_INVALID_OPCODE_VECTOR ||
        SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_M0_GENERATION !=
            SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_M0_FRESH_GENERATION ||
        vka_alloc_tcb(vka, &fresh_tcb) != seL4_NoError ||
        vka_alloc_cnode_object(vka,
                               SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_M0_CNODE_SLOT_BITS,
                               &fresh_cnode) != seL4_NoError ||
        vka_alloc_vspace_root(vka, &fresh_vspace_root) != seL4_NoError ||
        vka_alloc_notification(vka, &fresh_notification) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &fresh_ipc_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &fresh_entry_frame) != seL4_NoError ||
        vka_alloc_frame(vka, seL4_PageBits, &fresh_stack_frame) != seL4_NoError ||
        fresh_tcb.cptr == seL4_CapNull ||
        fresh_cnode.cptr == seL4_CapNull ||
        fresh_vspace_root.cptr == seL4_CapNull ||
        fresh_notification.cptr == seL4_CapNull ||
        fresh_ipc_frame.cptr == seL4_CapNull ||
        fresh_entry_frame.cptr == seL4_CapNull ||
        fresh_stack_frame.cptr == seL4_CapNull) {
        return false;
    }
    debug_puts("SeLinOS fresh target-bundle construction M0: one generation-2 inert TCB-CNode-PML4-notification-IPC-entry-stack bundle root-owned; no ASID, mapping, configuration or execution.\n");
#endif
#if CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    if (seL4_GetMR(seL4_UserException_FaultIP) !=
            SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_TERMINAL_IP ||
        seL4_GetMR(seL4_UserException_Number) !=
            SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_INVALID_OPCODE_VECTOR ||
        vka_alloc_endpoint(vka, &fresh_fault_endpoint) != seL4_NoError ||
        seL4_CNode_Copy(fresh_cnode.cptr,
                        SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_SELF_SLOT,
                        SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fresh_cnode.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Copy(fresh_cnode.cptr,
                        SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_IPC_FRAME_SLOT,
                        SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fresh_ipc_frame.cptr,
                        seL4_WordBits, seL4_AllRights) != seL4_NoError ||
        seL4_CNode_Mint(fresh_cnode.cptr,
                        SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_FAULT_ENDPOINT_SLOT,
                        SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_CNODE_SLOT_BITS,
                        seL4_CapInitThreadCNode, fresh_fault_endpoint.cptr,
                        seL4_WordBits, seL4_AllRights, 0x74u) != seL4_NoError ||
        seL4_X86_ASIDPool_Assign(seL4_CapInitThreadASIDPool,
                                 fresh_vspace_root.cptr) != seL4_NoError ||
        sel4utils_map_page(vka, fresh_vspace_root.cptr, fresh_ipc_frame.cptr,
                           (void *)SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_FIXED_IPC_VADDR,
                           seL4_AllRights, 1, fresh_paging_objects,
                           &fresh_paging_object_count) != seL4_NoError ||
        seL4_TCB_Configure(fresh_tcb.cptr,
                           SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_FAULT_ENDPOINT_SLOT,
                           fresh_cnode.cptr, 0u, fresh_vspace_root.cptr, 0u,
                           SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_M0_FIXED_IPC_VADDR,
                           fresh_ipc_frame.cptr) != seL4_NoError) {
        return false;
    }
    debug_puts("SeLinOS fresh target-bundle configuration M0: one fresh self-IPC-badged-fault CSpace, ASID, IPC map and suspended TCB configuration; no registers, resume or execution.\n");
#endif
    return true;
}
#endif

bool selinos_domain_manager_start(void)
{
    simple_t *const simple = &root_simple_context;
    vka_t *const vka = &root_vka_context;
    vspace_t *const vspace = &root_vspace_context;

    if (root_construction_context_initialized) {
        debug_puts("SeLinOS M0: root construction context reinitialization rejected.\n");
        return false;
    }
    if (!initialise_root_environment(simple, vka, vspace)) {
        debug_puts("SeLinOS M0: root allocator/vspace bootstrap failed.\n");
        return false;
    }
    root_construction_context_initialized = true;

#if CONFIG_SELINOS_TASKD_DYNAMIC_ALLOCATION_PROBE
    if (!start_taskd_dynamic_alloc_m0_bundle(vka, vspace)) {
        debug_puts("SeLinOS taskd dynamic M0: allocation prerequisite failed; task authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd dynamic M0: allocator reserve/release prerequisite passed.\n");
    debug_puts("SeLinOS taskd dynamic M0: no TCB/CSpace/VSpace or Linux process created.\n");
#endif

#if CONFIG_SELINOS_TASKD_DYNAMIC_TCB_OWNERSHIP_PROBE
    if (!start_taskd_dynamic_tcb_m0_bundle(vka, vspace)) {
        debug_puts("SeLinOS taskd dynamic TCB M0: allocation/move prerequisite failed; target authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd dynamic TCB M0: target TCB allocated then moved into taskd ownership slot.\n");
    debug_puts("SeLinOS taskd dynamic TCB M0: move helper released root source slot; target TCB remains unconfigured, unmapped and unresumed.\n");
#endif

#if CONFIG_SELINOS_TASKD_DYNAMIC_CSPACE_ROLLBACK_PROBE
    if (!start_taskd_dynamic_cspace_m0_bundle(vka, vspace)) {
        debug_puts("SeLinOS taskd dynamic CSpace M0: allocation, rollback or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd dynamic CSpace M0: root rolled back rejected CNode generation 1 before delegation.\n");
    debug_puts("SeLinOS taskd dynamic CSpace M0: replacement CNode generation 2 allocated then moved into taskd ownership slot.\n");
    debug_puts("SeLinOS taskd dynamic CSpace M0: final CNode remains empty, inert and is not a TCB CSpace root.\n");
#endif

#if CONFIG_SELINOS_TASKD_DYNAMIC_VSPACE_ROLLBACK_PROBE
    if (!start_taskd_dynamic_vspace_m0_bundle(vka, vspace)) {
        debug_puts("SeLinOS taskd dynamic VSpace M0: allocation, rollback or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd dynamic VSpace M0: root rolled back rejected VSpace root generation 1 before delegation.\n");
    debug_puts("SeLinOS taskd dynamic VSpace M0: replacement VSpace root generation 2 allocated then moved into taskd ownership slot.\n");
    debug_puts("SeLinOS taskd dynamic VSpace M0: final x86_64 PML4 remains inert, unassigned and unmapped.\n");
#endif

#if CONFIG_SELINOS_TASKD_INERT_BUNDLE_ROLLBACK_PROBE
    if (!start_taskd_inert_bundle_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd inert bundle M0: allocation, rollback or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd inert bundle M0: root rolled back rejected TCB/CNode/PML4 generation 1 before delegation.\n");
    debug_puts("SeLinOS taskd inert bundle M0: replacement TCB/CNode/PML4 generation 2 allocated then moved into taskd ownership slots.\n");
    debug_puts("SeLinOS taskd inert bundle M0: final resources remain mutually unlinked, inert and unexecuted.\n");
#endif

#if CONFIG_SELINOS_TASKD_SUSPENDED_LINKAGE_PROBE
    if (!start_taskd_suspended_linkage_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd suspended linkage M0: allocation, ASID, configuration or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd suspended linkage M0: root rolled back rejected TCB/CNode/PML4 generation 1 before configuration.\n");
    debug_puts("SeLinOS taskd suspended linkage M0: generation 2 PML4 ASID assigned and TCB CSpace/VSpace configured.\n");
    debug_puts("SeLinOS taskd suspended linkage M0: configured resource caps moved into taskd ownership slots; no registers or resume.\n");
#endif

#if CONFIG_SELINOS_TASKD_POPULATED_CSPACE_PROBE
    if (!start_taskd_populated_cspace_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd populated CSpace M0: allocation, population, ASID, configuration or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd populated CSpace M0: root rolled back rejected TCB/CNode/PML4 generation 1 before CSpace population.\n");
    debug_puts("SeLinOS taskd populated CSpace M0: one notification cap inserted into final CNode slot 1.\n");
    debug_puts("SeLinOS taskd populated CSpace M0: generation 2 PML4 ASID assigned and populated CSpace configured.\n");
    debug_puts("SeLinOS taskd populated CSpace M0: final caps moved into taskd ownership slots; no registers or resume.\n");
#endif

#if CONFIG_SELINOS_TASKD_SELF_ROOTED_CSPACE_PROBE
    if (!start_taskd_self_rooted_cspace_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd self-rooted CSpace M0: allocation, population, ASID, configuration or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd self-rooted CSpace M0: root rolled back rejected TCB/CNode/PML4 generation 1 before CSpace population.\n");
    debug_puts("SeLinOS taskd self-rooted CSpace M0: final CNode self cap slot 0 and notification cap slot 1 inserted.\n");
    debug_puts("SeLinOS taskd self-rooted CSpace M0: generation 2 PML4 ASID assigned and self-rooted CSpace configured.\n");
    debug_puts("SeLinOS taskd self-rooted CSpace M0: final caps moved into taskd ownership slots; no registers or resume.\n");
#endif

#if CONFIG_SELINOS_TASKD_IPC_BUFFER_PROBE
    if (!start_taskd_ipc_buffer_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd IPC buffer M0: rollback, mapping, configuration or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd IPC buffer M0: root rolled back rejected TCB/CNode/PML4/frame generation 1.\n");
    debug_puts("SeLinOS taskd IPC buffer M0: fixed 4 KiB IPC frame mapped into generation 2 PML4 and copied to target CSpace.\n");
    debug_puts("SeLinOS taskd IPC buffer M0: configured target remains without registers or resume.\n");
#endif

#if CONFIG_SELINOS_TASKD_ZEROED_CONTEXT_PROBE
    if (!start_taskd_zeroed_context_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd zeroed context M0: rollback, mapping, zero-context read-back or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd zeroed context M0: root rolled back rejected TCB/CNode/PML4/frame generation 1.\n");
    debug_puts("SeLinOS taskd zeroed context M0: complete x86_64 zero context written and read back with resume_target=0.\n");
    debug_puts("SeLinOS taskd zeroed context M0: configured target remains without entry point, stack or resume.\n");
#endif

#if CONFIG_SELINOS_TASKD_FAULT_WITNESS_PROBE
    if (!start_taskd_fault_witness_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd fault witness M0: construction, zero-context, fault delivery or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd fault witness M0: root observed exactly one badged instruction-fetch VM fault at zero.\n");
    debug_puts("SeLinOS taskd fault witness M0: no fault reply or second resume; target remains fault-blocked.\n");
    debug_puts("SeLinOS taskd fault witness M0: no instruction completion, entry point, stack or ELF claim.\n");
#endif

#if CONFIG_SELINOS_TASKD_ENTRY_STACK_PROBE
    if (!start_taskd_entry_stack_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd entry-stack M0: canonical, NX mapping, context read-back or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd entry-stack M0: canonical entry and 16-byte ABI-derived stack provenance read back.\n");
    debug_puts("SeLinOS taskd entry-stack M0: entry and stack leaves are NX; target has no resume or instruction fetch.\n");
    debug_puts("SeLinOS taskd entry-stack M0: no ELF, stack image, return address or successful execution claim.\n");
#endif

#if CONFIG_SELINOS_TASKD_EXEC_FETCH_PROBE
    if (!start_taskd_exec_fetch_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd exec-fetch M0: executable witness, post-NOP fault or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd exec-fetch M0: root observed one invalid-opcode fault at entry plus one, after NOP.\n");
    debug_puts("SeLinOS taskd exec-fetch M0: no fault reply or second resume; target remains blocked.\n");
    debug_puts("SeLinOS taskd exec-fetch M0: no ELF, C runtime, process lifecycle or Linux ABI claim.\n");
#endif

#if CONFIG_SELINOS_TASKD_VM_RESTART_PROBE
    if (!start_taskd_vm_restart_m0(vka, vspace)) {
        debug_puts("SeLinOS taskd VM-restart M0: delayed-map repair, one reply, terminal fault or ownership prerequisite failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS taskd VM-restart M0: one initial VM fault was repaired by executable entry mapping and one zero-label reply.\n");
    debug_puts("SeLinOS taskd VM-restart M0: repaired restart reached one terminal invalid-opcode user exception without a second resume.\n");
    debug_puts("SeLinOS taskd VM-restart M0: no generic continuation, ELF runtime, process lifecycle or Linux ABI claim.\n");
#endif

#if CONFIG_SELINOS_SEALED_STATIC_IMAGE_PROBE
    if (!start_sealed_static_image_m0(vka, vspace)) {
        debug_puts("SeLinOS sealed static-image M0: parser ledger or status-only isolation prerequisite failed.\n");
        return false;
    }
    debug_puts("SeLinOS sealed static-image M0: one SSIM-v1 RX source ledger accepted and five malformed cases rejected.\n");
    debug_puts("SeLinOS sealed static-image M0: parser-only; no loader frame, mapping, permission transition, task resume or ELF claim.\n");
#endif

#if CONFIG_SELINOS_SEALED_STATIC_IMAGE_MAPPING_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE || \
    CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE || \
    CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    if (!start_sealed_static_image_mapping_m0(vka, vspace)) {
        debug_puts("SeLinOS sealed static-image mapping M0: SSIM validation, one-frame W^X materialization or terminal witness failed.\n");
        return false;
    }
    debug_puts("SeLinOS sealed static-image mapping M0: one accepted SSIM payload copied through a root-private alias then unmapped before target executable mapping.\n");
    debug_puts("SeLinOS sealed static-image mapping M0: one NOP completed then terminal invalid-opcode witness received without reply or second resume.\n");
    debug_puts("SeLinOS sealed static-image mapping M0: no raw container mapping, ELF, lifecycle or Linux ABI claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_PROBE
    debug_puts("SeLinOS static-image terminal lifecycle M0: terminal ownership observation only; no exit, cleanup, reuse or Linux process claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_PROBE
    debug_puts("SeLinOS static-image teardown authorization M0: status-only authorization; no teardown, cleanup, reuse or Linux process claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_PROBE
    debug_puts("SeLinOS static-image mapping revocation authorization M0: status-only ordering ledger; no unmap, cleanup, reuse or Linux process claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_PROBE
    debug_puts("SeLinOS static-image mapping revocation M0: one target mapping transaction only; no cap deletion, object free, reuse or Linux process claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE
    debug_puts("SeLinOS static-image target capability deletion M0: target CNode cap copies only; no root-cap deletion, object free, reuse or Linux process claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE
    debug_puts("SeLinOS static-image object reclamation M0: one terminal disposal sequence only; no ASID, slot, object or Linux-process reuse claim.\n");
#endif
#if CONFIG_SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE
    debug_puts("SeLinOS static-image fresh-bundle authorization M0: status-only generation ledger; no allocation, reuse or Linux-process claim.\n");
#endif
#if CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONSTRUCTION_PROBE
    debug_puts("SeLinOS fresh target-bundle construction M0: one root-owned inert generation-2 bundle; no physical-reuse, ASID, mapping, execution or Linux-process claim.\n");
#endif
#if CONFIG_SELINOS_FRESH_TARGET_BUNDLE_CONFIGURATION_PROBE
    debug_puts("SeLinOS fresh target-bundle configuration M0: one suspended self-IPC-badged-fault configuration; no register context, resume, execution or Linux-process claim.\n");
#endif

#if CONFIG_SELINOS_ROOT_IOMMU_AVAILABILITY_PROBE
    /* Phase 49 M0: root reads only kernel-provided bootinfo topology. It does
     * not touch IOSpace, translation state, PCI command bits or any device. */
    const seL4_BootInfo *const iommu_bootinfo = sel4runtime_bootinfo();
    if (iommu_bootinfo == NULL) {
        debug_puts("SeLinOS IOMMU M0: bootinfo unavailable; containment authority withheld.\n");
        return false;
    }
    if (iommu_bootinfo->numIOPTLevels == 0u) {
        debug_puts("SeLinOS IOMMU M0: numIOPTLevels=0; QEMU containment blocker observed.\n");
    } else {
        debug_puts("SeLinOS IOMMU M0: numIOPTLevels>0 observed; no IOSpace or DMA authority granted.\n");
    }
#endif

#if CONFIG_SELINOS_ROOT_E1000_DISCOVERY_PROBE
    /* Phase 13 N0: root reads PCI configuration identity only. This scope
     * neither allocates/maps BARs nor gives a NIC domain any IRQ, DMA or bus
     * authority. */
    struct selinos_qemu_e1000_identity e1000_identity = {0};
    if (!selinos_pci_find_qemu_e1000(vka, &e1000_identity)) {
        debug_puts("SeLinOS network N0: quarantined e1000 PCI identity not found; NIC authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS network N0: quarantined e1000 PCI identity found; BAR/IRQ/DMA/network authority withheld.\n");
#endif
#if CONFIG_SELINOS_ROOT_E1000_BAR_RANGE_PROBE
    /* Phase 13 N1: root alone revalidates the N0 e1000 identity, sizes and
     * exactly restores its firmware BAR0, then retains scalar metadata only.
     * This creates no device frame cap, VSpace mapping, IRQ/DMA grant, bus
     * mastering permission or NIC-domain authority. */
    struct selinos_qemu_e1000_identity e1000_n1_identity = {0};
    struct selinos_pci_bar32_resource e1000_n1_range = {0};
    if (!selinos_pci_find_qemu_e1000(vka, &e1000_n1_identity) ||
        !selinos_pci_read_qemu_e1000_bar_range(vka, &e1000_n1_identity,
                                                &e1000_n1_range)) {
        debug_puts("SeLinOS network N1: e1000 firmware BAR range validation failed; NIC authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS network N1: e1000 firmware BAR range validated and restored; no MMIO/IRQ/DMA/NIC authority.\n");
    debug_puts("SeLinOS network N1: no BAR frame, mapping, IRQ, DMA, bus mastering or packet I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_PCI_BAR_ALLOCATOR_PROBE
    /* Quarantined M0 allocator experiment. The selected QEMU PC MMIO window
     * lies below IOAPIC at 0xfec00000 and is intentionally not represented as
     * a capability or forwarded to any domain. A failed transaction is a
     * hard gate: no NIC resource can be delegated. */
    struct selinos_pci_bar32_resource e1000_bar = {0};
    if (!selinos_pci_assign_qemu_e1000_bar0(vka, 0xc0000000u, 0xfec00000u,
                                            &e1000_bar)) {
        debug_puts("SeLinOS PCI M0: quarantined e1000 BAR0 assignment failed; NIC authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS PCI M0: quarantined e1000 BAR0 assignment read-back passed.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_DISCOVERY_PROBE
    /* Quarantined storage M0 experiment: identify one documented QEMU virtio-blk
     * function using root configuration authority only. It neither reads BARs
     * nor configures virtio status/features, and grants no frame/IRQ/DMA cap. */
    struct selinos_qemu_virtio_blk_identity virtio_blk = {0};
    if (!selinos_pci_find_qemu_virtio_blk(vka, &virtio_blk)) {
        debug_puts("SeLinOS storage M0: quarantined virtio-blk PCI identity not found; storage authority withheld.\n");
        return false;
    }
    if (virtio_blk.device_id == SELINOS_QEMU_VIRTIO_BLK_LEGACY_DEVICE_ID) {
        debug_puts("SeLinOS storage M0: quarantined legacy virtio-blk PCI identity found; authority withheld.\n");
    } else {
        debug_puts("SeLinOS storage M0: quarantined modern virtio-blk PCI identity found; authority withheld.\n");
    }
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_CAPABILITY_PROBE
    /* Quarantined storage M1 preparation: parse only modern virtio PCI
     * capability metadata. BAR bytes, feature/status registers, queue state,
     * interrupts and DMA remain inaccessible to every domain. */
    struct selinos_qemu_virtio_common_capability virtio_common = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_common)) {
        debug_puts("SeLinOS storage M1: quarantined virtio common capability inspection failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M1: quarantined virtio common capability metadata validated; BAR/feature/queue/IRQ/DMA authority withheld.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_BAR_FEATURE_PROBE
    /* Quarantined storage M2: root alone sizes/restores the already-assigned
     * BAR, maps one validated common-config page uncached and read-only, reads
     * the current selector and offered-feature word, and tears every object
     * down before this scope exits. No feature/status write, cap copy, queue,
     * IRQ, DMA, driver start or block request exists in this experiment. */
    struct selinos_qemu_virtio_common_capability virtio_m2_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m2_range = {0};
    struct selinos_qemu_virtio_feature_observation virtio_m2_features = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m2_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m2_common,
                                                            &virtio_m2_range) ||
        !selinos_pci_observe_qemu_virtio_blk_features(vka, vspace, &virtio_m2_range,
                                                       &virtio_m2_features)) {
        debug_puts("SeLinOS storage M2: quarantined virtio BAR/feature observation failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M2: root-only uncached common-config feature observation passed; authority withheld.\n");
    debug_puts("SeLinOS storage M2: no BAR cap delegation, feature acceptance, status write, queue, IRQ, DMA or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_FEATURE_STAGE_PROBE
    /* Root-only storage M3: repeat bounded common-capability/BAR validation,
     * write just reset/ACKNOWLEDGE/DRIVER/FEATURES_OK with zero driver features,
     * validate acceptance, reset to zero and tear all temporary MMIO state down.
     * No DRIVER_OK, queue, notify, IRQ, DMA, bus mastering or block request. */
    struct selinos_qemu_virtio_common_capability virtio_m3_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m3_range = {0};
    struct selinos_qemu_virtio_feature_stage virtio_m3_stage = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m3_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m3_common,
                                                            &virtio_m3_range) ||
        !selinos_pci_stage_qemu_virtio_blk_zero_features(vka, vspace, &virtio_m3_range,
                                                          &virtio_m3_stage)) {
        debug_puts("SeLinOS storage M3: root-only virtio zero-feature status stage failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M3: root-only zero-feature FEATURES_OK accepted then reset to zero.\n");
    debug_puts("SeLinOS storage M3: no DRIVER_OK, queue, IRQ, DMA, bus mastering or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_DRIVER_READY_PROBE
    /* Root-only storage M4: repeat bounded discovery/range validation, stage
     * zero-feature status through one DRIVER_OK read-back, reset to zero and
     * release every temporary object. No queue, IRQ, DMA, bus mastering or I/O. */
    struct selinos_qemu_virtio_common_capability virtio_m4_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m4_range = {0};
    struct selinos_qemu_virtio_driver_ready_stage virtio_m4_stage = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m4_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m4_common,
                                                            &virtio_m4_range) ||
        !selinos_pci_stage_qemu_virtio_blk_driver_ready_reset(vka, vspace,
                                                               &virtio_m4_range,
                                                               &virtio_m4_stage)) {
        debug_puts("SeLinOS storage M4: root-only virtio DRIVER_OK/reset stage failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M4: root-only zero-feature DRIVER_OK accepted then reset to zero.\n");
    debug_puts("SeLinOS storage M4: no queue, IRQ, DMA, bus mastering or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_QUEUE_ZERO_PROBE
    /* Root-only storage M5: temporarily stage zero-feature DRIVER_OK, select
     * only queue zero, read one non-zero queue-size value, reset and release
     * all temporary objects. No queue size/address/enable/notify, IRQ, DMA or I/O. */
    struct selinos_qemu_virtio_common_capability virtio_m5_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m5_range = {0};
    struct selinos_qemu_virtio_queue_zero_observation virtio_m5_queue = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m5_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m5_common,
                                                            &virtio_m5_range) ||
        !selinos_pci_observe_qemu_virtio_blk_queue_zero(vka, vspace, &virtio_m5_range,
                                                         &virtio_m5_queue)) {
        debug_puts("SeLinOS storage M5: root-only virtio queue-zero observation failed; authority withheld.\n");
        return false;
    }
        debug_puts("SeLinOS storage M5: root-only queue-zero size observed then reset to zero.\n");
    debug_puts("SeLinOS storage M5: no queue enable/address/notify, IRQ, DMA, bus mastering or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_QUEUE_LAYOUT_PROBE
    /* Root-only storage M6: allocate one ordinary root frame, zero one
     * one-slot split-ring layout, program only queue-zero size/address pairs,
     * observe queue_enable remains zero, reset and release every object. */
    struct selinos_qemu_virtio_common_capability virtio_m6_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m6_range = {0};
    struct selinos_qemu_virtio_queue_layout_stage virtio_m6_layout = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m6_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m6_common,
                                                            &virtio_m6_range) ||
        !selinos_pci_stage_qemu_virtio_blk_queue_layout(vka, vspace, &virtio_m6_range,
                                                         &virtio_m6_layout)) {
        debug_puts("SeLinOS storage M6: root-only disabled queue-layout stage failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M6: root-only one-slot queue layout/address read-backs passed.\n");
    debug_puts("SeLinOS storage M6: queue_enable stayed zero; no enable/notify, IRQ, DMA authority or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_QUEUE_ENABLE_PROBE
    /* Root-only storage M7: repeat one unpublished zeroed queue layout, write
     * queue_enable once, require read-back one, reset, then require zero.
     * No notification, IRQ, PCI-command write, block I/O or DMA claim. */
    struct selinos_qemu_virtio_common_capability virtio_m7_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m7_range = {0};
    struct selinos_qemu_virtio_queue_enable_stage virtio_m7_enable = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m7_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m7_common,
                                                            &virtio_m7_range) ||
        !selinos_pci_stage_qemu_virtio_blk_queue_enable_reset(vka, vspace,
                                                               &virtio_m7_range,
                                                               &virtio_m7_enable)) {
        debug_puts("SeLinOS storage M7: root-only queue-enable/reset stage failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M7: queue_enable one read-back then reset-to-zero passed.\n");
    debug_puts("SeLinOS storage M7: no notify, IRQ, PCI-command write, block I/O or DMA/containment claim.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_NOTIFICATION_OBSERVATION_PROBE
    /* Root-only storage M8: parse and bounds-check notification capability,
     * select queue zero in the common page, derive one scalar notify address,
     * reset and release. Notification BAR remains unmapped and unwritten. */
    struct selinos_qemu_virtio_common_capability virtio_m8_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m8_common_range = {0};
    struct selinos_qemu_virtio_notify_capability virtio_m8_notify = {0};
    struct selinos_qemu_virtio_notify_range virtio_m8_notify_range = {0};
    struct selinos_qemu_virtio_notification_observation virtio_m8_observation = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m8_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m8_common,
                                                            &virtio_m8_common_range) ||
        !selinos_pci_inspect_qemu_virtio_blk_notify_capability(vka, &virtio_m8_notify) ||
        !selinos_pci_validate_qemu_virtio_blk_notify_range(vka, &virtio_m8_notify,
                                                            &virtio_m8_notify_range) ||
        !selinos_pci_observe_qemu_virtio_blk_queue_zero_notification(
            vka, vspace, &virtio_m8_common_range, &virtio_m8_notify_range,
            &virtio_m8_observation)) {
        debug_puts("SeLinOS storage M8: root-only notification observation failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M8: queue-zero notification address observation passed.\n");
    debug_puts("SeLinOS storage M8: notification BAR unmapped; no notify, IRQ, DMA claim or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_ZERO_DESCRIPTOR_NOTIFICATION_PROBE
    /* Root-only storage M9: construct a fresh zeroed one-slot layout, enable
     * queue zero once, issue one notification value zero, reset and release.
     * This intentionally proves neither DMA behavior nor block I/O. */
    struct selinos_qemu_virtio_common_capability virtio_m9_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m9_common_range = {0};
    struct selinos_qemu_virtio_notify_capability virtio_m9_notify = {0};
    struct selinos_qemu_virtio_notify_range virtio_m9_notify_range = {0};
    struct selinos_qemu_virtio_zero_descriptor_notification_stage virtio_m9_stage = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m9_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m9_common,
                                                            &virtio_m9_common_range) ||
        !selinos_pci_inspect_qemu_virtio_blk_notify_capability(vka, &virtio_m9_notify) ||
        !selinos_pci_validate_qemu_virtio_blk_notify_range(vka, &virtio_m9_notify,
                                                            &virtio_m9_notify_range) ||
        !selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset(
            vka, vspace, &virtio_m9_common_range, &virtio_m9_notify_range,
            &virtio_m9_stage)) {
        debug_puts("SeLinOS storage M9: zero-descriptor notification/reset failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M9: one zero-descriptor queue-zero notification then reset passed.\n");
    debug_puts("SeLinOS storage M9: no IRQ, DMA claim, containment claim or block I/O.\n");
#endif
#if CONFIG_SELINOS_ROOT_VIRTIO_BLK_ZERO_INDEX_OBSERVATION_PROBE
    /* Root-only storage M10: repeat the zero-descriptor notification path and
     * sample only four local avail/used words immediately after it. This is a
     * QEMU observation, not a DMA, completion, IRQ or I/O claim. */
    struct selinos_qemu_virtio_common_capability virtio_m10_common = {0};
    struct selinos_qemu_virtio_common_range virtio_m10_common_range = {0};
    struct selinos_qemu_virtio_notify_capability virtio_m10_notify = {0};
    struct selinos_qemu_virtio_notify_range virtio_m10_notify_range = {0};
    struct selinos_qemu_virtio_zero_descriptor_notification_stage virtio_m10_stage = {0};
    if (!selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &virtio_m10_common) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &virtio_m10_common,
                                                            &virtio_m10_common_range) ||
        !selinos_pci_inspect_qemu_virtio_blk_notify_capability(vka, &virtio_m10_notify) ||
        !selinos_pci_validate_qemu_virtio_blk_notify_range(vka, &virtio_m10_notify,
                                                            &virtio_m10_notify_range) ||
        !selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset(
            vka, vspace, &virtio_m10_common_range, &virtio_m10_notify_range,
            &virtio_m10_stage)) {
        debug_puts("SeLinOS storage M10: post-notification zero-index observation failed; authority withheld.\n");
        return false;
    }
    debug_puts("SeLinOS storage M10: immediate avail/used zero-index observation passed.\n");
    debug_puts("SeLinOS storage M10: no completion, IRQ, DMA claim, containment claim or block I/O.\n");
#endif
    struct selinos_qemu_edu_resource edu_resources[SELINOS_MAX_QEMU_EDU_INSTANCES];
    const size_t qemu_edu_count = selinos_pci_enumerate_qemu_edu(
        vka, edu_resources, SELINOS_MAX_QEMU_EDU_INSTANCES);
    const size_t active_edu_count = qemu_edu_count < SELINOS_MAX_QEMU_EDU_INSTANCES
                                        ? qemu_edu_count
                                        : SELINOS_MAX_QEMU_EDU_INSTANCES;
    const bool has_qemu_edu = active_edu_count != 0u;
    if (has_qemu_edu) {
        debug_puts("SeLinOS M0: QEMU edu PCI function detected.\n");
        if (qemu_edu_count > 1u) {
            debug_puts("SeLinOS M3: multiple QEMU edu functions discovered; independent bundles starting.\n");
        }
        for (size_t instance = 0u; instance < active_edu_count; ++instance) {
            if (!selinos_start_deviced_domain(vka, vspace, &edu_resources[instance],
                                              (unsigned int)instance)) {
                debug_puts("SeLinOS M3: unable to start per-device deviced domain.\n");
                return false;
            }
        }
        for (size_t instance = 0u; instance < active_edu_count; ++instance) {
            if (!selinos_start_edu_driver_domain(vka, vspace, &edu_resources[instance],
                                                 (unsigned int)instance)) {
                debug_puts("SeLinOS M3: QEMU edu driver domain grant failed.\n");
                return false;
            }
        }
    } else {
        debug_puts("SeLinOS M0: QEMU edu PCI function not present.\n");
        if (!selinos_start_deviced_domain(vka, vspace, NULL, 0u)) {
            debug_puts("SeLinOS M0: unable to start dormant deviced domain.\n");
            return false;
        }
    }

    if (!start_objectd_fixed_inventory_m1_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start objectd fixed-inventory bundle.\n");
        return false;
    }
    if (!start_taskd_fixed_child_lifecycle_m1_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start taskd fixed-child lifecycle bundle.\n");
        return false;
    }
    if (!start_memd_fixed_mapping_inventory_m1_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start memd fixed-mapping-inventory bundle.\n");
        return false;
    }
    if (!start_taskd_combined_reservation_m1_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start taskd combined-reservation bundle.\n");
        return false;
    }
    if (!start_opaque_lease_m1_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start opaque lease-token bundle.\n");
        return false;
    }
    if (!start_tcb_control_lease_m1_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start TCB control-lease bundle.\n");
        return false;
    }
    if (!start_transferred_tcb_single_resume_m2_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start transferred-TCB single-resume M2 bundle.\n");
        return false;
    }
#if CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER
    if (!selinos_root_construct_m1_start_bundle(vka, vspace)) {
        debug_puts("SeLinOS root construction M1: unable to start taskd adapter client.\n");
        return false;
    }
#else
    if (!start_root_dispatch_m0_probe_bundle(vka, vspace)) {
        debug_puts("SeLinOS M0: unable to start root dispatch M0 probe bundle.\n");
        return false;
    }
#endif
    if (!start_romfs_bundle(vka, vspace)) {
        debug_puts("SeLinOS ROMFS: unable to start isolated server/client bundle.\n");
        return false;
    }
    if (!start_one_service(vka, vspace, "selinos-consoled", "selinos-consoled")) {
        debug_puts("SeLinOS M0: unable to start consoled domain.\n");
        return false;
    }
    if (!start_one_service(vka, vspace, "selinos-abi-gated", "selinos-abi-gated")) {
        debug_puts("SeLinOS M0: unable to start abi-gated domain.\n");
        return false;
    }
    if (!run_linux_syscall_abi_probe(vka, vspace)) {
        debug_puts("SeLinOS ABI gateway: Linux syscall ABI probe failed.\n");
        return false;
    }
#if CONFIG_SELINOS_X86_NX_MAPPING_PROBE
    if (!selinos_run_x86_nx_mapping_probe(vka, vspace)) {
        debug_puts("SeLinOS W^X NX: isolated mapping proof failed.\n");
        return false;
    }
#endif
    if (!start_one_service(vka, vspace, "selinos-modld", "selinos-modld")) {
        debug_puts("SeLinOS M0: unable to start modld domain.\n");
        return false;
    }
    if (!start_one_service(vka, vspace, "selinos-modexec", "selinos-modexec")) {
        debug_puts("SeLinOS M0: unable to start modexec domain.\n");
        return false;
    }
    if (!has_qemu_edu &&
        !start_one_service(vka, vspace, "selinos-edu-kapi-probe", "selinos-edu-kapi-probe")) {
        debug_puts("SeLinOS M0: unable to start dormant edu KAPI probe domain.\n");
        return false;
    }

#if CONFIG_SELINOS_ROOT_CONSTRUCT_M1_ADAPTER
    debug_puts("SeLinOS root construction M1: thirty runnable isolated service domains and one pre-existing suspended child fixture configured.\n");
#else
    debug_puts("SeLinOS M0: thirty runnable isolated service domains and one suspended child fixture configured.\n");
#endif
    return true;
}
