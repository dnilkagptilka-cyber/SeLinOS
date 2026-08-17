// SPDX-License-Identifier: MIT
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vka/vka.h>
#include <vspace/vspace.h>

/* QEMU edu PCI identity from the public QEMU device specification. */
#define SELINOS_QEMU_EDU_VENDOR_ID 0x1234u
#define SELINOS_QEMU_EDU_DEVICE_ID 0x11e8u
#define SELINOS_QEMU_EDU_BAR0_SIZE (1u << 20)

/* QEMU's legacy e1000 device identity. This is root discovery data only. */
#define SELINOS_QEMU_E1000_VENDOR_ID 0x8086u
#define SELINOS_QEMU_E1000_DEVICE_ID 0x100eu

/* N0 result from read-only root PCI configuration-space discovery. This
 * intentionally contains scalar BDF/identity metadata only. */
struct selinos_qemu_e1000_identity {
    unsigned int device;
    unsigned int function;
    uint16_t vendor_id;
    uint16_t device_id;
};

/* QEMU documents these virtio-pci block identities. They are accepted only
 * by the root-only discovery experiment, not by any driver domain. */
#define SELINOS_QEMU_VIRTIO_VENDOR_ID 0x1af4u
#define SELINOS_QEMU_VIRTIO_BLK_LEGACY_DEVICE_ID 0x1001u
#define SELINOS_QEMU_VIRTIO_BLK_MODERN_DEVICE_ID 0x1042u

struct selinos_qemu_virtio_blk_identity {
    unsigned int device;
    unsigned int function;
    uint16_t vendor_id;
    uint16_t device_id;
};

/* Result of root-only modern virtio PCI common-configuration capability
 * parsing. `bar_index`, `offset` and `length` are configuration-space metadata,
 * not a mapped MMIO resource or authority grant. */
struct selinos_qemu_virtio_common_capability {
    struct selinos_qemu_virtio_blk_identity identity;
    unsigned int capability_offset;
    unsigned int bar_index;
    uint32_t offset;
    uint32_t length;
};

/* Root-only M2 result after an assigned modern virtio BAR has been sized,
 * restored and bounds-checked against the M1 common-configuration metadata.
 * These are scalar audit values only: no seL4 frame cap, mapped virtual address
 * or authority can cross this API boundary. */
struct selinos_qemu_virtio_common_range {
    struct selinos_qemu_virtio_common_capability capability;
    uintptr_t bar_paddr;
    size_t bar_size;
    uintptr_t common_paddr;
    uintptr_t page_paddr;
    size_t page_offset;
};

/* Root-only notification-capability metadata. These configuration-space values
 * are not an MMIO mapping, notification permission or driver authority. */
struct selinos_qemu_virtio_notify_capability {
    struct selinos_qemu_virtio_blk_identity identity;
    unsigned int capability_offset;
    unsigned int bar_index;
    uint32_t offset;
    uint32_t length;
    uint32_t notify_off_multiplier;
};

/* Root-only validated notification-capability BAR range. The result contains
 * scalar physical bounds only and neither creates a frame cap nor maps MMIO. */
struct selinos_qemu_virtio_notify_range {
    struct selinos_qemu_virtio_notify_capability capability;
    uintptr_t bar_paddr;
    size_t bar_size;
    uintptr_t notification_cap_paddr;
};

/* Root-only, one-page read observation from the already-validated common
 * configuration. It captures the current selector and selected offered-feature
 * word without writing any common-config register. */
struct selinos_qemu_virtio_feature_observation {
    struct selinos_qemu_virtio_common_range range;
    uint32_t device_feature_select;
    uint32_t device_feature;
};

/* Root-only M3 status-stage result. The driver feature set is deliberately
 * zero: this records only a bounded FEATURES_OK acceptance followed by a
 * reset, not a usable virtio driver activation. */
struct selinos_qemu_virtio_feature_stage {
    struct selinos_qemu_virtio_common_range range;
    uint8_t status_after_features_ok;
    uint8_t status_after_reset;
};

/* Root-only M4 result. It observes one zero-feature DRIVER_OK read-back and
 * immediately resets; it does not retain a usable device state. */
