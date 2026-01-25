#include <fmt/format.h>
#define RUNTIME_VISUALIZER_IMPLEMENTATION
#include <runtime-visualizer-interrupter.hpp>
#include <runtime-visualizer.hpp>

#include <runtime-visualizer-image_watcher.hpp>

#include <chrono>
#include <iostream>
#include <thread>
using namespace std::chrono_literals;

interrupter g_interrupter;
image_watcher g_image_watcher;

int main(int argc, char* argv[])
{
    std::ignore = argc;
    std::ignore = argv;
    std::locale::global(std::locale("zh_CN.UTF-8"));

    std::cout << "Hello, World! 测试中文" << std::endl;

    runtime_visualizer viz;
    viz.register_destroy([]() { g_interrupter.destroy(); });
    viz.initialize();
    viz.main_render([]() {
        ImGui::Begin("Hello, world!");
        ImGui::Text("This is some useful text.");
        if (ImGui::Button("Continue"))
            g_interrupter.continue_execution();
        ImGui::End();
        g_image_watcher.render();
    });
    cv::Mat test_image = cv::Mat::zeros(480, 640, CV_8UC3);
    cv::Mat test_image_empty;
    cv::Mat test_image_gray = cv::Mat::zeros(480, 640, CV_8UC1);
    cv::Mat test_image_float = cv::Mat::zeros(480, 640, CV_32FC1);

    g_image_watcher.watch_image("test_image", test_image, []() { std::cout << "Image updated!" << std::endl; });
    g_image_watcher.watch_image("test_image_empty", test_image_empty);
    g_image_watcher.watch_image("test_image_gray", test_image_gray);
    g_image_watcher.watch_image("test_image_float", test_image_float);

    g_interrupter.interrupt();
    std::cout << "step 1..." << std::endl;
    g_image_watcher.remove_watcher("test_image");

    viz.wait_exit();
    return 0;
}