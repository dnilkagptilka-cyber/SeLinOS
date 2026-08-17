// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>

#include "selinos_romfs_archive.h"
#include "selinos_romfs_protocol.h"

#define SELINOS_CPIO_NEWC_HEADER_SIZE 110u
#define SELINOS_CPIO_MAX_ENTRIES 16u
#define SELINOS_CPIO_MODE_REGULAR 0100000u
#define SELINOS_CPIO_MODE_TYPE_MASK 0170000u

struct selinos_romfs_entry {
    const unsigned char *data;
    seL4_Word size;
};

struct selinos_romfs_file {
    seL4_Word file_id;
    seL4_Word fd;
    const char *pathname;
    seL4_Word expected_data_length;
    seL4_Word reported_length;
};

static const struct selinos_romfs_file selinos_romfs_files[] = {
    {SELINOS_ROMFS_FILE_RELEASE, SELINOS_ROMFS_FD_RELEASE, "/selinos-release",
     SELINOS_ROMFS_RELEASE_MAGIC_LENGTH, SELINOS_ROMFS_RELEASE_LENGTH},
    {SELINOS_ROMFS_FILE_BANNER, SELINOS_ROMFS_FD_BANNER, "/selinos-banner",
     SELINOS_ROMFS_BANNER_LENGTH, SELINOS_ROMFS_BANNER_LENGTH},
    {SELINOS_ROMFS_FILE_VERSION, SELINOS_ROMFS_FD_VERSION, "/selinos-version",
     SELINOS_ROMFS_VERSION_LENGTH, SELINOS_ROMFS_VERSION_LENGTH},
};

/* Phase 12 M0 is deliberately a single server-owned volatile word, not a
 * CPIO member and not a backing-store abstraction. It begins at each boot
 * with the declared value and is reachable only through fixed IPC messages. */
static seL4_Word selinos_volatile_state = SELINOS_ROMFS_VOLATILE_STATE_INITIAL;
static int selinos_volatile_state_open = 0;

static void reply_status(seL4_Word status, seL4_Word value)
{
    seL4_SetMR(0, status);
    seL4_SetMR(1, value);
    seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 2u));
}

static int add_bounded(size_t first, size_t second, size_t *result)
{
    if (result == NULL || first > selinos_romfs_cpio_archive_size ||
        second > selinos_romfs_cpio_archive_size - first) {
        return 0;
    }
    *result = first + second;
    return 1;
}

static int align4_bounded(size_t offset, size_t *result)
{
    if (offset > selinos_romfs_cpio_archive_size ||
        offset > selinos_romfs_cpio_archive_size - 3u) {
        return 0;
    }
    return add_bounded(0u, (offset + 3u) & ~((size_t)3u), result);
}

static int parse_hex8(const unsigned char *text, seL4_Word *value)
{
    seL4_Word parsed = 0u;

    if (text == NULL || value == NULL) {
        return 0;
    }
    for (size_t index = 0u; index < 8u; ++index) {
        const unsigned char digit = text[index];
        seL4_Word nibble;
        if (digit >= (unsigned char)'0' && digit <= (unsigned char)'9') {
            nibble = (seL4_Word)(digit - (unsigned char)'0');
        } else if (digit >= (unsigned char)'a' && digit <= (unsigned char)'f') {
            nibble = (seL4_Word)(digit - (unsigned char)'a' + 10u);
        } else if (digit >= (unsigned char)'A' && digit <= (unsigned char)'F') {
            nibble = (seL4_Word)(digit - (unsigned char)'A' + 10u);
        } else {
            return 0;
        }
        parsed = (parsed << 4u) | nibble;
    }
    *value = parsed;
    return 1;
}

static int literal_equals(const unsigned char *name, seL4_Word name_size, const char *literal)
{
    seL4_Word index = 0u;

    if (name == NULL || literal == NULL || name_size == 0u || name[name_size - 1u] != '\0') {
        return 0;
    }
    while (literal[index] != '\0') {
        if (index + 1u >= name_size || name[index] != (unsigned char)literal[index]) {
            return 0;
        }
        ++index;
    }
    return index + 1u == name_size;
}

