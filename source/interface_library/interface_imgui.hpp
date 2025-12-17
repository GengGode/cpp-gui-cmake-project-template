#pragma once
#include <memory>

struct imgui_widget;
struct interface_imgui
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_imgui() = 0;

    virtual void add_widget(std::shared_ptr<imgui_widget> widget) = 0;

    virtual int initialize() = 0;
    virtual void render() = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_imgui::~interface_imgui() = default;
