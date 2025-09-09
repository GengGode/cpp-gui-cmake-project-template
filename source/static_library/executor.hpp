#pragma once
#include <interface_executor.hpp>

#include <atomic>
#include <latch>
#include <stop_token>
#include <thread>

class executor : public interface_executor
{
public:
    int execute(std::shared_ptr<interface_application> app) override;
    void async_execute(std::shared_ptr<interface_application> app) override;

    void sync_wait_initialization() override;
    void sync_wait_destruction() override;

private:
    std::latch initialization_latch{ 1 };
    std::latch destruction_latch{ 1 };
    std::jthread worker_thread;
    std::stop_source stop_source;
};