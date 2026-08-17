# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16.0)
project(selinos-root C)

set(configure_string "")

find_package(musllibc REQUIRED)
find_package(util_libs REQUIRED)
find_package(seL4_libs REQUIRED)

# Import only the components required for bootstrap. Additional services will
# receive their own executable targets as the system becomes multi-server.
musllibc_setup_build_environment_with_sel4runtime()
sel4_import_libsel4()
util_libs_import_libraries()
sel4_libs_import_libraries()

config_option(
    SeLinBootDiagnostics
    SELINOS_BOOT_DIAGNOSTICS
    "Emit first-boot diagnostic records through the seL4 debug console"
    DEFAULT
    ON
)
config_option(
    SeLinRootPciBarAllocatorProbe
    SELINOS_ROOT_PCI_BAR_ALLOCATOR_PROBE
    "Enable the quarantined root-only QEMU e1000 BAR0 allocation experiment"
    DEFAULT
    OFF
)
config_option(
    SeLinRootE1000DiscoveryProbe
    SELINOS_ROOT_E1000_DISCOVERY_PROBE
    "Enable quarantined root-only QEMU e1000 PCI identity discovery"
    DEFAULT
    OFF
)
config_option(
    SeLinRootE1000BarRangeProbe
    SELINOS_ROOT_E1000_BAR_RANGE_PROBE
    "Enable quarantined root-only QEMU e1000 firmware BAR0 range validation"
    DEFAULT
    OFF
)
config_option(
    SeLinRootPciBarAllocatorForceUnassignedE1000
    SELINOS_ROOT_PCI_BAR_ALLOCATOR_FORCE_UNASSIGNED_E1000
    "In the quarantined QEMU proof build only, clear firmware e1000 BAR0 before allocator test"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkDiscoveryProbe
    SELINOS_ROOT_VIRTIO_BLK_DISCOVERY_PROBE
    "Enable the quarantined root-only QEMU virtio-blk PCI identity experiment"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkCapabilityProbe
    SELINOS_ROOT_VIRTIO_BLK_CAPABILITY_PROBE
    "Enable the quarantined root-only QEMU virtio-blk modern capability inspection experiment"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkBarFeatureProbe
    SELINOS_ROOT_VIRTIO_BLK_BAR_FEATURE_PROBE
    "Enable quarantined root-only virtio BAR validation and uncached feature observation"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkFeatureStageProbe
    SELINOS_ROOT_VIRTIO_BLK_FEATURE_STAGE_PROBE
    "Enable root-only zero-feature virtio status staging and reset proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkDriverReadyProbe
    SELINOS_ROOT_VIRTIO_BLK_DRIVER_READY_PROBE
    "Enable root-only zero-feature virtio DRIVER_OK/reset proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkQueueZeroProbe
    SELINOS_ROOT_VIRTIO_BLK_QUEUE_ZERO_PROBE
    "Enable root-only virtio queue-zero size observation/reset proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkQueueLayoutProbe
    SELINOS_ROOT_VIRTIO_BLK_QUEUE_LAYOUT_PROBE
    "Enable root-only disabled virtio queue-memory layout/address proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkQueueEnableProbe
    SELINOS_ROOT_VIRTIO_BLK_QUEUE_ENABLE_PROBE
    "Enable root-only virtio queue-enable/read-back/reset proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkNotificationObservationProbe
    SELINOS_ROOT_VIRTIO_BLK_NOTIFICATION_OBSERVATION_PROBE
    "Enable root-only virtio notification-capability/address observation proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkZeroDescriptorNotificationProbe
    SELINOS_ROOT_VIRTIO_BLK_ZERO_DESCRIPTOR_NOTIFICATION_PROBE
    "Enable root-only virtio zero-descriptor notification/reset proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootVirtioBlkZeroIndexObservationProbe
    SELINOS_ROOT_VIRTIO_BLK_ZERO_INDEX_OBSERVATION_PROBE
    "Enable root-only virtio post-notification zero-index observation proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdDynamicAllocationProbe
    SELINOS_TASKD_DYNAMIC_ALLOCATION_PROBE
    "Enable taskd status-only dynamic allocation prerequisite proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdDynamicTcbOwnershipProbe
    SELINOS_TASKD_DYNAMIC_TCB_OWNERSHIP_PROBE
    "Enable taskd-owned dynamically allocated inert TCB proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdDynamicCspaceRollbackProbe
    SELINOS_TASKD_DYNAMIC_CSPACE_ROLLBACK_PROBE
    "Enable taskd-owned dynamic CNode rollback and ownership proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootIommuAvailabilityProbe
    SELINOS_ROOT_IOMMU_AVAILABILITY_PROBE
    "Enable root-only x86 IOMMU bootinfo availability observation"
    DEFAULT
    OFF
)
config_option(
    SeLinX86NxMappingProbe
    SELINOS_X86_NX_MAPPING_PROBE
    "Enable the isolated x86_64 execute-disable versus executable-control mapping proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootConstructM1Adapter
    SELINOS_ROOT_CONSTRUCT_M1_ADAPTER
    "Enable the isolated root-held fixed-child construction adapter M1 proof"
    DEFAULT
    OFF
)
config_option(
    SeLinRootConstructM2ControlLease
    SELINOS_ROOT_CONSTRUCT_M2_CONTROL_LEASE
    "Enable the isolated M2 provenance child-control-cap lease proof after M1"
    DEFAULT
    OFF
)
config_option(
    SeLinRootConstructM3SingleResume
    SELINOS_ROOT_CONSTRUCT_M3_SINGLE_RESUME
    "Enable the isolated M3 single-resume witness proof after M1 and M2"
    DEFAULT
    OFF
)
config_option(
    SeLinRootConstructM4Completion
    SELINOS_ROOT_CONSTRUCT_M4_COMPLETION
    "Enable the isolated M4 child completion witness proof after M1–M3"
    DEFAULT
    OFF
)
if(SeLinRootConstructM2ControlLease AND NOT SeLinRootConstructM1Adapter)
    message(FATAL_ERROR "SeLinRootConstructM2ControlLease requires SeLinRootConstructM1Adapter")
