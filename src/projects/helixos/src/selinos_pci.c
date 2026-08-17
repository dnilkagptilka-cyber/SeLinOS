// SPDX-License-Identifier: MIT
#include <sel4/sel4.h>
#include <selinos-root/gen_config.h>
#include <vka/vka.h>
#include <vka/object.h>

#include "selinos_pci.h"

#define PCI_CONFIG_ADDRESS_PORT 0xcf8u
#define PCI_CONFIG_DATA_PORT    0xcfcu
#define PCI_CONFIG_LAST_PORT    0xcffu
#define PCI_CONFIG_ENABLE       0x80000000u
#define PCI_VENDOR_DEVICE_OFF   0x00u
#define PCI_COMMAND_STATUS_OFF  0x04u
#define PCI_BAR0_OFF            0x10u
#define PCI_BAR_COUNT            6u
#define PCI_INTERRUPT_LINE_OFF  0x3cu
#define PCI_BAR_MEMORY_MASK     0xfffffff0u
#define PCI_BAR_MEMORY_TYPE_MASK 0x00000006u
#define PCI_BAR_MEMORY_TYPE_32  0x00000000u
#define PCI_COMMAND_MEMORY      0x0002u
#define PCI_COMMAND_MASTER      0x0004u
#define PCI_32BIT_ADDRESS_LIMIT 0x100000000ull

#if CONFIG_SELINOS_ROOT_PCI_BAR_ALLOCATOR_PROBE
static void bar_probe_debug_bar_value(seL4_Uint32 value)
{
    static const char hex[] = "0123456789abcdef";
    char message[] = "SeLinOS PCI M0: BAR probe original BAR0=0x00000000.\n";
    for (unsigned int index = 0u; index < 8u; ++index) {
        message[(sizeof("SeLinOS PCI M0: BAR probe original BAR0=0x") - 1u) + index] =
            hex[(value >> (28u - 4u * index)) & 0xfu];
    }
    seL4_DebugPutString(message);
}

static void bar_probe_debug_assigned_bar(seL4_Uint32 value)
{
    static const char hex[] = "0123456789abcdef";
    char message[] = "SeLinOS PCI M0: BAR probe assigned BAR0=0x00000000.\n";
    for (unsigned int index = 0u; index < 8u; ++index) {
        message[(sizeof("SeLinOS PCI M0: BAR probe assigned BAR0=0x") - 1u) + index] =
            hex[(value >> (28u - 4u * index)) & 0xfu];
    }
    seL4_DebugPutString(message);
}

static void bar_probe_debug_stage(unsigned int stage)
{
    switch (stage) {
    case 1u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe could not acquire root config IOPort authority.\n");
        break;
    case 2u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe could not read command/BAR configuration.\n");
        break;
    case 3u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe could not quiesce command memory/master bits.\n");
        break;
    case 4u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe rejected an already assigned BAR.\n");
        break;
    case 5u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe sizing write/read/restore transaction failed.\n");
        break;
    case 6u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe rejected unsupported BAR type or size.\n");
        break;
    case 7u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe rejected proposed MMIO policy range.\n");
        break;
    case 8u:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe programming read-back failed.\n");
        break;
    default:
        seL4_DebugPutString("SeLinOS PCI M0: BAR probe failed before a classified stage.\n");
        break;
    }
}
#else
static void bar_probe_debug_bar_value(seL4_Uint32 value)
{
    (void)value;
}

static void bar_probe_debug_assigned_bar(seL4_Uint32 value)
{
    (void)value;
}

static void bar_probe_debug_stage(unsigned int stage)
{
    (void)stage;
}
#endif

static seL4_Uint32 config_address(unsigned int device, unsigned int function,
                                  unsigned int offset)
{
    return PCI_CONFIG_ENABLE | ((seL4_Uint32)device << 11) |
           ((seL4_Uint32)function << 8) | ((seL4_Uint32)offset & 0xfcu);
}

static bool read_config_dword(seL4_X86_IOPort ioport, unsigned int device,
                              unsigned int function, unsigned int offset,
                              seL4_Uint32 *value)
{
    seL4_Uint32 address = config_address(device, function, offset);
    if (seL4_X86_IOPort_Out32(ioport, PCI_CONFIG_ADDRESS_PORT, address) != seL4_NoError) {
        return false;
    }

    seL4_X86_IOPort_In32_t result = seL4_X86_IOPort_In32(ioport, PCI_CONFIG_DATA_PORT);
    if (result.error != seL4_NoError) {
        return false;
    }
    *value = result.result;
    return true;
}

static bool write_config_dword(seL4_X86_IOPort ioport, unsigned int device,
                               unsigned int function, unsigned int offset,
                               seL4_Uint32 value)
{
    if (seL4_X86_IOPort_Out32(ioport, PCI_CONFIG_ADDRESS_PORT,
                              config_address(device, function, offset)) != seL4_NoError) {
        return false;
    }
    return seL4_X86_IOPort_Out32(ioport, PCI_CONFIG_DATA_PORT, value) == seL4_NoError;
}

static bool read_config_word(seL4_X86_IOPort ioport, unsigned int device,
                             unsigned int function, unsigned int offset,
                             seL4_Uint16 *value)
{
    if (value == NULL ||
        seL4_X86_IOPort_Out32(ioport, PCI_CONFIG_ADDRESS_PORT,
                              config_address(device, function, offset)) != seL4_NoError) {
        return false;
    }
    seL4_X86_IOPort_In16_t result =
        seL4_X86_IOPort_In16(ioport, PCI_CONFIG_DATA_PORT + (offset & 0x2u));
    if (result.error != seL4_NoError) {
        return false;
    }
    *value = result.result;
    return true;
}

static bool write_config_word(seL4_X86_IOPort ioport, unsigned int device,
                              unsigned int function, unsigned int offset,
                              seL4_Uint16 value)
{
    if (seL4_X86_IOPort_Out32(ioport, PCI_CONFIG_ADDRESS_PORT,
                              config_address(device, function, offset)) != seL4_NoError) {
        return false;
    }
    return seL4_X86_IOPort_Out16(ioport, PCI_CONFIG_DATA_PORT + (offset & 0x2u), value) ==
           seL4_NoError;
}

static void release_config_ioport(vka_t *vka, seL4_CPtr ioport_slot)
{
    if (ioport_slot != 0u) {
        (void)seL4_CNode_Delete(seL4_CapInitThreadCNode, ioport_slot, seL4_WordBits);
        vka_cspace_free(vka, ioport_slot);
    }
}

static bool discover_qemu_edu_function(seL4_X86_IOPort ioport, unsigned int device,
                                       struct selinos_qemu_edu_resource *resource)
{
    seL4_Uint32 id = 0xffffffffu;
    if (!read_config_dword(ioport, device, 0u, PCI_VENDOR_DEVICE_OFF, &id)) {
        return false;
    }
    const seL4_Uint16 vendor = (seL4_Uint16)(id & 0xffffu);
    const seL4_Uint16 product = (seL4_Uint16)(id >> 16);
    if (vendor != SELINOS_QEMU_EDU_VENDOR_ID || product != SELINOS_QEMU_EDU_DEVICE_ID) {
        return false;
    }

    seL4_Uint32 bar0 = 0u;
    seL4_Uint32 irq_line = 0u;
    seL4_Uint32 command_status = 0u;
    if (!read_config_dword(ioport, device, 0u, PCI_BAR0_OFF, &bar0) ||
        !read_config_dword(ioport, device, 0u, PCI_INTERRUPT_LINE_OFF, &irq_line) ||
        !read_config_dword(ioport, device, 0u, PCI_COMMAND_STATUS_OFF, &command_status)) {
        return false;
    }
    /* QEMU edu exposes one 32-bit memory BAR0. I/O and 64-bit BARs remain
     * outside the declared profile. The config-space cap stays in rootd. */
    if ((bar0 & 0x1u) != 0u || (bar0 & 0x6u) == 0x4u ||
        (irq_line & 0xffu) == 0xffu || (bar0 & PCI_BAR_MEMORY_MASK) == 0u) {
        return false;
    }
    command_status |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    if (!write_config_dword(ioport, device, 0u, PCI_COMMAND_STATUS_OFF,
                            command_status)) {
        return false;
    }

    resource->bar0_paddr = (uintptr_t)(bar0 & PCI_BAR_MEMORY_MASK);
    resource->bar0_size = SELINOS_QEMU_EDU_BAR0_SIZE;
    resource->irq_line = (unsigned int)(irq_line & 0xffu);
    return true;
}

size_t selinos_pci_enumerate_qemu_edu(vka_t *vka,
                                      struct selinos_qemu_edu_resource *resources,
                                      size_t resource_capacity)
{
    if (vka == NULL || (resource_capacity != 0u && resources == NULL)) {
        return 0u;
    }

    seL4_CPtr ioport_slot = 0;
    if (vka_cspace_alloc(vka, &ioport_slot) != 0) {
        return 0u;
    }
    if (seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        vka_cspace_free(vka, ioport_slot);
        return 0u;
    }

    size_t discovered = 0u;
    for (unsigned int device = 0u; device < 32u; ++device) {
        struct selinos_qemu_edu_resource candidate = {0};
        if (!discover_qemu_edu_function(ioport_slot, device, &candidate)) {
            continue;
        }
        if (discovered < resource_capacity) {
            resources[discovered] = candidate;
        }
        ++discovered;
    }

    /* Retain this narrowly scoped IOPort cap in rootd for future pcid. */
    return discovered;
}

bool selinos_pci_find_qemu_edu(vka_t *vka, struct selinos_qemu_edu_resource *resource)
{
    return resource != NULL && selinos_pci_enumerate_qemu_edu(vka, resource, 1u) != 0u;
}

bool selinos_pci_assign_unconfigured_bar32(vka_t *vka, unsigned int device,
                                           unsigned int function,
                                           unsigned int bar_index,
                                           uintptr_t range_start,
                                           uintptr_t range_end,
                                           struct selinos_pci_bar32_resource *resource)
{
    seL4_CPtr ioport_slot = 0u;
    seL4_Uint16 original_command = 0u;
    seL4_Uint32 original_bar = 0u;
    seL4_Uint32 sizing_mask = 0u;
    seL4_Uint32 programmed_bar = 0u;
    seL4_Uint32 read_back_bar = 0u;
    uint64_t size64;
    uint64_t aligned_start;
    bool bar_touched = false;
    bool command_changed = false;
    bool success = false;
    unsigned int failure_stage = 0u;

    if (vka == NULL || resource == NULL || device >= 32u || function >= 8u ||
        bar_index >= PCI_BAR_COUNT || range_start >= range_end ||
        (uint64_t)range_end > PCI_32BIT_ADDRESS_LIMIT) {
        bar_probe_debug_stage(failure_stage);
        return false;
    }
    if (vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        bar_probe_debug_stage(1u);
        return false;
    }

    const unsigned int bar_offset = PCI_BAR0_OFF + 4u * bar_index;
    if (!read_config_word(ioport_slot, device, function, PCI_COMMAND_STATUS_OFF,
                          &original_command) ||
        !read_config_dword(ioport_slot, device, function, bar_offset, &original_bar)) {
        failure_stage = 2u;
        goto out;
    }

