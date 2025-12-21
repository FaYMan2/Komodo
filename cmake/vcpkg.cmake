if(DEFINED ENV{VCPKG_ROOT})
    set(_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
    set(_VCPKG_ROOT "$ENV{VCPKG_INSTALLATION_ROOT}")
elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
    # This repo vendors vcpkg under dev/vcpkg.
    get_filename_component(_VCPKG_ROOT "${CMAKE_CURRENT_LIST_DIR}/../dev/vcpkg" REALPATH)
else()
    message(FATAL_ERROR "Could not locate vcpkg. Set VCPKG_ROOT (or VCPKG_INSTALLATION_ROOT) to your vcpkg installation, or use the repo-local one at dev/vcpkg.")
endif()

set(_VCPKG_TOOLCHAIN "${_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
if(NOT EXISTS "${_VCPKG_TOOLCHAIN}")
    message(FATAL_ERROR "Could not find vcpkg toolchain file at ${_VCPKG_TOOLCHAIN}. Ensure your env var points at the vcpkg folder containing scripts/buildsystems/.")
endif()

set(CMAKE_TOOLCHAIN_FILE "${_VCPKG_TOOLCHAIN}" CACHE STRING "vcpkg toolchain file" FORCE)
