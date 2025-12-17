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

#include <frame_demo.hpp>

#include <glad/glad.h>

// #include <opencv2/opencv.hpp>

#include <iostream>

#include "include/ok_color.h"

struct opengl_frame : public interface_frame
{
    int initialize() override { return 0; }
    void next_frame() override
    {
        ImGui::Begin("OpenGL Info");
        ImGui::End();

        ImGui::Begin("oklch color table");
        static float lightness = 0.6f;
        static float chroma = 1.0f;
        static std::array<ok_color::HSL, 3> colors = { ok_color::HSL{ 0.16f, chroma, lightness }, ok_color::HSL{ 0.40f, chroma, lightness }, ok_color::HSL{ 0.75f, chroma, lightness } };

        auto last_lightness = lightness;
        ImGui::SliderFloat("Lightness", &lightness, 0.0f, 1.0f);
        if (last_lightness != lightness)
            for (auto& color : colors)
                color.l = lightness;

        auto last_chroma = chroma;
        ImGui::SliderFloat("Chroma", &chroma, 0.0f, 1.0f);
        if (last_chroma != chroma)
            for (auto& color : colors)
                color.s = chroma;

        int index = 0;
        for (const auto& color : colors)
        {
            ImGui::Text("L: %.2f, S: %.2f, H: %.2f", color.l, color.s, color.h);
            auto last_hue = color.h;
            ImGui::SliderFloat(("Hue##" + std::to_string(index)).c_str(), &last_hue, 0.0f, 1.0f);
            if (last_hue != colors[index].h)
                colors[index].h = last_hue;
            auto lab = ok_color::okhsl_to_oklab(color);
            ImGui::Text("L: %.2f, A: %.2f, B: %.2f", lab.L, lab.a, lab.b);
            auto c = ok_color::okhsl_to_srgb(color);
            ImGui::ColorButton(std::to_string(index).c_str(), ImVec4(c.r, c.g, c.b, 1.0f), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker, ImVec2(50, 50));
            index++;
        }

        static ok_color::Lab lab = { 0.6f, 0.0f, 0.0f };
        auto last_l = lab.L;
        ImGui::SliderFloat("L", &last_l, 0.0f, 1.0f);
        if (last_l != lab.L)
            lab.L = last_l;
        auto last_a = lab.a;
        ImGui::SliderFloat("a", &last_a, -0.5f, 0.5f);
        if (last_a != lab.a)
            lab.a = last_a;
        auto last_b = lab.b;
        ImGui::SliderFloat("b", &last_b, -0.5f, 0.5f);
        if (last_b != lab.b)
            lab.b = last_b;

        ImGui::Text("L: %.2f, A: %.2f, B: %.2f", lab.L, lab.a, lab.b);
        auto c = ok_color::oklab_to_srgb(lab);
        ImGui::ColorButton("Lab", ImVec4(c.r, c.g, c.b, 1.0f), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker, ImVec2(50, 50));

        ImGui::End();
    }
    void destroy() override {}
};

int executor_build()
{
    auto exec = std::make_shared<executor>();
    auto app = std::make_shared<application>();
    auto platform = std::make_shared<platform_glfw>();
    auto backend = std::make_shared<backend_opengl>();
    auto frame = std::make_shared<opengl_frame>();
    app->set_frame(frame);

    app->set_backend(backend);
    app->set_platform(platform);
    return exec->execute(app);
}