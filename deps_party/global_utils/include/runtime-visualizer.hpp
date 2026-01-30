#pragma once
// Copyright(c) 2025 Geng Gode
//
// need C++17 or higher and dependencies: GLFW, GLAD, ImGui, TBB, spdlog

#include <functional>
#include <memory>

struct runtime_visualizer
{
    struct impl_t;
    std::unique_ptr<impl_t> impl;
    runtime_visualizer();
    ~runtime_visualizer();
    void initialize(bool sync_wait = false);
    void destroy();
    void register_initialize(std::function<void()> func);
    void register_destroy(std::function<void()> func);
    void main_render(std::function<void()> func);
    void main_enqueue(std::function<void()> func);
    void main_execute(std::function<void()> func);
    void wait_exit();
};

#ifdef RUNTIME_VISUALIZER_IMPLEMENTATION
    #if __has_include(<glad/glad.h>)
        #include <glad/glad.h>
    #else
        #error "<glad/glad.h> is required for runtime_visualizer implementation"
    #endif
    #if __has_include(<GLFW/glfw3.h>)
        // glad must be included before glfw
        #include <GLFW/glfw3.h>
    #else
        #error "<GLFW/glfw3.h> is required for runtime_visualizer implementation"
    #endif
    #if __has_include(<imgui.h>)
        #define IMGUI_DEFINE_MATH_OPERATORS
        #include <imgui.h>
        #include <imgui_impl_glfw.h>
        #include <imgui_impl_opengl3.h>
        #include <imgui_internal.h>
    #else
        #error "<imgui.h> is required for runtime_visualizer implementation"
    #endif
    #if __has_include(<tbb/concurrent_queue.h>)
        #include <tbb/concurrent_queue.h>
    #else
        #error "<tbb/concurrent_queue.h> is required for runtime_visualizer implementation"
    #endif
    #if __has_include(<spdlog/spdlog.h>) && RUNTIME_VISUALIZER_ENABLE_LOGGING
        #include <spdlog/spdlog.h>
    #else
        #define SPDLOG_ERROR(...) ((void)0)
        #define SPDLOG_INFO(...) ((void)0)
        #if RUNTIME_VISUALIZER_ENABLE_LOGGING
            #warning "<spdlog/spdlog.h> not found, logging disabled in runtime_visualizer implementation"
        #endif
    #endif
    #if defined(_WIN32) || defined(_WIN64)
        #ifndef WIN32_LEAN_AND_MEAN
            #define WIN32_LEAN_AND_MEAN
        #endif
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #include <windows.h>
        #undef WIN32_LEAN_AND_MEAN
        #undef NOMINMAX
        #define set_current_thread_description(description) SetThreadDescription(GetCurrentThread(), L##description)
    #else
        #define set_current_thread_description(description)
    #endif

    #include <atomic>
    #include <latch>
    #include <mutex>
    #include <thread>

struct runtime_visualizer::impl_t
{
    tbb::concurrent_queue<std::function<void()>> main_queue = {};
    tbb::concurrent_queue<std::function<void()>> initialize_queue = {};
    tbb::concurrent_queue<std::function<void()>> destroy_queue = {};

    std::function<void()> main_render_func = {};
    std::mutex main_render_mutex = {};

    std::thread render_thread = {};
    std::shared_ptr<GLFWwindow> window = {};
    std::atomic<bool> running = false;
    std::atomic<bool> ready = false;

    const char* render_initialize()
    {
        glfwSetErrorCallback([](int error, const char* desc) { SPDLOG_ERROR("GLFW Error {}: {}", error, desc); });
        if (!glfwInit())
            return "Failed to initialize GLFW";

        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* glfw_window = glfwCreateWindow((int)(1280), (int)(800), "Visual", nullptr, nullptr);
        if (glfw_window == nullptr)
            return "Failed to create GLFW window";

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        window = std::shared_ptr<GLFWwindow>(glfw_window, glfwDestroyWindow);
        if (!ImGui_ImplGlfw_InitForOpenGL(glfw_window, true))
        {
            window.reset();
            return "Failed to initialize ImGui for GLFW";
        }
        glfwMakeContextCurrent(glfw_window);
        glfwShowWindow(glfw_window);
        glfwSwapInterval(1);

        if (!gladLoadGL())
            return "Failed to initialize GLAD";
        auto glsl_version = "#version 460";
        if (!ImGui_ImplOpenGL3_Init(glsl_version))
            return "Failed to initialize ImGui for OpenGL3";
        const GLubyte* version = glGetString(GL_VERSION);
        SPDLOG_INFO("OpenGL Version: {}", (const char*)version);

        ImGui::StyleColorsLight();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.IniFilename = nullptr;                             // disable imgui.ini
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());

        std::function<void()> task;
        while (initialize_queue.try_pop(task) && task)
            task();
        return nullptr;
    }

    void render_loop()
    {
        while (running)
        {
            glfwPollEvents();
            if (glfwWindowShouldClose(window.get()))
                break;
            if (glfwGetWindowAttrib(window.get(), GLFW_ICONIFIED) != 0)
            {
                ImGui_ImplGlfw_Sleep(10);
                continue;
            }

            std::function<void()> task;
            while (main_queue.try_pop(task) && task)
                task();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            render_window();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window.get(), &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window.get());
        }
    }

    void render_destroy()
    {
        std::function<void()> task;
        while (destroy_queue.try_pop(task) && task)
            task();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        if (window)
            window.reset();
        glfwTerminate();
        ImGui::DestroyContext();
    }
    void render_window()
    {
        std::lock_guard<std::mutex> lock(main_render_mutex);
        if (main_render_func)
            main_render_func();
    }
};

runtime_visualizer::runtime_visualizer()
{
    impl = std::make_unique<impl_t>();
}
runtime_visualizer::~runtime_visualizer()
{
    destroy();
}
void runtime_visualizer::initialize(bool sync_wait)
{
    if (impl->running)
        return;

    auto latch = sync_wait ? std::make_shared<std::latch>(1) : nullptr;

    impl->render_thread = std::thread([this, latch]() {
        impl->running = true;
        set_current_thread_description("User Visualization Thread");
        if (auto err = impl->render_initialize(); err != nullptr)
        {
            SPDLOG_ERROR("Visualization initialization error: {}", err);
            impl->running = false;
            latch ? latch->count_down() : void();
            return;
        }
        latch ? latch->count_down() : void();
        impl->render_loop();
        impl->render_destroy();
        impl->running = false;
    });
    latch ? latch->wait() : void();
}
void runtime_visualizer::destroy()
{
    if (impl->running)
        impl->running = false;
    if (impl->render_thread.joinable())
        impl->render_thread.join();
}
void runtime_visualizer::register_initialize(std::function<void()> func)
{
    impl->initialize_queue.push(func);
}
void runtime_visualizer::register_destroy(std::function<void()> func)
{
    impl->destroy_queue.push(func);
}
void runtime_visualizer::main_render(std::function<void()> func)
{
    std::lock_guard<std::mutex> lock(impl->main_render_mutex);
    impl->main_render_func = func;
}
void runtime_visualizer::main_enqueue(std::function<void()> func)
{
    impl->main_queue.push(func);
}
void runtime_visualizer::main_execute(std::function<void()> func)
{
    std::latch promise(1);
    impl->main_queue.push([&promise, func]() {
        func();
        promise.count_down();
    });
    promise.wait();
}
void runtime_visualizer::wait_exit()
{
    if (impl->render_thread.joinable())
        impl->render_thread.join();
}
#endif