#include "executor.hpp"
#include <interface_gui.hpp>

#include <global-register-error.hpp>

#define SPDLOG_NO_ATOMIC_LEVELS
#include <fmt/std.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

int executor::execute(std::shared_ptr<interface_gui> gui)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>("executor", spdlog::sinks_init_list{ console_sink, msvc_sink });
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [th-%-6t] [%s:%# %!] [%^%l%$] %v");

    SPDLOG_INFO("started");

    if (!gui)
    {
        initialization_latch.count_down();
        destruction_latch.count_down();
        return code_err("gui is nullptr");
    }

    // Initialization code
    if (auto init_result = gui->initialize(); init_result != 0)
    {
        initialization_latch.count_down();
        destruction_latch.count_down();
        return code_err("gui failed to initialize, init_result = {}", init_result);
    }
    initialization_latch.count_down();

    // Rendering loop
    auto token = stop_source.get_token();
    gui->render_loop(token);

    // Destruction code
    gui->destroy();
    destruction_latch.count_down();

    SPDLOG_INFO("stopped");
    logger->flush();
    spdlog::drop_all();
    spdlog::shutdown();
    return 0;
}

void executor::async_execute(std::shared_ptr<interface_gui> gui)
{
    worker_thread = std::jthread([this, gui]() { this->execute(gui); });
}

void executor::sync_wait_initialization()
{
    initialization_latch.wait();
}

void executor::sync_wait_destruction()
{
    destruction_latch.wait();
}