#pragma once

struct interface_frame
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_frame() = 0;

    virtual int initialize() = 0;
    virtual void next_frame() = 0;
    virtual void destroy() = 0;
};
// Provide the required definition for the pure virtual destructor.
inline interface_frame::~interface_frame() = default;