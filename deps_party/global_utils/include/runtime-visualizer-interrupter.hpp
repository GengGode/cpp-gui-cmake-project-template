#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>

class interrupter
{
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> destroyed = true;
    std::atomic<bool> interrupted = false;

public:
    void destroy()
    {
        destroyed = false;
        continue_execution();
    }
    void interrupt()
    {
        if (!destroyed)
            return;
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return interrupted || !destroyed; });
        interrupted = false;
    }
    void continue_execution()
    {
        interrupted = true;
        cv.notify_all();
    }
};