endif()
if(SeLinRootConstructM3SingleResume AND NOT SeLinRootConstructM2ControlLease)
    message(FATAL_ERROR "SeLinRootConstructM3SingleResume requires SeLinRootConstructM2ControlLease")
endif()
if(SeLinRootConstructM4Completion AND NOT SeLinRootConstructM3SingleResume)
    message(FATAL_ERROR "SeLinRootConstructM4Completion requires SeLinRootConstructM3SingleResume")
endif()
add_config_library(selinos-root "${configure_string}")

# Self-authored source-level Linux 6.18.44 KAPI profile. This is a narrow
# compatibility contract, not a Linux kernel or a Linux .ko binary loader.
add_library(selinos-kapi-6_18 STATIC kapi/src/kapi_core.c)
target_include_directories(selinos-kapi-6_18 PUBLIC kapi/include PRIVATE include)
target_link_libraries(selinos-kapi-6_18 PUBLIC sel4_autoconf sel4)
target_compile_options(selinos-kapi-6_18 PRIVATE -Wall -Wextra -Werror -g)

# Freestanding parser for version-pinned Linux ELF module artifacts. The parser
# only inspects a caller-supplied byte buffer; it cannot execute module code.
add_library(selinos-kabi-module STATIC
    kabi/src/selinos_kabi_module.c
    kabi/src/selinos_kabi_policy.c
    kabi/src/selinos_kabi_exports.c)
target_include_directories(selinos-kabi-module PUBLIC kabi/include)
target_compile_options(selinos-kabi-module PRIVATE -Wall -Wextra -Werror -g)

# Phase 5 begins with a non-executing ELF64 ET_EXEC/ET_DYN metadata parser.
# Mapping, relocation, interpreter invocation and dynamic linking remain out of scope.
add_library(selinos-elfrt STATIC elfrt/src/selinos_elfrt.c)
target_include_directories(selinos-elfrt PUBLIC elfrt/include)
target_compile_options(selinos-elfrt PRIVATE -Wall -Wextra -Werror -g)

function(selinos_add_service target_name service_label)
    add_executable(${target_name} EXCLUDE_FROM_ALL servers/service.c)
    target_compile_definitions(${target_name} PRIVATE SELINOS_SERVICE_NAME=\"${service_label}\")
    target_link_libraries(${target_name} PUBLIC sel4_autoconf muslc sel4 sel4runtime)
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror -g)
endfunction()

