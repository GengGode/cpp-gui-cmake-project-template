#pragma once
#include <memory>

struct interface_backend;
struct interface_platform
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_platform() = 0;

    virtual void set_backend(std::shared_ptr<interface_backend> backend) = 0;

    virtual int initialize() = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_platform::~interface_platform() = default;