static int cpio_magic_is_newc(const unsigned char *header)
{
    return header[0] == '0' && header[1] == '7' && header[2] == '0' &&
           header[3] == '7' && header[4] == '0' && header[5] == '1';
}

static int cpio_lookup_regular(const char *pathname, struct selinos_romfs_entry *entry)
{
    size_t offset = 0u;

    if (pathname == NULL || entry == NULL) {
        return 0;
    }
    for (unsigned int member = 0u; member < SELINOS_CPIO_MAX_ENTRIES; ++member) {
        size_t header_end;
        size_t name_start;
        size_t name_end;
        size_t data_start;
        size_t data_end;
        seL4_Word mode;
        seL4_Word file_size;
        seL4_Word name_size;
        const unsigned char *header;
        const unsigned char *name;

        if (!align4_bounded(offset, &offset) ||
            !add_bounded(offset, SELINOS_CPIO_NEWC_HEADER_SIZE, &header_end)) {
            return 0;
        }
        header = &selinos_romfs_cpio_archive[offset];
        if (!cpio_magic_is_newc(header) || !parse_hex8(&header[14u], &mode) ||
            !parse_hex8(&header[54u], &file_size) || !parse_hex8(&header[94u], &name_size) ||
            name_size == 0u) {
            return 0;
        }
        name_start = header_end;
        if (!add_bounded(name_start, (size_t)name_size, &name_end) ||
            !align4_bounded(name_end, &data_start) ||
            !add_bounded(data_start, (size_t)file_size, &data_end)) {
            return 0;
        }
        name = &selinos_romfs_cpio_archive[name_start];
        if (literal_equals(name, name_size, "TRAILER!!!")) {
            return file_size == 0u ? 0 : 0;
        }
        if (literal_equals(name, name_size, pathname)) {
            if ((mode & SELINOS_CPIO_MODE_TYPE_MASK) != SELINOS_CPIO_MODE_REGULAR) {
                return 0;
            }
            entry->data = &selinos_romfs_cpio_archive[data_start];
            entry->size = file_size;
            return 1;
        }
        if (!align4_bounded(data_end, &offset)) {
            return 0;
        }
    }
    return 0;
}

static const struct selinos_romfs_file *file_by_id(seL4_Word file_id)
{
    for (size_t index = 0u; index < sizeof(selinos_romfs_files) / sizeof(selinos_romfs_files[0]);
         ++index) {
        if (selinos_romfs_files[index].file_id == file_id) {
            return &selinos_romfs_files[index];
        }
    }
    return NULL;
}

static const struct selinos_romfs_file *file_by_packed_path(seL4_Word byte_length,
                                                              seL4_Word first_word,
                                                              seL4_Word second_word)
{
    char pathname[SELINOS_ROMFS_PATH_MAX_BYTES + 1u];

    if (byte_length == 0u || byte_length > SELINOS_ROMFS_PATH_MAX_BYTES) {
        return NULL;
    }
    for (seL4_Word index = 0u; index < byte_length; ++index) {
        const seL4_Word word = index < sizeof(seL4_Word) ? first_word : second_word;
        const unsigned int shift = (unsigned int)((sizeof(seL4_Word) - 1u -
                                                    (index % sizeof(seL4_Word))) * 8u);
        const unsigned char character = (unsigned char)(word >> shift);
        if (character <= (unsigned char)' ' || character > (unsigned char)'~') {
            return NULL;
        }
        pathname[index] = (char)character;
    }
    pathname[byte_length] = '\0';
    for (size_t index = 0u; index < sizeof(selinos_romfs_files) / sizeof(selinos_romfs_files[0]);
         ++index) {
        size_t character = 0u;
        while (character < byte_length &&
               selinos_romfs_files[index].pathname[character] == pathname[character]) {
            ++character;
        }
        if (character == byte_length && selinos_romfs_files[index].pathname[character] == '\0') {
            return &selinos_romfs_files[index];
        }
    }
    return NULL;
}

