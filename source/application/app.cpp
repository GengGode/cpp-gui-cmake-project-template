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

    int error_code = executor_build();
    if (error_code != 0)
        std::cerr << "Error: " << error_code_info(error_code) << std::endl;

    return 0;
}