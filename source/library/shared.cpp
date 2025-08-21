#include "shared.hpp"

#include <debugger.hpp>
#include <static.hpp>

int test_shared_regerr()
{
    return code_err("测试错误: {}", "shared test info");
}

const char* get_version()
{
    return "1.0.0";
}
const char* error_code_info(int error_code)
{
    if (error_code < 0 || error_code >= static_cast<int>(global::error_invoker::locations.size()))
    {
        return "Unknown error code";
    }
    return global::error_invoker::locations[error_code].error_msg.c_str();
}

#include <fmt/format.h>
#include <iostream>
int test()
{
    for (auto& errdef : global::error_invoker::locations)
        std::cout << fmt::format("Error: {}:{}:{}: {}", errdef.path, errdef.line, errdef.col, errdef.error_msg) << std::endl;
    return code_err("测试错误: {}", "shared test info");
}
static safe_debugger debugger;
int debugger_initialize()
{
    debugger.initialize();
    return 0;
}
int debugger_execute(bool auto_destroy)
{
    if (debugger.is_useable() == false)
        return code_err("Debugger is not usable");
    debugger.execute(auto_destroy);
    return 0;
}
int debugger_destroy()
{
    debugger.destroy();
    return 0;
}