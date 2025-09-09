#pragma once
#include <interface_application.hpp>

struct application : public interface_application
{
    void set_platform(std::shared_ptr<interface_platform> platform) override;
    void set_backend(std::shared_ptr<interface_backend> backend) override;

    int initialize() override;
    void render_loop(std::stop_token& token) override;
    void destroy() override;

private:
    std::shared_ptr<interface_platform> platform;
    std::shared_ptr<interface_backend> backend;
};