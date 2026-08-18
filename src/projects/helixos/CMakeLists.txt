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
    SeLinTaskdDynamicVspaceRollbackProbe
    SELINOS_TASKD_DYNAMIC_VSPACE_ROLLBACK_PROBE
    "Enable taskd-owned dynamic VSpace-root rollback and ownership proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdInertBundleRollbackProbe
    SELINOS_TASKD_INERT_BUNDLE_ROLLBACK_PROBE
    "Enable taskd-owned inert TCB/CNode/PML4 bundle rollback proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdSuspendedLinkageProbe
    SELINOS_TASKD_SUSPENDED_LINKAGE_PROBE
    "Enable configured but non-executing taskd TCB/CSpace/VSpace linkage proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdPopulatedCspaceProbe
    SELINOS_TASKD_POPULATED_CSPACE_PROBE
    "Enable one-cap populated CSpace configured non-executing taskd proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdSelfRootedCspaceProbe
    SELINOS_TASKD_SELF_ROOTED_CSPACE_PROBE
    "Enable self-rooted two-cap CSpace configured non-executing taskd proof"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdIpcBufferProbe
    SELINOS_TASKD_IPC_BUFFER_PROBE
    "Enable isolated IPC-buffer mapping with a configured but non-executing taskd target"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdZeroedContextProbe
    SELINOS_TASKD_ZEROED_CONTEXT_PROBE
    "Enable zeroed whole-register-context read-back for a configured non-executing taskd target"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdFaultWitnessProbe
    SELINOS_TASKD_FAULT_WITNESS_PROBE
    "Enable one fault-mediated first-execution witness with an unreplied target VM fault"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdEntryStackProbe
    SELINOS_TASKD_ENTRY_STACK_PROBE
    "Enable canonical NX entry and ABI-aligned stack provenance for a suspended task"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdExecFetchProbe
    SELINOS_TASKD_EXEC_FETCH_PROBE
    "Enable one executable NOP then unreplied invalid-opcode fault witness"
    DEFAULT
    OFF
)
config_option(
    SeLinTaskdVmRestartProbe
    SELINOS_TASKD_VM_RESTART_PROBE
    "Enable one VM-fault repair reply then terminal invalid-opcode witness"
    DEFAULT
    OFF
)
config_option(
    SeLinSealedStaticImageProbe
    SELINOS_SEALED_STATIC_IMAGE_PROBE
    "Enable one sealed static-image parser ledger with fail-closed W^X guards"
    DEFAULT
    OFF
)
config_option(
    SeLinSealedStaticImageMappingProbe
    SELINOS_SEALED_STATIC_IMAGE_MAPPING_PROBE
    "Enable one accepted sealed-image payload materialization and terminal W^X witness"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageTerminalLifecycleProbe
    SELINOS_STATIC_IMAGE_TERMINAL_LIFECYCLE_PROBE
    "Enable one terminal static-image ownership observation without reply or reclamation"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageTeardownAuthorizationProbe
    SELINOS_STATIC_IMAGE_TEARDOWN_AUTHORIZATION_PROBE
    "Enable one fail-closed terminal teardown authorization ledger without deallocation"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageMappingRevocationAuthorizationProbe
    SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_AUTHORIZATION_PROBE
    "Enable one mapping-revocation authorization ledger without unmap or reclamation"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageMappingRevocationProbe
    SELINOS_STATIC_IMAGE_MAPPING_REVOCATION_PROBE
    "Enable one terminal target entry-stack-IPC mapping revocation witness without object free"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageTargetCapabilityDeletionProbe
    SELINOS_STATIC_IMAGE_TARGET_CAPABILITY_DELETION_PROBE
    "Enable one terminal target-CNode capability deletion witness without object free"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageObjectReclamationProbe
    SELINOS_STATIC_IMAGE_OBJECT_RECLAMATION_PROBE
    "Enable one terminal static-image object reclamation witness without reuse"
    DEFAULT
    OFF
)
config_option(
    SeLinStaticImageFreshBundleAuthorizationProbe
    SELINOS_STATIC_IMAGE_FRESH_BUNDLE_AUTHORIZATION_PROBE
    "Enable one post-reclamation fresh-bundle authorization ledger without allocation"
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

# Phase 64 validates a self-authored sealed static-image grammar only. It maps
# nothing and is not an ELF or executable loader.
add_library(selinos-sealed-static-image-m0-parser STATIC
    src/sealed_static_image_m0.c
    src/sealed_static_image_m0_fixture.c)
target_include_directories(selinos-sealed-static-image-m0-parser PUBLIC include kabi/include)
target_link_libraries(selinos-sealed-static-image-m0-parser PUBLIC selinos-kabi-module)
target_compile_options(selinos-sealed-static-image-m0-parser PRIVATE -Wall -Wextra -Werror -g)

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

add_executable(selinos-taskd-dynamic-vspace-m0 EXCLUDE_FROM_ALL servers/taskd_dynamic_vspace_m0.c)
target_include_directories(selinos-taskd-dynamic-vspace-m0 PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-vspace-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-vspace-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-dynamic-vspace-m0-probe EXCLUDE_FROM_ALL drivers/taskd_dynamic_vspace_m0_probe.c)
target_include_directories(selinos-taskd-dynamic-vspace-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-dynamic-vspace-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-dynamic-vspace-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-inert-bundle-m0 EXCLUDE_FROM_ALL servers/taskd_inert_bundle_m0.c)
target_include_directories(selinos-taskd-inert-bundle-m0 PRIVATE include)
target_link_libraries(selinos-taskd-inert-bundle-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-inert-bundle-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-inert-bundle-m0-probe EXCLUDE_FROM_ALL drivers/taskd_inert_bundle_m0_probe.c)
target_include_directories(selinos-taskd-inert-bundle-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-inert-bundle-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-inert-bundle-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-suspended-linkage-m0 EXCLUDE_FROM_ALL servers/taskd_suspended_linkage_m0.c)
target_include_directories(selinos-taskd-suspended-linkage-m0 PRIVATE include)
target_link_libraries(selinos-taskd-suspended-linkage-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-suspended-linkage-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-suspended-linkage-m0-probe EXCLUDE_FROM_ALL drivers/taskd_suspended_linkage_m0_probe.c)
target_include_directories(selinos-taskd-suspended-linkage-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-suspended-linkage-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-suspended-linkage-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-populated-cspace-m0 EXCLUDE_FROM_ALL servers/taskd_populated_cspace_m0.c)
target_include_directories(selinos-taskd-populated-cspace-m0 PRIVATE include)
target_link_libraries(selinos-taskd-populated-cspace-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-populated-cspace-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-populated-cspace-m0-probe EXCLUDE_FROM_ALL drivers/taskd_populated_cspace_m0_probe.c)
target_include_directories(selinos-taskd-populated-cspace-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-populated-cspace-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-populated-cspace-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-self-rooted-cspace-m0 EXCLUDE_FROM_ALL servers/taskd_self_rooted_cspace_m0.c)
target_include_directories(selinos-taskd-self-rooted-cspace-m0 PRIVATE include)
target_link_libraries(selinos-taskd-self-rooted-cspace-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-self-rooted-cspace-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-self-rooted-cspace-m0-probe EXCLUDE_FROM_ALL drivers/taskd_self_rooted_cspace_m0_probe.c)
target_include_directories(selinos-taskd-self-rooted-cspace-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-self-rooted-cspace-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-self-rooted-cspace-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-ipc-buffer-m0 EXCLUDE_FROM_ALL servers/taskd_ipc_buffer_m0.c)
target_include_directories(selinos-taskd-ipc-buffer-m0 PRIVATE include)
target_link_libraries(selinos-taskd-ipc-buffer-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-ipc-buffer-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-ipc-buffer-m0-probe EXCLUDE_FROM_ALL drivers/taskd_ipc_buffer_m0_probe.c)
target_include_directories(selinos-taskd-ipc-buffer-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-ipc-buffer-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-ipc-buffer-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-zeroed-context-m0 EXCLUDE_FROM_ALL servers/taskd_zeroed_context_m0.c)
target_include_directories(selinos-taskd-zeroed-context-m0 PRIVATE include)
target_link_libraries(selinos-taskd-zeroed-context-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-zeroed-context-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-zeroed-context-m0-probe EXCLUDE_FROM_ALL drivers/taskd_zeroed_context_m0_probe.c)
target_include_directories(selinos-taskd-zeroed-context-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-zeroed-context-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-zeroed-context-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-fault-witness-m0 EXCLUDE_FROM_ALL servers/taskd_fault_witness_m0.c)
target_include_directories(selinos-taskd-fault-witness-m0 PRIVATE include)
target_link_libraries(selinos-taskd-fault-witness-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-fault-witness-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-fault-witness-m0-probe EXCLUDE_FROM_ALL drivers/taskd_fault_witness_m0_probe.c)
target_include_directories(selinos-taskd-fault-witness-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-fault-witness-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-fault-witness-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-entry-stack-m0 EXCLUDE_FROM_ALL servers/taskd_entry_stack_m0.c)
target_include_directories(selinos-taskd-entry-stack-m0 PRIVATE include)
target_link_libraries(selinos-taskd-entry-stack-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-entry-stack-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-entry-stack-m0-probe EXCLUDE_FROM_ALL drivers/taskd_entry_stack_m0_probe.c)
target_include_directories(selinos-taskd-entry-stack-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-entry-stack-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-entry-stack-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-exec-fetch-m0 EXCLUDE_FROM_ALL servers/taskd_exec_fetch_m0.c)
target_include_directories(selinos-taskd-exec-fetch-m0 PRIVATE include)
target_link_libraries(selinos-taskd-exec-fetch-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-exec-fetch-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-exec-fetch-m0-probe EXCLUDE_FROM_ALL drivers/taskd_exec_fetch_m0_probe.c)
target_include_directories(selinos-taskd-exec-fetch-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-exec-fetch-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-exec-fetch-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-vm-restart-m0 EXCLUDE_FROM_ALL servers/taskd_vm_restart_m0.c)
target_include_directories(selinos-taskd-vm-restart-m0 PRIVATE include)
target_link_libraries(selinos-taskd-vm-restart-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-vm-restart-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-taskd-vm-restart-m0-probe EXCLUDE_FROM_ALL drivers/taskd_vm_restart_m0_probe.c)
target_include_directories(selinos-taskd-vm-restart-m0-probe PRIVATE include)
target_link_libraries(selinos-taskd-vm-restart-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-taskd-vm-restart-m0-probe PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-sealed-static-image-m0 EXCLUDE_FROM_ALL servers/sealed_static_image_m0.c)
target_include_directories(selinos-sealed-static-image-m0 PRIVATE include)
target_link_libraries(selinos-sealed-static-image-m0 PUBLIC sel4_autoconf muslc sel4 sel4runtime selinos-sealed-static-image-m0-parser)
target_compile_options(selinos-sealed-static-image-m0 PRIVATE -Wall -Wextra -Werror -g)

add_executable(selinos-sealed-static-image-m0-probe EXCLUDE_FROM_ALL drivers/sealed_static_image_m0_probe.c)
target_include_directories(selinos-sealed-static-image-m0-probe PRIVATE include)
target_link_libraries(selinos-sealed-static-image-m0-probe PUBLIC sel4_autoconf muslc sel4 sel4runtime)
target_compile_options(selinos-sealed-static-image-m0-probe PRIVATE -Wall -Wextra -Werror -g)

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
if(SeLinTaskdDynamicVspaceRollbackProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-dynamic-vspace-m0>"
        "$<TARGET_FILE:selinos-taskd-dynamic-vspace-m0-probe>")
endif()
if(SeLinTaskdInertBundleRollbackProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-inert-bundle-m0>"
        "$<TARGET_FILE:selinos-taskd-inert-bundle-m0-probe>")
endif()
if(SeLinTaskdSuspendedLinkageProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-suspended-linkage-m0>"
        "$<TARGET_FILE:selinos-taskd-suspended-linkage-m0-probe>")
endif()
if(SeLinTaskdPopulatedCspaceProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-populated-cspace-m0>"
        "$<TARGET_FILE:selinos-taskd-populated-cspace-m0-probe>")
endif()
if(SeLinTaskdSelfRootedCspaceProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-self-rooted-cspace-m0>"
        "$<TARGET_FILE:selinos-taskd-self-rooted-cspace-m0-probe>")
endif()
if(SeLinTaskdIpcBufferProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-ipc-buffer-m0>"
        "$<TARGET_FILE:selinos-taskd-ipc-buffer-m0-probe>")
endif()
if(SeLinTaskdZeroedContextProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-zeroed-context-m0>"
        "$<TARGET_FILE:selinos-taskd-zeroed-context-m0-probe>")
endif()
if(SeLinTaskdFaultWitnessProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-fault-witness-m0>"
        "$<TARGET_FILE:selinos-taskd-fault-witness-m0-probe>")
endif()
if(SeLinTaskdEntryStackProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-entry-stack-m0>"
        "$<TARGET_FILE:selinos-taskd-entry-stack-m0-probe>")
endif()
if(SeLinTaskdExecFetchProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-exec-fetch-m0>"
        "$<TARGET_FILE:selinos-taskd-exec-fetch-m0-probe>")
endif()
if(SeLinTaskdVmRestartProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-taskd-vm-restart-m0>"
        "$<TARGET_FILE:selinos-taskd-vm-restart-m0-probe>")
endif()
if(SeLinSealedStaticImageProbe)
    list(APPEND SELINOS_SERVICE_IMAGES
        "$<TARGET_FILE:selinos-sealed-static-image-m0>"
        "$<TARGET_FILE:selinos-sealed-static-image-m0-probe>")
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
        selinos-sealed-static-image-m0-parser
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