static const struct selinos_romfs_file *file_by_fd(seL4_Word fd)
{
    for (size_t index = 0u; index < sizeof(selinos_romfs_files) / sizeof(selinos_romfs_files[0]);
         ++index) {
        if (selinos_romfs_files[index].fd == fd) {
            return &selinos_romfs_files[index];
        }
    }
    return NULL;
}

static int entry_payload_word(const struct selinos_romfs_entry *entry, seL4_Word *payload)
{
    seL4_Word value = 0u;

    if (entry == NULL || payload == NULL || entry->data == NULL || entry->size == 0u ||
        entry->size > sizeof(seL4_Word)) {
        return 0;
    }
    /* Encode bytes in numeric big-endian order so root's native x86_64 word
     * store preserves the existing user-visible compare constant. */
    for (seL4_Word index = 0u; index < entry->size; ++index) {
        value = (value << 8u) | entry->data[index];
    }
    *payload = value;
    return 1;
}

static int archive_self_check(void)
{
    struct selinos_romfs_entry release;
    struct selinos_romfs_entry banner;
    struct selinos_romfs_entry version;
    seL4_Word payload;

    return cpio_lookup_regular("/selinos-release", &release) &&
           release.size == SELINOS_ROMFS_RELEASE_MAGIC_LENGTH &&
           entry_payload_word(&release, &payload) && payload == SELINOS_ROMFS_RELEASE_MAGIC &&
           cpio_lookup_regular("/selinos-banner", &banner) &&
           banner.size == SELINOS_ROMFS_BANNER_LENGTH &&
           entry_payload_word(&banner, &payload) && payload == SELINOS_ROMFS_BANNER_MAGIC &&
           cpio_lookup_regular("/selinos-version", &version) &&
           version.size == SELINOS_ROMFS_VERSION_LENGTH &&
           entry_payload_word(&version, &payload) && payload == SELINOS_ROMFS_VERSION_MAGIC;
}

