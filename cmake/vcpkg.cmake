if(DEFINED ENV{VCPKG_ROOT})
    set(_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
elseif(DEFINED ENV{VCPKG_INSTALLATION_ROOT})
    set(_VCPKG_ROOT "$ENV{VCPKG_INSTALLATION_ROOT}")
else()
    message(FATAL_ERROR "Neither VCPKG_ROOT nor VCPKG_INSTALLATION_ROOT is set. Point one of them to your vcpkg installation (e.g. C:/vcpkg).")
endif()

set(_VCPKG_TOOLCHAIN "${_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
if(NOT EXISTS "${_VCPKG_TOOLCHAIN}")
    message(FATAL_ERROR "Could not find vcpkg toolchain file at ${_VCPKG_TOOLCHAIN}. Ensure your env var points at the vcpkg folder containing scripts/buildsystems/.")
endif()

set(CMAKE_TOOLCHAIN_FILE "${_VCPKG_TOOLCHAIN}" CACHE STRING "vcpkg toolchain file" FORCE)
