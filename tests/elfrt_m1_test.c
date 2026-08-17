// SPDX-License-Identifier: MIT
#include <stdio.h>
#include <stdlib.h>

#include "selinos_elfrt.h"

static unsigned char *read_file(const char *path, unsigned long *size)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
        return NULL;
    }
    long end = ftell(file);
    if (end <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    unsigned char *data = malloc((size_t)end);
    if (data == NULL || fread(data, 1u, (size_t)end, file) != (size_t)end) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (unsigned long)end;
    return data;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        return 2;
    }
    unsigned long size = 0u;
    unsigned char *image = read_file(argv[1], &size);
    struct selinos_elfrt_summary summary;
    if (image == NULL ||
        selinos_elfrt_parse_image(image, size, 1, &summary) != SELINOS_ELFRT_OK ||
        summary.elf_type != 3u || summary.elf_machine != 62u ||
        summary.load_segments == 0u || summary.has_dynamic == 0u ||
        summary.needed_count == 0u || summary.needed[0][0] == '\0' ||
        selinos_elfrt_validate_initial_load_policy(&summary) != SELINOS_ELFRT_OK ||
        selinos_elfrt_parse_image(image, 4u, 1, &summary) != SELINOS_ELFRT_E_TRUNCATED) {
        free(image);
        return 1;
    }
    free(image);

    size = 0u;
    image = read_file(argv[2], &size);
    if (image == NULL ||
        selinos_elfrt_parse_image(image, size, 0, &summary) != SELINOS_ELFRT_E_POLICY ||
        selinos_elfrt_parse_image(image, size, 1, &summary) != SELINOS_ELFRT_OK ||
        summary.has_interp != 1u || summary.interpreter[0] == '\0' ||
        selinos_elfrt_validate_initial_load_policy(&summary) != SELINOS_ELFRT_OK) {
        free(image);
        return 1;
    }
    summary.has_textrel = 1u;
    if (selinos_elfrt_validate_initial_load_policy(&summary) != SELINOS_ELFRT_E_POLICY) {
        free(image);
        return 1;
    }
    summary.has_textrel = 0u;
    summary.writable_executable_load_segments = 1u;
    if (selinos_elfrt_validate_initial_load_policy(&summary) != SELINOS_ELFRT_E_POLICY) {
        free(image);
        return 1;
    }
    printf("SeLinOS ELFRT M1: ET_DYN, PT_INTERP and initial W^X policy paths verified.\n");
    free(image);
    return 0;
}
