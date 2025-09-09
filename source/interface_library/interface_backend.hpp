#pragma once
#include <memory>

struct interface_backend
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_backend() = 0;

    virtual int initialize() = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_backend::~interface_backend() = default;
