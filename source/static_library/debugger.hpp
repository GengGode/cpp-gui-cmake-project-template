#pragma once
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>

class watcher;
class debugger
{
    struct impl_t;
    std::unique_ptr<impl_t> impl;
    std::atomic_flag useable = ATOMIC_FLAG_INIT;

public:
    debugger();
    ~debugger();
    void initialize();
    void execute(bool auto_destroy = true);
    void destroy();
    bool is_useable();
    std::shared_ptr<watcher> get_watcher();
};

class safe_debugger
{
    std::shared_mutex access_mutex;
    std::shared_ptr<debugger> impl;

public:
    safe_debugger() : impl(std::make_shared<debugger>()) {}
    safe_debugger(const std::shared_ptr<debugger>& impl) : impl(impl) {}
    safe_debugger(safe_debugger& other)
    {
        std::scoped_lock lock(other.access_mutex, access_mutex);
        impl = other.impl;
    }
    safe_debugger(safe_debugger&& other)
    {
        std::scoped_lock lock(other.access_mutex, access_mutex);
        impl = std::move(other.impl);
    }
    safe_debugger& operator=(safe_debugger& other)
    {
        if (this != &other)
        {
            std::scoped_lock lock(other.access_mutex, access_mutex);
            impl = other.impl;
        }
        return *this;
    }
    safe_debugger& operator=(safe_debugger&& other)
    {
        if (this != &other)
        {
            std::scoped_lock lock(other.access_mutex, access_mutex);
            impl = std::move(other.impl);
        }
        return *this;
    }
    ~safe_debugger() = default;
    void initialize()
    {
        std::shared_lock used(access_mutex);
        if (impl)
            impl->initialize();
    }
    void execute(bool auto_destroy = true)
    {
        std::shared_lock used(access_mutex);
        if (impl)
            impl->execute(auto_destroy);
    }
    void destroy()
    {
        std::shared_lock used(access_mutex);
        if (impl)
            impl->destroy();
    }
    bool is_useable()
    {
        std::shared_lock used(access_mutex);
        return impl && impl->is_useable();
    }
    std::shared_ptr<watcher> get_watcher()
    {
        std::shared_lock used(access_mutex);
        if (impl)
            return impl->get_watcher();
        return nullptr;
    }
};
class shared_debugger
{
    struct holder_t
    {
        std::shared_mutex access = {};
        std::shared_ptr<debugger> impl = {};
    };
    std::shared_ptr<holder_t> holder = {};

public:
    shared_debugger() : holder(std::make_shared<holder_t>()) { holder->impl = std::make_shared<debugger>(); }
    shared_debugger(const std::shared_ptr<debugger>& impl) : holder(std::make_shared<holder_t>()) { holder->impl = impl; }
    shared_debugger(const shared_debugger&) = default;
    shared_debugger& operator=(const shared_debugger&) = default;
    shared_debugger(shared_debugger&&) = default;
    shared_debugger& operator=(shared_debugger&&) = default;
    ~shared_debugger() = default;
    void initialize()
    {
        std::shared_lock used(holder->access);
        if (holder->impl)
            holder->impl->initialize();
    }
    void execute(bool auto_destroy = true)
    {
        std::shared_lock used(holder->access);
        if (holder->impl)
            holder->impl->execute(auto_destroy);
    }
    void destroy()
    {
        std::shared_lock used(holder->access);
        if (holder->impl)
            holder->impl->destroy();
    }
    bool is_useable()
    {
        std::shared_lock used(holder->access);
        return holder->impl && holder->impl->is_useable();
    }
    std::shared_ptr<watcher> get_watcher()
    {
        std::shared_lock used(holder->access);
        if (holder->impl)
            return holder->impl->get_watcher();
        return nullptr;
    }
};