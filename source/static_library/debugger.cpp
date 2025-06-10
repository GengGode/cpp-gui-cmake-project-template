
#include "debugger.hpp"
#include "watcher.hpp"
#include <future>
#include <latch>

#include "debugger_impl.hpp"

struct debugger::impl_t
{
    void* window = nullptr;
    std::future<void> main_thread;
    std::atomic_flag inited = ATOMIC_FLAG_INIT;
    std::atomic_flag running = ATOMIC_FLAG_INIT;
    std::atomic_flag destroyed = ATOMIC_FLAG_INIT;
    std::latch latch{ 2 }; // inited and destroyed

    std::shared_ptr<watcher> watcher;

    void initialize();
    void destroy();
};

debugger::debugger() : impl(std::make_unique<impl_t>()) {}

debugger::~debugger() {}

void debugger::initialize()
{
    if (useable.test())
        return;
    impl->initialize();
    useable.test_and_set();
}
void debugger::execute(bool auto_destroy)
{
    if (!useable.test())
        return;
    impl->latch.wait();
    if (auto_destroy && !impl->running.test())
        impl->destroy();
}

void debugger::destroy()
{
    if (!useable.test())
        return;
    impl->destroy();
    impl.reset();
    useable.clear();
}

bool debugger::is_useable()
{
    return useable.test();
}

std::shared_ptr<watcher> debugger::get_watcher()
{
    if (!is_useable())
        return nullptr;
    return impl->watcher;
}

void debugger::impl_t::initialize()
{
    if (inited.test())
        return;
    if (watcher == nullptr)
        watcher = std::make_shared<::watcher>();
    main_thread = std::async(std::launch::async, window_main_task, (GLFWwindow*)window, std::ref(*watcher), std::ref(latch), std::ref(inited), std::ref(running), std::ref(destroyed));
}
void debugger::impl_t::destroy()
{
    if (destroyed.test())
        return;
    running.clear();
    main_thread.wait();
    inited.clear();
    running.clear();
    destroyed.test_and_set();
}
