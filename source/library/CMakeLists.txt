add_library(cpp_gui_cmake_project_template SHARED)

if (MSVC)
    target_compile_options(cpp_gui_cmake_project_template
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
            # $<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t->
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>
            $<$<COMPILE_LANGUAGE:CXX>:/std:c++23preview>
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(cpp_gui_cmake_project_template
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
            $<$<COMPILE_LANGUAGE:CXX>:-std=c++2b>
            $<$<COMPILE_LANGUAGE:CXX>:-finput-charset=UTF-8>
            $<$<COMPILE_LANGUAGE:CXX>:-fexec-charset=UTF-8>
    )
endif()

target_compile_definitions(cpp_gui_cmake_project_template
    PRIVATE
        cpp_gui_cmake_project_template_identifier_EXPORTS
)

target_include_directories(cpp_gui_cmake_project_template
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_sources(cpp_gui_cmake_project_template
    PUBLIC
        shared.hpp
    PRIVATE
        shared.cpp
)

target_link_libraries(cpp_gui_cmake_project_template
    PUBLIC
        cpp_gui_cmake_project_template.static
)


find_package(fmt REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template
    PUBLIC
        fmt::fmt
)