// SPDX-License-Identifier: MIT
#define _GNU_SOURCE
#include "selinos_kabi_exports.h"
#include "selinos_kabi_module.h"
#include "selinos_kabi_policy.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static unsigned char *read_all(const char *path, size_t *out_size)
{
    FILE *stream = fopen(path, "rb");
    long length;
    unsigned char *data;

    if (stream == NULL) {
        return NULL;
    }
    if (fseek(stream, 0, SEEK_END) != 0 || (length = ftell(stream)) < 0 ||
        fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return NULL;
    }
    data = malloc((size_t)length);
    if (data == NULL || fread(data, 1, (size_t)length, stream) != (size_t)length) {
        free(data);
        fclose(stream);
        return NULL;
    }
    fclose(stream);
    *out_size = (size_t)length;
    return data;
}

int main(int argc, char **argv)
{
    struct selinos_kabi_module_summary summary;
    struct selinos_kabi_loaded_module loaded;
    struct selinos_kabi_export exports[3];
    struct selinos_kabi_export_table export_table;
    unsigned char *image;
    unsigned char *load_memory;
    size_t image_size;
    const size_t load_memory_size = 65536u;
    selinos_u64 load_base;
    selinos_u64 resolved_address = 0U;
    int status;
    static const selinos_u8 fixture_digest[SELINOS_KABI_SHA256_BYTES] = {
        0xa4U, 0xc3U, 0xa6U, 0x61U, 0x4eU, 0x39U, 0x19U, 0x1aU,
        0xf7U, 0x07U, 0x1bU, 0xa3U, 0x20U, 0xb2U, 0xaaU, 0xc4U,
        0xaeU, 0x30U, 0x3aU, 0x9aU, 0xc8U, 0x15U, 0xc2U, 0xcdU,
        0x4dU, 0x86U, 0xd2U, 0xe1U, 0x57U, 0x5aU, 0x72U, 0x3dU,
    };
    selinos_u8 wrong_digest[SELINOS_KABI_SHA256_BYTES];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <module.ko>\n", argv[0]);
        return 2;
    }
    image = read_all(argv[1], &image_size);
    if (image == NULL) {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    if (selinos_kabi_check_pinned_digest(image, image_size, fixture_digest) !=
        SELINOS_KABI_POLICY_OK) {
        fprintf(stderr, "pinned digest rejected real fixture\n");
        free(image);
        return 1;
    }
    memcpy(wrong_digest, fixture_digest, sizeof(wrong_digest));
    wrong_digest[0] ^= 1U;
    if (selinos_kabi_check_pinned_digest(image, image_size, wrong_digest) !=
        SELINOS_KABI_POLICY_E_HASH) {
        fprintf(stderr, "wrong pinned digest accepted fixture\n");
        free(image);
        return 1;
    }
    status = selinos_kabi_parse_module(image, image_size, "6.18.44", &summary);
    if (status != SELINOS_KABI_MODULE_OK) {
        fprintf(stderr, "parse failed: %d\n", status);
        free(image);
        return 1;
    }
    load_memory = mmap(NULL, load_memory_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (load_memory == MAP_FAILED) {
        free(image);
        return 2;
    }
    load_base = (selinos_u64)(unsigned long)load_memory;
    exports[0] = (struct selinos_kabi_export){"_printk", 0x122c3a7eU, load_base + 0x8000U};
    exports[1] = (struct selinos_kabi_export){"__x86_return_thunk", 0x5b8239caU,
                                               load_base + 0x8100U};
    exports[2] = (struct selinos_kabi_export){"module_layout", 0x3cfc2cadU,
                                               load_base + 0x8200U};
    export_table.entries = exports;
    export_table.count = 3U;
    if (!selinos_kabi_resolve_export("_printk", &resolved_address, &export_table) ||
        resolved_address != exports[0].runtime_address ||
        selinos_kabi_resolve_export("unlisted_symbol", &resolved_address, &export_table) ||
        selinos_kabi_verify_module_versions(image, image_size, "6.18.44",
                                            selinos_kabi_resolve_export_crc,
                                            &export_table) != SELINOS_KABI_MODULE_OK) {
        fprintf(stderr, "curated export policy test failed\n");
        (void)munmap(load_memory, load_memory_size);
        free(image);
        return 1;
    }
    exports[2].crc ^= 1U;
    if (selinos_kabi_verify_module_versions(image, image_size, "6.18.44",
                                            selinos_kabi_resolve_export_crc,
                                            &export_table) != SELINOS_KABI_MODULE_E_SYMBOL) {
        fprintf(stderr, "wrong curated CRC accepted fixture\n");
        (void)munmap(load_memory, load_memory_size);
        free(image);
        return 1;
    }
    exports[2].crc ^= 1U;
    status = selinos_kabi_relocate_module(image, image_size, "6.18.44",
                                          load_memory, load_memory_size,
                                          selinos_kabi_resolve_export, &export_table,
                                          &loaded);
    free(image);
    if (status != SELINOS_KABI_MODULE_OK || loaded.init_module == 0u ||
        loaded.cleanup_module == 0u || loaded.applied_relocations == 0u) {
        fprintf(stderr, "relocation failed: %d\n", status);
        (void)munmap(load_memory, load_memory_size);
        return 1;
    }
    printf("name=%s\nvermagic=%s\nrelocation_sections=%u\nundefined_symbols=%u\nmodinfo_entries=%u\nhas_versions=%d\npinned_digest_guard=passed\ncurated_export_guard=passed\nmodule_versions_guard=passed\nrelocated_sections=%u\napplied_relocations=%u\ninit_module=0x%lx\ncleanup_module=0x%lx\n",
           summary.name, summary.vermagic, summary.relocation_sections,
           summary.undefined_symbols, summary.modinfo_entries, summary.has_versions,
           loaded.relocated_sections, loaded.applied_relocations,
           (unsigned long)loaded.init_module, (unsigned long)loaded.cleanup_module);
    (void)munmap(load_memory, load_memory_size);
    return 0;
}
