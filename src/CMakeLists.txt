# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.16.0)

# settings.cmake must run before the kernel is imported, because it selects the
# target platform and exposes the seL4 configuration variables.
include(settings.cmake)
project(helixos C CXX ASM)

find_package(seL4 REQUIRED)
find_package(elfloader-tool REQUIRED)

# The root task owns bootstrap capabilities while creating the first services.
# The value is intentionally conservative for the initial multi-server layout.
set(KernelRootCNodeSizeBits 14 CACHE STRING "HelixOS bootstrap root CNode size" FORCE)

sel4_import_kernel()
elfloader_import_project()

add_subdirectory(projects/helixos)

if(SIMULATION)
    include(simulation)
    if(KernelSel4ArchX86_64)
        SetSimulationScriptProperty(MEM_SIZE "1G")
    endif()
    GenerateSimulateScript()
endif()
