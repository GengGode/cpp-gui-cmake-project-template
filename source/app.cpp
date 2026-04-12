#include <fmt/format.h>
#include <opencv2/opencv.hpp>

#define RUNTIME_VISUALIZER_IMPLEMENTATION
#include <runtime-visualizer.hpp>
// need to be included after runtime-visualizer.hpp
#include <runtime-visualizer-dock_builder.hpp>
#include <runtime-visualizer-gradient_editer.hpp>
#include <runtime-visualizer-image_watcher.hpp>
#include <runtime-visualizer-signal.hpp>

#include <implot.h>

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
using namespace std::chrono_literals;

#include "generate_random_image.h"
#include "streamable.h"

struct application
{
    runtime_visualizer visualizer;
    image_watcher watcher;

    signal_ptr<std::string> drop_path_signal = signal<std::string>::create();
    signal_ptr<> signal_color_changed = signal<>::create();

    cv::Mat test_image = generate_random_image(400, 600);

    cv::Mat original_f;
    cv::Mat global_result;
    cv::Mat stream_result;
    cv::Mat diff_image;

    cv::Mat disp_original;
    cv::Mat disp_global;
    cv::Mat disp_stream;
    cv::Mat disp_diff;

    int blur_ksize = 101;
    int chunk_rows = 64;
    int context = 51;
    int blend_rows = 16;

    double psnr_global_vs_stream = 0;
    bool processed = false;

