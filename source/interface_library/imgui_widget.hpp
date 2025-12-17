#pragma once
#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

struct imgui_widget
{
    imgui_widget(const std::string& title) : title(title) {}
    void add_child(std::shared_ptr<imgui_widget> widget) { children.push_back(widget); }
    virtual void render()
    {
        ImGui::Begin(title.c_str());
        for (auto& child : children)
            child->render();
        ImGui::End();
    }

private:
    std::string title;
    std::vector<std::shared_ptr<imgui_widget>> children;
};

struct imgui_text_widget : public imgui_widget
{
    imgui_text_widget(const std::string& text) : text(text) {}
    void render() override { ImGui::Text("%s", text.c_str()); }

private:
    std::string text;
};
