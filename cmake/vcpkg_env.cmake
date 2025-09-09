# configure the vcpkg toolchain file
macro(set_vcpkg_config)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    set(VCPKG_MANIFEST_MODE ON CACHE BOOL "Manifest mode")  
    set(VCPKG_MANIFEST_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "Manifest directory")
    set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET} CACHE STRING "Vcpkg target triplet")
    set(VCPKG_MANIFEST_INSTALL ON CACHE BOOL "Enable manifest install")
    set(VCPKG_APPLOCAL_DEPS ON CACHE BOOL "Enable applocal deps")
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
    
set_vcpkg_config()
