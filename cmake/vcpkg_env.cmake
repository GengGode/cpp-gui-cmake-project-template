# configure the vcpkg toolchain file
macro(set_vcpkg_config)
    if(VCPKG_INSTALLED)
        set(VCPKG_MANIFEST_INSTALL OFF CACHE BOOL "Enable manifest install" FORCE)
    else()
        set(VCPKG_MANIFEST_INSTALL ON CACHE BOOL "Enable manifest install" FORCE)
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file" FORCE)
    set(VCPKG_SKIP_DEPENDENCY_CHECK ON CACHE BOOL "Skip vcpkg dependency check" FORCE)
    set(VCPKG_APPLOCAL_DEPS ON CACHE BOOL "Enable applocal deps")
    set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Manifest mode")  
    set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "Manifest directory")
    set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET} CACHE STRING "Vcpkg target triplet")
    set(VCPKG_INSTALLED_DIR ${CMAKE_BINARY_DIR}/vcpkg_installed)
endmacro(set_vcpkg_config)
 
# load VCPKG_ROOT from the .vcpkg-root file
if(EXISTS "${CMAKE_SOURCE_DIR}/.vcpkg-root")
    file(STRINGS "${CMAKE_SOURCE_DIR}/.vcpkg-root" VCPKG_ROOT)
    set(ENV{VCPKG_ROOT} "${VCPKG_ROOT}")
    message(STATUS "VCPKG_ROOT found in the .vcpkg-root file, using: $ENV{VCPKG_ROOT}")
else()
    # load VCPKG_ROOT from the environment
    if(DEFINED ENV{VCPKG_ROOT})
        message(STATUS "VCPKG_ROOT found in the environment, using: $ENV{VCPKG_ROOT}")
    else()
        message(WARNING "VCPKG_ROOT not found in the environment or .vcpkg-root file
            Please set VCPKG_ROOT in the environment or create a .vcpkg-root file
            with the path to the vcpkg installation directory.
            For example, C:/vcpkg")
    endif()
endif()
 
# load VCPKG_TARGET_TRIPLET from the .vcpkg-target-triplet file
if(EXISTS "${CMAKE_SOURCE_DIR}/.vcpkg-target-triplet")
    file(STRINGS "${CMAKE_SOURCE_DIR}/.vcpkg-target-triplet" VCPKG_TARGET_TRIPLET)
    set(ENV{VCPKG_TARGET_TRIPLET} "${VCPKG_TARGET_TRIPLET}")
    message(STATUS "VCPKG_TARGET_TRIPLET found in the .vcpkg-target-triplet file: ${VCPKG_TARGET_TRIPLET}")
else()
    # load VCPKG_TARGET_TRIPLET from the environment
    if(DEFINED ENV{VCPKG_TARGET_TRIPLET})
        message(STATUS "VCPKG_TARGET_TRIPLET found in the environment: ${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

# load VCPKG_BINARY_SOURCES from the .vcpkg-binary-sources file
if(EXISTS "${CMAKE_SOURCE_DIR}/.vcpkg-binary-sources")
    file(READ "${CMAKE_SOURCE_DIR}/.vcpkg-binary-sources" VCPKG_BINARY_SOURCES)
    string(STRIP "${VCPKG_BINARY_SOURCES}" VCPKG_BINARY_SOURCES)
    set(ENV{VCPKG_BINARY_SOURCES} "${VCPKG_BINARY_SOURCES}")
    message(STATUS "VCPKG_BINARY_SOURCES found in the .vcpkg-binary-sources file: ${VCPKG_BINARY_SOURCES}")
else()
    # load VCPKG_BINARY_SOURCES from the environment
    if(DEFINED ENV{VCPKG_BINARY_SOURCES})
        message(STATUS "VCPKG_BINARY_SOURCES found in the environment: ${VCPKG_BINARY_SOURCES}")
    endif()
endif()

# load VCPKG_OVERLAY_PORTS from the .vcpkg-overlay-ports file
if(EXISTS "${CMAKE_SOURCE_DIR}/.vcpkg-overlay-ports")
    file(READ "${CMAKE_SOURCE_DIR}/.vcpkg-overlay-ports" VCPKG_OVERLAY_PORTS)
    string(STRIP "${VCPKG_OVERLAY_PORTS}" VCPKG_OVERLAY_PORTS)
    set(ENV{VCPKG_OVERLAY_PORTS} "${VCPKG_OVERLAY_PORTS}")
    message(STATUS "VCPKG_OVERLAY_PORTS found in the .vcpkg-overlay-ports file: ${VCPKG_OVERLAY_PORTS}")
else()
    # load VCPKG_OVERLAY_PORTS from the environment
    if(DEFINED ENV{VCPKG_OVERLAY_PORTS})
        message(STATUS "VCPKG_OVERLAY_PORTS found in the environment: ${VCPKG_OVERLAY_PORTS}")
    endif()
endif()
 
# check vcpkg_installed directory
if(NOT EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed")
    message(STATUS "vcpkg_installed directory not found, configuring vcpkg...")
    set(VCPKG_INSTALLED OFF)
else()
    set(VCPKG_INSTALLED ON)
    message(STATUS "vcpkg_installed directory found, skipping vcpkg configuration.")
endif()
 
set_vcpkg_config()