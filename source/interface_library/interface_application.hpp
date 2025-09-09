#pragma once
#include <memory>
#include <stop_token>

struct interface_platform;
struct interface_backend;
struct interface_application
{
    // Pure virtual destructor must have a definition, but not inline with the pure-specifier.
    virtual ~interface_application() = 0;

    virtual void set_platform(std::shared_ptr<interface_platform> platform) = 0;
    virtual void set_backend(std::shared_ptr<interface_backend> backend) = 0;

    virtual int initialize() = 0;
    virtual void render_loop(std::stop_token& token) = 0;
    virtual void destroy() = 0;
};

// Provide the required definition for the pure virtual destructor.
inline interface_application::~interface_application() = default;

// namespace std
// {
//     class stop_token;
// }
// struct interface_backend_wrapper;
// struct interface_application_wrapper
// {
//     struct impl_t;
//     impl_t* impl;
//     void (*delete_self)(impl_t* impl);
//     void (*set_window_title)(impl_t* impl, const char* title);
//     void (*set_window_size)(impl_t* impl, int width, int height);
//     void (*set_backend)(impl_t* impl, interface_backend_wrapper* backend);
//     int (*initialize)(impl_t* impl);
//     void (*render_loop)(impl_t* impl, std::stop_token* token);
//     void (*destroy)(impl_t* impl);
// };