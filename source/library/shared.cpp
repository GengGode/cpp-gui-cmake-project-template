#include "shared.hpp"
#include <global-register-error.hpp>
#include <global-variables-pool.hpp>

const char* get_version()
{
    return cpp_gui_cmake_project_template_identifier_VERSION_STRING;
}
const char* error_code_info(int error_code)
{
    if (error_code < 0 || error_code >= static_cast<int>(global::error_invoker::locations.size()))
    {
        return "Unknown error code";
    }
    return global::error_invoker::locations[error_code].error_msg.c_str();
}

#include <application.hpp>
#include <backend_opengl.hpp>
#include <executor.hpp>
#include <platform_glfw.hpp>

#include <frame_demo.hpp>

#include <glad/glad.h>

// #include <opencv2/opencv.hpp>

#include <iostream>

#include "include/ok_color.h"

// HSL 色相插值辅助函数，处理色相的循环特性
float interpolate_hue(float h1, float h2, float t)
{
    // 计算两个色相之间的最短路径
    float diff = h2 - h1;
    if (diff > 0.5f)
        diff -= 1.0f;
    else if (diff < -0.5f)
        diff += 1.0f;

    float result = h1 + diff * t;
    // 确保结果在 [0, 1) 范围内
    if (result < 0.0f)
        result += 1.0f;
    else if (result >= 1.0f)
        result -= 1.0f;
    return result;
}

// HSL 颜色插值
ok_color::HSL interpolate_hsl(const ok_color::HSL& c1, const ok_color::HSL& c2, float t)
{
    return { interpolate_hue(c1.h, c2.h, t), c1.s + (c2.s - c1.s) * t, c1.l + (c2.l - c1.l) * t };
}

// 生成一维颜色映射表（保留用于兼容）
std::vector<ok_color::HSL> generate_color_map(const std::map<int, ok_color::HSL>& colors, const std::vector<std::pair<float, int>>& colors_map, int num_samples)
{
    if (colors_map.empty() || num_samples <= 0)
        return {};

    // 找到值的范围
    float min_value = colors_map.front().first;
    float max_value = colors_map.back().first;
    float value_range = max_value - min_value;

    if (value_range <= 0.0f)
    {
        // 如果所有值相同，返回单一颜色
        auto it = colors.find(colors_map.front().second);
        if (it != colors.end())
            return std::vector<ok_color::HSL>(num_samples, it->second);
        return {};
    }

    std::vector<ok_color::HSL> result;
    result.reserve(num_samples);

    // 生成 num_samples 个均匀分布的值
    for (int i = 0; i < num_samples; ++i)
    {
        float value = min_value + (value_range * i) / (num_samples - 1.0f);

        // 找到 value 在 colors_map 中的位置
        // 找到第一个大于等于 value 的位置
        auto it = std::lower_bound(colors_map.begin(), colors_map.end(), value, [](const std::pair<float, int>& p, float v) { return p.first < v; });

        ok_color::HSL color;

        if (it == colors_map.begin())
        {
            // 值小于等于第一个关键点
            auto color_it = colors.find(colors_map.front().second);
            if (color_it != colors.end())
                color = color_it->second;
        }
        else if (it == colors_map.end())
        {
            // 值大于等于最后一个关键点
            auto color_it = colors.find(colors_map.back().second);
            if (color_it != colors.end())
                color = color_it->second;
        }
        else
        {
            // 值在两个关键点之间，进行插值
            auto prev_it = std::prev(it);
            float prev_value = prev_it->first;
            float next_value = it->first;
            int prev_index = prev_it->second;
            int next_index = it->second;

            // 计算插值系数
            float t = (value - prev_value) / (next_value - prev_value);

            // 获取两个颜色
            auto prev_color_it = colors.find(prev_index);
            auto next_color_it = colors.find(next_index);

            if (prev_color_it != colors.end() && next_color_it != colors.end())
            {
                color = interpolate_hsl(prev_color_it->second, next_color_it->second, t);
            }
            else if (prev_color_it != colors.end())
            {
                color = prev_color_it->second;
            }
            else if (next_color_it != colors.end())
            {
                color = next_color_it->second;
            }
        }

        result.push_back(color);
    }

    return result;
}