    /* BAR probing is performed with only the affected command bits quiesced,
     * avoiding a dword write that could inadvertently acknowledge status bits. */
    if (!write_config_word(ioport_slot, device, function, PCI_COMMAND_STATUS_OFF,
                           original_command & ~(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER))) {
        failure_stage = 3u;
        goto out;
    }
    command_changed = true;

    /* Never relocate a BAR programmed by firmware or another owner. Direct
     * QEMU boot can expose an unassigned BAR either as address zero or with
     * every address bit set, so both representations are admitted only to the
     * sizing path below. */
    const seL4_Uint32 original_address = original_bar & PCI_BAR_MEMORY_MASK;
    if (original_address != 0u && original_address != PCI_BAR_MEMORY_MASK) {
        bar_probe_debug_bar_value(original_bar);
        failure_stage = 4u;
        goto out;
    }

    if (!write_config_dword(ioport_slot, device, function, bar_offset, 0xffffffffu) ||
        !read_config_dword(ioport_slot, device, function, bar_offset, &sizing_mask) ||
        !write_config_dword(ioport_slot, device, function, bar_offset, original_bar)) {
        failure_stage = 5u;
        goto out;
    }
    bar_touched = true;

    /* The sizing response—not the possibly all-ones unassigned value—is the
     * authoritative source for BAR space/type attributes. M0 excludes I/O,
     * 64-bit and legacy below-1MiB BARs. */
    if ((sizing_mask & 0x1u) != 0u ||
        (sizing_mask & PCI_BAR_MEMORY_TYPE_MASK) != PCI_BAR_MEMORY_TYPE_32) {
        failure_stage = 6u;
        goto out;
    }
    sizing_mask &= PCI_BAR_MEMORY_MASK;
    if (sizing_mask == 0u) {
        failure_stage = 6u;
        goto out;
    }
    size64 = ((~(uint64_t)sizing_mask) & 0xffffffffull) + 1ull;
    if (size64 == 0u || (size64 & (size64 - 1ull)) != 0ull ||
        (uint64_t)range_start > UINT64_MAX - (size64 - 1ull)) {
        failure_stage = 6u;
        goto out;
    }
    aligned_start = ((uint64_t)range_start + size64 - 1ull) & ~(size64 - 1ull);
    if (aligned_start < (uint64_t)range_start ||
        aligned_start > (uint64_t)range_end ||
        size64 > (uint64_t)range_end - aligned_start ||
        aligned_start + size64 > PCI_32BIT_ADDRESS_LIMIT) {
        failure_stage = 7u;
        goto out;
    }

    programmed_bar = (seL4_Uint32)aligned_start | (sizing_mask & 0x0fu);
    if (!write_config_dword(ioport_slot, device, function, bar_offset, programmed_bar) ||
        !read_config_dword(ioport_slot, device, function, bar_offset, &read_back_bar) ||
        read_back_bar != programmed_bar) {
        failure_stage = 8u;
        goto out;
    }

    resource->paddr = (uintptr_t)aligned_start;
    resource->size = (size_t)size64;
    resource->bar_index = bar_index;
    bar_probe_debug_assigned_bar(programmed_bar & PCI_BAR_MEMORY_MASK);
    success = true;

out:
    if (!success) {
        bar_probe_debug_stage(failure_stage);
    }
    if (!success && bar_touched) {
        (void)write_config_dword(ioport_slot, device, function, bar_offset, original_bar);
    }
    if (command_changed) {
        (void)write_config_word(ioport_slot, device, function, PCI_COMMAND_STATUS_OFF,
                                original_command);
    }
    release_config_ioport(vka, ioport_slot);
    return success;
}

bool selinos_pci_assign_qemu_e1000_bar0(vka_t *vka, uintptr_t range_start,
                                        uintptr_t range_end,
                                        struct selinos_pci_bar32_resource *resource)
{
    seL4_CPtr ioport_slot = 0u;
    unsigned int matched_device = 0u;
    bool matched = false;

    if (vka == NULL || resource == NULL ||
        vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    for (unsigned int device = 0u; device < 32u; ++device) {
        seL4_Uint32 id = 0xffffffffu;
        if (!read_config_dword(ioport_slot, device, 0u, PCI_VENDOR_DEVICE_OFF, &id)) {
            continue;
        }
        if ((seL4_Uint16)(id & 0xffffu) == SELINOS_QEMU_E1000_VENDOR_ID &&
            (seL4_Uint16)(id >> 16) == SELINOS_QEMU_E1000_DEVICE_ID) {
            matched_device = device;
            matched = true;
            break;
        }
    }
#if CONFIG_SELINOS_ROOT_PCI_BAR_ALLOCATOR_FORCE_UNASSIGNED_E1000
    /* Deliberately destructive QEMU proof-fixture operation. This is not a
     * resource manager policy: it exists only to turn an already firmware-set
     * e1000 BAR into the direct-boot unassigned input expected by M0 sizing. */
    if (matched &&
        !write_config_dword(ioport_slot, matched_device, 0u, PCI_BAR0_OFF, 0u)) {
        release_config_ioport(vka, ioport_slot);
        return false;
    }
    if (matched) {
        seL4_DebugPutString("SeLinOS PCI M0: destructive QEMU fixture cleared e1000 BAR0 before allocator proof.\n");
    }
#endif
    release_config_ioport(vka, ioport_slot);
    if (!matched) {
        return false;
    }
    return selinos_pci_assign_unconfigured_bar32(vka, matched_device, 0u, 0u,
                                                 range_start, range_end, resource);
}


bool selinos_pci_find_qemu_virtio_blk(vka_t *vka,
                                      struct selinos_qemu_virtio_blk_identity *identity)
{
    seL4_CPtr ioport_slot = 0u;