struct selinos_qemu_virtio_driver_ready_stage {
    struct selinos_qemu_virtio_common_range range;
    uint8_t status_after_driver_ok;
    uint8_t status_after_reset;
};

/* Root-only M5 result: only queue zero and its non-zero size are observed
 * while the temporary zero-feature device state is DRIVER_OK, then reset. */
struct selinos_qemu_virtio_queue_zero_observation {
    struct selinos_qemu_virtio_common_range range;
    uint16_t queue_index;
    uint16_t queue_size;
    uint8_t status_after_reset;
};

/* Root-only M6 result: one ordinary root-owned 4 KiB frame contains an
 * all-zero, one-slot split-ring layout. The temporary common-config mapping
 * receives only queue-select, queue-size and address-pair writes. Queue enable
 * is read before/after and must remain zero; it is never written. */
struct selinos_qemu_virtio_queue_layout_stage {
    struct selinos_qemu_virtio_common_range range;
    uint16_t queue_index;
    uint16_t maximum_queue_size;
    uint16_t programmed_queue_size;
    uint16_t queue_enable_before;
    uint16_t queue_enable_after;
    uintptr_t frame_paddr;
    uintptr_t descriptor_paddr;
    uintptr_t driver_paddr;
    uintptr_t device_paddr;
    uint8_t status_after_reset;
};

/* Root-only M7 result: M6's zeroed one-slot layout is programmed, queue_enable
 * is read as zero, written one/read back once, then device reset requires a
 * final zero read-back. No notification or descriptor publication occurs. */
struct selinos_qemu_virtio_queue_enable_stage {
    struct selinos_qemu_virtio_common_range range;
    uint16_t queue_index;
    uint16_t maximum_queue_size;
    uint16_t programmed_queue_size;
    uint16_t queue_enable_before;
    uint16_t queue_enable_enabled;
    uint16_t queue_enable_after_reset;
    uintptr_t frame_paddr;
    uintptr_t descriptor_paddr;
    uintptr_t driver_paddr;
    uintptr_t device_paddr;
    uint8_t status_after_reset;
};

/* Root-only M8 result: queue-zero notification offset and one checked scalar
 * physical notification location. No notification BAR/page is mapped or read. */
struct selinos_qemu_virtio_notification_observation {
    struct selinos_qemu_virtio_common_range common_range;
    struct selinos_qemu_virtio_notify_capability notify_capability;
    uint16_t queue_index;
    uint16_t maximum_queue_size;
    uint16_t queue_notify_off;
    uintptr_t notification_base_paddr;
    uintptr_t notification_paddr;
    uint8_t status_after_reset;
};

/* Root-only M9 result: one zero-value notification write after a fresh zeroed
 * one-slot layout, then reset. These scalar witnesses are not a DMA or I/O
 * capability, and no notification mapping crosses the root API boundary. */
struct selinos_qemu_virtio_zero_descriptor_notification_stage {
    struct selinos_qemu_virtio_common_range common_range;
    struct selinos_qemu_virtio_notify_range notify_range;
    uint16_t queue_index;
    uint16_t maximum_queue_size;
    uint16_t programmed_queue_size;
    uint16_t queue_enable_before;
    uint16_t queue_enable_enabled;
    uint16_t queue_enable_after_reset;
    uint16_t queue_notify_off;
    uintptr_t layout_frame_paddr;
    uintptr_t descriptor_paddr;
    uintptr_t driver_paddr;
    uintptr_t device_paddr;
    uintptr_t notification_paddr;
    uint16_t avail_flags_before;
    uint16_t avail_index_before;
    uint16_t used_flags_before;
    uint16_t used_index_before;
    uint16_t avail_flags_after;
    uint16_t avail_index_after;
    uint16_t used_flags_after;
    uint16_t used_index_after;
    uint8_t status_after_reset;
};

struct selinos_qemu_edu_resource {
    uintptr_t bar0_paddr;
    size_t bar0_size;
    unsigned int irq_line;
};

/*
 * Scan bus 0 using only an IOPort capability issued for ports 0xcf8..0xcff.
 * This is discovery only; it does not delegate BAR, IRQ, DMA or PCI-control
 * authority to a driver domain.
 */
size_t selinos_pci_enumerate_qemu_edu(vka_t *vka,
                                      struct selinos_qemu_edu_resource *resources,
                                      size_t resource_capacity);