int main(void)
{
    seL4_Word badge = 0u;

    if (!archive_self_check()) {
        seL4_DebugPutString("SeLinOS romfsd: embedded newc CPIO archive validation failed.\n");
        return 1;
    }
    seL4_DebugPutString("SeLinOS romfsd: immutable release record service online.\n");
    seL4_DebugPutString("SeLinOS romfsd: immutable release and version records service online.\n");
    seL4_DebugPutString("SeLinOS romfsd: embedded newc CPIO multi-file parser ready.\n");
    seL4_DebugPutString("SeLinOS vfs M0: one fixed volatile state record is server-owned and non-persistent.\n");
    for (;;) {
        seL4_MessageInfo_t request = seL4_Recv(SELINOS_ROMFS_ENDPOINT_SLOT, &badge);
        const seL4_Word length = seL4_MessageInfo_get_length(request);
        const seL4_Word operation = length > 0u ? seL4_GetMR(0) : 0u;

        if (seL4_MessageInfo_get_label(request) != 0u || length < 1u) {
            reply_status(SELINOS_ROMFS_STATUS_EINVAL, 0u);
            continue;
        }
        if (operation == SELINOS_ROMFS_OP_OPEN) {
            const struct selinos_romfs_file *file;
            struct selinos_romfs_entry entry;

            if (length != 2u) {
                reply_status(SELINOS_ROMFS_STATUS_ENOENT, 0u);
            } else if (seL4_GetMR(1) == SELINOS_ROMFS_FILE_VOLATILE_STATE) {
                selinos_volatile_state = SELINOS_ROMFS_VOLATILE_STATE_INITIAL;
                selinos_volatile_state_open = 1;
                reply_status(SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_FD_VOLATILE_STATE);
            } else if ((file = file_by_id(seL4_GetMR(1))) == NULL ||
                       !cpio_lookup_regular(file->pathname, &entry) ||
                       entry.size != file->expected_data_length) {
                reply_status(SELINOS_ROMFS_STATUS_ENOENT, 0u);
            } else {
                reply_status(SELINOS_ROMFS_STATUS_OK, file->fd);
            }
            continue;
        }
        if (operation == SELINOS_ROMFS_OP_OPEN_PATH) {
            const struct selinos_romfs_file *file;
            struct selinos_romfs_entry entry;

            if (length == SELINOS_ROMFS_OPEN_PATH_REQUEST_WORDS &&
                seL4_GetMR(1) == 14u && seL4_GetMR(2) == 0x2f73656c696e6f73ull &&
                seL4_GetMR(3) == 0x2d73746174650000ull) { /* "/selinos-state" */
                selinos_volatile_state = SELINOS_ROMFS_VOLATILE_STATE_INITIAL;
                selinos_volatile_state_open = 1;
                reply_status(SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_FD_VOLATILE_STATE);
            } else if (length != SELINOS_ROMFS_OPEN_PATH_REQUEST_WORDS ||
                       (file = file_by_packed_path(seL4_GetMR(1), seL4_GetMR(2), seL4_GetMR(3))) == NULL ||
                       !cpio_lookup_regular(file->pathname, &entry) ||
                       entry.size != file->expected_data_length) {
                reply_status(SELINOS_ROMFS_STATUS_ENOENT, 0u);
            } else {
                reply_status(SELINOS_ROMFS_STATUS_OK, file->fd);
            }
            continue;
        }
        if (operation == SELINOS_ROMFS_OP_READ) {
            const struct selinos_romfs_file *file;
            struct selinos_romfs_entry entry;
            seL4_Word payload;

            if (length == 3u && seL4_GetMR(1) == SELINOS_ROMFS_FD_VOLATILE_STATE &&
                seL4_GetMR(2) == 0u && selinos_volatile_state_open) {
                seL4_SetMR(0, SELINOS_ROMFS_STATUS_OK);
                seL4_SetMR(1, SELINOS_ROMFS_VOLATILE_STATE_LENGTH);
                seL4_SetMR(2, selinos_volatile_state);
                seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 3u));
            } else if (length != 3u || seL4_GetMR(2) != 0u ||
                       (file = file_by_fd(seL4_GetMR(1))) == NULL ||
                       !cpio_lookup_regular(file->pathname, &entry) ||
                       entry.size != file->expected_data_length || !entry_payload_word(&entry, &payload)) {
                reply_status(SELINOS_ROMFS_STATUS_EBADF, 0u);
            } else {
                seL4_SetMR(0, SELINOS_ROMFS_STATUS_OK);
                seL4_SetMR(1, file->reported_length);
                seL4_SetMR(2, payload);
                seL4_Reply(seL4_MessageInfo_new(0u, 0u, 0u, 3u));
            }
            continue;
        }
        if (operation == SELINOS_ROMFS_OP_WRITE) {
            if (length != 3u || seL4_GetMR(1) != SELINOS_ROMFS_FD_VOLATILE_STATE ||
                !selinos_volatile_state_open) {
                reply_status(SELINOS_ROMFS_STATUS_EBADF, 0u);
            } else {
                selinos_volatile_state = seL4_GetMR(2);
                reply_status(SELINOS_ROMFS_STATUS_OK, SELINOS_ROMFS_VOLATILE_STATE_LENGTH);
            }
            continue;
        }
        if (operation == SELINOS_ROMFS_OP_CLOSE) {
            if (length != 2u) {
                reply_status(SELINOS_ROMFS_STATUS_EBADF, 0u);
            } else if (seL4_GetMR(1) == SELINOS_ROMFS_FD_VOLATILE_STATE) {
                if (!selinos_volatile_state_open) {
                    reply_status(SELINOS_ROMFS_STATUS_EBADF, 0u);
                } else {
                    selinos_volatile_state_open = 0;
                    reply_status(SELINOS_ROMFS_STATUS_OK, 0u);
                }
            } else if (file_by_fd(seL4_GetMR(1)) == NULL) {
                reply_status(SELINOS_ROMFS_STATUS_EBADF, 0u);
            } else {
                reply_status(SELINOS_ROMFS_STATUS_OK, 0u);
            }
            continue;
        }
        reply_status(SELINOS_ROMFS_STATUS_EINVAL, 0u);
    }
}