add_executable(selinos-objectd EXCLUDE_FROM_ALL servers/objectd_m1.c)
target_include_directories(selinos-objectd PRIVATE include)
target_link_libraries(selinos-objectd PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-objectd PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-objectd-m1-probe EXCLUDE_FROM_ALL drivers/objectd_m1_probe.c)
target_include_directories(selinos-objectd-m1-probe PRIVATE include)
target_link_libraries(selinos-objectd-m1-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-objectd-m1-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd EXCLUDE_FROM_ALL servers/taskd_m1.c)
target_include_directories(selinos-taskd PRIVATE include)
target_link_libraries(selinos-taskd PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-task-lifecycle-child EXCLUDE_FROM_ALL drivers/task_lifecycle_child.c)
target_include_directories(selinos-task-lifecycle-child PRIVATE include)
target_link_libraries(selinos-task-lifecycle-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-task-lifecycle-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-task-lifecycle-probe EXCLUDE_FROM_ALL drivers/task_lifecycle_probe.c)
target_include_directories(selinos-task-lifecycle-probe PRIVATE include)
target_link_libraries(selinos-task-lifecycle-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-task-lifecycle-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-alloc-m0 EXCLUDE_FROM_ALL servers/taskd_dynamic_alloc_m0.c)
target_include_directories(selinos-taskd-dynamic-alloc-m0 PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-alloc-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-alloc-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-alloc-m0-probe EXCLUDE_FROM_ALL drivers/taskd_dynamic_alloc_m0_probe.c)
target_include_directories(selinos-taskd-dynamic-alloc-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-alloc-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-alloc-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-tcb-m0 EXCLUDE_FROM_ALL servers/taskd_dynamic_tcb_m0.c)
target_include_directories(selinos-taskd-dynamic-tcb-m0 PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-tcb-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-tcb-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-tcb-m0-probe EXCLUDE_FROM_ALL drivers/taskd_dynamic_tcb_m0_probe.c)
target_include_directories(selinos-taskd-dynamic-tcb-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-tcb-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-tcb-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-cspace-m0 EXCLUDE_FROM_ALL servers/taskd_dynamic_cspace_m0.c)
target_include_directories(selinos-taskd-dynamic-cspace-m0 PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-cspace-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-cspace-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-cspace-m0-probe EXCLUDE_FROM_ALL drivers/taskd_dynamic_cspace_m0_probe.c)
target_include_directories(selinos-taskd-dynamic-cspace-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-cspace-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-cspace-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-reservation-m1 EXCLUDE_FROM_ALL servers/taskd_reservation_m1.c)
target_include_directories(selinos-taskd-reservation-m1 PRIVATE include)
target_link_libraries(selinos-taskd-reservation-m1 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-reservation-m1 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-objectd-reservation-m2 EXCLUDE_FROM_ALL servers/objectd_reservation_m2.c)
target_include_directories(selinos-objectd-reservation-m2 PRIVATE include)
target_link_libraries(selinos-objectd-reservation-m2 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-objectd-reservation-m2 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-memd-reservation-m2 EXCLUDE_FROM_ALL servers/memd_reservation_m2.c)
target_include_directories(selinos-memd-reservation-m2 PRIVATE include)
target_link_libraries(selinos-memd-reservation-m2 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-memd-reservation-m2 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-reservation-m1-probe EXCLUDE_FROM_ALL drivers/taskd_reservation_m1_probe.c)
target_include_directories(selinos-taskd-reservation-m1-probe PRIVATE include)
target_link_libraries(selinos-taskd-reservation-m1-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-reservation-m1-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-opaque-lease-m1 EXCLUDE_FROM_ALL servers/taskd_opaque_lease_m1.c)
target_include_directories(selinos-taskd-opaque-lease-m1 PRIVATE include)
target_link_libraries(selinos-taskd-opaque-lease-m1 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-opaque-lease-m1 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-objectd-opaque-lease-m1 EXCLUDE_FROM_ALL servers/objectd_opaque_lease_m1.c)
target_include_directories(selinos-objectd-opaque-lease-m1 PRIVATE include)
target_link_libraries(selinos-objectd-opaque-lease-m1 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-objectd-opaque-lease-m1 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-opaque-lease-m1-probe EXCLUDE_FROM_ALL drivers/taskd_opaque_lease_m1_probe.c)
target_include_directories(selinos-taskd-opaque-lease-m1-probe PRIVATE include)
target_link_libraries(selinos-taskd-opaque-lease-m1-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-opaque-lease-m1-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-tcb-lease-m1 EXCLUDE_FROM_ALL servers/taskd_tcb_lease_m1.c)
target_include_directories(selinos-taskd-tcb-lease-m1 PRIVATE include)
target_link_libraries(selinos-taskd-tcb-lease-m1 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-tcb-lease-m1 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-objectd-tcb-lease-m1 EXCLUDE_FROM_ALL servers/objectd_tcb_lease_m1.c)
target_include_directories(selinos-objectd-tcb-lease-m1 PRIVATE include)
target_link_libraries(selinos-objectd-tcb-lease-m1 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-objectd-tcb-lease-m1 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-tcb-lease-suspended-child EXCLUDE_FROM_ALL drivers/tcb_lease_suspended_child.c)
target_link_libraries(selinos-tcb-lease-suspended-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-tcb-lease-suspended-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-tcb-lease-m1-probe EXCLUDE_FROM_ALL drivers/taskd_tcb_lease_m1_probe.c)
target_include_directories(selinos-taskd-tcb-lease-m1-probe PRIVATE include)
target_link_libraries(selinos-taskd-tcb-lease-m1-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-tcb-lease-m1-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-tcb-resume-m2 EXCLUDE_FROM_ALL servers/taskd_tcb_resume_m2.c)
target_include_directories(selinos-taskd-tcb-resume-m2 PRIVATE include)
target_link_libraries(selinos-taskd-tcb-resume-m2 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-tcb-resume-m2 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-objectd-tcb-resume-m2 EXCLUDE_FROM_ALL servers/objectd_tcb_resume_m2.c)
target_include_directories(selinos-objectd-tcb-resume-m2 PRIVATE include)
target_link_libraries(selinos-objectd-tcb-resume-m2 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-objectd-tcb-resume-m2 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-tcb-resume-m2-child EXCLUDE_FROM_ALL drivers/tcb_resume_m2_child.c)
target_include_directories(selinos-tcb-resume-m2-child PRIVATE include)
target_link_libraries(selinos-tcb-resume-m2-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-tcb-resume-m2-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-tcb-resume-m2-probe EXCLUDE_FROM_ALL drivers/taskd_tcb_resume_m2_probe.c)
target_include_directories(selinos-taskd-tcb-resume-m2-probe PRIVATE include)
target_link_libraries(selinos-taskd-tcb-resume-m2-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-tcb-resume-m2-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-root-dispatch-m0-probe EXCLUDE_FROM_ALL drivers/root_dispatch_m0_probe.c)
target_include_directories(selinos-root-dispatch-m0-probe PRIVATE include)
target_link_libraries(selinos-root-dispatch-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-root-dispatch-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-root-construct-m1 EXCLUDE_FROM_ALL
    servers/taskd_root_construct_m1_client.c)