    if (vka == NULL || identity == NULL || vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    for (unsigned int device = 0u; device < 32u; ++device) {
        seL4_Uint32 raw_identity = 0xffffffffu;
        if (!read_config_dword(ioport_slot, device, 0u, PCI_VENDOR_DEVICE_OFF,
                               &raw_identity)) {
            continue;
        }
        const seL4_Uint16 vendor = (seL4_Uint16)(raw_identity & 0xffffu);
        const seL4_Uint16 product = (seL4_Uint16)(raw_identity >> 16);
        if (vendor != SELINOS_QEMU_VIRTIO_VENDOR_ID ||
            (product != SELINOS_QEMU_VIRTIO_BLK_LEGACY_DEVICE_ID &&
             product != SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID)) {
            continue;
        }
        identity->device = device;
        identity->function = 0u;
        identity->vendor_id = vendor;
        identity->device_id = product;
        release_config_ioport(vka, ioport_slot);
        return true;
    }

    release_config_ioport(vka, ioport_slot);
    return false;
}


#define PCI_STATUS_CAPABILITIES 0x0010u
#define PCI_HEADER_TYPE_OFF 0x0cu
#define PCI_CAPABILITY_LIST_OFF 0x34u
#define PCI_CAPABILITY_FIRST 0x40u
#define PCI_CAPABILITY_LAST 0xf0u
#define PCI_CAPABILITY_LIMIT 48u
#define PCI_CAP_ID_VENDOR_SPECIFIC 0x09u
#define VIRTIO_PCI_CAP_COMMON_CFG 0x01u
#define VIRTIO_PCI_CAP_NOTIFY_CFG 0x02u
#define VIRTIO_COMMON_CFG_MIN_LENGTH 0x38u
#define VIRTIO_NOTIFY_CFG_MIN_LENGTH 0x14u
#define VIRTIO_NOTIFY_CFG_MULTIPLIER_OFF 0x10u

static bool read_config_byte(seL4_X86_IOPort ioport, unsigned int device,
                             unsigned int function, unsigned int offset,
                             seL4_Uint8 *value)
{
    seL4_Uint32 word = 0u;
    if (value == NULL || !read_config_dword(ioport, device, function,
                                             offset & 0xfcu, &word)) {
        return false;
    }
    *value = (seL4_Uint8)(word >> (8u * (offset & 0x3u)));
    return true;
}

bool selinos_pci_inspect_qemu_virtio_blk_common_capability(
    vka_t *vka, struct selinos_qemu_virtio_common_capability *capability)
{
    seL4_CPtr ioport_slot = 0u;
    unsigned int matched_device = 0u;
    seL4_Uint16 status = 0u;
    seL4_Uint32 header = 0u;
    seL4_Uint8 current = 0u;
    bool matched = false;

    if (vka == NULL || capability == NULL || vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    for (unsigned int device = 0u; device < 32u; ++device) {
        seL4_Uint32 raw_identity = 0xffffffffu;
        if (!read_config_dword(ioport_slot, device, 0u, PCI_VENDOR_DEVICE_OFF,
                               &raw_identity)) {
            continue;
        }
        if ((seL4_Uint16)(raw_identity & 0xffffu) == SELINOS_QEMU_VIRTIO_VENDOR_ID &&
            (seL4_Uint16)(raw_identity >> 16) == SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID) {
            matched_device = device;
            matched = true;
            break;
        }
    }
    if (!matched ||
        !read_config_word(ioport_slot, matched_device, 0u, PCI_COMMAND_STATUS_OFF + 2u, &status) ||
        !read_config_dword(ioport_slot, matched_device, 0u, PCI_HEADER_TYPE_OFF, &header) ||
        (status & PCI_STATUS_CAPABILITIES) == 0u ||
        ((header >> 16) & 0x7fu) != 0u ||
        !read_config_byte(ioport_slot, matched_device, 0u, PCI_CAPABILITY_LIST_OFF, &current)) {
        release_config_ioport(vka, ioport_slot);
        return false;
    }

    for (unsigned int seen = 0u; seen < PCI_CAPABILITY_LIMIT; ++seen) {
        seL4_Uint8 cap_id = 0u;
        seL4_Uint8 next = 0u;
        seL4_Uint8 cfg_type = 0u;
        seL4_Uint8 bar = 0u;
        seL4_Uint32 offset = 0u;
        seL4_Uint32 length = 0u;
        if (current < PCI_CAPABILITY_FIRST || current > PCI_CAPABILITY_LAST ||
            (current & 0x3u) != 0u ||
            !read_config_byte(ioport_slot, matched_device, 0u, current, &cap_id) ||
            !read_config_byte(ioport_slot, matched_device, 0u, current + 1u, &next)) {
            release_config_ioport(vka, ioport_slot);
            return false;
        }
        if (cap_id == PCI_CAP_ID_VENDOR_SPECIFIC &&
            read_config_byte(ioport_slot, matched_device, 0u, current + 3u, &cfg_type) &&
            read_config_byte(ioport_slot, matched_device, 0u, current + 4u, &bar) &&
            read_config_dword(ioport_slot, matched_device, 0u, current + 8u, &offset) &&
            read_config_dword(ioport_slot, matched_device, 0u, current + 12u, &length) &&
            cfg_type == VIRTIO_PCI_CAP_COMMON_CFG && bar < PCI_BAR_COUNT &&
            (offset & 0x3u) == 0u && length >= VIRTIO_COMMON_CFG_MIN_LENGTH &&
            (length & 0x3u) == 0u) {
            capability->identity.device = matched_device;
            capability->identity.function = 0u;
            capability->identity.vendor_id = SELINOS_QEMU_VIRTIO_VENDOR_ID;
            capability->identity.device_id = SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID;
            capability->capability_offset = current;
            capability->bar_index = bar;
            capability->offset = offset;
            capability->length = length;
            release_config_ioport(vka, ioport_slot);
            return true;
        }
        if (next == 0u) {
            break;
        }
        current = next;
    }

    release_config_ioport(vka, ioport_slot);
    return false;
}

bool selinos_pci_inspect_qemu_virtio_blk_notify_capability(
    vka_t *vka, struct selinos_qemu_virtio_notify_capability *capability)
{
    seL4_CPtr ioport_slot = 0u;
    unsigned int matched_device = 0u;
    seL4_Uint16 status = 0u;
    seL4_Uint32 header = 0u;
    seL4_Uint8 current = 0u;
    bool matched = false;

    if (vka == NULL || capability == NULL || vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }
    for (unsigned int device = 0u; device < 32u; ++device) {
        seL4_Uint32 raw_identity = 0xffffffffu;
        if (!read_config_dword(ioport_slot, device, 0u, PCI_VENDOR_DEVICE_OFF,
                               &raw_identity)) {
            continue;
        }
        if ((seL4_Uint16)(raw_identity & 0xffffu) == SELINOS_QEMU_VIRTIO_VENDOR_ID &&
            (seL4_Uint16)(raw_identity >> 16) == SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID) {
            matched_device = device;
            matched = true;
            break;
        }
    }
    if (!matched ||
        !read_config_word(ioport_slot, matched_device, 0u, PCI_COMMAND_STATUS_OFF + 2u, &status) ||
        !read_config_dword(ioport_slot, matched_device, 0u, PCI_HEADER_TYPE_OFF, &header) ||
        (status & PCI_STATUS_CAPABILITIES) == 0u ||
        ((header >> 16) & 0x7fu) != 0u ||
        !read_config_byte(ioport_slot, matched_device, 0u, PCI_CAPABILITY_LIST_OFF, &current)) {
        release_config_ioport(vka, ioport_slot);
        return false;
    }
    for (unsigned int seen = 0u; seen < PCI_CAPABILITY_LIMIT; ++seen) {
        seL4_Uint8 cap_id = 0u;
        seL4_Uint8 next = 0u;
        seL4_Uint8 cap_length = 0u;
        seL4_Uint8 cfg_type = 0u;
        seL4_Uint8 bar = 0u;
        seL4_Uint32 offset = 0u;
        seL4_Uint32 length = 0u;
        seL4_Uint32 multiplier = 0u;
        if (current < PCI_CAPABILITY_FIRST || current > PCI_CAPABILITY_LAST ||
            (current & 0x3u) != 0u ||
            !read_config_byte(ioport_slot, matched_device, 0u, current, &cap_id) ||
            !read_config_byte(ioport_slot, matched_device, 0u, current + 1u, &next) ||
            !read_config_byte(ioport_slot, matched_device, 0u, current + 2u, &cap_length)) {
            release_config_ioport(vka, ioport_slot);
            return false;
        }
        if (cap_id == PCI_CAP_ID_VENDOR_SPECIFIC &&
            cap_length >= VIRTIO_NOTIFY_CFG_MIN_LENGTH &&
            read_config_byte(ioport_slot, matched_device, 0u, current + 3u, &cfg_type) &&
            read_config_byte(ioport_slot, matched_device, 0u, current + 4u, &bar) &&
            read_config_dword(ioport_slot, matched_device, 0u, current + 8u, &offset) &&
            read_config_dword(ioport_slot, matched_device, 0u, current + 12u, &length) &&
            read_config_dword(ioport_slot, matched_device, 0u,
                              current + VIRTIO_NOTIFY_CFG_MULTIPLIER_OFF, &multiplier) &&
            cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG && bar < PCI_BAR_COUNT &&
            (offset & 0x3u) == 0u && length >= VIRTIO_NOTIFY_CFG_MIN_LENGTH &&
            (length & 0x3u) == 0u && multiplier != 0u) {
            capability->identity.device = matched_device;
            capability->identity.function = 0u;
            capability->identity.vendor_id = SELINOS_QEMU_VIRTIO_VENDOR_ID;
            capability->identity.device_id = SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID;
            capability->capability_offset = current;
            capability->bar_index = bar;
            capability->offset = offset;
            capability->length = length;
            capability->notify_off_multiplier = multiplier;
            release_config_ioport(vka, ioport_slot);
            return true;
        }
        if (next == 0u) {
            break;
        }
        current = next;
    }
    release_config_ioport(vka, ioport_slot);
    return false;
}

/* Modern virtio common configuration offsets used by the M2 observation.
 * The selector is read, never written: M2 captures only the current offered
 * feature window and does not negotiate or accept any feature bit. */
#define VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF 0x00u
#define VIRTIO_COMMON_DEVICE_FEATURE_OFF        0x04u

static bool virtio_common_range_is_sane(const struct selinos_qemu_virtio_common_range *range)
{
    const uintptr_t page_size = (uintptr_t)1u << seL4_PageBits;
    const size_t feature_end = VIRTIO_COMMON_DEVICE_FEATURE_OFF + sizeof(uint32_t);

    return range != NULL &&
           range->capability.identity.vendor_id == SELINOS_QEMU_VIRTIO_VENDOR_ID &&
           range->capability.identity.device_id == SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID &&
           range->capability.bar_index < PCI_BAR_COUNT &&
           range->bar_paddr != 0u && range->bar_size != 0u &&
           range->common_paddr >= range->bar_paddr &&
           range->common_paddr - range->bar_paddr == range->capability.offset &&
           range->capability.offset <= range->bar_size &&
           range->capability.length <= range->bar_size - range->capability.offset &&
           (range->page_paddr & (page_size - 1u)) == 0u &&
           range->common_paddr - range->page_paddr == range->page_offset &&
           range->page_offset <= page_size && feature_end <= page_size - range->page_offset;
}

static bool selinos_pci_validate_qemu_virtio_blk_common_range_32(
    vka_t *vka, const struct selinos_qemu_virtio_common_capability *capability,
    struct selinos_qemu_virtio_common_range *range)
{
    seL4_CPtr ioport_slot = 0u;
    seL4_Uint16 original_command = 0u;
    seL4_Uint32 original_bar = 0u;
    seL4_Uint32 sizing_mask = 0u;
    seL4_Uint32 restored_bar = 0u;
    uint64_t bar_size64;
    uintptr_t bar_paddr;
    uintptr_t common_paddr;
    const uintptr_t page_size = (uintptr_t)1u << seL4_PageBits;
    bool command_quiesced = false;
    bool bar_touched = false;
    bool success = false;

    if (vka == NULL || capability == NULL || range == NULL ||
        capability->identity.vendor_id != SELINOS_QEMU_VIRTIO_VENDOR_ID ||
        capability->identity.device_id != SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID ||
        capability->identity.device >= 32u || capability->identity.function >= 8u ||
        capability->bar_index >= PCI_BAR_COUNT || capability->length < VIRTIO_COMMON_CFG_MIN_LENGTH ||
        (capability->offset & 0x3u) != 0u || (capability->length & 0x3u) != 0u ||
        vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    const unsigned int bar_offset = PCI_BAR0_OFF + 4u * capability->bar_index;
    if (!read_config_word(ioport_slot, capability->identity.device,
                          capability->identity.function, PCI_COMMAND_STATUS_OFF,
                          &original_command) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset, &original_bar) ||
        (original_bar & 0x1u) != 0u ||
        (original_bar & PCI_BAR_MEMORY_TYPE_MASK) != PCI_BAR_MEMORY_TYPE_32 ||
        (original_bar & PCI_BAR_MEMORY_MASK) == 0u) {
        goto out;
    }

    /* Sizing is a root-only transaction. The two resource-enabling bits are
     * quiesced before the standard all-ones probe, then both configuration
     * registers are restored before any return. */
    if (!write_config_word(ioport_slot, capability->identity.device,
                           capability->identity.function, PCI_COMMAND_STATUS_OFF,
                           original_command & ~(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER))) {
        goto out;
    }
    command_quiesced = true;
    if (!write_config_dword(ioport_slot, capability->identity.device,
                            capability->identity.function, bar_offset, 0xffffffffu)) {
        goto out;
    }
    bar_touched = true;
    if (!read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset, &sizing_mask) ||
        !write_config_dword(ioport_slot, capability->identity.device,
                            capability->identity.function, bar_offset, original_bar) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset, &restored_bar) ||
        restored_bar != original_bar) {
        goto out;
    }

    if ((sizing_mask & 0x1u) != 0u ||
        (sizing_mask & PCI_BAR_MEMORY_TYPE_MASK) != PCI_BAR_MEMORY_TYPE_32) {
        goto out;
    }
    sizing_mask &= PCI_BAR_MEMORY_MASK;
    if (sizing_mask == 0u) {
        goto out;
    }
    bar_size64 = ((~(uint64_t)sizing_mask) & 0xffffffffull) + 1ull;
    if (bar_size64 == 0u || (bar_size64 & (bar_size64 - 1ull)) != 0ull ||
        bar_size64 > (uint64_t)SIZE_MAX) {
        goto out;
    }

    bar_paddr = (uintptr_t)(original_bar & PCI_BAR_MEMORY_MASK);
    if ((uint64_t)bar_paddr > UINT64_MAX - bar_size64 ||
        (uint64_t)capability->offset > bar_size64 ||
        (uint64_t)capability->length > bar_size64 - (uint64_t)capability->offset) {
        goto out;
    }
    common_paddr = bar_paddr + (uintptr_t)capability->offset;
    if (common_paddr < bar_paddr ||
        common_paddr > UINTPTR_MAX - (uintptr_t)VIRTIO_COMMON_DEVICE_FEATURE_OFF ||
        (common_paddr & 0x3u) != 0u) {
        goto out;
    }

