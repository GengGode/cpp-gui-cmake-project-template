#pragma once
#include <interface_platform.hpp>

struct GLFWwindow;
struct platform_glfw : public interface_platform
{
    platform_processing_action process_events() override;
    int initialize(std::shared_ptr<interface_backend> backend) override;
    void new_frame() override;
    void get_frame_buffer_size(int& width, int& height) override;
    void swap_buffers() override;
    void destroy() override;

private:
    std::shared_ptr<GLFWwindow> window;
};