// 生成二维颜色映射表
// 列方向：使用统一的亮度控制点，每一列代表一个亮度级别（从暗到亮）
// 行方向：使用一维颜色表，每一行代表一个颜色
// colors: 颜色索引到HSL的映射
// colors_map: 一维颜色表的控制点映射
// unified_lightness_controls: 统一的亮度控制点，格式为 {亮度值, 位置(0-1)}，所有列共用
// num_cols: 列数（亮度级别的采样数）
// num_rows: 行数（一维颜色表的采样数）
std::vector<std::vector<ok_color::HSL>> generate_2d_color_map(const std::map<int, ok_color::HSL>& colors, const std::vector<std::pair<float, int>>& colors_map,
                                                              const std::vector<std::pair<float, float>>& unified_lightness_controls, int num_cols, int num_rows)
{
    if (colors_map.empty() || num_cols <= 0 || num_rows <= 0)
        return {};

    // 首先生成一维颜色表（行方向）
    std::vector<ok_color::HSL> one_d_colors = generate_color_map(colors, colors_map, num_rows);
    if (one_d_colors.empty())
        return {};

    // 确保统一的亮度控制点按位置排序
    auto sorted_lightness_controls = unified_lightness_controls;
    std::sort(sorted_lightness_controls.begin(), sorted_lightness_controls.end(), [](const std::pair<float, float>& a, const std::pair<float, float>& b) { return a.second < b.second; });

    // 如果亮度控制点为空，使用默认值（从暗到亮）
    if (sorted_lightness_controls.empty())
    {
        sorted_lightness_controls = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
    }

    std::vector<std::vector<ok_color::HSL>> result;
    result.reserve(num_rows);

    // 生成每一行（不同的颜色）
    for (int row = 0; row < num_rows; ++row)
    {
        // 获取该行的基础颜色（从一维颜色表中获取）
        ok_color::HSL base_color = one_d_colors[row];

        // 生成该行的所有列（不同的亮度级别）
        std::vector<ok_color::HSL> row_colors;
        row_colors.reserve(num_cols);

        for (int col = 0; col < num_cols; ++col)
        {
            float lightness_position = static_cast<float>(col) / (num_cols - 1.0f);

            // 根据位置计算该列的亮度值
            float lightness;
            auto it =
                std::lower_bound(sorted_lightness_controls.begin(), sorted_lightness_controls.end(), lightness_position, [](const std::pair<float, float>& p, float pos) { return p.second < pos; });

            if (it == sorted_lightness_controls.begin())
            {
                lightness = sorted_lightness_controls.front().first;
            }
            else if (it == sorted_lightness_controls.end())
            {
                lightness = sorted_lightness_controls.back().first;
            }
            else
            {
                // 位置在两个控制点之间，进行插值
                auto prev_it = std::prev(it);
                float prev_lightness = prev_it->first;
                float prev_pos = prev_it->second;
                float next_lightness = it->first;
                float next_pos = it->second;

                // 计算插值系数
                float t = (lightness_position - prev_pos) / (next_pos - prev_pos);
                lightness = prev_lightness + (next_lightness - prev_lightness) * t;
            }

            // 使用该行的基础颜色，但应用该列的亮度
            ok_color::HSL color = base_color;
            color.l = std::clamp(lightness, 0.0f, 1.0f);
            row_colors.push_back(color);
        }

        result.push_back(std::move(row_colors));
    }

    return result;
}

// 将 HSL 颜色向量转换为 cv::Mat 颜色查找表（一维）
cv::Mat hsl_to_colormap_mat(const std::vector<ok_color::HSL>& hsl_colors)
{
    if (hsl_colors.empty())
        return cv::Mat();

    // 创建 1 行 N 列的 BGR 颜色查找表
    cv::Mat colormap(1, static_cast<int>(hsl_colors.size()), CV_8UC3);

    for (size_t i = 0; i < hsl_colors.size(); ++i)
    {
        // 将 HSL 转换为 RGB
        ok_color::RGB rgb = ok_color::okhsl_to_srgb(hsl_colors[i]);

        // 缩放到 0-255 范围并转换为 BGR（OpenCV 使用 BGR 格式）
        colormap.at<cv::Vec3b>(0, static_cast<int>(i)) = cv::Vec3b(static_cast<uchar>(std::clamp(rgb.b * 255.0f, 0.0f, 255.0f)), // B
                                                                   static_cast<uchar>(std::clamp(rgb.g * 255.0f, 0.0f, 255.0f)), // G
                                                                   static_cast<uchar>(std::clamp(rgb.r * 255.0f, 0.0f, 255.0f))  // R
        );
    }

    return colormap;
}

// 将二维 HSL 颜色矩阵转换为 cv::Mat 颜色查找表（二维）
cv::Mat hsl_to_colormap_mat_2d(const std::vector<std::vector<ok_color::HSL>>& hsl_colors_2d)
{
    if (hsl_colors_2d.empty() || hsl_colors_2d[0].empty())
        return cv::Mat();

    int rows = static_cast<int>(hsl_colors_2d.size());
    int cols = static_cast<int>(hsl_colors_2d[0].size());

    // 创建 rows 行 cols 列的 BGRA 颜色查找表（coloring函数期望CV_8UC4格式）
    cv::Mat colormap(rows, cols, CV_8UC4);

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            // 将 HSL 转换为 RGB
            ok_color::RGB rgb = ok_color::okhsl_to_srgb(hsl_colors_2d[r][c]);

            // 缩放到 0-255 范围并转换为 BGRA（OpenCV 使用 BGR 格式，alpha通道设为255）
            colormap.at<cv::Vec4b>(r, c) = cv::Vec4b(static_cast<uchar>(std::clamp(rgb.b * 255.0f, 0.0f, 255.0f)), // B
                                                     static_cast<uchar>(std::clamp(rgb.g * 255.0f, 0.0f, 255.0f)), // G
                                                     static_cast<uchar>(std::clamp(rgb.r * 255.0f, 0.0f, 255.0f)), // R
                                                     255                                                           // A (alpha通道，完全不透明)
            );
        }
    }

    return colormap;
}