target_include_directories(selinos-taskd-root-construct-m1 PRIVATE include)
target_link_libraries(selinos-taskd-root-construct-m1 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-root-construct-m1 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-root-construct-m1-inert-child EXCLUDE_FROM_ALL
    drivers/root_construct_m1_inert_child.c)
target_link_libraries(selinos-root-construct-m1-inert-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-root-construct-m1-inert-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-root-construct-m2 EXCLUDE_FROM_ALL
    servers/taskd_root_construct_m2_client.c)
target_include_directories(selinos-taskd-root-construct-m2 PRIVATE include)
target_link_libraries(selinos-taskd-root-construct-m2 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-root-construct-m2 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-root-construct-m3 EXCLUDE_FROM_ALL
    servers/taskd_root_construct_m3_client.c)
target_include_directories(selinos-taskd-root-construct-m3 PRIVATE include)
target_link_libraries(selinos-taskd-root-construct-m3 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-root-construct-m3 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-root-construct-m3-witness-child EXCLUDE_FROM_ALL
    drivers/root_construct_m3_witness_child.c)
target_link_libraries(selinos-root-construct-m3-witness-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-root-construct-m3-witness-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-root-construct-m4 EXCLUDE_FROM_ALL
    servers/taskd_root_construct_m4_client.c)
target_include_directories(selinos-taskd-root-construct-m4 PRIVATE include)
target_link_libraries(selinos-taskd-root-construct-m4 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-root-construct-m4 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-root-construct-m4-completion-child EXCLUDE_FROM_ALL
    drivers/root_construct_m4_completion_child.c)
