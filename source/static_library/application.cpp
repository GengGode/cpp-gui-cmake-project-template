#include "application.hpp"
#include <interface_backend.hpp>
#include <interface_frame.hpp>
#include <interface_platform.hpp>

#include <global-register-error.hpp>

#include <imgui.h>

void application::set_platform(std::shared_ptr<interface_platform> platform)
{
    this->platform = platform;
}
void application::set_backend(std::shared_ptr<interface_backend> backend)
{
    this->backend = backend;
}
void application::set_frame(std::shared_ptr<interface_frame> frame)
{
    this->frame = frame;
}
int application::initialize()
{
    if (!platform)
        return code_err("platform is nullptr");
    if (!backend)
        return code_err("backend is nullptr");

    ImGui::CreateContext();

    if (auto init_result = platform->initialize(backend); init_result != 0)
        return code_err("platform failed to initialize, init_result = {}", init_result);
    if (auto init_result = backend->initialize(platform); init_result != 0)
        return code_err("backend failed to initialize, init_result = {}", init_result);

    ImGui::StyleColorsLight();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

    if (frame)
    {
        if (auto init_result = frame->initialize(); init_result != 0)
            return code_err("frame failed to initialize, init_result = {}", init_result);
    }
    return 0;
}
void application::render_loop(std::stop_token& token)
{
    if (!platform || !backend)
        return; // Platform or backend is not set
    while (!token.stop_requested())
    {
        auto action = platform->process_events();
        if (action == platform_processing_action::exit)
            break;
        else if (action == platform_processing_action::skip)
            continue;

        // Rendering code would go here
        backend->new_frame();
        ImGui::NewFrame();
        // (Your ImGui rendering code would go here)
        if (frame)
            frame->next_frame();

        ImGui::Render();
        backend->render_draw_data();
    }
}
void application::destroy()
{
    if (frame)
        frame->destroy();
    if (backend)
        backend->destroy();
    if (platform)
        platform->destroy();

    ImGui::DestroyContext();
}