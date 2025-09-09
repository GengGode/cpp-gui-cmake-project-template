#pragma once
#include <interface_backend.hpp>

struct backend_opengl : public interface_backend
{
    backend_impl_type backend_type() const override;

    int initialize(std::shared_ptr<interface_platform> platform) override;
    void new_frame() override;
    void render_draw_data() override;
    void destroy() override;

private:
    std::shared_ptr<interface_platform> platform;
};