// 将 cv::Mat 转换为 OpenGL 纹理
GLuint cvMatToTexture(const cv::Mat& mat, GLuint existing_texture = 0)
{
    if (mat.empty())
        return 0;

    GLuint texture_id = existing_texture;
    if (texture_id == 0)
        glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 根据 Mat 的类型设置纹理格式
    GLenum internal_format = GL_RGB;
    GLenum format = GL_BGR; // OpenCV 使用 BGR
    GLenum type = GL_UNSIGNED_BYTE;

    if (mat.type() == CV_8UC3)
    {
        internal_format = GL_RGB;
        format = GL_BGR;
        type = GL_UNSIGNED_BYTE;
    }
    else if (mat.type() == CV_8UC4)
    {
        internal_format = GL_RGBA;
        format = GL_BGRA;
        type = GL_UNSIGNED_BYTE;
    }
    else if (mat.type() == CV_8UC1)
    {
        internal_format = GL_RED;
        format = GL_RED;
        type = GL_UNSIGNED_BYTE;
    }

    // 上传纹理数据
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, mat.cols, mat.rows, 0, format, type, mat.data);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture_id;
}

struct opengl_frame : public interface_frame
{
    int initialize() override { return 0; }
    void next_frame() override
    {
        ImGui::Begin("OpenGL Info");
        ImGui::End();

        ImGui::Begin("oklch color table");
        static float lightness = 0.6f;
        static float chroma = 1.0f;
        static std::array<ok_color::HSL, 3> colors = { ok_color::HSL{ 0.16f, chroma, lightness }, ok_color::HSL{ 0.40f, chroma, lightness }, ok_color::HSL{ 0.75f, chroma, lightness } };

        auto last_lightness = lightness;
        ImGui::SliderFloat("Lightness", &lightness, 0.0f, 1.0f);
        if (last_lightness != lightness)
            for (auto& color : colors)
                color.l = lightness;

        auto last_chroma = chroma;
        ImGui::SliderFloat("Chroma", &chroma, 0.0f, 1.0f);
        if (last_chroma != chroma)
            for (auto& color : colors)
                color.s = chroma;

        int index = 0;
        for (const auto& color : colors)
        {
            ImGui::Text("L: %.2f, S: %.2f, H: %.2f", color.l, color.s, color.h);
            auto last_hue = color.h;
            ImGui::SliderFloat(("Hue##" + std::to_string(index)).c_str(), &last_hue, 0.0f, 1.0f);
            if (last_hue != colors[index].h)
                colors[index].h = last_hue;
            auto lab = ok_color::okhsl_to_oklab(color);
            ImGui::Text("L: %.2f, A: %.2f, B: %.2f", lab.L, lab.a, lab.b);
            auto c = ok_color::okhsl_to_srgb(color);
            ImGui::ColorButton(std::to_string(index).c_str(), ImVec4(c.r, c.g, c.b, 1.0f), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker, ImVec2(50, 50));
            index++;
        }

        static ok_color::Lab lab = { 0.6f, 0.0f, 0.0f };
        auto last_l = lab.L;
        ImGui::SliderFloat("L", &last_l, 0.0f, 1.0f);
        if (last_l != lab.L)
            lab.L = last_l;
        auto last_a = lab.a;
        ImGui::SliderFloat("a", &last_a, -0.5f, 0.5f);
        if (last_a != lab.a)
            lab.a = last_a;
        auto last_b = lab.b;
        ImGui::SliderFloat("b", &last_b, -0.5f, 0.5f);
        if (last_b != lab.b)
            lab.b = last_b;

        ImGui::Text("L: %.2f, A: %.2f, B: %.2f", lab.L, lab.a, lab.b);
        auto c = ok_color::oklab_to_srgb(lab);
        ImGui::ColorButton("Lab", ImVec4(c.r, c.g, c.b, 1.0f), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker, ImVec2(50, 50));

        ImGui::End();
    }
    void destroy() override {}
};

int executor_build()
{
    auto exec = std::make_shared<executor>();
    auto app = std::make_shared<application>();
    auto platform = std::make_shared<platform_glfw>();
    auto backend = std::make_shared<backend_opengl>();
    auto frame = std::make_shared<opengl_frame>();
    app->set_frame(frame);

    app->set_backend(backend);
    app->set_platform(platform);
    return exec->execute(app);
}