    range->capability = *capability;
    range->bar_paddr = bar_paddr;
    range->bar_size = (size_t)bar_size64;
    range->common_paddr = common_paddr;
    range->page_paddr = common_paddr & ~(page_size - 1u);
    range->page_offset = (size_t)(common_paddr - range->page_paddr);
    success = virtio_common_range_is_sane(range);

out:
    if (bar_touched && !success) {
        (void)write_config_dword(ioport_slot, capability->identity.device,
                                 capability->identity.function, bar_offset, original_bar);
    }
    if (command_quiesced) {
        (void)write_config_word(ioport_slot, capability->identity.device,
                                capability->identity.function, PCI_COMMAND_STATUS_OFF,
                                original_command);
    }
    release_config_ioport(vka, ioport_slot);
    return success;
}

bool selinos_pci_observe_qemu_virtio_blk_features(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_feature_observation *observation)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_only = seL4_CapRights_new(0, 0, 1, 0);
    vka_object_t frame = {0};
    reservation_t reservation = { .res = NULL };
    void *mapping = NULL;
    bool frame_allocated = false;
    bool mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || observation == NULL ||
        !virtio_common_range_is_sane(range)) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &frame) != 0) {
        return false;
    }
    frame_allocated = true;

    reservation = vspace_reserve_range_aligned(root_vspace, page_size, seL4_PageBits,
                                               read_only, 0, &mapping);
    if (reservation.res == NULL || mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &frame.cptr, NULL, mapping, 1u,
                                  seL4_PageBits, reservation) != 0) {
        goto out;
    }
    mapped = true;

    volatile const uint32_t *common =
        (volatile const uint32_t *)((const char *)mapping + range->page_offset);
    observation->range = *range;
    observation->device_feature_select =
        common[VIRTIO_COMMON_DEVICE_FEATURE_SELECT_OFF / sizeof(uint32_t)];
    observation->device_feature = common[VIRTIO_COMMON_DEVICE_FEATURE_OFF / sizeof(uint32_t)];
    success = true;

out:
    if (mapped) {
        vspace_unmap_pages(root_vspace, mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    }
    if (reservation.res != NULL) {
        vspace_free_reservation(root_vspace, reservation);
    }
    if (frame_allocated) {
        vka_free_object(vka, &frame);
    }
    return success;
}

static bool selinos_pci_validate_qemu_virtio_blk_common_range_64(
    vka_t *vka, const struct selinos_qemu_virtio_common_capability *capability,
    struct selinos_qemu_virtio_common_range *range)
{
    seL4_CPtr ioport_slot = 0u;
    seL4_Uint16 original_command = 0u;
    seL4_Uint32 original_bar_low = 0u;
    seL4_Uint32 original_bar_high = 0u;
    seL4_Uint32 sizing_low = 0u;
    seL4_Uint32 sizing_high = 0u;
    seL4_Uint32 restored_bar_low = 0u;
    seL4_Uint32 restored_bar_high = 0u;
    uint64_t bar_base64;
    uint64_t bar_mask64;
    uint64_t bar_size64;
    uint64_t common_paddr64;
    const uintptr_t page_size = (uintptr_t)1u << seL4_PageBits;
    bool command_quiesced = false;
    bool bar_touched = false;
    bool success = false;

    if (vka == NULL || capability == NULL || range == NULL ||
        capability->identity.vendor_id != SELINOS_QEMU_VIRTIO_VENDOR_ID ||
        capability->identity.device_id != SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID ||
        capability->identity.device >= 32u || capability->identity.function >= 8u ||
        capability->bar_index >= PCI_BAR_COUNT - 1u ||
        capability->length < VIRTIO_COMMON_CFG_MIN_LENGTH ||
        (capability->offset & 0x3u) != 0u || (capability->length & 0x3u) != 0u ||
        vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    const unsigned int bar_offset = PCI_BAR0_OFF + 4u * capability->bar_index;
    if (!read_config_word(ioport_slot, capability->identity.device,
                          capability->identity.function, PCI_COMMAND_STATUS_OFF,
                          &original_command) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset, &original_bar_low) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset + 4u,
                           &original_bar_high) ||
        (original_bar_low & 0x1u) != 0u ||
        (original_bar_low & PCI_BAR_MEMORY_TYPE_MASK) != 0x4u) {
        goto out;
    }

    if (!write_config_word(ioport_slot, capability->identity.device,
                           capability->identity.function, PCI_COMMAND_STATUS_OFF,
                           original_command & ~(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER))) {
        goto out;
    }
    command_quiesced = true;

    /* A 64-bit BAR consumes this and the following dword. Both sizing words
     * are restored before command decoding is restored, including on failure. */
    if (!write_config_dword(ioport_slot, capability->identity.device,
                            capability->identity.function, bar_offset, 0xffffffffu)) {
        goto out;
    }
    bar_touched = true;
    if (!write_config_dword(ioport_slot, capability->identity.device,
                            capability->identity.function, bar_offset + 4u,
                            0xffffffffu) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset, &sizing_low) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset + 4u,
                           &sizing_high) ||
        !write_config_dword(ioport_slot, capability->identity.device,
                            capability->identity.function, bar_offset + 4u,
                            original_bar_high) ||
        !write_config_dword(ioport_slot, capability->identity.device,
                            capability->identity.function, bar_offset,
                            original_bar_low) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset,
                           &restored_bar_low) ||
        !read_config_dword(ioport_slot, capability->identity.device,
                           capability->identity.function, bar_offset + 4u,
                           &restored_bar_high) ||
        restored_bar_low != original_bar_low || restored_bar_high != original_bar_high) {
        goto out;
    }

    bar_mask64 = ((uint64_t)sizing_high << 32) |
                 ((uint64_t)sizing_low & (uint64_t)PCI_BAR_MEMORY_MASK);
    if (bar_mask64 == 0u) {
        goto out;
    }
    bar_size64 = (~bar_mask64) + 1u;
    if (bar_size64 == 0u || (bar_size64 & (bar_size64 - 1u)) != 0u ||
        bar_size64 > (uint64_t)SIZE_MAX) {
        goto out;
    }

    bar_base64 = ((uint64_t)original_bar_high << 32) |
                 ((uint64_t)original_bar_low & (uint64_t)PCI_BAR_MEMORY_MASK);
    if (bar_base64 == 0u || bar_base64 > (uint64_t)UINTPTR_MAX ||
        (uint64_t)capability->offset > bar_size64 ||
        (uint64_t)capability->length > bar_size64 - (uint64_t)capability->offset ||
        bar_base64 > UINT64_MAX - (uint64_t)capability->offset) {
        goto out;
    }
    common_paddr64 = bar_base64 + (uint64_t)capability->offset;
    if (common_paddr64 > (uint64_t)UINTPTR_MAX ||
        common_paddr64 > UINT64_MAX - (uint64_t)VIRTIO_COMMON_DEVICE_FEATURE_OFF ||
        (common_paddr64 & 0x3u) != 0u) {
        goto out;
    }

    range->capability = *capability;
    range->bar_paddr = (uintptr_t)bar_base64;
    range->bar_size = (size_t)bar_size64;
    range->common_paddr = (uintptr_t)common_paddr64;
    range->page_paddr = range->common_paddr & ~(page_size - 1u);
    range->page_offset = (size_t)(range->common_paddr - range->page_paddr);
    success = virtio_common_range_is_sane(range);

out:
    if (bar_touched) {
        (void)write_config_dword(ioport_slot, capability->identity.device,
                                 capability->identity.function, bar_offset + 4u,
                                 original_bar_high);
        (void)write_config_dword(ioport_slot, capability->identity.device,
                                 capability->identity.function, bar_offset,
                                 original_bar_low);
    }
    if (command_quiesced) {
        (void)write_config_word(ioport_slot, capability->identity.device,
                                capability->identity.function, PCI_COMMAND_STATUS_OFF,
                                original_command);
    }
    release_config_ioport(vka, ioport_slot);
    return success;
}

bool selinos_pci_validate_qemu_virtio_blk_common_range(
    vka_t *vka, const struct selinos_qemu_virtio_common_capability *capability,
    struct selinos_qemu_virtio_common_range *range)
{
    seL4_CPtr ioport_slot = 0u;
    seL4_Uint32 bar = 0u;
    bool is_32_bit;
    bool is_64_bit;

    if (vka == NULL || capability == NULL || range == NULL ||
        capability->identity.device >= 32u || capability->identity.function >= 8u ||
        capability->bar_index >= PCI_BAR_COUNT || vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }
    const bool read_ok = read_config_dword(ioport_slot, capability->identity.device,
                                           capability->identity.function,
                                           PCI_BAR0_OFF + 4u * capability->bar_index, &bar);
    release_config_ioport(vka, ioport_slot);
    if (!read_ok || (bar & 0x1u) != 0u) {
        return false;
    }
    is_32_bit = (bar & PCI_BAR_MEMORY_TYPE_MASK) == PCI_BAR_MEMORY_TYPE_32;
    is_64_bit = (bar & PCI_BAR_MEMORY_TYPE_MASK) == 0x4u;
    if (is_32_bit) {
        return selinos_pci_validate_qemu_virtio_blk_common_range_32(vka, capability, range);
    }
    if (is_64_bit) {
        return selinos_pci_validate_qemu_virtio_blk_common_range_64(vka, capability, range);
    }
    return false;
}


bool selinos_pci_read_qemu_e1000_bar_range(
    vka_t *vka, const struct selinos_qemu_e1000_identity *identity,
    struct selinos_pci_bar32_resource *resource)
{
    seL4_CPtr ioport_slot = 0u;
    seL4_Uint32 observed_identity = 0u;
    seL4_Uint16 original_command = 0u;
    seL4_Uint16 restored_command = 0u;
    seL4_Uint32 original_bar = 0u;
    seL4_Uint32 sizing_mask = 0u;
    seL4_Uint32 restored_bar = 0u;
    uint64_t size64 = 0u;
    uintptr_t paddr = 0u;
    bool command_quiesced = false;
    bool bar_touched = false;
    bool success = false;

