#include "platform_glfw.hpp"
#include <interface_backend.hpp>

#include <global-register-error.hpp>

#include <GLFW/glfw3.h>

#include <imgui_impl_glfw.h>

platform_processing_action platform_glfw::process_events()
{
    glfwPollEvents();
    if (glfwWindowShouldClose(window.get()))
        return platform_processing_action::exit;
    if (glfwGetWindowAttrib(window.get(), GLFW_ICONIFIED) != 0)
    {
        ImGui_ImplGlfw_Sleep(10);
        return platform_processing_action::skip;
    }
    return platform_processing_action::continue_loop;
}

int platform_glfw::initialize(std::shared_ptr<interface_backend> backend)
{
    if (!glfwInit())
        return code_err("failed to initialize glfw");

    if (!backend)
        return code_err("backend is not set");
    auto glfw_init_impl = &ImGui_ImplGlfw_InitForOther;
    if (backend && backend->backend_type() == backend_impl_type::opengl)
        glfw_init_impl = &ImGui_ImplGlfw_InitForOpenGL;
    else if (backend && backend->backend_type() == backend_impl_type::vulkan)
        glfw_init_impl = &ImGui_ImplGlfw_InitForVulkan;

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* glfw_window = glfwCreateWindow((int)(1280), (int)(800), "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (glfw_window == nullptr)
        return code_err("failed to create glfw window");

    window = std::shared_ptr<GLFWwindow>(glfw_window, glfwDestroyWindow);
    if (glfw_init_impl && !glfw_init_impl(glfw_window, true))
    {
        window.reset();
        return code_err("failed to initialize backend for glfw, backend type = {}", static_cast<int>(backend->backend_type()));
    }
    glfwMakeContextCurrent(glfw_window);
    glfwShowWindow(glfw_window);
    return 0;
}
void platform_glfw::new_frame()
{
    ImGui_ImplGlfw_NewFrame();
}
void platform_glfw::get_frame_buffer_size(int& width, int& height)
{
    glfwGetFramebufferSize(window.get(), &width, &height);
}
void platform_glfw::swap_buffers()
{
    glfwSwapBuffers(window.get());
}

void platform_glfw::destroy()
{
    ImGui_ImplGlfw_Shutdown();
    if (window)
        window.reset();
    glfwTerminate();
}