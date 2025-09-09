#pragma once
#include <memory>

struct interface_gui;
struct interface_executor
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_executor() = 0;

    /// @brief Initialize and render frame and destroy the renderer in the current thread.
    virtual int execute(std::shared_ptr<interface_gui> gui) = 0;
    /// @brief Initialize and render frame and destroy the renderer in a separate thread.
    virtual void async_execute(std::shared_ptr<interface_gui> gui) = 0;

    /// @brief Wait for the initialization to complete in async mode.
    virtual void sync_wait_initialization() = 0;
    /// @brief Wait for the destruction to complete in async mode.
    virtual void sync_wait_destruction() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_executor::~interface_executor() = default;