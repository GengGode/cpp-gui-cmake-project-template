#pragma once
#include <memory>
#include <stop_token>

struct interface_platform;
struct interface_backend;
struct interface_frame;
struct interface_application
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_application() = 0;

    virtual void set_platform(std::shared_ptr<interface_platform> platform) = 0;
    virtual void set_backend(std::shared_ptr<interface_backend> backend) = 0;
    virtual void set_frame(std::shared_ptr<interface_frame> frame) = 0;

    virtual int initialize() = 0;
    virtual void render_loop(std::stop_token& token) = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_application::~interface_application() = default;
