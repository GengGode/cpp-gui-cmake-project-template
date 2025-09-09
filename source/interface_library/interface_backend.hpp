#pragma once
#include <memory>

enum class backend_impl_type
{
    other,
    opengl,
    vulkan,
    directx11,
    directx12,
    metal
};

struct interface_platform;
struct interface_backend
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_backend() = 0;

    virtual backend_impl_type backend_type() const = 0;

    virtual int initialize(std::shared_ptr<interface_platform> platform) = 0;
    virtual void new_frame() = 0;
    virtual void render_draw_data() = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_backend::~interface_backend() = default;
