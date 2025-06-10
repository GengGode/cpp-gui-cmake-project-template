#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <parallel_container>

struct watcher::impl_t
{
    struct var
    {
        std::vector<int> render;
        std::mutex render_mutex;

        std::vector<int> write;
        std::mutex write_mutex;
        bool has_changed = false;
        void push(int value)
        {
            std::unique_lock lock(write_mutex);
            write.push_back(value);
            has_changed = true;
        }
        void sync()
        {
            std::scoped_lock lock(render_mutex, write_mutex);
            if (!has_changed)
                return;
            render = write;
            has_changed = false;
        }
    };
    // var ints_var;
    stdex::syncer<std::vector<int>> ints_var;
    std::vector<int> ints_render;
    std::vector<int> ints_writer;

    void render() { ints_var.try_sync(ints_render); }
    void async_watch(std::string name, int value)
    {
        ints_writer.push_back(value);
        ints_var.set(ints_writer);
    }
    void watch(std::string name, double value) {}
    void watch(std::string name, std::vector<int>& value) {}
    void watch(std::string name, std::vector<double>& value) {}
};