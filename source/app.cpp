#include <fmt/format.h>
#include <opencv2/opencv.hpp>

#define RUNTIME_VISUALIZER_IMPLEMENTATION
#include <runtime-visualizer.hpp>
// need to be included after runtime-visualizer.hpp
#include <runtime-visualizer-dock_builder.hpp>
#include <runtime-visualizer-gradient_editer.hpp>
#include <runtime-visualizer-image_watcher.hpp>
#include <runtime-visualizer-signal.hpp>

#include <implot.h>

#include <chrono>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;

struct button
{
    std::string label;
    std::function<void()> on_click;
    button(std::string label, std::function<void()> on_click) : label(std::move(label)), on_click(std::move(on_click)) {}

    void operator()() const
    {
        if (ImGui::Button(label.c_str()))
            on_click();
    }
};
#pragma once
#include <imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class menu_builder
{
public:
    using callback_t = std::function<void()>;

    menu_builder(menu_builder* parent = nullptr) : parent(parent) {}

    // 开始菜单栏
    menu_builder& begin_bar()
    {
        if (parent == nullptr)
        {
            if (ImGui::BeginMenuBar())
                active = true;
        }
        return *this;
    }

    // 菜单
    menu_builder& menu(const char* name)
    {
        bool opened = false;

        if (is_active())
            opened = ImGui::BeginMenu(name);

        auto child = std::make_unique<menu_builder>(this);
        child->active = opened;

        children.emplace_back(std::move(child));
        return *children.back();
    }

    // 普通项
    menu_builder& item(const char* name, callback_t cb = nullptr)
    {
        if (is_active())
        {
            if (ImGui::MenuItem(name))
            {
                if (cb)
                    cb();
            }
        }
        return *this;
    }

    // 带勾选
    menu_builder& toggle(const char* name, bool* value, callback_t cb = nullptr)
    {
        if (is_active())
        {
            if (ImGui::MenuItem(name, nullptr, value))
            {
                if (cb)
                    cb();
            }
        }
        return *this;
    }

    // 返回父节点
    menu_builder& done()
    {
        if (active)
            ImGui::EndMenu();

        return *parent;
    }

    // 结束菜单栏
    void end_bar()
    {
        if (parent == nullptr && active)
            ImGui::EndMenuBar();
    }

private:
    bool is_active() const { return parent ? parent->active && active : active; }

private:
    menu_builder* parent = nullptr;
    bool active = false;
    std::vector<std::unique_ptr<menu_builder>> children;
};
struct application
{
    runtime_visualizer visualizer;
    button save_button = button("Save", []() { std::cout << "Save button clicked!" << std::endl; });

    signal_ptr<std::string> drop_path_signal = signal<std::string>::create();
    signal_ptr<> signal_color_changed = signal<>::create();

    application()
    {

        signal_color_changed->connect([]() { std::cout << "Color changed signal emitted!" << std::endl; });
    }

    void render()
    {
        ImGuiID main_id = ImGui::GetID("Main DockSpace");

        if (ImGui::DockBuilderGetNode(main_id) == nullptr)
        {
            dock_builder(main_id).window("Main").dock_left(0.25, "Left Main").dock_down(0.25f, "Left Bottom").done().dock_up(0.25f, "Left Top").done().done().dock_right(0.25f, "Right");
        }
        // 在窗口中使用
        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar);
        static bool fullscreen = false;
        menu_builder()
            .begin_bar()
            .menu("File")
            .menu("File")
            .item("Open", [] { printf("Open\n"); })
            .item("Save", [] { printf("Save\n"); })
            .done()
            .item("Open", [] { printf("Open\n"); })
            .item("Save", [] { printf("Save\n"); })
            .done()
            .menu("View")
            .done()
            .menu("Options")
            .toggle("Fullscreen", &fullscreen, [] { printf("Toggle fullscreen\n"); })
            .done()
            .end_bar();

        ImGui::End();
        ImGui::DockSpaceOverViewport(main_id);

        ImGui::Begin("Main");
        ImGui::End();

        ImGui::Begin("Left Main");
        ImGui::End();

        ImGui::Begin("Left Bottom");
        ImGui::End();

        ImGui::Begin("Left Top");
        ImGui::End();

        ImGui::Begin("Right");
        ImGui::End();
    }
    int exec()
    {
        visualizer.register_initialize([&]() {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
            ImPlot::CreateContext();
        });
        visualizer.register_destroy([&]() { ImPlot::DestroyContext(); });
        visualizer.render_enqueue([&]() { signal_color_changed->emit(); });
        visualizer.register_render([&] { render(); });
        visualizer.initialize();
        visualizer.wait_exit();
        return 0;
    }
};

#include <opencv2/core/utils/logger.hpp>
int main(int argc, char* argv[])
{
    std::ignore = argc;
    std::ignore = argv;
    std::locale::global(std::locale("zh_CN.UTF-8"));
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    application app;
    return app.exec();
}
