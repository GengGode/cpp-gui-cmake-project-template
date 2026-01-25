add_executable(cpp_gui_cmake_project_template.app)

if (MSVC)
    target_compile_options(cpp_gui_cmake_project_template.app
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
            # $<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t->
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>
            $<$<COMPILE_LANGUAGE:CXX>:/std:c++23preview>
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(cpp_gui_cmake_project_template.app
        PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall>
            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
            $<$<COMPILE_LANGUAGE:CXX>:-std=c++2b>
            $<$<COMPILE_LANGUAGE:CXX>:-finput-charset=UTF-8>
            $<$<COMPILE_LANGUAGE:CXX>:-fexec-charset=UTF-8>
    )
endif()


target_include_directories(cpp_gui_cmake_project_template.app
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_sources(cpp_gui_cmake_project_template.app
    PRIVATE
        app.cpp
)

find_package(global_utils CONFIG REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.app
    PRIVATE
        global_utils::global_utils
)

find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.app PRIVATE 
        $<$<PLATFORM_ID:Windows>:opengl32>
        glfw
        glm::glm
        glad::glad
        imgui::imgui
        implot::implot
        TBB::tbb
        fmt::fmt-header-only
        spdlog::spdlog_header_only
)
find_package(OpenCV REQUIRED)
target_link_libraries(cpp_gui_cmake_project_template.app PRIVATE
        opencv_core
        opencv_imgproc
)
