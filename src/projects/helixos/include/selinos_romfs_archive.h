// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>

/* Immutable root-owned CPIO bytes linked only into selinos-romfsd. */
extern const unsigned char selinos_romfs_cpio_archive[];
extern const size_t selinos_romfs_cpio_archive_size;
