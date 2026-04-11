#pragma once

#include <imgui.h>

#include <memory>

class dock_builder
{
public:
    dock_builder(ImGuiID parent_node_id, dock_builder* parent = nullptr) : parent_node_id{ parent_node_id }, parent{ parent } {}

    dock_builder& window(const char* name)
    {
        if (parent == nullptr)
        {
            ImGui::DockBuilderAddNode(parent_node_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(parent_node_id, ImGui::GetMainViewport()->Size);
        }
        ImGui::DockBuilderDockWindow(name, parent_node_id);
        return *this;
    }
    dock_builder& dock_left(float size, const char* name = nullptr)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(parent_node_id, ImGuiDir_Left, size, nullptr, &parent_node_id);
        left = std::make_unique<dock_builder>(dock_id, this);
        if (name)
            left->window(name);
        return *left;
    }

    dock_builder& dock_right(float size, const char* name = nullptr)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(parent_node_id, ImGuiDir_Right, size, nullptr, &parent_node_id);
        right = std::make_unique<dock_builder>(dock_id, this);
        if (name)
            right->window(name);
        return *right;
    }

    dock_builder& dock_up(float size, const char* name = nullptr)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(parent_node_id, ImGuiDir_Up, size, nullptr, &parent_node_id);
        up = std::make_unique<dock_builder>(dock_id, this);
        if (name)
            up->window(name);
        return *up;
    }

    dock_builder& dock_down(float size, const char* name = nullptr)
    {
        ImGuiID dock_id = ImGui::DockBuilderSplitNode(parent_node_id, ImGuiDir_Down, size, nullptr, &parent_node_id);
        down = std::make_unique<dock_builder>(dock_id, this);
        if (name)
            down->window(name);
        return *down;
    }

    dock_builder& done()
    {
        assert(parent && "dock_builder has no parent");
        return *parent;
    }

private:
    dock_builder* parent;
    ImGuiID parent_node_id;
    std::unique_ptr<dock_builder> left;
    std::unique_ptr<dock_builder> right;
    std::unique_ptr<dock_builder> up;
    std::unique_ptr<dock_builder> down;
};
