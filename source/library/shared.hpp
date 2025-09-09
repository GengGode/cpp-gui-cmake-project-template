#pragma once
#if defined(_WIN32)
    #ifdef cpp_gui_cmake_project_template_identifier_EXPORTS
        #define cpp_gui_cmake_project_template_identifier_API __declspec(dllexport)
    #else
        #define cpp_gui_cmake_project_template_identifier_API __declspec(dllimport)
    #endif
#else
    #ifdef cpp_gui_cmake_project_template_identifier_EXPORTS
        #define cpp_gui_cmake_project_template_identifier_API __attribute__((visibility("default")))
    #else
        #define cpp_gui_cmake_project_template_identifier_API
    #endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    cpp_gui_cmake_project_template_identifier_API const char* get_version();
    cpp_gui_cmake_project_template_identifier_API const char* error_code_info(int error_code);

    cpp_gui_cmake_project_template_identifier_API int executor_build();

#ifdef __cplusplus
}
#endif

#undef cpp_gui_cmake_project_template_identifier_API