/* Convenience compatibility wrapper: returns true and writes the first match. */
bool selinos_pci_find_qemu_edu(vka_t *vka, struct selinos_qemu_edu_resource *resource);

/*
 * Root-only result of a PCI type-0 32-bit memory-BAR assignment. This is
 * configuration evidence, not a frame capability and not driver authority.
 */
struct selinos_pci_bar32_resource {
    uintptr_t paddr;
    size_t size;
    unsigned int bar_index;
};

/*
 * Perform one guarded BAR sizing/assignment transaction through root's
 * temporary PCI configuration IOPort cap. The function accepts only an
 * unconfigured, non-prefetchable or prefetchable 32-bit memory BAR and keeps
 * memory decoding and bus mastering disabled while it probes and programs
 * the BAR. `range_start` and `range_end` form a half-open root policy window.
 * The caller must enforce whole-bus non-overlap before calling this primitive.
 */
bool selinos_pci_assign_unconfigured_bar32(vka_t *vka, unsigned int device,
                                           unsigned int function,
                                           unsigned int bar_index,
                                           uintptr_t range_start,
                                           uintptr_t range_end,
                                           struct selinos_pci_bar32_resource *resource);

/*
 * Quarantine-only root helper for the unconfigured QEMU e1000 BAR0. It is
 * suitable solely for proving the allocator transaction in a dedicated build;
 * it must not be used to start a NIC domain, map MMIO, issue IRQ capability or
 * delegate DMA authority until a separate gate approves those steps.
 */
bool selinos_pci_assign_qemu_e1000_bar0(vka_t *vka, uintptr_t range_start,
                                        uintptr_t range_end,
                                        struct selinos_pci_bar32_resource *resource);

/* Quarantine-only e1000 identity lookup. It performs configuration reads only
 * and returns no MMIO resource, IRQ, DMA lease, command-register change or
 * capability transferable to a driver domain. */
bool selinos_pci_find_qemu_e1000(vka_t *vka,
                                 struct selinos_qemu_e1000_identity *identity);

/* Quarantine-only N1 validation of the firmware-assigned QEMU e1000 BAR0.
 * Root re-reads and pins the supplied N0 identity, checks one non-zero 32-bit
 * memory BAR, performs a quiesced all-ones size probe with exact BAR/command
 * restoration and readback verification, and returns scalar range metadata
 * only. It never returns a frame cap, maps MMIO, enables bus mastering, issues
 * an IRQ/DMA capability or delegates authority to a NIC domain. */
bool selinos_pci_read_qemu_e1000_bar_range(
    vka_t *vka, const struct selinos_qemu_e1000_identity *identity,
    struct selinos_pci_bar32_resource *resource);

/* Quarantine-only PCI identity lookup for one documented QEMU virtio-blk
 * device. It returns no resource capability and performs no BAR/IRQ/DMA
 * configuration or device-status/feature negotiation. */
bool selinos_pci_find_qemu_virtio_blk(vka_t *vka,
                                      struct selinos_qemu_virtio_blk_identity *identity);

/* Quarantine-only modern virtio PCI capability-list inspection. The routine
 * reads configuration space only and accepts a bounded, aligned common-config
 * capability. It does not read or program a BAR, map MMIO, negotiate features,
 * create queues, issue interrupts, map DMA or start block I/O. */
bool selinos_pci_inspect_qemu_virtio_blk_common_capability(
    vka_t *vka, struct selinos_qemu_virtio_common_capability *capability);

/* Root-only M8 notification-capability parsing. It returns PCI configuration
 * metadata only and neither maps nor writes the notification location. */
bool selinos_pci_inspect_qemu_virtio_blk_notify_capability(
    vka_t *vka, struct selinos_qemu_virtio_notify_capability *capability);

/* Root-only M8 notification BAR sizing/bounds validation. PCI command memory
 * and master bits are temporarily quiesced and exactly restored; no notification
 * BAR frame or mapping is created. */
bool selinos_pci_validate_qemu_virtio_blk_notify_range(
    vka_t *vka, const struct selinos_qemu_virtio_notify_capability *capability,
    struct selinos_qemu_virtio_notify_range *range);

