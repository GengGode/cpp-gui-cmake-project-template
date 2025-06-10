#include <fmt/format.h>
#include <shared.hpp>

#include <iostream>
#include <thread>

#include <chrono>
using namespace std::chrono_literals;

int main(int argc, char* argv[])
{
    std::ignore = argc;
    std::ignore = argv;
    std::locale::global(std::locale("zh_CN.UTF-8"));

    std::cout << "Hello, World! 测试中文" << std::endl;
    std::cout << "Version: " << get_version() << std::endl;

    int error_code = test();

    if (error_code != 0)
        std::cerr << "Error occurred: " << error_code_info(error_code) << std::endl;

    debugger_initialize();
    std::cout << "Debugger initialized." << std::endl;

    // std::this_thread::sleep_for(2s); // Simulate some processing time
    // std::cout << "Processing complete." << std::endl;

    debugger_execute(true);

    // debugger_destroy();
    // std::cout << "Debugger destroyed." << std::endl;

    return 0;
}