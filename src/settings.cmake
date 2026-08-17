# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16.0)

set(project_dir "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB project_modules "${project_dir}/projects/*")
list(
    APPEND CMAKE_MODULE_PATH
    "${project_dir}/kernel"
    "${project_dir}/tools/seL4/cmake-tool/helpers"
    "${project_dir}/tools/seL4/elfloader-tool"
    ${project_modules}
)

set(SEL4_CONFIG_DEFAULT_ADVANCED ON)

include(application_settings)
correct_platform_strings()

# HelixOS v0.1 supports exactly this simulator target. A real PC boot path is a
# later configuration and must be deliberately selected rather than accidental.
# The current seL4 CMake configuration consumes KernelPlatform directly.
# PLATFORM is retained as HelixOS' human-facing build selector.
set(PLATFORM "pc99" CACHE STRING "seL4 platform" FORCE)
set(KernelPlatform "pc99" CACHE STRING "seL4 kernel platform" FORCE)
set(KernelSel4Arch "x86_64" CACHE STRING "Use 64-bit seL4 on PC99" FORCE)
set(KernelFSGSBase "msr" CACHE STRING "Use MSR FS/GS access for TCG QEMU compatibility" FORCE)
set(KernelSupportPCID OFF CACHE BOOL "TCG QEMU lacks PCID support" FORCE)
set(SIMULATION ON CACHE BOOL "Build the QEMU simulation image" FORCE)
set(RELEASE OFF CACHE BOOL "Build with debug checks for early development" FORCE)
set(VERIFICATION OFF CACHE BOOL "Verification build mode is not the v0.1 target" FORCE)
set(KernelVerificationBuild OFF CACHE BOOL "HelixOS M0 is a debug development build" FORCE)
set(MCS OFF CACHE BOOL "Use the classic seL4 scheduler in the first boot milestone" FORCE)
set(SMP OFF CACHE BOOL "Single-core QEMU is the initial reproducibility target" FORCE)
# These kernel settings must precede sel4_configure_platform_settings(), which
# generates the libsel4 API headers consumed by the root task.
set(KernelDebugBuild ON CACHE BOOL "Enable seL4 debug output in HelixOS MVP" FORCE)
set(KernelPrinting ON CACHE BOOL "Enable seL4 debug serial output in HelixOS MVP" FORCE)

find_package(seL4 REQUIRED)
sel4_configure_platform_settings()

set(valid_platforms ${KernelPlatform_all_strings} ${correct_platform_strings_platform_aliases})
set_property(CACHE PLATFORM PROPERTY STRINGS ${valid_platforms})
if(NOT "${PLATFORM}" IN_LIST valid_platforms)
    message(FATAL_ERROR "Invalid PLATFORM selected: ${PLATFORM}. Valid values: ${valid_platforms}")
endif()

