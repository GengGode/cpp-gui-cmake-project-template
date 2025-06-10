add_library(cpp_gui_cmake_project_template.static STATIC)

if (MSVC)
    target_compile_options(cpp_gui_cmake_project_template.static
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
            # $<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t->
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>
            $<$<COMPILE_LANGUAGE:CXX>:/std:c++23preview>
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(cpp_gui_cmake_project_template.static
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
            $<$<COMPILE_LANGUAGE:CXX>:-std=c++2b>
            $<$<COMPILE_LANGUAGE:CXX>:-finput-charset=UTF-8>
            $<$<COMPILE_LANGUAGE:CXX>:-fexec-charset=UTF-8>
    )
endif()

target_include_directories(cpp_gui_cmake_project_template.static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_sources(cpp_gui_cmake_project_template.static
    PUBLIC
        static.cpp
        static.hpp
        debugger.hpp
        watcher.hpp
    PRIVATE
        debugger.cpp
        watcher.cpp
)

target_link_libraries(cpp_gui_cmake_project_template.static
    PUBLIC
        cpp_gui_cmake_project_template.interface
)

find_package(fmt REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.static
    PUBLIC
        fmt::fmt
        imgui::imgui
        implot::implot
        ${OpenCV_LIBS}
)
target_include_directories(cpp_gui_cmake_project_template.static
    PUBLIC
        ${OpenCV_INCLUDE_DIRS}
)