    application()
    {
        watcher.visitor("Test Image", test_image, true);
        signal_color_changed->connect([]() { std::cout << "Color changed signal emitted!" << std::endl; });
        drop_path_signal->connect([this](const std::string& path) {
            try
            {
                cv::Mat img = cv::imread(path);
                if (img.empty())
                    std::cerr << "Failed to load image: " << path << std::endl;
                else
                    std::cout << "Loaded image: " << path << " (" << img.cols << "x" << img.rows << ")" << std::endl;
                if (img.cols > 0 && img.rows > 0)
                {
                    static int img_count = 1;
                    std::string var_name = fmt::format("Dropped Image {}", img_count++);
                    visualizer.render_enqueue([this, var_name, img]() { watcher.visitor(var_name, img, true); });
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error loading image: " << e.what() << std::endl;
            }
        });
    }

    static double compute_psnr(const cv::Mat& a, const cv::Mat& b)
    {
        cv::Mat d;
        cv::absdiff(a, b, d);
        d = d.mul(d);
        cv::Scalar s = cv::mean(d);
        double mse = 0;
        for (int c = 0; c < a.channels(); ++c)
            mse += s[c];
        mse /= a.channels();
        if (mse < 1e-10)
            return 100.0;
        return 10.0 * std::log10(1.0 / mse);
    }

    void run_streaming_blur()
    {
        test_image.convertTo(original_f, CV_32F, 1.0 / 255.0);

        int ksize = blur_ksize | 1;

        auto process = [ksize](const cv::Mat& input) -> cv::Mat {
            cv::Mat output;
            cv::blur(input, output, cv::Size(ksize, ksize));
            return output;
        };

        global_result = process(original_f);

        auto dcs = make_delay_chunk_streaming(process, detail::default_blend, context, blend_rows);

        std::vector<cv::Mat> outputs;
        int rows = original_f.rows;
        for (int start = 0; start < rows; start += chunk_rows)
        {
            int end = std::min(start + chunk_rows, rows);
            cv::Mat chunk = original_f.rowRange(start, end).clone();
            auto out = dcs->feed(chunk);
            if (out.has_value())
                outputs.push_back(out.value());
        }
        auto tail = dcs->flush();
        if (tail.has_value())
            outputs.push_back(tail.value());

        if (!outputs.empty())
            cv::vconcat(outputs, stream_result);
        else
            stream_result = cv::Mat::zeros(original_f.size(), original_f.type());

        if (stream_result.rows != global_result.rows)
        {
            int target = global_result.rows;
            if (stream_result.rows > target)
                stream_result = stream_result.rowRange(0, target).clone();
            else
            {
                cv::Mat padded = cv::Mat::zeros(target, stream_result.cols, stream_result.type());
                stream_result.copyTo(padded.rowRange(0, stream_result.rows));
                stream_result = padded;
            }
        }

        cv::Mat raw_diff;
        cv::absdiff(global_result, stream_result, raw_diff);
        diff_image = raw_diff * 10.0f;

        psnr_global_vs_stream = compute_psnr(global_result, stream_result);
        processed = true;

        original_f.convertTo(disp_original, CV_8U, 255.0);
        global_result.convertTo(disp_global, CV_8U, 255.0);
        stream_result.convertTo(disp_stream, CV_8U, 255.0);
        diff_image.convertTo(disp_diff, CV_8U, 255.0);

        watcher.visitor("原图", disp_original, true);
        watcher.visitor("全局均值滤波", disp_global, true);
        watcher.visitor("流式均值滤波", disp_stream, true);
        watcher.visitor("差异图 x10", disp_diff, true);

        std::cout << fmt::format("PSNR 全局 vs 流式: {:.2f} dB", psnr_global_vs_stream) << std::endl;
    }

    void render()
    {
        ImGuiID main_id = ImGui::GetID("Main DockSpace");

        if (ImGui::DockBuilderGetNode(main_id) == nullptr)
        {
            dock_builder(main_id).window("图像监视器").dock_left(0.25, "流式均值滤波").dock_down(0.25f, "Left Bottom").done().dock_up(0.25f, "Left Top").done().done().dock_right(0.25f, "Right");
        }
        ImGui::DockSpaceOverViewport(main_id);

        watcher.render();

        ImGui::Begin("流式均值滤波");

        ImGui::Text("原图: %d x %d (%d 通道)", test_image.cols, test_image.rows, test_image.channels());

        ImGui::Separator();
        ImGui::Text("均值滤波参数");
        ImGui::SliderInt("核大小", &blur_ksize, 3, 301);
        if (blur_ksize % 2 == 0)
            blur_ksize++;

        ImGui::Separator();
        ImGui::Text("流式处理参数");
        ImGui::SliderInt("块行数 (chunk_rows)", &chunk_rows, 8, 256);
        ImGui::SliderInt("上下文行数 (context)", &context, 1, 200);
        ImGui::SliderInt("混合行数 (blend_rows)", &blend_rows, 0, context);
        if (blend_rows > context)
            blend_rows = context;

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "建议: context >= ksize/2 = %d", blur_ksize / 2);

        ImGui::Separator();
        if (ImGui::Button("执行对比", ImVec2(-1, 40)))
            run_streaming_blur();

        if (processed)
        {
            ImGui::Separator();
            ImGui::Text("对比结果");
            ImGui::BulletText("PSNR 全局 vs 流式: %.2f dB", psnr_global_vs_stream);
            if (psnr_global_vs_stream > 60)
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "几乎无差异");
            else if (psnr_global_vs_stream > 40)
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "轻微差异");
            else
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "明显差异 - 增大 context 或 blend_rows");
        }

        ImGui::End();
    }

    int exec()
    {
        visualizer.render_enqueue([&]() { signal_color_changed->emit(); });
        visualizer.register_initialize([&]() {
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            ImPlot::CreateContext();
        });
        visualizer.register_destroy([&]() {
            ImPlot::DestroyContext();
            watcher.destroy();
        });
        visualizer.register_drop([&](int count, const char** paths) {
            for (int i = 0; i < count; ++i)
                drop_path_signal->emit(paths[i]);
        });
        visualizer.register_render([&] { render(); });
        visualizer.initialize();
        visualizer.wait_exit();
        return 0;
    }
};

#include <opencv2/core/utils/logger.hpp>
int main(int argc, char* argv[])
{
    std::ignore = argc;
    std::ignore = argv;
    std::locale::global(std::locale("zh_CN.UTF-8"));
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

    application app;
    return app.exec();
}
