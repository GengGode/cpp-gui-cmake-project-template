#pragma once
#include <memory>

enum class platform_processing_action
{
    continue_loop,
    skip,
    exit
};

struct interface_backend;
struct interface_platform
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_platform() = 0;

    virtual platform_processing_action process_events() = 0;

    virtual int initialize(std::shared_ptr<interface_backend> backend) = 0;
    virtual void new_frame() = 0;
    virtual void get_frame_buffer_size(int& width, int& height) = 0;
    virtual void swap_buffers() = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_platform::~interface_platform() = default;