/* Quarantine-only M2 BAR validation. The root temporarily sizes an already
 * assigned 32-bit memory BAR while memory decoding and bus mastering are
 * quiesced, restores both registers, and returns only scalar bounds metadata.
 * It does not map MMIO, alter virtio status/features, create queues or issue
 * any grant to another domain. */
bool selinos_pci_validate_qemu_virtio_blk_common_range(
    vka_t *vka, const struct selinos_qemu_virtio_common_capability *capability,
    struct selinos_qemu_virtio_common_range *range);

/* Quarantine-only M2 feature observation. The root allocates one physical
 * device frame only for an already bounds-checked common-config page, maps it
 * uncached and read-only in its own VSpace, performs two volatile reads, then
 * unmaps and frees the root frame before returning. No cap is copied to a
 * driver domain and the routine performs no MMIO write. */
bool selinos_pci_observe_qemu_virtio_blk_features(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_feature_observation *observation);

/* Root-only M3 zero-feature status staging. It maps one validated common page
 * temporarily, writes only device_status through reset/ACKNOWLEDGE/DRIVER/
 * FEATURES_OK, requires the FEATURES_OK read-back bit, resets status to zero
 * and tears the mapping down. It never writes DRIVER_OK, selectors, features,
 * queue fields or notifications and does not enable bus mastering, issue IRQ,
 * DMA or block I/O. */
bool selinos_pci_stage_qemu_virtio_blk_zero_features(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_feature_stage *stage);

/* Root-only M4 extends M3 with one zero-feature DRIVER_OK read-back, then
 * resets before teardown. It never touches feature, queue, notify, IRQ, DMA
 * or block-I/O state. */
bool selinos_pci_stage_qemu_virtio_blk_driver_ready_reset(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_driver_ready_stage *stage);

/* Root-only M5 selects only queue zero and reads only its size during a
 * temporary zero-feature DRIVER_OK state, then resets. It never writes queue
 * size/address/enable, notification, IRQ, DMA or block-I/O state. */
bool selinos_pci_observe_qemu_virtio_blk_queue_zero(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_queue_zero_observation *observation);

/* Root-only M6: stage a zero-filled one-slot split-ring layout in a temporary
 * ordinary root frame, program only queue zero's size/address pairs during a
 * zero-feature DRIVER_OK state, require queue_enable to remain zero, reset and
 * release every temporary object. It does not enable or notify a queue, touch
 * PCI command/MSI-X/IRQ state, create DMA/IOSpace authority or issue I/O. */
bool selinos_pci_stage_qemu_virtio_blk_queue_layout(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_queue_layout_stage *stage);

/* Root-only M7: repeat the temporary M6 zeroed one-slot layout and write
 * queue_enable=1 exactly once, require its read-back, reset device status and
 * require queue_enable=0 before teardown. No notification, IRQ, PCI-command,
 * bus-mastering, IOSpace/DMA authority, descriptor publication or I/O occurs;
 * this is not a DMA-behavior or containment claim. */
bool selinos_pci_stage_qemu_virtio_blk_queue_enable_reset(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *range,
    struct selinos_qemu_virtio_queue_enable_stage *stage);

/* Root-only M8: with the common and notification capabilities already parsed,
 * select queue zero under temporary zero-feature DRIVER_OK, read its scalar
 * notification offset and calculate one bounded physical location. The
 * notification BAR remains unmapped and is never read or written. */
bool selinos_pci_observe_qemu_virtio_blk_queue_zero_notification(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *common_range,
    const struct selinos_qemu_virtio_notify_range *notify_range,
    struct selinos_qemu_virtio_notification_observation *observation);

/* Root-only M9: prepare a zeroed one-slot layout, enable queue zero once,
 * write notification value zero exactly once through a temporary root mapping,
 * then reset and release all local state. This is not a DMA/I/O claim. */
bool selinos_pci_stage_qemu_virtio_blk_zero_descriptor_notification_reset(
    vka_t *vka, vspace_t *root_vspace,
    const struct selinos_qemu_virtio_common_range *common_range,
    const struct selinos_qemu_virtio_notify_range *notify_range,
    struct selinos_qemu_virtio_zero_descriptor_notification_stage *stage);