target_include_directories(selinos-root-construct-m4-completion-child PRIVATE include)
target_link_libraries(selinos-root-construct-m4-completion-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-root-construct-m4-completion-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-memd EXCLUDE_FROM_ALL servers/memd_m1.c)
target_include_directories(selinos-memd PRIVATE include)
target_link_libraries(selinos-memd PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-memd PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-memd-m1-probe EXCLUDE_FROM_ALL drivers/memd_m1_probe.c)
target_include_directories(selinos-memd-m1-probe PRIVATE include)
target_link_libraries(selinos-memd-m1-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-memd-m1-probe PRIVATE -Wall -Wextra -Werror -g)

selinos_add_service(selinos-consoled "consoled")
selinos_add_service(selinos-abi-gated "abi-gated")

add_executable(selinos-romfsd EXCLUDE_FROM_ALL
    servers/romfsd.c
    servers/romfs_archive.c)
target_include_directories(selinos-romfsd PRIVATE include)
target_link_libraries(selinos-romfsd PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-romfsd PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-romfs-probe EXCLUDE_FROM_ALL drivers/romfs_probe.c)
target_include_directories(selinos-romfs-probe PRIVATE include)
target_link_libraries(selinos-romfs-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-romfs-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-deviced EXCLUDE_FROM_ALL servers/deviced.c)
target_include_directories(selinos-deviced PRIVATE include)
target_link_libraries(selinos-deviced PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-deviced PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-irqd EXCLUDE_FROM_ALL servers/irqd.c)
target_include_directories(selinos-irqd PRIVATE include)
target_link_libraries(selinos-irqd PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-irqd PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-dmad EXCLUDE_FROM_ALL servers/dmad.c)
target_include_directories(selinos-dmad PRIVATE include)
target_link_libraries(selinos-dmad PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-dmad PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-modld EXCLUDE_FROM_ALL
    servers/modld.c
    kabi/fixtures/selinos_kabi_probe_6_18_44.c)
target_include_directories(selinos-modld PRIVATE kabi/include)
target_link_libraries(selinos-modld PUBLIC selinos-kabi-module sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-modld PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-modexec EXCLUDE_FROM_ALL
    servers/modexec.c
    kabi/fixtures/selinos_kabi_probe_6_18_44.c)
target_include_directories(selinos-modexec PRIVATE kabi/include)
target_link_libraries(selinos-modexec PUBLIC selinos-kabi-module sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-modexec PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-edu-kapi-probe EXCLUDE_FROM_ALL drivers/edu_kapi_probe.c)
target_include_directories(selinos-edu-kapi-probe PRIVATE kapi/include include)
target_link_libraries(selinos-edu-kapi-probe PUBLIC selinos-kapi-6_18 sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-edu-kapi-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-linux-syscall-probe EXCLUDE_FROM_ALL drivers/linux_syscall_probe.c)
target_link_libraries(selinos-linux-syscall-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-linux-syscall-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-x86-nx-mapping-probe-child EXCLUDE_FROM_ALL
    drivers/x86_nx_mapping_probe_child.c)
target_include_directories(selinos-x86-nx-mapping-probe-child PRIVATE include)
target_link_libraries(selinos-x86-nx-mapping-probe-child PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-x86-nx-mapping-probe-child PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-edu-mmio-driver EXCLUDE_FROM_ALL drivers/edu_mmio_driver.c)
target_include_directories(selinos-edu-mmio-driver PRIVATE include)
target_link_libraries(selinos-edu-mmio-driver PUBLIC selinos-kapi-6_18 sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-edu-mmio-driver PRIVATE -Wall -Wextra -Werror -g)