    if (vka == NULL || identity == NULL || resource == NULL ||
        identity->device >= 32u || identity->function >= 8u ||
        identity->vendor_id != SELINOS_QEMU_E1000_VENDOR_ID ||
        identity->device_id != SELINOS_QEMU_E1000_DEVICE_ID ||
        vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    /* Pin N1 to the exact N0 function before any configuration write. */
    if (!read_config_dword(ioport_slot, identity->device, identity->function,
                           PCI_VENDOR_DEVICE_OFF, &observed_identity) ||
        (seL4_Uint16)(observed_identity & 0xffffu) != identity->vendor_id ||
        (seL4_Uint16)(observed_identity >> 16) != identity->device_id ||
        !read_config_word(ioport_slot, identity->device, identity->function,
                          PCI_COMMAND_STATUS_OFF, &original_command) ||
        !read_config_dword(ioport_slot, identity->device, identity->function,
                           PCI_BAR0_OFF, &original_bar) ||
        (original_bar & 0x1u) != 0u ||
        (original_bar & PCI_BAR_MEMORY_TYPE_MASK) != PCI_BAR_MEMORY_TYPE_32 ||
        (original_bar & PCI_BAR_MEMORY_MASK) == 0u) {
        goto out;
    }

    /* Standard BAR sizing is the only write in N1. Decoding and bus mastering
     * are quiesced first, then the original configuration is exactly restored
     * and verified before scalar metadata is accepted. */
    if (!write_config_word(ioport_slot, identity->device, identity->function,
                           PCI_COMMAND_STATUS_OFF,
                           original_command & ~(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER))) {
        goto out;
    }
    command_quiesced = true;
    if (!write_config_dword(ioport_slot, identity->device, identity->function,
                            PCI_BAR0_OFF, 0xffffffffu)) {
        goto out;
    }
    bar_touched = true;
    if (!read_config_dword(ioport_slot, identity->device, identity->function,
                           PCI_BAR0_OFF, &sizing_mask) ||
        !write_config_dword(ioport_slot, identity->device, identity->function,
                            PCI_BAR0_OFF, original_bar) ||
        !read_config_dword(ioport_slot, identity->device, identity->function,
                           PCI_BAR0_OFF, &restored_bar) ||
        restored_bar != original_bar) {
        goto out;
    }
    bar_touched = false;

    if ((sizing_mask & 0x1u) != 0u ||
        (sizing_mask & PCI_BAR_MEMORY_TYPE_MASK) != PCI_BAR_MEMORY_TYPE_32) {
        goto out;
    }
    sizing_mask &= PCI_BAR_MEMORY_MASK;
    if (sizing_mask == 0u) {
        goto out;
    }
    size64 = ((~(uint64_t)sizing_mask) & 0xffffffffull) + 1ull;
    paddr = (uintptr_t)(original_bar & PCI_BAR_MEMORY_MASK);
    if (size64 == 0u || (size64 & (size64 - 1ull)) != 0ull ||
        size64 > (uint64_t)SIZE_MAX || (uint64_t)paddr > PCI_32BIT_ADDRESS_LIMIT - size64) {
        goto out;
    }

    resource->paddr = paddr;
    resource->size = (size_t)size64;
    resource->bar_index = 0u;
    success = true;

out:
    if (bar_touched &&
        (!write_config_dword(ioport_slot, identity->device, identity->function,
                             PCI_BAR0_OFF, original_bar) ||
         !read_config_dword(ioport_slot, identity->device, identity->function,
                            PCI_BAR0_OFF, &restored_bar) ||
         restored_bar != original_bar)) {
        success = false;
    }
    if (command_quiesced &&
        (!write_config_word(ioport_slot, identity->device, identity->function,
                            PCI_COMMAND_STATUS_OFF, original_command) ||
         !read_config_word(ioport_slot, identity->device, identity->function,
                           PCI_COMMAND_STATUS_OFF, &restored_command) ||
         restored_command != original_command)) {
        success = false;
    }
    release_config_ioport(vka, ioport_slot);
    return success;
}

bool selinos_pci_find_qemu_e1000(vka_t *vka,
                                 struct selinos_qemu_e1000_identity *identity)
{
    seL4_CPtr ioport_slot = 0u;

    if (vka == NULL || identity == NULL || vka_cspace_alloc(vka, &ioport_slot) != 0 ||
        seL4_X86_IOPortControl_Issue(seL4_CapIOPortControl,
                                     PCI_CONFIG_ADDRESS_PORT, PCI_CONFIG_LAST_PORT,
                                     seL4_CapInitThreadCNode, ioport_slot,
                                     seL4_WordBits) != seL4_NoError) {
        if (ioport_slot != 0u) {
            vka_cspace_free(vka, ioport_slot);
        }
        return false;
    }

    for (unsigned int device = 0u; device < 32u; ++device) {
        seL4_Uint32 raw_identity = 0xffffffffu;
        if (!read_config_dword(ioport_slot, device, 0u, PCI_VENDOR_DEVICE_OFF,
                               &raw_identity)) {
            continue;
        }
        if ((seL4_Uint16)(raw_identity & 0xffffu) != SELINOS_QEMU_E1000_VENDOR_ID ||
            (seL4_Uint16)(raw_identity >> 16) != SELINOS_QEMU_E1000_DEVICE_ID) {
            continue;
        }
        identity->device = device;
        identity->function = 0u;
        identity->vendor_id = (seL4_Uint16)(raw_identity & 0xffffu);
        identity->device_id = (seL4_Uint16)(raw_identity >> 16);
        release_config_ioport(vka, ioport_slot);
        return true;
    }

    release_config_ioport(vka, ioport_slot);
    return false;
}

/* Modern virtio common configuration device-status field and the only status
 * bits admitted by the M3 root-only feature-stage proof. DRIVER_OK remains
 * deliberately excluded. */
#define VIRTIO_COMMON_DEVICE_STATUS_OFF 0x14u
#define VIRTIO_STATUS_ACKNOWLEDGE        0x01u
#define VIRTIO_STATUS_DRIVER             0x02u
#define VIRTIO_STATUS_FEATURES_OK        0x08u
#define VIRTIO_STATUS_DRIVER_OK          0x04u

bool selinos_pci_stage_qemu_virtio_blk_driver_ready_reset(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_driver_ready_stage *stage)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    const uint8_t features_ok = VIRTIO_STATUS_ACKNOWLEDGE |
                                VIRTIO_STATUS_DRIVER |
                                VIRTIO_STATUS_FEATURES_OK;
    const uint8_t driver_ok = features_ok | VIRTIO_STATUS_DRIVER_OK;
    vka_object_t frame = {0};
    reservation_t reservation = { .res = NULL };
    void *mapping = NULL;
    volatile uint8_t *common = NULL;
    bool frame_allocated = false;
    bool mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || stage == NULL ||
        !virtio_common_range_is_sane(range) ||
        range->capability.length < VIRTIO_COMMON_DEVICE_STATUS_OFF + 1u) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &frame) != 0) {
        return false;
    }
    frame_allocated = true;
    reservation = vspace_reserve_range_aligned(root_vspace, page_size, seL4_PageBits,
                                               read_write, 0, &mapping);
    if (reservation.res == NULL || mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &frame.cptr, NULL, mapping, 1u,
                                  seL4_PageBits, reservation) != 0) {
        goto out;
    }
    mapped = true;
    common = (volatile uint8_t *)((char *)mapping + range->page_offset);

    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != features_ok) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;
    stage->status_after_driver_ok = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    if (stage->status_after_driver_ok != driver_ok) {
        goto out;
    }
    stage->range = *range;
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    success = stage->status_after_reset == 0u;

out:
    if (mapped && !success) {
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (mapped) {
        vspace_unmap_pages(root_vspace, mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    }
    if (reservation.res != NULL) {
        vspace_free_reservation(root_vspace, reservation);
    }
    if (frame_allocated) {
        vka_free_object(vka, &frame);
    }
    return success;
}

bool selinos_pci_stage_qemu_virtio_blk_zero_features(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_feature_stage *stage)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    vka_object_t frame = {0};
    reservation_t reservation = { .res = NULL };
    void *mapping = NULL;
    volatile uint8_t *common = NULL;
    bool frame_allocated = false;
    bool mapped = false;
    bool success = false;
    uint8_t status;

    if (vka == NULL || root_vspace == NULL || stage == NULL ||
        !virtio_common_range_is_sane(range) ||
        range->capability.length < VIRTIO_COMMON_DEVICE_STATUS_OFF + 1u) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &frame) != 0) {
        return false;
    }
    frame_allocated = true;
    reservation = vspace_reserve_range_aligned(root_vspace, page_size, seL4_PageBits,
                                               read_write, 0, &mapping);
    if (reservation.res == NULL || mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &frame.cptr, NULL, mapping, 1u,
                                  seL4_PageBits, reservation) != 0) {
        goto out;
    }
    mapped = true;
    common = (volatile uint8_t *)((char *)mapping + range->page_offset);

    /* A reset establishes zero driver-feature selectors/features by virtio
     * device contract; this proof does not write either feature register. */
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK;
    status = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    if ((status & VIRTIO_STATUS_FEATURES_OK) == 0u) {
        goto out;
    }
    stage->range = *range;
    stage->status_after_features_ok = status;

    /* DRIVER_OK is never written. Reset immediately before releasing the
     * temporary root-only device mapping and frame. */
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    success = stage->status_after_reset == 0u;

out:
    if (mapped && !success) {
        /* Best-effort fail-closed reset before teardown; failures still return
         * false and make no activation claim. */
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (mapped) {
        vspace_unmap_pages(root_vspace, mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    }
    if (reservation.res != NULL) {
        vspace_free_reservation(root_vspace, reservation);
    }
    if (frame_allocated) {
        vka_free_object(vka, &frame);
    }
    return success;
}

/* Modern virtio common configuration queue offsets. M5 admits only the
 * selector/size observation; the default-OFF M6 layout gate additionally
 * admits the queue-size and address fields while retaining queue-enable as a
 * read-only zero assertion. Notification and IRQ controls remain absent. */
#define VIRTIO_COMMON_QUEUE_SELECT_OFF 0x16u
#define VIRTIO_COMMON_QUEUE_SIZE_OFF   0x18u
#define VIRTIO_COMMON_QUEUE_NOTIFY_OFF 0x1eu
#define VIRTIO_COMMON_QUEUE_ENABLE_OFF    0x1cu
#define VIRTIO_COMMON_QUEUE_DESC_LO_OFF   0x20u
#define VIRTIO_COMMON_QUEUE_DESC_HI_OFF   0x24u
#define VIRTIO_COMMON_QUEUE_DRIVER_LO_OFF 0x28u
#define VIRTIO_COMMON_QUEUE_DRIVER_HI_OFF 0x2cu
#define VIRTIO_COMMON_QUEUE_DEVICE_LO_OFF 0x30u
#define VIRTIO_COMMON_QUEUE_DEVICE_HI_OFF 0x34u
#define VIRTIO_COMMON_QUEUE_ZERO       0u
#define VIRTIO_COMMON_QUEUE_LAYOUT_SIZE   1u
#define VIRTIO_COMMON_QUEUE_LAYOUT_END    \
    (VIRTIO_COMMON_QUEUE_DEVICE_HI_OFF + sizeof(uint32_t))

#define VIRTIO_SPLIT_DESC_OFFSET   0u
#define VIRTIO_SPLIT_DRIVER_OFFSET 16u
#define VIRTIO_SPLIT_DEVICE_OFFSET 24u
#define VIRTIO_SPLIT_FLAGS_OFFSET   0u
#define VIRTIO_SPLIT_INDEX_OFFSET   2u
#define VIRTIO_SPLIT_LAYOUT_BYTES  36u

bool selinos_pci_observe_qemu_virtio_blk_queue_zero(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_queue_zero_observation *observation)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    const uint8_t features_ok = VIRTIO_STATUS_ACKNOWLEDGE |
                                VIRTIO_STATUS_DRIVER |
                                VIRTIO_STATUS_FEATURES_OK;
    const uint8_t driver_ok = features_ok | VIRTIO_STATUS_DRIVER_OK;
    vka_object_t frame = {0};
    reservation_t reservation = { .res = NULL };
    void *mapping = NULL;
    volatile uint8_t *common = NULL;
    volatile uint16_t *queue_select = NULL;
    volatile uint16_t *queue_size = NULL;
    bool frame_allocated = false;
    bool mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || observation == NULL ||
        !virtio_common_range_is_sane(range) ||
        range->capability.length < VIRTIO_COMMON_QUEUE_SIZE_OFF + sizeof(uint16_t)) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &frame) != 0) {
        return false;
    }
    frame_allocated = true;
    reservation = vspace_reserve_range_aligned(root_vspace, page_size, seL4_PageBits,
                                               read_write, 0, &mapping);
    if (reservation.res == NULL || mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &frame.cptr, NULL, mapping, 1u,
                                  seL4_PageBits, reservation) != 0) {
        goto out;
    }
    mapped = true;
    common = (volatile uint8_t *)((char *)mapping + range->page_offset);

    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != features_ok) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != driver_ok) {
        goto out;
    }

    queue_select = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SELECT_OFF);
    queue_size = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SIZE_OFF);
    *queue_select = VIRTIO_COMMON_QUEUE_ZERO;
    observation->queue_index = *queue_select;
    observation->queue_size = *queue_size;
    if (observation->queue_index != VIRTIO_COMMON_QUEUE_ZERO || observation->queue_size == 0u) {
        goto out;
    }
    observation->range = *range;
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    observation->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    success = observation->status_after_reset == 0u;

