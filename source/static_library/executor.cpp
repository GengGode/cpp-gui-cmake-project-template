#include "executor.hpp"
#include <interface_application.hpp>

#include <global-register-error.hpp>

#define SPDLOG_NO_ATOMIC_LEVELS
#include <fmt/std.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

int executor::execute(std::shared_ptr<interface_application> app)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("executor", spdlog::sinks_init_list{ console_sink, msvc_sink });
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [th-%-6t] [%s:%# %!] [%^%l%$] %v");

    SPDLOG_INFO("started");

    if (!app)
    {
        initialization_latch.count_down();
        destruction_latch.count_down();
        return code_err("app is nullptr");
    }

    // Initialization code
    if (auto init_result = app->initialize(); init_result != 0)
    {
        initialization_latch.count_down();
        destruction_latch.count_down();
        return code_err("app failed to initialize, init_result = {}", init_result);
    }
    initialization_latch.count_down();

    // Rendering loop
    auto token = stop_source.get_token();
    app->render_loop(token);

    // Destruction code
    app->destroy();
    destruction_latch.count_down();

    SPDLOG_INFO("stopped");
    logger->flush();
    spdlog::drop_all();
    spdlog::shutdown();
    return 0;
}

void executor::async_execute(std::shared_ptr<interface_application> app)
{
    worker_thread = std::jthread([this, app]() { this->execute(app); });
}

void executor::sync_wait_initialization()
{
    initialization_latch.wait();
}

void executor::sync_wait_destruction()
{
    destruction_latch.wait();
}