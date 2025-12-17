#pragma once
#include <functional>
#include <memory>

struct visualizer
{
    struct impl_t;
    std::unique_ptr<impl_t> impl;
    visualizer();
    ~visualizer();
    void initialize(bool sync_wait = false);
    void destroy();
    void main_render(std::function<void()> func);
    void main_enqueue(std::function<void()> func);
    void main_execute(std::function<void()> func);
    void wait_exit();
};