out:
    if (mapped && !success) {
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (mapped) {
        vspace_unmap_pages(root_vspace, mapping, 1u, seL4_PageBits, VSPACE_PRESERVE);
    }
    if (reservation.res != NULL) {
        vspace_free_reservation(root_vspace, reservation);
    }
    if (frame_allocated) {
        vka_free_object(vka, &frame);
    }
    return success;
}

bool selinos_pci_stage_qemu_virtio_blk_queue_layout(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_queue_layout_stage *stage)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    const uint8_t features_ok = VIRTIO_STATUS_ACKNOWLEDGE |
                                VIRTIO_STATUS_DRIVER |
                                VIRTIO_STATUS_FEATURES_OK;
    const uint8_t driver_ok = features_ok | VIRTIO_STATUS_DRIVER_OK;
    vka_object_t common_frame = {0};
    vka_object_t layout_frame = {0};
    reservation_t common_reservation = { .res = NULL };
    reservation_t layout_reservation = { .res = NULL };
    void *common_mapping = NULL;
    void *layout_mapping = NULL;
    volatile uint8_t *common = NULL;
    volatile uint8_t *layout = NULL;
    volatile uint16_t *queue_select = NULL;
    volatile uint16_t *queue_size = NULL;
    volatile uint16_t *queue_enable = NULL;
    volatile uint32_t *queue_desc_lo = NULL;
    volatile uint32_t *queue_desc_hi = NULL;
    volatile uint32_t *queue_driver_lo = NULL;
    volatile uint32_t *queue_driver_hi = NULL;
    volatile uint32_t *queue_device_lo = NULL;
    volatile uint32_t *queue_device_hi = NULL;
    bool common_frame_allocated = false;
    bool layout_frame_allocated = false;
    bool common_mapped = false;
    bool layout_mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || stage == NULL ||
        !virtio_common_range_is_sane(range) ||
        range->capability.length < VIRTIO_COMMON_QUEUE_LAYOUT_END ||
        VIRTIO_SPLIT_LAYOUT_BYTES > page_size) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &common_frame) != 0) {
        return false;
    }
    common_frame_allocated = true;
    if (vka_alloc_frame(vka, seL4_PageBits, &layout_frame) != 0) {
        goto out;
    }
    layout_frame_allocated = true;
    stage->frame_paddr = vka_object_paddr(vka, &layout_frame);
    if (stage->frame_paddr == 0u ||
        (stage->frame_paddr & (page_size - 1u)) != 0u ||
        stage->frame_paddr > UINTPTR_MAX - page_size) {
        goto out;
    }
    stage->descriptor_paddr = stage->frame_paddr + VIRTIO_SPLIT_DESC_OFFSET;
    stage->driver_paddr = stage->frame_paddr + VIRTIO_SPLIT_DRIVER_OFFSET;
    stage->device_paddr = stage->frame_paddr + VIRTIO_SPLIT_DEVICE_OFFSET;
    if (stage->descriptor_paddr < stage->frame_paddr ||
        stage->driver_paddr < stage->frame_paddr ||
        stage->device_paddr < stage->frame_paddr ||
        stage->device_paddr > stage->frame_paddr + page_size - VIRTIO_SPLIT_LAYOUT_BYTES +
                              VIRTIO_SPLIT_DEVICE_OFFSET) {
        goto out;
    }

    common_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &common_mapping);
    if (common_reservation.res == NULL || common_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &common_frame.cptr, NULL,
                                  common_mapping, 1u, seL4_PageBits,
                                  common_reservation) != 0) {
        goto out;
    }
    common_mapped = true;
    layout_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &layout_mapping);
    if (layout_reservation.res == NULL || layout_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &layout_frame.cptr, NULL,
                                  layout_mapping, 1u, seL4_PageBits,
                                  layout_reservation) != 0) {
        goto out;
    }
    layout_mapped = true;
    layout = (volatile uint8_t *)layout_mapping;
    for (size_t index = 0u; index < page_size; ++index) {
        layout[index] = 0u;
    }

    common = (volatile uint8_t *)((char *)common_mapping + range->page_offset);
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != features_ok) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != driver_ok) {
        goto out;
    }

    queue_select = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SELECT_OFF);
    queue_size = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SIZE_OFF);
    queue_enable = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_ENABLE_OFF);
    queue_desc_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DESC_LO_OFF);
    queue_desc_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DESC_HI_OFF);
    queue_driver_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DRIVER_LO_OFF);
    queue_driver_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DRIVER_HI_OFF);
    queue_device_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DEVICE_LO_OFF);
    queue_device_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DEVICE_HI_OFF);
    *queue_select = VIRTIO_COMMON_QUEUE_ZERO;
    stage->queue_index = *queue_select;
    stage->maximum_queue_size = *queue_size;
    stage->queue_enable_before = *queue_enable;
    if (stage->queue_index != VIRTIO_COMMON_QUEUE_ZERO ||
        stage->maximum_queue_size == 0u || stage->queue_enable_before != 0u) {
        goto out;
    }
    *queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;
    stage->programmed_queue_size = *queue_size;
    if (stage->programmed_queue_size != VIRTIO_COMMON_QUEUE_LAYOUT_SIZE) {
        goto out;
    }
    *queue_desc_lo = (uint32_t)stage->descriptor_paddr;
    *queue_desc_hi = (uint32_t)(stage->descriptor_paddr >> 32u);
    *queue_driver_lo = (uint32_t)stage->driver_paddr;
    *queue_driver_hi = (uint32_t)(stage->driver_paddr >> 32u);
    *queue_device_lo = (uint32_t)stage->device_paddr;
    *queue_device_hi = (uint32_t)(stage->device_paddr >> 32u);
    if (*queue_desc_lo != (uint32_t)stage->descriptor_paddr ||
        *queue_desc_hi != (uint32_t)(stage->descriptor_paddr >> 32u) ||
        *queue_driver_lo != (uint32_t)stage->driver_paddr ||
        *queue_driver_hi != (uint32_t)(stage->driver_paddr >> 32u) ||
        *queue_device_lo != (uint32_t)stage->device_paddr ||
        *queue_device_hi != (uint32_t)(stage->device_paddr >> 32u)) {
        goto out;
    }
    stage->queue_enable_after = *queue_enable;
    if (stage->queue_enable_after != 0u) {
        goto out;
    }
    stage->range = *range;
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    success = stage->status_after_reset == 0u;

out:
    if (common_mapped && !success) {
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (layout_mapped) {
        vspace_unmap_pages(root_vspace, layout_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (layout_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, layout_reservation);
    }
    if (common_mapped) {
        vspace_unmap_pages(root_vspace, common_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (common_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, common_reservation);
    }
    if (layout_frame_allocated) {
        vka_free_object(vka, &layout_frame);
    }
    if (common_frame_allocated) {
        vka_free_object(vka, &common_frame);
    }
    return success;
}

bool selinos_pci_stage_qemu_virtio_blk_queue_enable_reset(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_queue_enable_stage *stage)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    const uint8_t features_ok = VIRTIO_STATUS_ACKNOWLEDGE |
                                VIRTIO_STATUS_DRIVER |
                                VIRTIO_STATUS_FEATURES_OK;
    const uint8_t driver_ok = features_ok | VIRTIO_STATUS_DRIVER_OK;
    vka_object_t common_frame = {0};
    vka_object_t layout_frame = {0};
    reservation_t common_reservation = { .res = NULL };
    reservation_t layout_reservation = { .res = NULL };
    void *common_mapping = NULL;
    void *layout_mapping = NULL;
    volatile uint8_t *common = NULL;
    volatile uint8_t *layout = NULL;
    volatile uint16_t *queue_select = NULL;
    volatile uint16_t *queue_size = NULL;
    volatile uint16_t *queue_enable = NULL;
    volatile uint32_t *queue_desc_lo = NULL;
    volatile uint32_t *queue_desc_hi = NULL;
    volatile uint32_t *queue_driver_lo = NULL;
    volatile uint32_t *queue_driver_hi = NULL;
    volatile uint32_t *queue_device_lo = NULL;
    volatile uint32_t *queue_device_hi = NULL;
    bool common_frame_allocated = false;
    bool layout_frame_allocated = false;
    bool common_mapped = false;
    bool layout_mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || stage == NULL ||
        !virtio_common_range_is_sane(range) ||
        range->capability.length < VIRTIO_COMMON_QUEUE_LAYOUT_END ||
        VIRTIO_SPLIT_LAYOUT_BYTES > page_size) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, range->page_paddr, &common_frame) != 0) {
        return false;
    }
    common_frame_allocated = true;
    if (vka_alloc_frame(vka, seL4_PageBits, &layout_frame) != 0) {
        goto out;
    }
    layout_frame_allocated = true;
    stage->frame_paddr = vka_object_paddr(vka, &layout_frame);
    if (stage->frame_paddr == 0u ||
        (stage->frame_paddr & (page_size - 1u)) != 0u ||
        stage->frame_paddr > UINTPTR_MAX - page_size) {
        goto out;
    }
    stage->descriptor_paddr = stage->frame_paddr + VIRTIO_SPLIT_DESC_OFFSET;
    stage->driver_paddr = stage->frame_paddr + VIRTIO_SPLIT_DRIVER_OFFSET;
    stage->device_paddr = stage->frame_paddr + VIRTIO_SPLIT_DEVICE_OFFSET;
    if (stage->descriptor_paddr < stage->frame_paddr ||
        stage->driver_paddr < stage->frame_paddr ||
        stage->device_paddr < stage->frame_paddr ||
        stage->device_paddr > stage->frame_paddr + page_size - VIRTIO_SPLIT_LAYOUT_BYTES +
                              VIRTIO_SPLIT_DEVICE_OFFSET) {
        goto out;
    }

    common_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &common_mapping);
    if (common_reservation.res == NULL || common_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &common_frame.cptr, NULL,
                                  common_mapping, 1u, seL4_PageBits,
                                  common_reservation) != 0) {
        goto out;
    }
    common_mapped = true;
    layout_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &layout_mapping);
    if (layout_reservation.res == NULL || layout_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &layout_frame.cptr, NULL,
                                  layout_mapping, 1u, seL4_PageBits,
                                  layout_reservation) != 0) {
        goto out;
    }
    layout_mapped = true;
    layout = (volatile uint8_t *)layout_mapping;
    for (size_t index = 0u; index < page_size; ++index) {
        layout[index] = 0u;
    }

    common = (volatile uint8_t *)((char *)common_mapping + range->page_offset);
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != features_ok) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != driver_ok) {
        goto out;
    }

    queue_select = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SELECT_OFF);
    queue_size = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SIZE_OFF);
    queue_enable = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_ENABLE_OFF);
    queue_desc_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DESC_LO_OFF);
    queue_desc_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DESC_HI_OFF);
    queue_driver_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DRIVER_LO_OFF);
    queue_driver_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DRIVER_HI_OFF);
    queue_device_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DEVICE_LO_OFF);
    queue_device_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DEVICE_HI_OFF);
    *queue_select = VIRTIO_COMMON_QUEUE_ZERO;
    stage->queue_index = *queue_select;
    stage->maximum_queue_size = *queue_size;
    stage->queue_enable_before = *queue_enable;
    if (stage->queue_index != VIRTIO_COMMON_QUEUE_ZERO ||
        stage->maximum_queue_size == 0u || stage->queue_enable_before != 0u) {
        goto out;
    }
    *queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;
    stage->programmed_queue_size = *queue_size;
    if (stage->programmed_queue_size != VIRTIO_COMMON_QUEUE_LAYOUT_SIZE) {
        goto out;
    }
    *queue_desc_lo = (uint32_t)stage->descriptor_paddr;
    *queue_desc_hi = (uint32_t)(stage->descriptor_paddr >> 32u);
    *queue_driver_lo = (uint32_t)stage->driver_paddr;
    *queue_driver_hi = (uint32_t)(stage->driver_paddr >> 32u);
    *queue_device_lo = (uint32_t)stage->device_paddr;
    *queue_device_hi = (uint32_t)(stage->device_paddr >> 32u);
    if (*queue_desc_lo != (uint32_t)stage->descriptor_paddr ||
        *queue_desc_hi != (uint32_t)(stage->descriptor_paddr >> 32u) ||
        *queue_driver_lo != (uint32_t)stage->driver_paddr ||
        *queue_driver_hi != (uint32_t)(stage->driver_paddr >> 32u) ||
        *queue_device_lo != (uint32_t)stage->device_paddr ||
        *queue_device_hi != (uint32_t)(stage->device_paddr >> 32u)) {
        goto out;
    }
    *queue_enable = 1u;
    stage->queue_enable_enabled = *queue_enable;
    if (stage->queue_enable_enabled != 1u) {
        goto out;
    }
    stage->range = *range;
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    stage->queue_enable_after_reset = *queue_enable;
    success = stage->status_after_reset == 0u &&
              stage->queue_enable_after_reset == 0u;

