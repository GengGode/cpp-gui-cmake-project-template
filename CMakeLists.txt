cmake_minimum_required(VERSION 3.25)

include(cmake/vcpkg_env.cmake)

project(cpp_gui_cmake_project_template LANGUAGES CXX)

include(cmake/project_setting.cmake)

include(cmake/auto_increment_version.cmake)
auto_generate_version(${CMAKE_SOURCE_DIR}/.version ${CMAKE_SOURCE_DIR}/.build_version)

add_subdirectory(source)

if (BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

if (BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()