#include <fmt/format.h>
#define RUNTIME_VISUALIZER_IMPLEMENTATION
#include <runtime-visualizer.hpp>

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

    runtime_visualizer viz;
    viz.initialize();
    viz.wait_exit();
    return 0;
}