out:
    if (common_mapped && !success) {
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (layout_mapped) {
        vspace_unmap_pages(root_vspace, layout_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (layout_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, layout_reservation);
    }
    if (common_mapped) {
        vspace_unmap_pages(root_vspace, common_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (common_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, common_reservation);
    }
    if (layout_frame_allocated) {
        vka_free_object(vka, &layout_frame);
    }
    if (common_frame_allocated) {
        vka_free_object(vka, &common_frame);
    }
    return success;
}

static bool virtio_notify_range_is_sane(const struct selinos_qemu_virtio_notify_range *range)
{
    return range != NULL &&
           range->capability.identity.vendor_id == SELINOS_QEMU_VIRTIO_VENDOR_ID &&
           range->capability.identity.device_id == SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID &&
           range->capability.bar_index < PCI_BAR_COUNT &&
           range->capability.length >= VIRTIO_NOTIFY_CFG_MIN_LENGTH &&
           range->capability.notify_off_multiplier != 0u &&
           range->bar_paddr != 0u && range->bar_size != 0u &&
           range->notification_cap_paddr >= range->bar_paddr &&
           range->notification_cap_paddr - range->bar_paddr == range->capability.offset &&
           range->capability.offset <= range->bar_size &&
           range->capability.length <= range->bar_size - range->capability.offset;
}

bool selinos_pci_validate_qemu_virtio_blk_notify_range(
    vka_t *vka, const struct selinos_qemu_virtio_notify_capability *capability,
    struct selinos_qemu_virtio_notify_range *range)
{
    struct selinos_qemu_virtio_common_capability common_capability = {0};
    struct selinos_qemu_virtio_common_range common_range = {0};

    if (vka == NULL || capability == NULL || range == NULL ||
        capability->identity.vendor_id != SELINOS_QEMU_VIRTIO_VENDOR_ID ||
        capability->identity.device_id != SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID ||
        capability->identity.device >= 32u || capability->identity.function >= 8u ||
        capability->bar_index >= PCI_BAR_COUNT ||
        capability->length < VIRTIO_NOTIFY_CFG_MIN_LENGTH ||
        (capability->offset & 0x3u) != 0u || (capability->length & 0x3u) != 0u ||
        capability->notify_off_multiplier == 0u ||
        !selinos_pci_inspect_qemu_virtio_blk_common_capability(vka, &common_capability) ||
        !selinos_pci_validate_qemu_virtio_blk_common_range(vka, &common_capability,
                                                            &common_range) ||
        capability->identity.device != common_range.capability.identity.device ||
        capability->identity.function != common_range.capability.identity.function ||
        capability->bar_index != common_range.capability.bar_index ||
        (size_t)capability->offset > common_range.bar_size ||
        (size_t)capability->length > common_range.bar_size - (size_t)capability->offset ||
        common_range.bar_paddr > UINTPTR_MAX - (uintptr_t)capability->offset) {
        return false;
    }
    range->capability = *capability;
    range->bar_paddr = common_range.bar_paddr;
    range->bar_size = common_range.bar_size;
    range->notification_cap_paddr = common_range.bar_paddr + (uintptr_t)capability->offset;
    return virtio_notify_range_is_sane(range);
}

bool selinos_pci_observe_qemu_virtio_blk_queue_zero_notification(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *common_range,
    const struct selinos_qemu_virtio_notify_range *notify_range,
    struct selinos_qemu_virtio_notification_observation *observation)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    const uint8_t features_ok = VIRTIO_STATUS_ACKNOWLEDGE |
                                VIRTIO_STATUS_DRIVER |
                                VIRTIO_STATUS_FEATURES_OK;
    const uint8_t driver_ok = features_ok | VIRTIO_STATUS_DRIVER_OK;
    vka_object_t common_frame = {0};
    reservation_t common_reservation = { .res = NULL };
    void *common_mapping = NULL;
    volatile uint8_t *common = NULL;
    volatile uint16_t *queue_select = NULL;
    volatile uint16_t *queue_size = NULL;
    volatile uint16_t *queue_notify_off = NULL;
    uintptr_t notification_relative = 0u;
    bool common_frame_allocated = false;
    bool common_mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || observation == NULL ||
        !virtio_common_range_is_sane(common_range) ||
        !virtio_notify_range_is_sane(notify_range) ||
        common_range->capability.identity.device != notify_range->capability.identity.device ||
        common_range->capability.identity.function != notify_range->capability.identity.function ||
        common_range->capability.bar_index != notify_range->capability.bar_index ||
        common_range->capability.length < VIRTIO_COMMON_QUEUE_NOTIFY_OFF + sizeof(uint16_t)) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, common_range->page_paddr, &common_frame) != 0) {
        return false;
    }
    common_frame_allocated = true;
    common_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &common_mapping);
    if (common_reservation.res == NULL || common_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &common_frame.cptr, NULL,
                                  common_mapping, 1u, seL4_PageBits,
                                  common_reservation) != 0) {
        goto out;
    }
    common_mapped = true;
    common = (volatile uint8_t *)((char *)common_mapping + common_range->page_offset);
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != features_ok) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != driver_ok) {
        goto out;
    }

    queue_select = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SELECT_OFF);
    queue_size = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SIZE_OFF);
    queue_notify_off = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_NOTIFY_OFF);
    *queue_select = VIRTIO_COMMON_QUEUE_ZERO;
    observation->queue_index = *queue_select;
    observation->maximum_queue_size = *queue_size;
    observation->queue_notify_off = *queue_notify_off;
    if (observation->queue_index != VIRTIO_COMMON_QUEUE_ZERO ||
        observation->maximum_queue_size == 0u ||
        (observation->queue_notify_off != 0u &&
         (uintptr_t)notify_range->capability.notify_off_multiplier >
             UINTPTR_MAX / (uintptr_t)observation->queue_notify_off)) {
        goto out;
    }
    notification_relative = (uintptr_t)observation->queue_notify_off *
                            (uintptr_t)notify_range->capability.notify_off_multiplier;
    if (notify_range->notification_cap_paddr > UINTPTR_MAX - notification_relative ||
        notification_relative > notify_range->capability.length ||
        sizeof(uint16_t) > notify_range->capability.length - notification_relative) {
        goto out;
    }
    observation->notification_base_paddr = notify_range->notification_cap_paddr;
    observation->notification_paddr = notify_range->notification_cap_paddr + notification_relative;
    if (observation->notification_paddr < observation->notification_base_paddr) {
        goto out;
    }
    observation->common_range = *common_range;
    observation->notify_capability = notify_range->capability;
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    observation->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    success = observation->status_after_reset == 0u;

out:
    if (common_mapped && !success) {
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (common_mapped) {
        vspace_unmap_pages(root_vspace, common_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (common_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, common_reservation);
    }
    if (common_frame_allocated) {
        vka_free_object(vka, &common_frame);
    }
    return success;
}

bool selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *common_range,
    const struct selinos_qemu_virtio_notify_range *notify_range,
    struct selinos_qemu_virtio_zero_descriptor_notification_stage *stage)
{
    const size_t page_size = (size_t)1u << seL4_PageBits;
    const seL4_CapRights_t read_write = seL4_CapRights_new(0, 0, 1, 1);
    const uint8_t features_ok = VIRTIO_STATUS_ACKNOWLEDGE |
                                VIRTIO_STATUS_DRIVER |
                                VIRTIO_STATUS_FEATURES_OK;
    const uint8_t driver_ok = features_ok | VIRTIO_STATUS_DRIVER_OK;
    vka_object_t common_frame = {0};
    vka_object_t layout_frame = {0};
    vka_object_t notification_frame = {0};
    reservation_t common_reservation = { .res = NULL };
    reservation_t layout_reservation = { .res = NULL };
    reservation_t notification_reservation = { .res = NULL };
    void *common_mapping = NULL;
    void *layout_mapping = NULL;
    void *notification_mapping = NULL;
    volatile uint8_t *common = NULL;
    volatile uint8_t *layout = NULL;
    volatile uint16_t *queue_select = NULL;
    volatile uint16_t *queue_size = NULL;
    volatile uint16_t *queue_notify_off = NULL;
    volatile uint16_t *queue_enable = NULL;
    volatile uint32_t *queue_desc_lo = NULL;
    volatile uint32_t *queue_desc_hi = NULL;
    volatile uint32_t *queue_driver_lo = NULL;
    volatile uint32_t *queue_driver_hi = NULL;
    volatile uint32_t *queue_device_lo = NULL;
    volatile uint32_t *queue_device_hi = NULL;
    volatile uint16_t *notification = NULL;
    volatile uint16_t *avail_flags = NULL;
    volatile uint16_t *avail_index = NULL;
    volatile uint16_t *used_flags = NULL;
    volatile uint16_t *used_index = NULL;
    uintptr_t notification_relative = 0u;
    uintptr_t notification_page_paddr = 0u;
    size_t notification_page_offset = 0u;
    bool common_frame_allocated = false;
    bool layout_frame_allocated = false;
    bool notification_frame_allocated = false;
    bool common_mapped = false;
    bool layout_mapped = false;
    bool notification_mapped = false;
    bool success = false;

