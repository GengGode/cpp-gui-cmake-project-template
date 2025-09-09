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
    PRIVATE
        executor.cpp
        application.cpp
        platform_glfw.cpp
        backend_opengl.cpp
)

target_link_libraries(cpp_gui_cmake_project_template.static
    PUBLIC
        cpp_gui_cmake_project_template.interface
)

find_package(global_utils CONFIG REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.static
    PUBLIC
        global_utils::global_utils
)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.static
    PUBLIC
        glfw
        glad::glad
)
if (WIN32)
    target_link_libraries(cpp_gui_cmake_project_template.static
        PRIVATE
            opengl32
    )
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(OpenGL_LIBRARY OpenGL)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    target_link_libraries(cpp_gui_cmake_project_template.static
        PRIVATE
            ${COCOA_LIBRARY}
            ${OpenGL_LIBRARY}
            ${IOKIT_LIBRARY}
            ${COREVIDEO_LIBRARY}
    )
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(cpp_gui_cmake_project_template.static
        PRIVATE
            ${OPENGL_gl_LIBRARY}
            ${X11_LIBRARIES}
    )
    target_include_directories(cpp_gui_cmake_project_template.static
        PRIVATE
            ${X11_INCLUDE_DIR}
    )
endif()

find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(OpenCV REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.static
    PUBLIC
        imgui::imgui
        implot::implot
        ${OpenCV_LIBS}
)
target_include_directories(cpp_gui_cmake_project_template.static
    PUBLIC
        ${OpenCV_INCLUDE_DIRS}
)