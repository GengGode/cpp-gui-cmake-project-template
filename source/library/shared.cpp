#include "shared.hpp"
#include <global-register-error.hpp>
#include <global-variables-pool.hpp>

const char* get_version()
{
    return cpp_gui_cmake_project_template_identifier_VERSION_STRING;
}
const char* error_code_info(int error_code)
{
    if (error_code < 0 || error_code >= static_cast<int>(global::error_invoker::locations.size()))
    {
        return "Unknown error code";
    }
    return global::error_invoker::locations[error_code].error_msg.c_str();
}

#include <application.hpp>
#include <backend_opengl.hpp>
#include <executor.hpp>
#include <platform_glfw.hpp>
int executor_build()
{
    auto exec = std::make_shared<executor>();
    auto app = std::make_shared<application>();
    auto platform = std::make_shared<platform_glfw>();
    auto backend = std::make_shared<backend_opengl>();

    app->set_backend(backend);
    app->set_platform(platform);
    return exec->execute(app);
}