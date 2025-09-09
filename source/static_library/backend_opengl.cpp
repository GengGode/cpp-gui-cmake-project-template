#include <backend_opengl.hpp>
#include <interface_platform.hpp>

#include <global-register-error.hpp>

#include <glad/glad.h>

#include <imgui_impl_opengl3.h>

backend_impl_type backend_opengl::backend_type() const
{
    return backend_impl_type::opengl;
}
int backend_opengl::initialize(std::shared_ptr<interface_platform> platform)
{
    if (!platform)
        return code_err("platform is nullptr");
    this->platform = platform;
    if (!gladLoadGL())
        return code_err("failed to initialize glad");
    auto glsl_version = "#version 130";
    if (!ImGui_ImplOpenGL3_Init(glsl_version))
        return code_err("failed to initialize ImGui OpenGL3 backend");
    return 0;
}
void backend_opengl::new_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    platform->new_frame();
}
void backend_opengl::render_draw_data()
{
    int width = 0, height = 0;
    platform->get_frame_buffer_size(width, height);
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    platform->swap_buffers();
}
void backend_opengl::destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
}