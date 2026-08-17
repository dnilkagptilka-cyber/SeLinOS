// SPDX-License-Identifier: MIT
#define _GNU_SOURCE
#include "selinos_kabi_exports.h"
#include "selinos_kabi_module.h"
#include "selinos_kabi_policy.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static int printk_calls;

static void fault_handler(int signal_number, siginfo_t *info, void *context)
{
    const ucontext_t *state = (const ucontext_t *)context;
    fprintf(stderr, "execution fault: signal=%d address=%p rip=%llx\n", signal_number,
            info->si_addr, (unsigned long long)state->uc_mcontext.gregs[REG_RIP]);
    _Exit(128 + signal_number);
}

/* The kernel-built fixture passes only its format pointer and does not
 * establish host System V variadic vector-register metadata. */
__attribute__((noinline)) static int fixture_printk(const char *format)
{
    (void)format;
    ++printk_calls;
    puts("host fixture _printk stub invoked");
    return 0;
}

__attribute__((naked, noinline)) static void fixture_return_thunk(void)
{
    __asm__("ret");
}

static unsigned char *read_all(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    long length;
    unsigned char *data;

    if (file == NULL || fseek(file, 0, SEEK_END) != 0 ||
        (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0) {
        if (file != NULL) {
            fclose(file);
        }
        return NULL;
    }
    data = malloc((size_t)length);
    if (data == NULL || fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (size_t)length;
    return data;
}

int main(int argc, char **argv)
{
    static const selinos_u8 fixture_digest[SELINOS_KABI_SHA256_BYTES] = {
        0xa4U, 0xc3U, 0xa6U, 0x61U, 0x4eU, 0x39U, 0x19U, 0x1aU,
        0xf7U, 0x07U, 0x1bU, 0xa3U, 0x20U, 0xb2U, 0xaaU, 0xc4U,
        0xaeU, 0x30U, 0x3aU, 0x9aU, 0xc8U, 0x15U, 0xc2U, 0xcdU,
        0x4dU, 0x86U, 0xd2U, 0xe1U, 0x57U, 0x5aU, 0x72U, 0x3dU,
    };
    struct selinos_kabi_export exports[3];
    struct selinos_kabi_export_table table;
    struct selinos_kabi_loaded_module loaded;
    unsigned char *image;
    unsigned char *memory;
    size_t image_size;
    const size_t memory_size = 65536u;
    int (*init_module)(void);
    void (*cleanup_module)(void);

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = fault_handler;
    action.sa_flags = SA_SIGINFO;
    (void)sigaction(SIGSEGV, &action, NULL);
    (void)sigaction(SIGILL, &action, NULL);
    if (argc != 2 || (image = read_all(argv[1], &image_size)) == NULL) {
        return 2;
    }
    memory = mmap(NULL, memory_size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (memory == MAP_FAILED) {
        free(image);
        return 2;
    }
    exports[0] = (struct selinos_kabi_export){"_printk", 0x122c3a7eU,
        (selinos_u64)(unsigned long)fixture_printk};
    exports[1] = (struct selinos_kabi_export){"__x86_return_thunk", 0x5b8239caU,
        (selinos_u64)(unsigned long)fixture_return_thunk};
    exports[2] = (struct selinos_kabi_export){"module_layout", 0x3cfc2cadU,
        (selinos_u64)(unsigned long)(memory + 0x8200u)};
    table.entries = exports;
    table.count = 3u;
    if (selinos_kabi_check_pinned_digest(image, image_size, fixture_digest) != SELINOS_KABI_POLICY_OK ||
        selinos_kabi_verify_module_versions(image, image_size, "6.18.44",
                                            selinos_kabi_resolve_export_crc, &table) !=
            SELINOS_KABI_MODULE_OK ||
        selinos_kabi_relocate_module(image, image_size, "6.18.44", memory, memory_size,
                                     selinos_kabi_resolve_export, &table, &loaded) !=
            SELINOS_KABI_MODULE_OK ||
        mprotect(memory, memory_size, PROT_READ | PROT_EXEC) != 0) {
        free(image);
        (void)munmap(memory, memory_size);
        return 1;
    }
    free(image);
    init_module = (int (*)(void))(unsigned long)loaded.init_module;
    cleanup_module = (void (*)(void))(unsigned long)loaded.cleanup_module;
    printf("execution addresses: init=%p cleanup=%p printk=%p thunk=%p\n",
           (void *)init_module, (void *)cleanup_module, (void *)fixture_printk,
           (void *)fixture_return_thunk);
    puts("execution stage: init");
    if (init_module() != 0 || printk_calls != 1) {
        (void)munmap(memory, memory_size);
        return 1;
    }
    puts("execution stage: cleanup");
    cleanup_module();
    if (printk_calls != 2) {
        (void)munmap(memory, memory_size);
        return 1;
    }
    puts("SeLinOS KABI controlled fixture execution regression passed.");
    (void)munmap(memory, memory_size);
    return 0;
}