include(cpio)
set(
    SELINOS_SERVICE_IMAGES
    "$<TARGET_FILE:selinos-objectd>"
    "$<TARGET_FILE:selinos-objectd-m1-probe>"
    "$<TARGET_FILE:selinos-taskd>"
    "$<TARGET_FILE:selinos-task-lifecycle-child>"
    "$<TARGET_FILE:selinos-task-lifecycle-probe>"
    "$<TARGET_FILE:selinos-taskd-reservation-m1>"
    "$<TARGET_FILE:selinos-objectd-reservation-m2>"
    "$<TARGET_FILE:selinos-memd-reservation-m2>"
    "$<TARGET_FILE:selinos-taskd-reservation-m1-probe>"
    "$<TARGET_FILE:selinos-taskd-opaque-lease-m1>"
    "$<TARGET_FILE:selinos-objectd-opaque-lease-m1>"
    "$<TARGET_FILE:selinos-taskd-opaque-lease-m1-probe>"
    "$<TARGET_FILE:selinos-taskd-tcb-lease-m1>"
    "$<TARGET_FILE:selinos-objectd-tcb-lease-m1>"
    "$<TARGET_FILE:selinos-tcb-lease-suspended-child>"
    "$<TARGET_FILE:selinos-taskd-tcb-lease-m1-probe>"
    "$<TARGET_FILE:selinos-taskd-tcb-resume-m2>"
    "$<TARGET_FILE:selinos-objectd-tcb-resume-m2>"
    "$<TARGET_FILE:selinos-tcb-resume-m2-child>"
    "$<TARGET_FILE:selinos-taskd-tcb-resume-m2-probe>"
    "$<TARGET_FILE:selinos-memd>"
    "$<TARGET_FILE:selinos-memd-m1-probe>"
    "$<TARGET_FILE:selinos-deviced>"
    "$<TARGET_FILE:selinos-romfsd>"
    "$<TARGET_FILE:selinos-romfs-probe>"
    "$<TARGET_FILE:selinos-consoled>"
    "$<TARGET_FILE:selinos-abi-gated>"
    "$<TARGET_FILE:selinos-irqd>"
    "$<TARGET_FILE:selinos-dmad>"
    "$<TARGET_FILE:selinos-modld>"
    "$<TARGET_FILE:selinos-modexec>"
    "$<TARGET_FILE:selinos-linux-syscall-probe>"
    "$<TARGET_FILE:selinos-edu-kapi-probe>"
    "$<TARGET_FILE:selinos-edu-mmio-driver>"
)
if(SeLinRootConstructM4Completion)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-root-construct-m4>"
        "$<TARGET_FILE:selinos-root-construct-m4-completion-child>")
elseif(SeLinRootConstructM3SingleResume)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-root-construct-m3>"
        "$<TARGET_FILE:selinos-root-construct-m3-witness-child>")
elseif(SeLinRootConstructM2ControlLease)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-root-construct-m2>"
        "$<TARGET_FILE:selinos-root-construct-m1-inert-child>")
elseif(SeLinRootConstructM1Adapter)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-root-construct-m1>"
        "$<TARGET_FILE:selinos-root-construct-m1-inert-child>")
else()
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-root-dispatch-m0-probe>")
endif()
if(SeLinTaskdDynamicAllocationProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-dynamic-alloc-m0>"
        "$<TARGET_FILE:selinos-taskd-dynamic-alloc-m0-probe>")
endif()
if(SeLinTaskdDynamicTcbOwnershipProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-dynamic-tcb-m0>"
        "$<TARGET_FILE:selinos-taskd-dynamic-tcb-m0-probe>")
endif()
if(SeLinTaskdDynamicCspaceRollbackProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-dynamic-cspace-m0>"
        "$<TARGET_FILE:selinos-taskd-dynamic-cspace-m0-probe>")
endif()
if(SeLinX86NxMappingProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-x86-nx-mapping-probe-child>")
endif()
MakeCPIO(selinos_services_archive.o "${SELINOS_SERVICE_IMAGES}")

add_executable(
    selinos-root
    EXCLUDE_FROM_ALL
    src/main.c
    src/bootstrap.c
    src/cap_policy.c
    src/domain_manager.c
    src/root_construct_m1.c
    src/selinos_pci.c
    src/selinos_edu_domain.c
    selinos_services_archive.o
)

target_include_directories(selinos-root PRIVATE include)
target_link_libraries(
    selinos-root
    PUBLIC
        sel4_autoconf
        muslc
        sel4
        sel4runtime
        sel4allocman
        sel4vka
        sel4utils
        sel4simple
        sel4simple-default
        sel4platsupport
        sel4muslcsys
        selinos-kabi-module
    PRIVATE
        selinos-root_Config
)
target_compile_options(selinos-root PRIVATE -Wall -Wextra -Wno-error=unused-parameter -Werror -g)
if(SeLinRootConstructM2ControlLease)
    target_sources(selinos-root PRIVATE src/root_construct_m2.c)
endif()
if(SeLinRootConstructM4Completion)
    target_sources(selinos-root PRIVATE src/root_construct_m4.c)
endif()
if(SeLinX86NxMappingProbe)
    target_sources(selinos-root PRIVATE src/x86_nx_mapping_probe.c)
endif()

include(rootserver)
DeclareRootserver(selinos-root)
