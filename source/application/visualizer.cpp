#include "visualizer.h"

#include <glad/glad.h>
// glad must be included before glfw
#include <GLFW/glfw3.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <spdlog/spdlog.h>
#if __has_include(<tbb/concurrent_queue.h>)
    #include <tbb/concurrent_queue.h>
template <typename T> using safe_queue = tbb::concurrent_queue<T>;
#else
    #include <concurrent_queue.h>
template <typename T> using safe_queue = Concurrency::concurrent_queue<T>;
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

struct visualizer::impl_t
{
    safe_queue<std::function<void()>> main_queue = {};

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
        // ImGuiTexInspect::CreateContext();
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
        io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msyh.ttc", 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        return nullptr;
    }

    void render_loop(safe_queue<std::function<void()>>& queue)
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
            while (queue.try_pop(task) && task)
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
        // ImGuiTexInspect::ImplOpenGl3_Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        if (window)
            window.reset();
        glfwTerminate();
        // ImGuiTexInspect::DestroyContext(nullptr);
        ImGui::DestroyContext();
    }
    void render_window()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Open", "Ctrl+O")) { /* do something */ }
                if (ImGui::MenuItem("Save", "Ctrl+S")) { /* do something */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                    glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::SetNextWindowPos(ImVec2(0, 20), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize - ImVec2(0, 20), ImGuiCond_Always);
        ImGui::Begin("##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        {
            std::lock_guard<std::mutex> lock(main_render_mutex);
            if (main_render_func)
                main_render_func();
        }

        ImGui::End();
    }
};

visualizer::visualizer() : impl(std::make_unique<impl_t>()) {}
visualizer::~visualizer()
{
    destroy();
}
void visualizer::initialize(bool sync_wait)
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
            if (latch)
                latch->count_down();
            return;
        }
        if (latch)
            latch->count_down();
        impl->render_loop(impl->main_queue);
        impl->render_destroy();
        impl->running = false;
    });

    if (latch)
        latch->wait();
}
void visualizer::destroy()
{
    if (impl->running)
        impl->running = false;
    if (impl->render_thread.joinable())
        impl->render_thread.join();
}
void visualizer::main_render(std::function<void()> func)
{
    std::lock_guard<std::mutex> lock(impl->main_render_mutex);
    impl->main_render_func = func;
}
void visualizer::main_enqueue(std::function<void()> func)
{
    impl->main_queue.push(func);
}
void visualizer::main_execute(std::function<void()> func)
{
    std::latch promise(1);
    impl->main_queue.push([&promise, func]() {
        func();
        promise.count_down();
    });
    promise.wait();
}
void visualizer::wait_exit()
{
    if (impl->render_thread.joinable())
        impl->render_thread.join();
}
