#pragma once
#include <interface_frame.hpp>

#include <imgui.h>

struct frame_demo : public interface_frame
{

    int initialize() override { return 0; }
    void next_frame() override { ImGui::ShowDemoWindow(); }
    void destroy() override {}
};