    if (vka == NULL || root_vspace == NULL || stage == NULL ||
        !virtio_common_range_is_sane(common_range) ||
        !virtio_notify_range_is_sane(notify_range) ||
        common_range->capability.identity.device != notify_range->capability.identity.device ||
        common_range->capability.identity.function != notify_range->capability.identity.function ||
        common_range->capability.bar_index != notify_range->capability.bar_index ||
        common_range->capability.length < VIRTIO_COMMON_QUEUE_LAYOUT_END ||
        common_range->capability.length < VIRTIO_COMMON_QUEUE_NOTIFY_OFF + sizeof(uint16_t)) {
        return false;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, common_range->page_paddr, &common_frame) != 0) {
        return false;
    }
    common_frame_allocated = true;
    if (vka_alloc_frame(vka, seL4_PageBits, &layout_frame) != 0) {
        goto out;
    }
    layout_frame_allocated = true;
    stage->layout_frame_paddr = vka_object_paddr(vka, &layout_frame);
    if (stage->layout_frame_paddr == 0u ||
        stage->layout_frame_paddr > UINTPTR_MAX - (page_size - 1u)) {
        goto out;
    }

    common_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &common_mapping);
    if (common_reservation.res == NULL || common_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &common_frame.cptr, NULL,
                                  common_mapping, 1u, seL4_PageBits,
                                  common_reservation) != 0) {
        goto out;
    }
    common_mapped = true;
    layout_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                       seL4_PageBits, read_write, 0,
                                                       &layout_mapping);
    if (layout_reservation.res == NULL || layout_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &layout_frame.cptr, NULL,
                                  layout_mapping, 1u, seL4_PageBits,
                                  layout_reservation) != 0) {
        goto out;
    }
    layout_mapped = true;
    layout = (volatile uint8_t *)layout_mapping;
    for (size_t index = 0u; index < page_size; ++index) {
        layout[index] = 0u;
    }
    avail_flags = (volatile uint16_t *)(layout + VIRTIO_SPLIT_DRIVER_OFFSET +
                                        VIRTIO_SPLIT_FLAGS_OFFSET);
    avail_index = (volatile uint16_t *)(layout + VIRTIO_SPLIT_DRIVER_OFFSET +
                                        VIRTIO_SPLIT_INDEX_OFFSET);
    used_flags = (volatile uint16_t *)(layout + VIRTIO_SPLIT_DEVICE_OFFSET +
                                       VIRTIO_SPLIT_FLAGS_OFFSET);
    used_index = (volatile uint16_t *)(layout + VIRTIO_SPLIT_DEVICE_OFFSET +
                                       VIRTIO_SPLIT_INDEX_OFFSET);
    stage->avail_flags_before = *avail_flags;
    stage->avail_index_before = *avail_index;
    stage->used_flags_before = *used_flags;
    stage->used_index_before = *used_index;
    if (stage->avail_flags_before != 0u || stage->avail_index_before != 0u ||
        stage->used_flags_before != 0u || stage->used_index_before != 0u) {
        goto out;
    }

    common = (volatile uint8_t *)((char *)common_mapping + common_range->page_offset);
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != 0u) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = VIRTIO_STATUS_ACKNOWLEDGE;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != VIRTIO_STATUS_ACKNOWLEDGE) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] =
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] !=
        (VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER)) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = features_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != features_ok) {
        goto out;
    }
    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = driver_ok;
    if (common[VIRTIO_COMMON_DEVICE_STATUS_OFF] != driver_ok) {
        goto out;
    }

    queue_select = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SELECT_OFF);
    queue_size = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_SIZE_OFF);
    queue_notify_off = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_NOTIFY_OFF);
    queue_enable = (volatile uint16_t *)(common + VIRTIO_COMMON_QUEUE_ENABLE_OFF);
    queue_desc_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DESC_LO_OFF);
    queue_desc_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DESC_HI_OFF);
    queue_driver_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DRIVER_LO_OFF);
    queue_driver_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DRIVER_HI_OFF);
    queue_device_lo = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DEVICE_LO_OFF);
    queue_device_hi = (volatile uint32_t *)(common + VIRTIO_COMMON_QUEUE_DEVICE_HI_OFF);
    *queue_select = VIRTIO_COMMON_QUEUE_ZERO;
    stage->queue_index = *queue_select;
    stage->maximum_queue_size = *queue_size;
    stage->queue_enable_before = *queue_enable;
    stage->queue_notify_off = *queue_notify_off;
    if (stage->queue_index != VIRTIO_COMMON_QUEUE_ZERO ||
        stage->maximum_queue_size == 0u || stage->queue_enable_before != 0u ||
        (stage->queue_notify_off != 0u &&
         (uintptr_t)notify_range->capability.notify_off_multiplier >
             UINTPTR_MAX / (uintptr_t)stage->queue_notify_off)) {
        goto out;
    }

    stage->descriptor_paddr = stage->layout_frame_paddr + VIRTIO_SPLIT_DESC_OFFSET;
    stage->driver_paddr = stage->layout_frame_paddr + VIRTIO_SPLIT_DRIVER_OFFSET;
    stage->device_paddr = stage->layout_frame_paddr + VIRTIO_SPLIT_DEVICE_OFFSET;
    if (stage->descriptor_paddr < stage->layout_frame_paddr ||
        stage->driver_paddr < stage->layout_frame_paddr ||
        stage->device_paddr < stage->layout_frame_paddr ||
        stage->device_paddr > stage->layout_frame_paddr + page_size - VIRTIO_SPLIT_LAYOUT_BYTES) {
        goto out;
    }
    *queue_size = VIRTIO_COMMON_QUEUE_LAYOUT_SIZE;
    stage->programmed_queue_size = *queue_size;
    if (stage->programmed_queue_size != VIRTIO_COMMON_QUEUE_LAYOUT_SIZE) {
        goto out;
    }
    *queue_desc_lo = (uint32_t)stage->descriptor_paddr;
    *queue_desc_hi = (uint32_t)(stage->descriptor_paddr >> 32);
    *queue_driver_lo = (uint32_t)stage->driver_paddr;
    *queue_driver_hi = (uint32_t)(stage->driver_paddr >> 32);
    *queue_device_lo = (uint32_t)stage->device_paddr;
    *queue_device_hi = (uint32_t)(stage->device_paddr >> 32);
    if (*queue_desc_lo != (uint32_t)stage->descriptor_paddr ||
        *queue_desc_hi != (uint32_t)(stage->descriptor_paddr >> 32) ||
        *queue_driver_lo != (uint32_t)stage->driver_paddr ||
        *queue_driver_hi != (uint32_t)(stage->driver_paddr >> 32) ||
        *queue_device_lo != (uint32_t)stage->device_paddr ||
        *queue_device_hi != (uint32_t)(stage->device_paddr >> 32)) {
        goto out;
    }
    *queue_enable = 1u;
    stage->queue_enable_enabled = *queue_enable;
    if (stage->queue_enable_enabled != 1u) {
        goto out;
    }

    notification_relative = (uintptr_t)stage->queue_notify_off *
                            (uintptr_t)notify_range->capability.notify_off_multiplier;
    if (notify_range->notification_cap_paddr > UINTPTR_MAX - notification_relative ||
        notification_relative > notify_range->capability.length ||
        sizeof(uint16_t) > notify_range->capability.length - notification_relative) {
        goto out;
    }
    stage->notification_paddr = notify_range->notification_cap_paddr + notification_relative;
    notification_page_paddr = stage->notification_paddr & ~(page_size - 1u);
    notification_page_offset = (size_t)(stage->notification_paddr - notification_page_paddr);
    if (notify_range->capability.length < page_size ||
        notification_page_paddr < notify_range->notification_cap_paddr ||
        notification_page_offset > page_size - sizeof(uint16_t) ||
        notification_page_paddr - notify_range->notification_cap_paddr >
            notify_range->capability.length - page_size) {
        goto out;
    }
    if (vka_alloc_frame_at(vka, seL4_PageBits, notification_page_paddr,
                           &notification_frame) != 0) {
        goto out;
    }
    notification_frame_allocated = true;
    notification_reservation = vspace_reserve_range_aligned(root_vspace, page_size,
                                                            seL4_PageBits, read_write, 0,
                                                            &notification_mapping);
    if (notification_reservation.res == NULL || notification_mapping == NULL ||
        vspace_map_pages_at_vaddr(root_vspace, &notification_frame.cptr, NULL,
                                  notification_mapping, 1u, seL4_PageBits,
                                  notification_reservation) != 0) {
        goto out;
    }
    notification_mapped = true;
    notification = (volatile uint16_t *)((char *)notification_mapping + notification_page_offset);
    *notification = VIRTIO_COMMON_QUEUE_ZERO;
    stage->avail_flags_after = *avail_flags;
    stage->avail_index_after = *avail_index;
    stage->used_flags_after = *used_flags;
    stage->used_index_after = *used_index;
    if (stage->avail_flags_after != 0u || stage->avail_index_after != 0u ||
        stage->used_flags_after != 0u || stage->used_index_after != 0u) {
        goto out;
    }

    common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    stage->status_after_reset = common[VIRTIO_COMMON_DEVICE_STATUS_OFF];
    stage->queue_enable_after_reset = *queue_enable;
    stage->common_range = *common_range;
    stage->notify_range = *notify_range;
    success = stage->status_after_reset == 0u && stage->queue_enable_after_reset == 0u;

out:
    if (common_mapped && !success) {
        common[VIRTIO_COMMON_DEVICE_STATUS_OFF] = 0u;
    }
    if (notification_mapped) {
        vspace_unmap_pages(root_vspace, notification_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (notification_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, notification_reservation);
    }
    if (notification_frame_allocated) {
        vka_free_object(vka, &notification_frame);
    }
    if (layout_mapped) {
        vspace_unmap_pages(root_vspace, layout_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (layout_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, layout_reservation);
    }
    if (layout_frame_allocated) {
        vka_free_object(vka, &layout_frame);
    }
    if (common_mapped) {
        vspace_unmap_pages(root_vspace, common_mapping, 1u, seL4_PageBits,
                           VSPACE_PRESERVE);
    }
    if (common_reservation.res != NULL) {
        vspace_free_reservation(root_vspace, common_reservation);
    }
    if (common_frame_allocated) {
        vka_free_object(vka, &common_frame);
    }
    return success;
}
