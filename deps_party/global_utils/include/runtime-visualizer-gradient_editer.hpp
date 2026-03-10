#pragma once
#include <imgui.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#if __has_include(<glad/glad.h>)
    #include <glad/glad.h>
#elif __has_include(<GL/gl.h>)
    #include <GL/gl.h>
#endif

#include <algorithm>
#include <atomic>
#include <cfloat>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include <runtime-visualizer-curve_editer.hpp>

namespace ok_color
{

    struct Lab
    {
        float L;
        float a;
        float b;
    };
    struct RGB
    {
        float r;
        float g;
        float b;
    };
    struct HSV
    {
        float h;
        float s;
        float v;
    };
    struct HSL
    {
        float h;
        float s;
        float l;
    };
    static inline RGB okhsl_to_srgb(HSL hsl);
    static inline HSL srgb_to_okhsl(RGB rgb);
    static inline RGB okhsv_to_srgb(HSV hsv);
    static inline HSV srgb_to_okhsv(RGB rgb);
    static inline Lab okhsl_to_oklab(HSL hsl);
    static inline RGB oklab_to_srgb(Lab lab);
    static inline Lab srgb_to_oklab(RGB rgb);
    static inline HSL oklab_to_okhsl(Lab lab);
    static inline HSV oklab_to_okhsv(Lab lab);
    static inline Lab okhsv_to_oklab(HSV hsv);
    static inline HSL okhsv_to_okhsl(HSV hsv);
    static inline HSV okhsl_to_okhsv(HSL hsl);
} // namespace ok_color

class gradient_editer
{
    struct color_point
    {
        std::variant<ok_color::Lab, ok_color::HSL, ok_color::HSV> color;
        ok_color::RGB get_rgb()
        {
            if (std::holds_alternative<ok_color::Lab>(color))
            {
                return ok_color::oklab_to_srgb(std::get<ok_color::Lab>(color));
            }
            else if (std::holds_alternative<ok_color::HSL>(color))
            {
                return ok_color::okhsl_to_srgb(std::get<ok_color::HSL>(color));
            }
            else if (std::holds_alternative<ok_color::HSV>(color))
            {
                return ok_color::okhsv_to_srgb(std::get<ok_color::HSV>(color));
            }
            return { 0, 0, 0 };
        }
        void render_color()
        {
            ok_color::RGB rgb = get_rgb();
            ImGui::ColorButton("##preview", ImVec4(rgb.r, rgb.g, rgb.b, 1.0f), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker, ImVec2(20, 20));
        }
        void render_color_editor()
        {
            // 颜色预览（只读）
            ok_color::RGB rgb = get_rgb();
            ImGui::ColorButton("##preview", ImVec4(rgb.r, rgb.g, rgb.b, 1.0f), ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker, ImVec2(20, 20));
            ImGui::SameLine();

            // 颜色空间选择器（三个按钮）
            bool is_lab = std::holds_alternative<ok_color::Lab>(color);
            bool is_hsl = std::holds_alternative<ok_color::HSL>(color);
            bool is_hsv = std::holds_alternative<ok_color::HSV>(color);

            auto convert_to_lab = [&]() {
                if (is_hsl)
                    color = ok_color::okhsl_to_oklab(std::get<ok_color::HSL>(color));
                else if (is_hsv)
                    color = ok_color::okhsv_to_oklab(std::get<ok_color::HSV>(color));
            };
            auto convert_to_hsl = [&]() {
                if (is_lab)
                    color = ok_color::oklab_to_okhsl(std::get<ok_color::Lab>(color));
                else if (is_hsv)
                    color = ok_color::okhsv_to_okhsl(std::get<ok_color::HSV>(color));
            };
            auto convert_to_hsv = [&]() {
                if (is_lab)
                    color = ok_color::oklab_to_okhsv(std::get<ok_color::Lab>(color));
                else if (is_hsl)
                    color = ok_color::okhsl_to_okhsv(std::get<ok_color::HSL>(color));
            };

            if (is_lab)
                ImGui::BeginDisabled();
            if (ImGui::SmallButton("Lab"))
                convert_to_lab();
            if (is_lab)
                ImGui::EndDisabled();

            ImGui::SameLine();
            if (is_hsl)
                ImGui::BeginDisabled();
            if (ImGui::SmallButton("HSL"))
                convert_to_hsl();
            if (is_hsl)
                ImGui::EndDisabled();

            ImGui::SameLine();
            if (is_hsv)
                ImGui::BeginDisabled();
            if (ImGui::SmallButton("HSV"))
                convert_to_hsv();
            if (is_hsv)
                ImGui::EndDisabled();

            // 根据 variant 类型显示对应编辑器
            if (auto* lab = std::get_if<ok_color::Lab>(&color))
            {
                ImGui::DragFloat("L", &lab->L, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("a", &lab->a, 0.001f, -0.5f, 0.5f);
                ImGui::DragFloat("b", &lab->b, 0.001f, -0.5f, 0.5f);
            }
            else if (auto* hsl = std::get_if<ok_color::HSL>(&color))
            {
                ImGui::DragFloat("H", &hsl->h, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("S", &hsl->s, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("L", &hsl->l, 0.001f, 0.0f, 1.0f);
            }
            else if (auto* hsv = std::get_if<ok_color::HSV>(&color))
            {
                ImGui::DragFloat("H", &hsv->h, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("S", &hsv->s, 0.001f, 0.0f, 1.0f);
                ImGui::DragFloat("V", &hsv->v, 0.001f, 0.0f, 1.0f);
            }
        }
    };

    // 颜色关键点：引导线上的一个点，包含位置、颜色和贝塞尔控制点
    struct color_keypoint
    {
        float x = 0.0f;                      // 水平位置 [0, 1]
        float y = 0.5f;                      // 垂直位置 [0, 1]（每个点独立）
        color_point color;                   // 颜色（支持 Lab/HSL/HSV）
        ImVec2 control_in = { -0.1f, 0.0f }; // 入控制点偏移（相对于关键点）
        ImVec2 control_out = { 0.1f, 0.0f }; // 出控制点偏移
        bool use_bezier = true;              // true=贝塞尔, false=线性
        bool y_locked = false;               // Y位置是否锁定（用于固定右端点）

        color_keypoint() : color{ ok_color::Lab{ 0.5f, 0.0f, 0.0f } } {}
        color_keypoint(float x, float y, ok_color::Lab lab, bool locked = false) : x(x), y(y), color{ lab }, y_locked(locked) {}
        color_keypoint(float x, float y, ok_color::HSL hsl, bool locked = false) : x(x), y(y), color{ hsl }, y_locked(locked) {}
        color_keypoint(float x, float y, ok_color::HSV hsv, bool locked = false) : x(x), y(y), color{ hsv }, y_locked(locked) {}
    };

    // 引导线：可变形的曲线，每个关键点有独立的Y位置，并有属性曲线
    struct guide_line
    {
        float base_y = 0.5f;                   // 基准Y位置（右端固定点的Y）
        std::vector<color_keypoint> keypoints; // 颜色关键点
        static constexpr int CURVE_HUE = 0;
        static constexpr int CURVE_SAT = 1;
        static constexpr int CURVE_LUM = 2;
        curve_editor_multi hsl_curves; // 色调/饱和度/亮度三条曲线合一
        std::string name = "Guide";
        bool selected = false;
        ImU32 line_color = IM_COL32(200, 200, 200, 255); // 引导线颜色

        guide_line() = default;
        guide_line(float y, const std::string& name, ImU32 color = IM_COL32(200, 200, 200, 255)) : base_y(y), name(name), line_color(color)
        {
            // 默认创建两个端点：左端可调，右端固定
            keypoints.push_back(color_keypoint(0.0f, y, ok_color::Lab{ 0.2f, 0.0f, 0.0f }, false)); // 左端可调
            keypoints.push_back(color_keypoint(1.0f, y, ok_color::Lab{ 0.8f, 0.0f, 0.0f }, true));  // 右端固定

            hsl_curves.range_min = { 0, 0 };
            hsl_curves.range_max = { 1, 1 };
            hsl_curves.add_curve("Hue", IM_COL32(255, 100, 100, 255));
            hsl_curves.add_curve("Sat", IM_COL32(100, 255, 100, 255));
            hsl_curves.add_curve("Lum", IM_COL32(200, 200, 255, 255));

            auto set_endpoints = [&](int ci, float y0, float y1) {
                auto& p = hsl_curves[ci];
                p.nodes.clear();
                p.segment_modes.clear();
                p.add_node({ { 0.0f, y0 }, { -0.1f, 0 }, { 0.1f, 0 }, curve_handle_mode::smooth });
                p.add_node({ { 1.0f, y1 }, { -0.1f, 0 }, { 0.1f, 0 }, curve_handle_mode::smooth });
            };
            set_endpoints(CURVE_HUE, 0.0f, 0.0f);
            set_endpoints(CURVE_SAT, 1.0f, 1.0f);
            set_endpoints(CURVE_LUM, 0.0f, 1.0f);
        }

        // 对关键点按X位置排序
        void sort_keypoints()
        {
            std::sort(keypoints.begin(), keypoints.end(), [](const color_keypoint& a, const color_keypoint& b) { return a.x < b.x; });
        }

        // 获取右端固定点的Y位置
        float get_fixed_y() const
        {
            if (!keypoints.empty())
                return keypoints.back().y;
            return base_y;
        }

        float get_hue_at(float x) const { return hsl_curves.evaluate_y(CURVE_HUE, x); }
        float get_saturation_at(float x) const { return hsl_curves.evaluate_y(CURVE_SAT, x); }
        float get_luminance_at(float x) const { return hsl_curves.evaluate_y(CURVE_LUM, x); }

        // 在指定X位置采样颜色（贝塞尔/线性插值）
        ok_color::Lab sample_color_at_lab(float x) const;

        // 获取指定X位置的曲线Y值
        float get_curve_y_at(float x) const;
    };

    // 颜色插值空间
    enum class InterpolationSpace
    {
        OkLab,
        OkHSL,
        OkHSV
    };

    // 渐变画布：包含多条引导线，处理渲染和交互
    class gradient_canvas
    {
    public:
        std::vector<guide_line> guide_lines;
        ImVec2 canvas_size = { 500, 300 };           // 主画布尺寸
        ImVec2 luminance_canvas_size = { 500, 150 }; // 亮度曲线画布尺寸
        int output_width = 256;
        int output_height = 256;
        InterpolationSpace interp_space = InterpolationSpace::OkLab;

        // Y轴范围参数（用于量化Y位置，默认256）
        float y_range = 256.0f;

        // 导出PNG参数
        int export_width = 256;
        int export_height = 256;
        std::string export_path = "color.png";

        // 选择状态 - 主画布
        int selected_line = -1;
        int selected_keypoint = -1;
        int dragging_control = 0; // 0=无/关键点, 1=入控制点, 2=出控制点
        bool is_dragging = false;

        // 曲线编辑状态（用于色调/饱和度/亮度曲线）
        enum class CurveType
        {
            Hue,
            Saturation,
            Luminance
        };
        CurveType active_curve = CurveType::Luminance;

        // 曲线编辑窗口尺寸
        ImVec2 curve_canvas_size = { 600, 300 };

        // 预览纹理 - 最终预览（包含亮度曲线）
        GLuint preview_texture = 0;
        cv::Mat preview_image;
        bool preview_dirty = true;

        // 预览参数
        float preview_lightness = 0.6f;  // 色调预览的固定亮度值
        float preview_saturation = 1.0f; // 色调预览的固定饱和度值

        // 饱和度预览纹理（黑白）
        GLuint saturation_preview_texture = 0;
        cv::Mat saturation_preview_image;

        // 色调预览纹理（固定亮度和饱和度）
        GLuint hue_preview_texture = 0;
        cv::Mat hue_preview_image;

        // 亮度预览纹理（黑白，仅亮度通道）
        GLuint luminance_preview_texture = 0;
        cv::Mat luminance_preview_image;

        gradient_canvas()
        {
            // 根据预设参数创建引导线
            // 颜色定义 (HSL格式)
            struct ColorDef
            {
                float h, s, l;
            };
            ColorDef colors[] = {
                { 0.100f, 1.000f, 0.380f }, // 0
                { 0.400f, 1.000f, 0.380f }, // 1
                { 0.700f, 1.000f, 0.380f }, // 2
            };

            // 颜色位置映射 (使用原始position值，将根据y_range归一化)
            struct ColorMap
            {
                int index;
                float position;
            };
            ColorMap color_map[] = { { 0, 0.000f }, // position原始值
                                     { 1, 128.000f },
                                     { 2, 256.000f } };

            // 亮度曲线控制点
            struct LumPoint
            {
                float position, lightness;
            };
            std::vector<std::vector<LumPoint>> lightness_controls = {
                // Color 0
                { { 0.000f, 1.000f }, { 0.5f, 0.5f }, { 1.000f, 0.000f } },
                // Color 1
                { { 0.000f, 1.000f }, { 0.5f, 0.5f }, { 1.000f, 0.000f } },
                // Color 2
                { { 0.000f, 1.000f }, { 0.5f, 0.5f }, { 1.000f, 0.000f } },
            };

            // 引导线颜色
            ImU32 line_colors[] = {
                IM_COL32(255, 0, 0, 255),
                IM_COL32(0, 255, 0, 255),
                IM_COL32(0, 0, 255, 255), // 蓝
            };

            // 创建7条引导线
            for (int i = 0; i < 3; ++i)
            {
                char name[32];
                snprintf(name, sizeof(name), "Color %d", i);

                // Y位置：使用原始position值归一化到0-1（基于y_range）
                float y_pos = color_map[i].position / y_range;

                guide_line gl(y_pos, name, line_colors[i]);

                // 设置颜色（HSL）
                ok_color::HSL hsl{ colors[i].h, colors[i].s, colors[i].l };
                ok_color::HSL hsl_max{ colors[i].h, colors[i].s, 1.0f };

                // 清除默认关键点，重新设置
                gl.keypoints.clear();

                // 创建两个颜色关键点（左端和右端）
                color_keypoint kp_left(0.0f, y_pos, hsl, false);     // 左端X=0
                color_keypoint kp_right(1.0f, y_pos, hsl_max, true); // 右端X=1

                gl.keypoints.push_back(kp_left);
                gl.keypoints.push_back(kp_right);

                auto set_curve_pts = [&](int ci, std::initializer_list<ImVec2> pts) {
                    auto& p = gl.hsl_curves[ci];
                    p.nodes.clear();
                    p.segment_modes.clear();
                    for (auto& pt : pts)
                        p.add_node({ pt, { -0.1f, 0 }, { 0.1f, 0 }, curve_handle_mode::smooth });
                };

                set_curve_pts(guide_line::CURVE_HUE, { { 0.0f, colors[i].h }, { 1.0f, colors[i].h } });
                set_curve_pts(guide_line::CURVE_SAT, { { 0.0f, colors[i].s }, { 1.0f, colors[i].s } });

                // 设置亮度曲线
                {
                    auto& lp_path = gl.hsl_curves[guide_line::CURVE_LUM];
                    lp_path.nodes.clear();
                    lp_path.segment_modes.clear();
                    for (const auto& lp : lightness_controls[i])
                        lp_path.add_node({ { lp.position, lp.lightness }, { -0.1f, 0 }, { 0.1f, 0 }, curve_handle_mode::smooth });
                }

                guide_lines.push_back(gl);
            }
        }

        ~gradient_canvas()
        {
            if (preview_texture != 0)
            {
                glDeleteTextures(1, &preview_texture);
                preview_texture = 0;
            }
        }

        // 渲染整个画布
        void render();

        // 处理交互 - 主画布
        void handle_interaction(ImVec2 canvas_p0, ImVec2 canvas_p1);

        // 渲染画布内容
        void render_canvas_content(ImDrawList* draw_list, ImVec2 canvas_p0, ImVec2 canvas_p1);

        // 渲染曲线编辑窗口（色调/饱和度/亮度）
        void render_curve_editor_window();

        // 渲染UI控件
        void render_ui_controls();

        // 渲染预览
        void render_preview();

        // ---- Docking 面板窗口 ----
        void render_controls_window();
        void render_canvas_window();
        void render_saturation_preview_window();
        void render_luminance_preview_window();
        void render_hue_only_preview_window();
        void render_final_preview_window();

        // 2D采样生成图像（包含亮度曲线）
        cv::Mat sample_gradient(int width, int height);

        // 2D采样生成图像（仅色调，固定亮度和饱和度）
        cv::Mat sample_gradient_hue_only(int width, int height, float fixed_lightness, float fixed_saturation);

        // 2D采样生成图像（仅饱和度，黑白输出）
        cv::Mat sample_gradient_saturation(int width, int height);

        // 2D采样生成图像（仅亮度，黑白输出）
        cv::Mat sample_gradient_luminance(int width, int height);

        // 更新预览纹理
        void update_preview_texture();

        // 辅助函数：归一化坐标到画布坐标（Y轴：0在上，1在下）
        ImVec2 normalized_to_canvas(float x, float y, ImVec2 canvas_p0, ImVec2 canvas_p1) const
        {
            return ImVec2(canvas_p0.x + x * (canvas_p1.x - canvas_p0.x),
                          canvas_p0.y + y * (canvas_p1.y - canvas_p0.y) // Y=0在上，Y=1在下
            );
        }

        // 辅助函数：画布坐标到归一化坐标（Y轴：0在上，1在下）
        ImVec2 canvas_to_normalized(ImVec2 pos, ImVec2 canvas_p0, ImVec2 canvas_p1) const
        {
            return ImVec2((pos.x - canvas_p0.x) / (canvas_p1.x - canvas_p0.x),
                          (pos.y - canvas_p0.y) / (canvas_p1.y - canvas_p0.y) // Y=0在上，Y=1在下
            );
        }

        // 在选定颜色空间中插值两个颜色
        ok_color::Lab interpolate_color_lab(ok_color::Lab c1, ok_color::Lab c2, float t) const;

        // 标记预览需要更新
        void mark_dirty() { preview_dirty = true; }

        // 保存渐变到PNG文件
        bool save_to_png(const std::string& path, int width, int height) const;

        // JSON序列化/反序列化
        nlohmann::json to_json() const;
        void from_json(const nlohmann::json& j);
    };

    gradient_canvas canvas;

    // ==================== 配置管理 ====================
    struct config_entry
    {
        std::string name;
        std::string timestamp;
        nlohmann::json data;
    };

    std::vector<config_entry> config_list;
    std::string config_file_path = "gradient_configs.json";
    char config_name_buf[128] = "default";
    int selected_config_idx = -1;
    bool configs_loaded_ = false;
    bool dockspace_initialized_ = false;

    static std::string get_current_timestamp()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t_val = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t_val);
#else
        localtime_r(&time_t_val, &tm_buf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    void save_config(const std::string& name);
    void load_config(int index);
    void delete_config(int index);
    void save_configs_to_file();
    void load_configs_from_file();
    void render_config_ui();
    void render_config_window();

public:
    void destroy()
    {
        if (canvas.preview_texture != 0)
        {
            glDeleteTextures(1, &canvas.preview_texture);
            canvas.preview_texture = 0;
        }
        if (canvas.saturation_preview_texture != 0)
        {
            glDeleteTextures(1, &canvas.saturation_preview_texture);
            canvas.saturation_preview_texture = 0;
        }
        if (canvas.hue_preview_texture != 0)
        {
            glDeleteTextures(1, &canvas.hue_preview_texture);
            canvas.hue_preview_texture = 0;
        }
        if (canvas.luminance_preview_texture != 0)
        {
            glDeleteTextures(1, &canvas.luminance_preview_texture);
            canvas.luminance_preview_texture = 0;
        }
    }

    void render()
    {
        if (!configs_loaded_)
        {
            load_configs_from_file();
            configs_loaded_ = true;
        }

        // 全屏宿主窗口，承载 DockSpace
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("##GradientEditorDockHost", nullptr, host_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("GradientEditorDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

        if (!dockspace_initialized_)
        {
            dockspace_initialized_ = true;

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

            // 左侧：控制面板（22%宽）
            ImGuiID left_id, right_area_id;
            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.22f, &left_id, &right_area_id);

            // 左侧下方：配置管理（左侧的30%高）
            ImGuiID ctrl_id, config_id;
            ImGui::DockBuilderSplitNode(left_id, ImGuiDir_Down, 0.30f, &config_id, &ctrl_id);

            // 右侧整体：上方工作区（60%高）+ 下方预览行（全宽）
            ImGuiID top_area_id, preview_id;
            ImGui::DockBuilderSplitNode(right_area_id, ImGuiDir_Down, 0.38f, &preview_id, &top_area_id);

            // 上方工作区：左侧颜色位置编辑 + 右侧曲线编辑器（35%宽）
            ImGuiID canvas_id, curve_id;
            ImGui::DockBuilderSplitNode(top_area_id, ImGuiDir_Right, 0.40f, &curve_id, &canvas_id);

            // 预览行：四等分 —— 饱和度 | 亮度 | 色调 | 最终
            ImGuiID sat_id, preview_rest1;
            ImGui::DockBuilderSplitNode(preview_id, ImGuiDir_Left, 0.25f, &sat_id, &preview_rest1);
            ImGuiID lum_id, preview_rest2;
            ImGui::DockBuilderSplitNode(preview_rest1, ImGuiDir_Left, 0.333f, &lum_id, &preview_rest2);
            ImGuiID hue_only_id, final_id;
            ImGui::DockBuilderSplitNode(preview_rest2, ImGuiDir_Left, 0.50f, &hue_only_id, &final_id);

            ImGui::DockBuilderDockWindow("控制面板##ge", ctrl_id);
            ImGui::DockBuilderDockWindow("配置管理##ge", config_id);
            ImGui::DockBuilderDockWindow("颜色位置编辑##ge", canvas_id);
            ImGui::DockBuilderDockWindow("曲线编辑器##ge", curve_id);
            ImGui::DockBuilderDockWindow("饱和度预览##ge", sat_id);
            ImGui::DockBuilderDockWindow("亮度预览##ge", lum_id);
            ImGui::DockBuilderDockWindow("色调预览##ge", hue_only_id);
            ImGui::DockBuilderDockWindow("最终预览##ge", final_id);
            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::End();

        canvas.render_controls_window();
        canvas.render_canvas_window();
        canvas.render_saturation_preview_window();
        canvas.render_luminance_preview_window();
        canvas.render_hue_only_preview_window();
        canvas.render_final_preview_window();
        canvas.render_curve_editor_window();
        render_config_window();
    }

    cv::Mat get_gradient_image(int width = 0, int height = 0)
    {
        if (width <= 0)
            width = canvas.output_width;
        if (height <= 0)
            height = canvas.output_height;
        return canvas.sample_gradient(width, height);
    }
};

// ==================== gradient_editer 成员函数实现 ====================

// guide_line: 在指定X位置采样颜色
// 色调/饱和度/亮度均由各自的 curve 通过 evaluate() 决定
inline ok_color::Lab gradient_editer::guide_line::sample_color_at_lab(float x) const
{
    // 直接从三条属性曲线（curve::evaluate）获取 HSL 分量
    ok_color::HSL okhsl;
    okhsl.h = std::clamp(get_hue_at(x), 0.0f, 1.0f);        // hue_curve.evaluate(x)
    okhsl.s = std::clamp(get_saturation_at(x), 0.0f, 1.0f); // saturation_curve.evaluate(x)
    okhsl.l = std::clamp(get_luminance_at(x), 0.0f, 1.0f);  // luminance_curve.evaluate(x)

    return ok_color::okhsl_to_oklab(okhsl);
}

// guide_line: 获取指定X位置的曲线Y值
inline float gradient_editer::guide_line::get_curve_y_at(float x) const
{
    if (keypoints.empty())
        return base_y;

    curve_path y_path;
    y_path.function_mode = true;
    for (size_t i = 0; i < keypoints.size(); ++i)
    {
        const auto& kp = keypoints[i];
        curve_node nd;
        nd.position = { kp.x, kp.y };
        nd.handle_in = kp.control_in;
        nd.handle_out = kp.control_out;
        nd.handle_mode = curve_handle_mode::smooth;
        y_path.add_node(nd);
        if (i > 0)
            y_path.segment_modes[i - 1] = kp.use_bezier ? curve_interpolation_mode::cubic_bezier : curve_interpolation_mode::linear;
    }
    return y_path.evaluate_y(x);
}

// gradient_canvas: 在选定颜色空间中插值
inline ok_color::Lab gradient_editer::gradient_canvas::interpolate_color_lab(ok_color::Lab c1, ok_color::Lab c2, float t) const
{
    switch (interp_space)
    {
        case InterpolationSpace::OkLab: return ok_color::Lab{ c1.L + t * (c2.L - c1.L), c1.a + t * (c2.a - c1.a), c1.b + t * (c2.b - c1.b) };

        case InterpolationSpace::OkHSL:
            {
                auto hsl1 = ok_color::oklab_to_okhsl(c1);
                auto hsl2 = ok_color::oklab_to_okhsl(c2);

                // 色相环形插值
                float h1 = hsl1.h, h2 = hsl2.h;
                float dh = h2 - h1;
                if (dh > 0.5f)
                    dh -= 1.0f;
                if (dh < -0.5f)
                    dh += 1.0f;
                float h = h1 + t * dh;
                if (h < 0.0f)
                    h += 1.0f;
                if (h > 1.0f)
                    h -= 1.0f;

                ok_color::HSL result{ h, hsl1.s + t * (hsl2.s - hsl1.s), hsl1.l + t * (hsl2.l - hsl1.l) };
                return ok_color::okhsl_to_oklab(result);
            }

        case InterpolationSpace::OkHSV:
            {
                auto hsv1 = ok_color::oklab_to_okhsv(c1);
                auto hsv2 = ok_color::oklab_to_okhsv(c2);

                // 色相环形插值
                float h1 = hsv1.h, h2 = hsv2.h;
                float dh = h2 - h1;
                if (dh > 0.5f)
                    dh -= 1.0f;
                if (dh < -0.5f)
                    dh += 1.0f;
                float h = h1 + t * dh;
                if (h < 0.0f)
                    h += 1.0f;
                if (h > 1.0f)
                    h -= 1.0f;

                ok_color::HSV result{ h, hsv1.s + t * (hsv2.s - hsv1.s), hsv1.v + t * (hsv2.v - hsv1.v) };
                return ok_color::okhsv_to_oklab(result);
            }

        default: return c1;
    }
}

// gradient_canvas: 2D采样生成图像
inline cv::Mat gradient_editer::gradient_canvas::sample_gradient(int width, int height)
{
    cv::Mat result(height, width, CV_8UC3);

    if (guide_lines.empty())
    {
        result.setTo(cv::Scalar(128, 128, 128));
        return result;
    }

    // 存储引导线指针用于排序
    std::vector<guide_line*> line_ptrs;
    for (auto& gl : guide_lines)
        line_ptrs.push_back(&gl);

    for (int px = 0; px < width; px++)
    {
        float x = px / (float)(width - 1);

        // 在当前X位置，按曲线Y值排序引导线
        std::vector<std::pair<float, guide_line*>> lines_at_x;
        for (auto* gl : line_ptrs)
        {
            float curve_y = gl->get_curve_y_at(x);
            lines_at_x.push_back({ curve_y, gl });
        }
        std::sort(lines_at_x.begin(), lines_at_x.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        for (int py = 0; py < height; py++)
        {
            float y = py / (float)(height - 1);

            // 找到y上下最近的两条引导线
            guide_line* lower = nullptr;
            guide_line* upper = nullptr;
            float lower_y = 0.0f;
            float upper_y = 1.0f;

            for (const auto& [curve_y, gl] : lines_at_x)
            {
                if (curve_y <= y)
                {
                    lower = gl;
                    lower_y = curve_y;
                }
                if (curve_y >= y && upper == nullptr)
                {
                    upper = gl;
                    upper_y = curve_y;
                }
            }

            // 边界情况
            if (lower == nullptr)
            {
                lower = lines_at_x.front().second;
                lower_y = lines_at_x.front().first;
            }
            if (upper == nullptr)
            {
                upper = lines_at_x.back().second;
                upper_y = lines_at_x.back().first;
            }

            // 在两条引导线上分别采样颜色
            auto color_lower = lower->sample_color_at_lab(x);
            auto color_upper = upper->sample_color_at_lab(x);

            // 计算垂直插值参数
            float t = 0.0f;
            if (lower != upper && (upper_y - lower_y) > 0.0001f)
            {
                t = (y - lower_y) / (upper_y - lower_y);
            }

            // 在选定颜色空间中插值
            auto final_lab = interpolate_color_lab(color_lower, color_upper, t);
            auto final_rgb = ok_color::oklab_to_srgb(final_lab);

            // 转换为BGR并写入图像
            uchar b = static_cast<uchar>(std::clamp(final_rgb.b, 0.0f, 1.0f) * 255.0f);
            uchar g = static_cast<uchar>(std::clamp(final_rgb.g, 0.0f, 1.0f) * 255.0f);
            uchar r = static_cast<uchar>(std::clamp(final_rgb.r, 0.0f, 1.0f) * 255.0f);
            result.at<cv::Vec3b>(py, px) = cv::Vec3b(b, g, r); // Y=0在上
        }
    }

    return result;
}

// gradient_canvas: 2D采样生成图像（仅色调，固定亮度和饱和度）
inline cv::Mat gradient_editer::gradient_canvas::sample_gradient_hue_only(int width, int height, float fixed_lightness, float fixed_saturation)
{
    cv::Mat result(height, width, CV_8UC3);

    if (guide_lines.empty())
    {
        result.setTo(cv::Scalar(128, 128, 128));
        return result;
    }

    // 存储引导线指针用于排序
    std::vector<guide_line*> line_ptrs;
    for (auto& gl : guide_lines)
        line_ptrs.push_back(&gl);

    for (int px = 0; px < width; px++)
    {
        float x = px / (float)(width - 1);

        // 在当前X位置，按曲线Y值排序引导线
        std::vector<std::pair<float, guide_line*>> lines_at_x;
        for (auto* gl : line_ptrs)
        {
            float curve_y = gl->get_curve_y_at(x);
            lines_at_x.push_back({ curve_y, gl });
        }
        std::sort(lines_at_x.begin(), lines_at_x.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        for (int py = 0; py < height; py++)
        {
            float y = py / (float)(height - 1);

            // 找到y上下最近的两条引导线
            guide_line* lower = nullptr;
            guide_line* upper = nullptr;
            float lower_y = 0.0f;
            float upper_y = 1.0f;

            for (const auto& [curve_y, gl] : lines_at_x)
            {
                if (curve_y <= y)
                {
                    lower = gl;
                    lower_y = curve_y;
                }
                if (curve_y >= y && upper == nullptr)
                {
                    upper = gl;
                    upper_y = curve_y;
                }
            }

            // 边界情况
            if (lower == nullptr)
            {
                lower = lines_at_x.front().second;
                lower_y = lines_at_x.front().first;
            }
            if (upper == nullptr)
            {
                upper = lines_at_x.back().second;
                upper_y = lines_at_x.back().first;
            }

            // 在两条引导线上分别采样颜色
            auto color_lower = lower->sample_color_at_lab(x);
            auto color_upper = upper->sample_color_at_lab(x);

            // 计算垂直插值参数
            float t = 0.0f;
            if (lower != upper && (upper_y - lower_y) > 0.0001f)
            {
                t = (y - lower_y) / (upper_y - lower_y);
            }

            // 在Lab空间插值获得色调（a和b分量）
            auto final_lab = interpolate_color_lab(color_lower, color_upper, t);

            // 转换到OkHSL获取色调H，然后用固定的L和S
            auto okhsl = ok_color::oklab_to_okhsl(final_lab);
            okhsl.s = fixed_saturation;
            okhsl.l = fixed_lightness;

            // 转回RGB
            auto final_rgb = ok_color::okhsl_to_srgb(okhsl);

            // 转换为BGR并写入图像
            uchar b = static_cast<uchar>(std::clamp(final_rgb.b, 0.0f, 1.0f) * 255.0f);
            uchar g = static_cast<uchar>(std::clamp(final_rgb.g, 0.0f, 1.0f) * 255.0f);
            uchar r = static_cast<uchar>(std::clamp(final_rgb.r, 0.0f, 1.0f) * 255.0f);
            result.at<cv::Vec3b>(py, px) = cv::Vec3b(b, g, r); // Y=0在上
        }
    }

    return result;
}

// gradient_canvas: 2D采样生成图像（仅饱和度，黑白输出）
inline cv::Mat gradient_editer::gradient_canvas::sample_gradient_saturation(int width, int height)
{
    cv::Mat result(height, width, CV_8UC3);

    if (guide_lines.empty())
    {
        result.setTo(cv::Scalar(128, 128, 128));
        return result;
    }

    // 存储引导线指针用于排序
    std::vector<guide_line*> line_ptrs;
    for (auto& gl : guide_lines)
        line_ptrs.push_back(&gl);

    for (int px = 0; px < width; px++)
    {
        float x = px / (float)(width - 1);

        // 在当前X位置，按曲线Y值排序引导线
        std::vector<std::pair<float, guide_line*>> lines_at_x;
        for (auto* gl : line_ptrs)
        {
            float curve_y = gl->get_curve_y_at(x);
            lines_at_x.push_back({ curve_y, gl });
        }
        std::sort(lines_at_x.begin(), lines_at_x.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        for (int py = 0; py < height; py++)
        {
            float y = py / (float)(height - 1);

            // 找到y上下最近的两条引导线
            guide_line* lower = nullptr;
            guide_line* upper = nullptr;
            float lower_y = 0.0f;
            float upper_y = 1.0f;

            for (const auto& [curve_y, gl] : lines_at_x)
            {
                if (curve_y <= y)
                {
                    lower = gl;
                    lower_y = curve_y;
                }
                if (curve_y >= y && upper == nullptr)
                {
                    upper = gl;
                    upper_y = curve_y;
                }
            }

            // 边界情况
            if (lower == nullptr)
            {
                lower = lines_at_x.front().second;
                lower_y = lines_at_x.front().first;
            }
            if (upper == nullptr)
            {
                upper = lines_at_x.back().second;
                upper_y = lines_at_x.back().first;
            }

            // 在两条引导线上分别采样颜色
            auto color_lower = lower->sample_color_at_lab(x);
            auto color_upper = upper->sample_color_at_lab(x);

            // 计算垂直插值参数
            float t = 0.0f;
            if (lower != upper && (upper_y - lower_y) > 0.0001f)
            {
                t = (y - lower_y) / (upper_y - lower_y);
            }

            // 在Lab空间插值
            auto final_lab = interpolate_color_lab(color_lower, color_upper, t);

            // 转换到OkHSL获取饱和度S
            auto okhsl = ok_color::oklab_to_okhsl(final_lab);
            float saturation = std::clamp(okhsl.s, 0.0f, 1.0f);

            // 饱和度作为灰度值输出
            uchar gray = static_cast<uchar>(saturation * 255.0f);
            result.at<cv::Vec3b>(py, px) = cv::Vec3b(gray, gray, gray); // Y=0在上
        }
    }

    return result;
}

// gradient_canvas: 2D采样生成图像（仅亮度，黑白输出）
inline cv::Mat gradient_editer::gradient_canvas::sample_gradient_luminance(int width, int height)
{
    cv::Mat result(height, width, CV_8UC3);

    if (guide_lines.empty())
    {
        result.setTo(cv::Scalar(128, 128, 128));
        return result;
    }

    std::vector<guide_line*> line_ptrs;
    for (auto& gl : guide_lines)
        line_ptrs.push_back(&gl);

    for (int px = 0; px < width; px++)
    {
        float x = px / (float)(width - 1);

        std::vector<std::pair<float, guide_line*>> lines_at_x;
        for (auto* gl : line_ptrs)
        {
            float curve_y = gl->get_curve_y_at(x);
            lines_at_x.push_back({ curve_y, gl });
        }
        std::sort(lines_at_x.begin(), lines_at_x.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        for (int py = 0; py < height; py++)
        {
            float y = py / (float)(height - 1);

            guide_line* lower = nullptr;
            guide_line* upper = nullptr;
            float lower_y = 0.0f;
            float upper_y = 1.0f;

            for (const auto& [curve_y, gl] : lines_at_x)
            {
                if (curve_y <= y)
                {
                    lower = gl;
                    lower_y = curve_y;
                }
                if (curve_y >= y && upper == nullptr)
                {
                    upper = gl;
                    upper_y = curve_y;
                }
            }

            if (lower == nullptr)
            {
                lower = lines_at_x.front().second;
                lower_y = lines_at_x.front().first;
            }
            if (upper == nullptr)
            {
                upper = lines_at_x.back().second;
                upper_y = lines_at_x.back().first;
            }

            auto color_lower = lower->sample_color_at_lab(x);
            auto color_upper = upper->sample_color_at_lab(x);

            float t = 0.0f;
            if (lower != upper && (upper_y - lower_y) > 0.0001f)
                t = (y - lower_y) / (upper_y - lower_y);

            auto final_lab = interpolate_color_lab(color_lower, color_upper, t);

            // 取OkHSL的亮度L值输出灰度
            auto okhsl = ok_color::oklab_to_okhsl(final_lab);
            float lum = std::clamp(okhsl.l, 0.0f, 1.0f);

            uchar gray = static_cast<uchar>(lum * 255.0f);
            result.at<cv::Vec3b>(py, px) = cv::Vec3b(gray, gray, gray);
        }
    }

    return result;
}

// gradient_canvas: 保存渐变到PNG文件
inline bool gradient_editer::gradient_canvas::save_to_png(const std::string& path, int width, int height) const
{
    cv::Mat result(height, width, CV_8UC4); // RGBA格式

    if (guide_lines.empty())
    {
        result.setTo(cv::Scalar(128, 128, 128, 255));
        return cv::imwrite(path, result);
    }

    // 存储引导线指针用于排序
    std::vector<const guide_line*> line_ptrs;
    for (const auto& gl : guide_lines)
        line_ptrs.push_back(&gl);

    for (int px = 0; px < width; px++)
    {
        float x = px / (float)(width - 1);

        // 在当前X位置，按曲线Y值排序引导线
        std::vector<std::pair<float, const guide_line*>> lines_at_x;
        for (const auto* gl : line_ptrs)
        {
            float curve_y = gl->get_curve_y_at(x);
            lines_at_x.push_back({ curve_y, gl });
        }
        std::sort(lines_at_x.begin(), lines_at_x.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        for (int py = 0; py < height; py++)
        {
            float y = py / (float)(height - 1);

            // 找到y上下最近的两条引导线
            const guide_line* lower = nullptr;
            const guide_line* upper = nullptr;
            float lower_y = 0.0f;
            float upper_y = 1.0f;

            for (const auto& [curve_y, gl] : lines_at_x)
            {
                if (curve_y <= y)
                {
                    lower = gl;
                    lower_y = curve_y;
                }
                if (curve_y >= y && upper == nullptr)
                {
                    upper = gl;
                    upper_y = curve_y;
                }
            }

            // 边界情况
            if (lower == nullptr)
            {
                lower = lines_at_x.front().second;
                lower_y = lines_at_x.front().first;
            }
            if (upper == nullptr)
            {
                upper = lines_at_x.back().second;
                upper_y = lines_at_x.back().first;
            }

            // 在两条引导线上分别采样颜色
            auto color_lower = lower->sample_color_at_lab(x);
            auto color_upper = upper->sample_color_at_lab(x);

            // 计算垂直插值参数
            float t = 0.0f;
            if (lower != upper && (upper_y - lower_y) > 0.0001f)
            {
                t = (y - lower_y) / (upper_y - lower_y);
            }

            // 在选定颜色空间中插值
            auto final_lab = const_cast<gradient_canvas*>(this)->interpolate_color_lab(color_lower, color_upper, t);
            auto final_rgb = ok_color::oklab_to_srgb(final_lab);

            // 转换为BGRA并写入图像
            uchar b = static_cast<uchar>(std::clamp(final_rgb.b, 0.0f, 1.0f) * 255.0f);
            uchar g = static_cast<uchar>(std::clamp(final_rgb.g, 0.0f, 1.0f) * 255.0f);
            uchar r = static_cast<uchar>(std::clamp(final_rgb.r, 0.0f, 1.0f) * 255.0f);
            uchar a = 255;                                        // 完全不透明
            result.at<cv::Vec4b>(py, px) = cv::Vec4b(b, g, r, a); // Y=0在上
        }
    }

    return cv::imwrite(path, result);
}

// gradient_canvas: 更新预览纹理
inline void gradient_editer::gradient_canvas::update_preview_texture()
{
    if (!preview_dirty)
        return;

    // 1. 最终预览（包含亮度曲线）
    preview_image = sample_gradient(output_width, output_height);
    cv::cvtColor(preview_image, preview_image, cv::COLOR_BGR2RGB);

    if (preview_texture == 0)
    {
        glGenTextures(1, &preview_texture);
    }

    glBindTexture(GL_TEXTURE_2D, preview_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, preview_image.cols, preview_image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, preview_image.data);

    // 2. 饱和度预览（黑白）
    saturation_preview_image = sample_gradient_saturation(output_width, output_height);
    cv::cvtColor(saturation_preview_image, saturation_preview_image, cv::COLOR_BGR2RGB);

    if (saturation_preview_texture == 0)
    {
        glGenTextures(1, &saturation_preview_texture);
    }

    glBindTexture(GL_TEXTURE_2D, saturation_preview_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, saturation_preview_image.cols, saturation_preview_image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, saturation_preview_image.data);

    // 3. 色调预览（固定亮度和饱和度）- 用于画布背景
    hue_preview_image = sample_gradient_hue_only(output_width, output_height, preview_lightness, preview_saturation);
    cv::cvtColor(hue_preview_image, hue_preview_image, cv::COLOR_BGR2RGB);

    if (hue_preview_texture == 0)
    {
        glGenTextures(1, &hue_preview_texture);
    }

    glBindTexture(GL_TEXTURE_2D, hue_preview_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, hue_preview_image.cols, hue_preview_image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, hue_preview_image.data);

    // 4. 亮度预览（黑白）
    luminance_preview_image = sample_gradient_luminance(output_width, output_height);
    cv::cvtColor(luminance_preview_image, luminance_preview_image, cv::COLOR_BGR2RGB);

    if (luminance_preview_texture == 0)
    {
        glGenTextures(1, &luminance_preview_texture);
    }

    glBindTexture(GL_TEXTURE_2D, luminance_preview_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, luminance_preview_image.cols, luminance_preview_image.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, luminance_preview_image.data);

    preview_dirty = false;
}

// gradient_canvas: 渲染画布内容
inline void gradient_editer::gradient_canvas::render_canvas_content(ImDrawList* draw_list, ImVec2 canvas_p0, ImVec2 canvas_p1)
{
    // 预览图已经作为背景绘制，不再绘制实心背景
    // 添加边框
    draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(100, 100, 100, 255));

    // 绘制网格（半透明，覆盖在预览图上）
    const float grid_step = 0.25f;
    ImU32 grid_color = IM_COL32(255, 255, 255, 60);

    for (float x = 0.0f; x <= 1.0f; x += grid_step)
    {
        ImVec2 p0 = normalized_to_canvas(x, 0.0f, canvas_p0, canvas_p1);
        ImVec2 p1 = normalized_to_canvas(x, 1.0f, canvas_p0, canvas_p1);
        draw_list->AddLine(p0, p1, grid_color);
    }
    for (float y = 0.0f; y <= 1.0f; y += grid_step)
    {
        ImVec2 p0 = normalized_to_canvas(0.0f, y, canvas_p0, canvas_p1);
        ImVec2 p1 = normalized_to_canvas(1.0f, y, canvas_p0, canvas_p1);
        draw_list->AddLine(p0, p1, grid_color);
    }

    // 绘制每条引导线
    for (int line_idx = 0; line_idx < (int)guide_lines.size(); ++line_idx)
    {
        auto& gl = guide_lines[line_idx];
        bool line_selected = (line_idx == selected_line);

        if (gl.keypoints.size() < 2)
            continue;

        // 绘制曲线段
        for (size_t i = 0; i < gl.keypoints.size() - 1; ++i)
        {
            const auto& kp1 = gl.keypoints[i];
            const auto& kp2 = gl.keypoints[i + 1];

            // 使用每个关键点独立的Y位置
            ImVec2 p1 = normalized_to_canvas(kp1.x, kp1.y, canvas_p0, canvas_p1);
            ImVec2 p4 = normalized_to_canvas(kp2.x, kp2.y, canvas_p0, canvas_p1);

            if (kp1.use_bezier)
            {
                // 贝塞尔曲线 - 控制点相对于各自的关键点
                ImVec2 p2 = normalized_to_canvas(kp1.x + kp1.control_out.x, kp1.y + kp1.control_out.y, canvas_p0, canvas_p1);
                ImVec2 p3 = normalized_to_canvas(kp2.x + kp2.control_in.x, kp2.y + kp2.control_in.y, canvas_p0, canvas_p1);
                draw_list->AddBezierCubic(p1, p2, p3, p4, gl.line_color, 2.0f);
            }
            else
            {
                // 线性
                draw_list->AddLine(p1, p4, gl.line_color, 2.0f);
            }
        }

        // 绘制关键点
        for (int kp_idx = 0; kp_idx < (int)gl.keypoints.size(); ++kp_idx)
        {
            const auto& kp = gl.keypoints[kp_idx];
            ImVec2 kp_pos = normalized_to_canvas(kp.x, kp.y, canvas_p0, canvas_p1);

            // 关键点颜色预览
            auto rgb = const_cast<color_keypoint&>(kp).color.get_rgb();
            ImU32 kp_color = IM_COL32((int)(std::clamp(rgb.r, 0.0f, 1.0f) * 255), (int)(std::clamp(rgb.g, 0.0f, 1.0f) * 255), (int)(std::clamp(rgb.b, 0.0f, 1.0f) * 255), 255);

            // 绘制关键点圆圈（锁定的点用不同边框）
            bool kp_selected = (line_idx == selected_line && kp_idx == selected_keypoint);
            float radius = kp_selected ? 10.0f : 8.0f;
            draw_list->AddCircleFilled(kp_pos, radius, kp_color);
            ImU32 border_color = kp.y_locked ? IM_COL32(255, 100, 100, 255) : (kp_selected ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255));
            draw_list->AddCircle(kp_pos, radius, border_color, 0, 2.0f);

            // 如果选中，绘制控制点
            if (kp_selected && kp.use_bezier)
            {
                // 入控制点
                ImVec2 ctrl_in = normalized_to_canvas(kp.x + kp.control_in.x, kp.y + kp.control_in.y, canvas_p0, canvas_p1);
                draw_list->AddLine(kp_pos, ctrl_in, IM_COL32(200, 100, 255, 200), 1.5f);
                draw_list->AddCircleFilled(ctrl_in, 5.0f, IM_COL32(200, 100, 255, 255));

                // 出控制点
                ImVec2 ctrl_out = normalized_to_canvas(kp.x + kp.control_out.x, kp.y + kp.control_out.y, canvas_p0, canvas_p1);
                draw_list->AddLine(kp_pos, ctrl_out, IM_COL32(200, 100, 255, 200), 1.5f);
                draw_list->AddCircleFilled(ctrl_out, 5.0f, IM_COL32(200, 100, 255, 255));
            }
        }

        // 绘制引导线名称标签（在右端固定点旁边）
        float fixed_y = gl.get_fixed_y();
        ImVec2 label_pos = normalized_to_canvas(1.02f, fixed_y, canvas_p0, canvas_p1);
        draw_list->AddText(ImVec2(label_pos.x + 5, label_pos.y - 7), gl.line_color, gl.name.c_str());
    }

    // 绘制坐标轴标签
    for (float v = 0.0f; v <= 1.0f; v += 0.25f)
    {
        char label[16];
        snprintf(label, sizeof(label), "%.2f", v);

        // X轴
        ImVec2 x_label_pos = normalized_to_canvas(v, -0.05f, canvas_p0, canvas_p1);
        draw_list->AddText(ImVec2(x_label_pos.x - 15, x_label_pos.y), IM_COL32(150, 150, 150, 255), label);

        // Y轴
        ImVec2 y_label_pos = normalized_to_canvas(1.02f, v, canvas_p0, canvas_p1);
        draw_list->AddText(ImVec2(y_label_pos.x, y_label_pos.y - 7), IM_COL32(150, 150, 150, 255), label);
    }
}

// gradient_canvas: 处理交互
inline void gradient_editer::gradient_canvas::handle_interaction(ImVec2 canvas_p0, ImVec2 canvas_p1)
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mouse_pos = io.MousePos;

    // 检查鼠标是否在画布内
    bool in_canvas = mouse_pos.x >= canvas_p0.x && mouse_pos.x <= canvas_p1.x && mouse_pos.y >= canvas_p0.y && mouse_pos.y <= canvas_p1.y;

    if (!in_canvas && !is_dragging)
        return;

    ImVec2 normalized_mouse = canvas_to_normalized(mouse_pos, canvas_p0, canvas_p1);

    // 鼠标按下 - 选择或开始拖拽
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && in_canvas)
    {
        bool found = false;

        // 首先检查控制点（如果有选中的关键点）
        if (selected_line >= 0 && selected_keypoint >= 0)
        {
            auto& kp = guide_lines[selected_line].keypoints[selected_keypoint];

            if (kp.use_bezier)
            {
                // 检查入控制点 - 使用关键点自己的Y位置
                ImVec2 ctrl_in_pos = normalized_to_canvas(kp.x + kp.control_in.x, kp.y + kp.control_in.y, canvas_p0, canvas_p1);
                float dist_in = sqrtf((mouse_pos.x - ctrl_in_pos.x) * (mouse_pos.x - ctrl_in_pos.x) + (mouse_pos.y - ctrl_in_pos.y) * (mouse_pos.y - ctrl_in_pos.y));
                if (dist_in < 10.0f)
                {
                    dragging_control = 1;
                    is_dragging = true;
                    found = true;
                }

                // 检查出控制点
                if (!found)
                {
                    ImVec2 ctrl_out_pos = normalized_to_canvas(kp.x + kp.control_out.x, kp.y + kp.control_out.y, canvas_p0, canvas_p1);
                    float dist_out = sqrtf((mouse_pos.x - ctrl_out_pos.x) * (mouse_pos.x - ctrl_out_pos.x) + (mouse_pos.y - ctrl_out_pos.y) * (mouse_pos.y - ctrl_out_pos.y));
                    if (dist_out < 10.0f)
                    {
                        dragging_control = 2;
                        is_dragging = true;
                        found = true;
                    }
                }
            }
        }

        // 检查关键点
        if (!found)
        {
            for (int line_idx = 0; line_idx < (int)guide_lines.size(); ++line_idx)
            {
                auto& gl = guide_lines[line_idx];
                for (int kp_idx = 0; kp_idx < (int)gl.keypoints.size(); ++kp_idx)
                {
                    const auto& kp = gl.keypoints[kp_idx];
                    // 使用关键点自己的Y位置
                    ImVec2 kp_pos = normalized_to_canvas(kp.x, kp.y, canvas_p0, canvas_p1);

                    float dist = sqrtf((mouse_pos.x - kp_pos.x) * (mouse_pos.x - kp_pos.x) + (mouse_pos.y - kp_pos.y) * (mouse_pos.y - kp_pos.y));

                    if (dist < 12.0f)
                    {
                        selected_line = line_idx;
                        selected_keypoint = kp_idx;
                        dragging_control = 0;
                        is_dragging = true;
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }

        // 没有点中任何东西，取消选择
        if (!found)
        {
            selected_line = -1;
            selected_keypoint = -1;
            dragging_control = 0;
        }
    }

    // 鼠标拖拽
    if (is_dragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        if (selected_line >= 0 && selected_keypoint >= 0)
        {
            auto& gl = guide_lines[selected_line];
            auto& kp = gl.keypoints[selected_keypoint];

            if (dragging_control == 0)
            {
                // 拖拽关键点 - 只修改该关键点的位置
                kp.x = std::clamp(normalized_mouse.x, 0.0f, 1.0f);
                // 只有非锁定的点才能调整Y位置
                if (!kp.y_locked)
                {
                    kp.y = std::clamp(normalized_mouse.y, 0.0f, 1.0f);
                }
            }
            else if (dragging_control == 1)
            {
                // 拖拽入控制点 - 相对于关键点自己的Y位置
                kp.control_in.x = normalized_mouse.x - kp.x;
                kp.control_in.y = normalized_mouse.y - kp.y;
            }
            else if (dragging_control == 2)
            {
                // 拖拽出控制点 - 相对于关键点自己的Y位置
                kp.control_out.x = normalized_mouse.x - kp.x;
                kp.control_out.y = normalized_mouse.y - kp.y;
            }

            mark_dirty();
        }
    }

    // 鼠标释放
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (is_dragging && selected_line >= 0)
        {
            guide_lines[selected_line].sort_keypoints();
        }
        is_dragging = false;
        dragging_control = 0;
    }

    // 双击添加关键点
    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && in_canvas)
    {
        // 找到最近的引导线（基于曲线在该X位置的Y值）
        int nearest_line = -1;
        float min_dist = FLT_MAX;

        for (int i = 0; i < (int)guide_lines.size(); ++i)
        {
            // 计算引导线在鼠标X位置的Y值
            float curve_y = guide_lines[i].get_curve_y_at(normalized_mouse.x);
            float dist = fabsf(curve_y - normalized_mouse.y);
            if (dist < min_dist)
            {
                min_dist = dist;
                nearest_line = i;
            }
        }

        if (nearest_line >= 0 && min_dist < 0.15f)
        {
            // 在最近的引导线上添加关键点
            auto& gl = guide_lines[nearest_line];
            color_keypoint new_kp;
            new_kp.x = normalized_mouse.x;
            // 新关键点的Y位置设为鼠标点击位置（允许用户自由放置）
            new_kp.y = normalized_mouse.y;
            new_kp.y_locked = false; // 中间点不锁定

            // 采样当前位置的颜色作为新关键点颜色
            auto sampled_lab = gl.sample_color_at_lab(new_kp.x);
            new_kp.color.color = sampled_lab;

            gl.keypoints.push_back(new_kp);
            gl.sort_keypoints();

            // 选中新添加的关键点
            selected_line = nearest_line;
            for (int i = 0; i < (int)gl.keypoints.size(); ++i)
            {
                if (fabsf(gl.keypoints[i].x - new_kp.x) < 0.001f)
                {
                    selected_keypoint = i;
                    break;
                }
            }

            mark_dirty();
        }
    }

    // 右键菜单
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && selected_line >= 0 && selected_keypoint >= 0)
    {
        ImGui::OpenPopup("KeypointContextMenu");
    }

    if (ImGui::BeginPopup("KeypointContextMenu"))
    {
        if (selected_line >= 0 && selected_keypoint >= 0)
        {
            auto& kp = guide_lines[selected_line].keypoints[selected_keypoint];

            if (ImGui::MenuItem(kp.use_bezier ? "切换为线性" : "切换为贝塞尔"))
            {
                kp.use_bezier = !kp.use_bezier;
                mark_dirty();
            }

            if (guide_lines[selected_line].keypoints.size() > 2)
            {
                if (ImGui::MenuItem("删除关键点"))
                {
                    guide_lines[selected_line].keypoints.erase(guide_lines[selected_line].keypoints.begin() + selected_keypoint);
                    selected_keypoint = -1;
                    mark_dirty();
                }
            }

            ImGui::Separator();
            ImGui::Text("颜色编辑:");
            kp.color.render_color_editor();
            if (ImGui::IsItemDeactivatedAfterEdit())
                mark_dirty();
        }
        ImGui::EndPopup();
    }

    // Delete键删除关键点
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && selected_line >= 0 && selected_keypoint >= 0)
    {
        if (guide_lines[selected_line].keypoints.size() > 2)
        {
            guide_lines[selected_line].keypoints.erase(guide_lines[selected_line].keypoints.begin() + selected_keypoint);
            selected_keypoint = -1;
            mark_dirty();
        }
    }
}

// gradient_canvas: 渲染UI控件
inline void gradient_editer::gradient_canvas::render_ui_controls()
{
    // 引导线管理
    ImGui::Text("引导线管理:");
    ImGui::SameLine();

    if (ImGui::SmallButton("+添加"))
    {
        float new_y = 0.5f;
        // 找一个不冲突的Y位置（基于右端固定点）
        for (int i = 0; i < 10; ++i)
        {
            bool conflict = false;
            for (const auto& gl : guide_lines)
            {
                if (fabsf(gl.get_fixed_y() - new_y) < 0.1f)
                {
                    conflict = true;
                    break;
                }
            }
            if (!conflict)
                break;
            new_y = fmodf(new_y + 0.3f, 1.0f);
        }

        char name[32];
        snprintf(name, sizeof(name), "Guide %d", (int)guide_lines.size() + 1);

        // 随机颜色
        ImU32 colors[] = {
            IM_COL32(255, 100, 100, 255), IM_COL32(100, 255, 100, 255), IM_COL32(100, 100, 255, 255), IM_COL32(255, 255, 100, 255), IM_COL32(255, 100, 255, 255), IM_COL32(100, 255, 255, 255),
        };
        ImU32 color = colors[guide_lines.size() % 6];

        guide_lines.push_back(guide_line(new_y, name, color));
        mark_dirty();
    }

    ImGui::SameLine();
    if (ImGui::SmallButton("-删除") && selected_line >= 0 && guide_lines.size() > 1)
    {
        guide_lines.erase(guide_lines.begin() + selected_line);
        selected_line = -1;
        selected_keypoint = -1;
        mark_dirty();
    }

    // 引导线列表
    ImGui::BeginChild("GuideLineList", ImVec2(0, 80), true);
    for (int i = 0; i < (int)guide_lines.size(); ++i)
    {
        auto& gl = guide_lines[i];
        ImGui::PushID(i);

        bool is_selected = (i == selected_line);
        ImVec4 color = ImGui::ColorConvertU32ToFloat4(gl.line_color);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        if (ImGui::Selectable(gl.name.c_str(), is_selected))
        {
            selected_line = i;
            selected_keypoint = -1;
        }

        ImGui::PopStyleColor();
        ImGui::PopID();
    }
    ImGui::EndChild();

    // 选中的引导线属性
    if (selected_line >= 0)
    {
        ImGui::BeginChild("SelectedLineProps", ImVec2(0, 120), true);
        auto& gl = guide_lines[selected_line];

        // 显示固定端Y位置（归一化值和实际值）
        float fixed_y = gl.get_fixed_y();
        ImGui::Text("固定端Y: %.3f (pos: %.2f)", fixed_y, fixed_y * y_range);

        if (selected_keypoint >= 0 && selected_keypoint < (int)gl.keypoints.size())
        {
            auto& kp = gl.keypoints[selected_keypoint];

            // X位置滑块
            ImGui::Text("X:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            if (ImGui::SliderFloat("##kpx", &kp.x, 0.0f, 1.0f))
            {
                gl.sort_keypoints();
                mark_dirty();
            }

            // Y位置滑块（如果未锁定）
            ImGui::Text("Y:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(80);
            if (kp.y_locked)
            {
                ImGui::BeginDisabled();
                ImGui::SliderFloat("##kpy", &kp.y, 0.0f, 1.0f);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "(锁定)");
            }
            else
            {
                if (ImGui::SliderFloat("##kpy", &kp.y, 0.0f, 1.0f))
                    mark_dirty();
            }
            ImGui::SameLine();
            ImGui::Text("(%.1f)", kp.y * y_range); // 显示实际position值

            // 锁定Y位置复选框
            if (ImGui::Checkbox("锁定Y", &kp.y_locked))
                mark_dirty();
        }
        else
        {
            ImGui::Text("点击选择关键点");
        }

        ImGui::EndChild();
    }

    // 颜色空间和输出设置
    ImGui::BeginChild("Settings", ImVec2(0, 145), true);

    ImGui::Text("颜色插值:");
    ImGui::SameLine();
    const char* space_names[] = { "OkLab", "OkHSL", "OkHSV" };
    int current_space = (int)interp_space;
    ImGui::SetNextItemWidth(80);
    if (ImGui::Combo("##interp", &current_space, space_names, 3))
    {
        interp_space = (InterpolationSpace)current_space;
        mark_dirty();
    }

    // Y轴范围参数
    ImGui::Text("Y范围:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80);
    if (ImGui::InputFloat("##yrange", &y_range, 0, 0, "%.1f"))
    {
        y_range = (std::max)(1.0f, (std::min)(y_range, 1000.0f));
        mark_dirty();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Y轴量化范围，用于归一化position值\n例如：范围32表示position=16对应Y=0.5");

    // 色调预览参数（用于颜色位置编辑器背景）
    ImGui::Text("色调预览:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    if (ImGui::SliderFloat("##prevL", &preview_lightness, 0.0f, 1.0f, "L%.2f"))
    {
        mark_dirty();
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    if (ImGui::SliderFloat("##prevS", &preview_saturation, 0.0f, 1.0f, "S%.2f"))
    {
        mark_dirty();
    }

    ImGui::Text("预览尺寸:");
    ImGui::SetNextItemWidth(60);
    if (ImGui::InputInt("##outw", &output_width, 0))
    {
        output_width = (std::max)(16, (std::min)(output_width, 2048));
        mark_dirty();
    }
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    if (ImGui::InputInt("##outh", &output_height, 0))
    {
        output_height = (std::max)(16, (std::min)(output_height, 2048));
        mark_dirty();
    }

    ImGui::EndChild();

    // 导出PNG设置
    ImGui::BeginChild("ExportSettings", ImVec2(0, 120), true);

    ImGui::Text("导出PNG:");

    // 导出尺寸
    ImGui::Text("尺寸:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70);
    ImGui::InputInt("##expw", &export_width, 0);
    export_width = (std::max)(16, (std::min)(export_width, 16384));
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputInt("##exph", &export_height, 0);
    export_height = (std::max)(16, (std::min)(export_height, 16384));

    // 文件路径
    ImGui::Text("路径:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180);
    char path_buf[256];
    strncpy(path_buf, export_path.c_str(), sizeof(path_buf) - 1);
    path_buf[sizeof(path_buf) - 1] = '\0';
    if (ImGui::InputText("##exppath", path_buf, sizeof(path_buf)))
    {
        export_path = path_buf;
    }

    // 保存按钮
    if (ImGui::Button("保存PNG", ImVec2(120, 30)))
    {
        if (save_to_png(export_path, export_width, export_height))
        {
            // 成功提示可以在这里添加
        }
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(%dx%d RGBA)", export_width, export_height);

    ImGui::EndChild();
}

// gradient_canvas: 渲染预览
inline void gradient_editer::gradient_canvas::render_preview()
{
    // 预览图现在叠加在画布上显示，此函数保留用于单独显示预览（如果需要）
    update_preview_texture();
}

// gradient_canvas: 主渲染函数（已由 docking 布局接管，各面板独立渲染）
inline void gradient_editer::gradient_canvas::render()
{
    render_controls_window();
    render_canvas_window();
    render_saturation_preview_window();
    render_final_preview_window();
    render_curve_editor_window();
}

// gradient_canvas: 渲染曲线编辑窗口
inline void gradient_editer::gradient_canvas::render_curve_editor_window()
{
    if (selected_line < 0 || selected_line >= (int)guide_lines.size())
        return;

    auto& gl = guide_lines[selected_line];

    ImGui::Begin("曲线编辑器##ge", nullptr, ImGuiWindowFlags_None);

    ImGui::Text("引导线: %s", gl.name.c_str());
    ImGui::TextDisabled("Hue(红) / Sat(绿) / Lum(蓝)");
    ImGui::Separator();

    if (gl.hsl_curves.render("##hsl_curves", curve_canvas_size))
        mark_dirty();

    ImGui::End();
}

// ==================== Docking 面板窗口实现 ====================

// 控制面板窗口：引导线管理 + 选中属性 + 颜色空间/输出/导出设置
inline void gradient_editer::gradient_canvas::render_controls_window()
{
    ImGui::Begin("控制面板##ge");
    render_ui_controls();
    ImGui::End();
}

// 颜色位置编辑窗口：主画布 + 关键点颜色编辑器
inline void gradient_editer::gradient_canvas::render_canvas_window()
{
    ImGui::Begin("颜色位置编辑##ge");

    update_preview_texture();

    ImGui::Text("颜色位置编辑 (X=位置, Y=垂直位置):");
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
    canvas_p0.x += 40;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_size.x, canvas_p0.y + canvas_size.y);

    ImGui::InvisibleButton("canvas", ImVec2(canvas_size.x + 50, canvas_size.y + 30));
    bool is_hovered = ImGui::IsItemHovered();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    if (hue_preview_texture != 0)
        draw_list->AddImage((ImTextureID)(intptr_t)hue_preview_texture, canvas_p0, canvas_p1);

    render_canvas_content(draw_list, canvas_p0, canvas_p1);

    if (is_hovered || is_dragging)
        handle_interaction(canvas_p0, canvas_p1);

    if (selected_line >= 0 && selected_keypoint >= 0)
    {
        ImGui::Separator();
        ImGui::Text("选中关键点颜色:");
        auto& kp = guide_lines[selected_line].keypoints[selected_keypoint];
        kp.color.render_color_editor();
        if (ImGui::IsItemDeactivatedAfterEdit())
            mark_dirty();
    }

    ImGui::End();
}

// 饱和度预览窗口（黑白）
inline void gradient_editer::gradient_canvas::render_saturation_preview_window()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    ImGui::Begin("饱和度预览##ge");
    ImGui::PopStyleVar();

    if (saturation_preview_texture != 0)
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 0 && avail.y > 0)
            ImGui::Image((ImTextureID)(intptr_t)saturation_preview_texture, avail);
    }
    else
    {
        ImGui::TextDisabled("暂无预览");
    }

    ImGui::End();
}

// 亮度预览窗口（黑白）
inline void gradient_editer::gradient_canvas::render_luminance_preview_window()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    ImGui::Begin("亮度预览##ge");
    ImGui::PopStyleVar();

    if (luminance_preview_texture != 0)
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 0 && avail.y > 0)
            ImGui::Image((ImTextureID)(intptr_t)luminance_preview_texture, avail);
    }
    else
    {
        ImGui::TextDisabled("暂无预览");
    }

    ImGui::End();
}

// 色调预览窗口（固定亮度和饱和度）
inline void gradient_editer::gradient_canvas::render_hue_only_preview_window()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    ImGui::Begin("色调预览##ge");
    ImGui::PopStyleVar();

    if (hue_preview_texture != 0)
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 0 && avail.y > 0)
            ImGui::Image((ImTextureID)(intptr_t)hue_preview_texture, avail);
    }
    else
    {
        ImGui::TextDisabled("暂无预览");
    }

    ImGui::End();
}

// 最终预览窗口（彩色）
inline void gradient_editer::gradient_canvas::render_final_preview_window()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    ImGui::Begin("最终预览##ge");
    ImGui::PopStyleVar();

    if (preview_texture != 0)
    {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x > 0 && avail.y > 0)
            ImGui::Image((ImTextureID)(intptr_t)preview_texture, avail);
    }
    else
    {
        ImGui::TextDisabled("暂无预览");
    }

    ImGui::End();
}

// 配置管理窗口
inline void gradient_editer::render_config_window()
{
    ImGui::Begin("配置管理##ge");
    render_config_ui();
    ImGui::End();
}

// ==================== JSON 序列化/反序列化实现 ====================

inline nlohmann::json gradient_editer::gradient_canvas::to_json() const
{
    nlohmann::json j;
    j["interp_space"] = (int)interp_space;
    j["y_range"] = y_range;
    j["output_width"] = output_width;
    j["output_height"] = output_height;
    j["export_width"] = export_width;
    j["export_height"] = export_height;
    j["export_path"] = export_path;
    j["preview_lightness"] = preview_lightness;
    j["preview_saturation"] = preview_saturation;

    auto serialize_curve_path = [](const curve_path& cp) -> nlohmann::json {
        nlohmann::json cj;
        cj["default_interpolation"] = (int)cp.default_mode;
        nlohmann::json pts = nlohmann::json::array();
        for (size_t i = 0; i < cp.nodes.size(); ++i)
        {
            const auto& nd = cp.nodes[i];
            nlohmann::json pj;
            pj["position"] = { { "x", nd.position.x }, { "y", nd.position.y } };
            pj["handle_in"] = { { "x", nd.handle_in.x }, { "y", nd.handle_in.y } };
            pj["handle_out"] = { { "x", nd.handle_out.x }, { "y", nd.handle_out.y } };
            pj["handle_mode"] = (int)nd.handle_mode;
            if (i < cp.segment_modes.size())
                pj["segment_mode"] = (int)cp.segment_modes[i];
            pts.push_back(pj);
        }
        cj["nodes"] = pts;
        return cj;
    };

    // 序列化引导线
    nlohmann::json lines_json = nlohmann::json::array();
    for (const auto& gl : guide_lines)
    {
        nlohmann::json gl_json;
        gl_json["base_y"] = gl.base_y;
        gl_json["name"] = gl.name;
        gl_json["line_color"] = gl.line_color;

        // 序列化关键点
        nlohmann::json kps_json = nlohmann::json::array();
        for (const auto& kp : gl.keypoints)
        {
            nlohmann::json kp_json;
            kp_json["x"] = kp.x;
            kp_json["y"] = kp.y;
            kp_json["control_in"] = { { "x", kp.control_in.x }, { "y", kp.control_in.y } };
            kp_json["control_out"] = { { "x", kp.control_out.x }, { "y", kp.control_out.y } };
            kp_json["use_bezier"] = kp.use_bezier;
            kp_json["y_locked"] = kp.y_locked;

            // 序列化颜色 (variant)
            if (std::holds_alternative<ok_color::Lab>(kp.color.color))
            {
                const auto& lab = std::get<ok_color::Lab>(kp.color.color);
                kp_json["color"] = { { "type", "lab" }, { "L", lab.L }, { "a", lab.a }, { "b", lab.b } };
            }
            else if (std::holds_alternative<ok_color::HSL>(kp.color.color))
            {
                const auto& hsl = std::get<ok_color::HSL>(kp.color.color);
                kp_json["color"] = { { "type", "hsl" }, { "h", hsl.h }, { "s", hsl.s }, { "l", hsl.l } };
            }
            else if (std::holds_alternative<ok_color::HSV>(kp.color.color))
            {
                const auto& hsv = std::get<ok_color::HSV>(kp.color.color);
                kp_json["color"] = { { "type", "hsv" }, { "h", hsv.h }, { "s", hsv.s }, { "v", hsv.v } };
            }

            kps_json.push_back(kp_json);
        }
        gl_json["keypoints"] = kps_json;

        // 序列化 HSL 曲线族
        if (gl.hsl_curves.count() >= 3)
        {
            gl_json["hue_curve"] = serialize_curve_path(gl.hsl_curves[guide_line::CURVE_HUE]);
            gl_json["saturation_curve"] = serialize_curve_path(gl.hsl_curves[guide_line::CURVE_SAT]);
            gl_json["luminance_curve"] = serialize_curve_path(gl.hsl_curves[guide_line::CURVE_LUM]);
        }

        lines_json.push_back(gl_json);
    }
    j["guide_lines"] = lines_json;

    return j;
}

inline void gradient_editer::gradient_canvas::from_json(const nlohmann::json& j)
{
    auto deserialize_curve_path = [](const nlohmann::json& cj, curve_path& cp) {
        cp.nodes.clear();
        cp.segment_modes.clear();
        cp.function_mode = true;
        cp.range_min = { 0, 0 };
        cp.range_max = { 1, 1 };

        int def_interp = cj.value("default_interpolation", 0);
        cp.default_mode = (def_interp == 1) ? curve_interpolation_mode::cubic_bezier : (curve_interpolation_mode)def_interp;

        auto read_nodes = [&](const nlohmann::json& arr, bool old_format) {
            std::vector<int> per_node_interp;
            for (const auto& pj : arr)
            {
                curve_node nd;
                nd.position.x = pj["position"]["x"].get<float>();
                nd.position.y = pj["position"]["y"].get<float>();

                if (old_format)
                {
                    nd.handle_in.x = pj["control_in"]["x"].get<float>();
                    nd.handle_in.y = pj["control_in"]["y"].get<float>();
                    nd.handle_out.x = pj["control_out"]["x"].get<float>();
                    nd.handle_out.y = pj["control_out"]["y"].get<float>();
                    bool smooth = pj.value("smooth", true);
                    nd.handle_mode = smooth ? curve_handle_mode::smooth : curve_handle_mode::free;
                    int interp = pj.value("interpolation", 0);
                    per_node_interp.push_back(interp == 1 ? (int)curve_interpolation_mode::cubic_bezier : interp);
                }
                else
                {
                    nd.handle_in.x = pj["handle_in"]["x"].get<float>();
                    nd.handle_in.y = pj["handle_in"]["y"].get<float>();
                    nd.handle_out.x = pj["handle_out"]["x"].get<float>();
                    nd.handle_out.y = pj["handle_out"]["y"].get<float>();
                    nd.handle_mode = (curve_handle_mode)pj.value("handle_mode", (int)curve_handle_mode::smooth);
                    if (pj.contains("segment_mode"))
                        per_node_interp.push_back(pj["segment_mode"].get<int>());
                    else
                        per_node_interp.push_back(-1);
                }
                cp.add_node(nd);
            }
            for (size_t i = 0; i + 1 < per_node_interp.size() && i < cp.segment_modes.size(); ++i)
            {
                if (per_node_interp[i] >= 0)
                    cp.segment_modes[i] = (curve_interpolation_mode)per_node_interp[i];
            }
        };

        if (cj.contains("nodes"))
            read_nodes(cj["nodes"], false);
        else if (cj.contains("points"))
            read_nodes(cj["points"], true);
    };

    // 读取基本参数
    interp_space = (InterpolationSpace)j.value("interp_space", 0);
    y_range = j.value("y_range", 30.0f);
    output_width = j.value("output_width", 256);
    output_height = j.value("output_height", 256);
    export_width = j.value("export_width", 256);
    export_height = j.value("export_height", 256);
    export_path = j.value("export_path", std::string("color.png"));
    preview_lightness = j.value("preview_lightness", 0.6f);
    preview_saturation = j.value("preview_saturation", 1.0f);

    // 读取引导线
    guide_lines.clear();
    if (j.contains("guide_lines"))
    {
        for (const auto& gl_json : j["guide_lines"])
        {
            guide_line gl;
            gl.base_y = gl_json.value("base_y", 0.5f);
            gl.name = gl_json.value("name", std::string("Guide"));
            gl.line_color = gl_json.value("line_color", (ImU32)IM_COL32(200, 200, 200, 255));

            // 读取关键点
            gl.keypoints.clear();
            if (gl_json.contains("keypoints"))
            {
                for (const auto& kp_json : gl_json["keypoints"])
                {
                    color_keypoint kp;
                    kp.x = kp_json["x"].get<float>();
                    kp.y = kp_json["y"].get<float>();
                    kp.control_in.x = kp_json["control_in"]["x"].get<float>();
                    kp.control_in.y = kp_json["control_in"]["y"].get<float>();
                    kp.control_out.x = kp_json["control_out"]["x"].get<float>();
                    kp.control_out.y = kp_json["control_out"]["y"].get<float>();
                    kp.use_bezier = kp_json.value("use_bezier", true);
                    kp.y_locked = kp_json.value("y_locked", false);

                    // 读取颜色 (variant)
                    if (kp_json.contains("color"))
                    {
                        const auto& cj = kp_json["color"];
                        std::string type = cj.value("type", std::string("lab"));
                        if (type == "lab")
                        {
                            kp.color.color = ok_color::Lab{ cj["L"].get<float>(), cj["a"].get<float>(), cj["b"].get<float>() };
                        }
                        else if (type == "hsl")
                        {
                            kp.color.color = ok_color::HSL{ cj["h"].get<float>(), cj["s"].get<float>(), cj["l"].get<float>() };
                        }
                        else if (type == "hsv")
                        {
                            kp.color.color = ok_color::HSV{ cj["h"].get<float>(), cj["s"].get<float>(), cj["v"].get<float>() };
                        }
                    }

                    gl.keypoints.push_back(kp);
                }
            }

            // 初始化 hsl_curves 并读取三条曲线
            gl.hsl_curves.curves.clear();
            gl.hsl_curves.range_min = { 0, 0 };
            gl.hsl_curves.range_max = { 1, 1 };
            gl.hsl_curves.add_curve("Hue", IM_COL32(255, 100, 100, 255));
            gl.hsl_curves.add_curve("Sat", IM_COL32(100, 255, 100, 255));
            gl.hsl_curves.add_curve("Lum", IM_COL32(200, 200, 255, 255));
            if (gl_json.contains("hue_curve"))
                deserialize_curve_path(gl_json["hue_curve"], gl.hsl_curves[guide_line::CURVE_HUE]);
            if (gl_json.contains("saturation_curve"))
                deserialize_curve_path(gl_json["saturation_curve"], gl.hsl_curves[guide_line::CURVE_SAT]);
            if (gl_json.contains("luminance_curve"))
                deserialize_curve_path(gl_json["luminance_curve"], gl.hsl_curves[guide_line::CURVE_LUM]);

            guide_lines.push_back(std::move(gl));
        }
    }

    // 重置选择状态并标记需要更新预览
    selected_line = -1;
    selected_keypoint = -1;
    is_dragging = false;
    dragging_control = 0;
    preview_dirty = true;
}

// ==================== 配置管理实现 ====================

inline void gradient_editer::save_config(const std::string& name)
{
    config_entry entry;
    entry.name = name.empty() ? "unnamed" : name;
    entry.timestamp = get_current_timestamp();
    entry.data = canvas.to_json();
    config_list.push_back(entry);
    selected_config_idx = (int)config_list.size() - 1;
    save_configs_to_file();
}

inline void gradient_editer::load_config(int index)
{
    if (index < 0 || index >= (int)config_list.size())
        return;
    try
    {
        canvas.from_json(config_list[index].data);
        selected_config_idx = index;
    }
    catch (const std::exception& e)
    {
        (void)e; // 解析失败时忽略
    }
}

inline void gradient_editer::delete_config(int index)
{
    if (index < 0 || index >= (int)config_list.size())
        return;
    config_list.erase(config_list.begin() + index);
    if (selected_config_idx >= (int)config_list.size())
        selected_config_idx = (int)config_list.size() - 1;
    save_configs_to_file();
}

inline void gradient_editer::save_configs_to_file()
{
    try
    {
        nlohmann::json root;
        nlohmann::json configs_json = nlohmann::json::array();
        for (const auto& entry : config_list)
        {
            nlohmann::json ej;
            ej["name"] = entry.name;
            ej["timestamp"] = entry.timestamp;
            ej["data"] = entry.data;
            configs_json.push_back(ej);
        }
        root["configs"] = configs_json;

        std::ofstream ofs(config_file_path);
        if (ofs.is_open())
        {
            ofs << root.dump(2);
        }
    }
    catch (const std::exception& e)
    {
        (void)e;
    }
}

inline void gradient_editer::load_configs_from_file()
{
    try
    {
        std::ifstream ifs(config_file_path);
        if (!ifs.is_open())
            return;

        nlohmann::json root = nlohmann::json::parse(ifs);
        config_list.clear();

        if (root.contains("configs"))
        {
            for (const auto& ej : root["configs"])
            {
                config_entry entry;
                entry.name = ej.value("name", std::string("unnamed"));
                entry.timestamp = ej.value("timestamp", std::string(""));
                if (ej.contains("data"))
                    entry.data = ej["data"];
                config_list.push_back(entry);
            }
        }
    }
    catch (const std::exception& e)
    {
        (void)e;
    }
}

inline void gradient_editer::render_config_ui()
{
    ImGui::Text("配置管理");

    // 配置名称输入 + 保存按钮
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##config_name", config_name_buf, sizeof(config_name_buf));
    ImGui::SameLine();
    if (ImGui::Button("保存当前配置"))
    {
        save_config(config_name_buf);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(共%d个配置)", (int)config_list.size());

    // 配置文件路径
    ImGui::Text("配置文件:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(250);
    char path_buf[256];
    strncpy(path_buf, config_file_path.c_str(), sizeof(path_buf) - 1);
    path_buf[sizeof(path_buf) - 1] = '\0';
    if (ImGui::InputText("##config_path", path_buf, sizeof(path_buf)))
    {
        config_file_path = path_buf;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("重新加载"))
    {
        load_configs_from_file();
    }

    // 配置列表
    if (!config_list.empty())
    {
        ImGui::BeginChild("ConfigList", ImVec2(0, (std::min)(200.0f, (float)config_list.size() * 28.0f + 10.0f)), true);
        for (int i = 0; i < (int)config_list.size(); ++i)
        {
            ImGui::PushID(i);

            bool is_selected = (i == selected_config_idx);

            // 使用带颜色的选择按钮区分当前选中项
            if (is_selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.4f, 1.0f));
            }

            // 格式化显示：名称 + 时间戳
            char label[512];
            snprintf(label, sizeof(label), "%s%s  |  %s", is_selected ? "> " : "  ", config_list[i].name.c_str(), config_list[i].timestamp.c_str());

            if (ImGui::Selectable(label, is_selected, ImGuiSelectableFlags_None, ImVec2(ImGui::GetContentRegionAvail().x - 30, 0)))
            {
                load_config(i);
            }

            if (is_selected)
            {
                ImGui::PopStyleColor();
            }

            // 删除按钮
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 15);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            if (ImGui::SmallButton("X"))
            {
                delete_config(i);
                ImGui::PopStyleColor();
                ImGui::PopID();
                break; // 删除后跳出循环，避免越界
            }
            ImGui::PopStyleColor();

            ImGui::PopID();
        }
        ImGui::EndChild();
    }
    else
    {
        ImGui::TextDisabled("暂无保存的配置");
    }
}

#pragma once
// Copyright(c) 2021 Björn Ottosson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this softwareand associated documentation files(the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and /or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions :
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cfloat>
#include <cmath>

namespace ok_color
{

    // struct Lab
    // {
    //     float L;
    //     float a;
    //     float b;
    // };
    // struct RGB
    // {
    //     float r;
    //     float g;
    //     float b;
    // };
    // struct HSV
    // {
    //     float h;
    //     float s;
    //     float v;
    // };
    // struct HSL
    // {
    //     float h;
    //     float s;
    //     float l;
    // };
    struct LC
    {
        float L;
        float C;
    };

    // Alternative representation of (L_cusp, C_cusp)
    // Encoded so S = C_cusp/L_cusp and T = C_cusp/(1-L_cusp)
    // The maximum value for C in the triangle is then found as fmin(S*L, T*(1-L)), for a given L
    struct ST
    {
        float S;
        float T;
    };

    constexpr float pi = 3.1415926535897932384626433832795028841971693993751058209749445923078164062f;

    static inline float clamp(float x, float min, float max)
    {
        if (x < min)
            return min;
        if (x > max)
            return max;

        return x;
    }

    static inline float sgn(float x)
    {
        return (float)(0.f < x) - (float)(x < 0.f);
    }

    static inline float srgb_transfer_function(float a)
    {
        return .0031308f >= a ? 12.92f * a : 1.055f * powf(a, .4166666666666667f) - .055f;
    }

    static inline float srgb_transfer_function_inv(float a)
    {
        return .04045f < a ? powf((a + .055f) / 1.055f, 2.4f) : a / 12.92f;
    }

    static inline Lab linear_srgb_to_oklab(RGB c)
    {
        float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
        float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
        float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

        float l_ = cbrtf(l);
        float m_ = cbrtf(m);
        float s_ = cbrtf(s);

        return {
            0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_,
            1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_,
            0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_,
        };
    }

    static inline RGB oklab_to_linear_srgb(Lab c)
    {
        float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
        float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
        float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

        float l = l_ * l_ * l_;
        float m = m_ * m_ * m_;
        float s = s_ * s_ * s_;

        return {
            +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
            -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
            -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
        };
    }

    // Finds the maximum saturation possible for a given hue that fits in sRGB
    // Saturation here is defined as S = C/L
    // a and b must be normalized so a^2 + b^2 == 1
    static inline float compute_max_saturation(float a, float b)
    {
        // Max saturation will be when one of r, g or b goes below zero.

        // Select different coefficients depending on which component goes below zero first
        float k0, k1, k2, k3, k4, wl, wm, ws;

        if (-1.88170328f * a - 0.80936493f * b > 1)
        {
            // Red component
            k0 = +1.19086277f;
            k1 = +1.76576728f;
            k2 = +0.59662641f;
            k3 = +0.75515197f;
            k4 = +0.56771245f;
            wl = +4.0767416621f;
            wm = -3.3077115913f;
            ws = +0.2309699292f;
        }
        else if (1.81444104f * a - 1.19445276f * b > 1)
        {
            // Green component
            k0 = +0.73956515f;
            k1 = -0.45954404f;
            k2 = +0.08285427f;
            k3 = +0.12541070f;
            k4 = +0.14503204f;
            wl = -1.2684380046f;
            wm = +2.6097574011f;
            ws = -0.3413193965f;
        }
        else
        {
            // Blue component
            k0 = +1.35733652f;
            k1 = -0.00915799f;
            k2 = -1.15130210f;
            k3 = -0.50559606f;
            k4 = +0.00692167f;
            wl = -0.0041960863f;
            wm = -0.7034186147f;
            ws = +1.7076147010f;
        }

        // Approximate max saturation using a polynomial:
        float S = k0 + k1 * a + k2 * b + k3 * a * a + k4 * a * b;

        // Do one step Halley's method to get closer
        // this gives an error less than 10e6, except for some blue hues where the dS/dh is close to infinite
        // this should be sufficient for most applications, otherwise do two/three steps

        float k_l = +0.3963377774f * a + 0.2158037573f * b;
        float k_m = -0.1055613458f * a - 0.0638541728f * b;
        float k_s = -0.0894841775f * a - 1.2914855480f * b;

        {
            float l_ = 1.f + S * k_l;
            float m_ = 1.f + S * k_m;
            float s_ = 1.f + S * k_s;

            float l = l_ * l_ * l_;
            float m = m_ * m_ * m_;
            float s = s_ * s_ * s_;

            float l_dS = 3.f * k_l * l_ * l_;
            float m_dS = 3.f * k_m * m_ * m_;
            float s_dS = 3.f * k_s * s_ * s_;

            float l_dS2 = 6.f * k_l * k_l * l_;
            float m_dS2 = 6.f * k_m * k_m * m_;
            float s_dS2 = 6.f * k_s * k_s * s_;

            float f = wl * l + wm * m + ws * s;
            float f1 = wl * l_dS + wm * m_dS + ws * s_dS;
            float f2 = wl * l_dS2 + wm * m_dS2 + ws * s_dS2;

            S = S - f * f1 / (f1 * f1 - 0.5f * f * f2);
        }

        return S;
    }

    // finds L_cusp and C_cusp for a given hue
    // a and b must be normalized so a^2 + b^2 == 1
    static inline LC find_cusp(float a, float b)
    {
        // First, find the maximum saturation (saturation S = C/L)
        float S_cusp = compute_max_saturation(a, b);

        // Convert to linear sRGB to find the first point where at least one of r,g or b >= 1:
        RGB rgb_at_max = oklab_to_linear_srgb({ 1, S_cusp * a, S_cusp * b });
        float L_cusp = cbrtf(1.f / fmax(fmax(rgb_at_max.r, rgb_at_max.g), rgb_at_max.b));
        float C_cusp = L_cusp * S_cusp;

        return { L_cusp, C_cusp };
    }

    // Finds intersection of the line defined by
    // L = L0 * (1 - t) + t * L1;
    // C = t * C1;
    // a and b must be normalized so a^2 + b^2 == 1
    static inline float find_gamut_intersection(float a, float b, float L1, float C1, float L0, LC cusp)
    {
        // Find the intersection for upper and lower half seprately
        float t;
        if (((L1 - L0) * cusp.C - (cusp.L - L0) * C1) <= 0.f)
        {
            // Lower half

            t = cusp.C * L0 / (C1 * cusp.L + cusp.C * (L0 - L1));
        }
        else
        {
            // Upper half

            // First intersect with triangle
            t = cusp.C * (L0 - 1.f) / (C1 * (cusp.L - 1.f) + cusp.C * (L0 - L1));

            // Then one step Halley's method
            {
                float dL = L1 - L0;
                float dC = C1;

                float k_l = +0.3963377774f * a + 0.2158037573f * b;
                float k_m = -0.1055613458f * a - 0.0638541728f * b;
                float k_s = -0.0894841775f * a - 1.2914855480f * b;

                float l_dt = dL + dC * k_l;
                float m_dt = dL + dC * k_m;
                float s_dt = dL + dC * k_s;

                // If higher accuracy is required, 2 or 3 iterations of the following block can be used:
                {
                    float L = L0 * (1.f - t) + t * L1;
                    float C = t * C1;

                    float l_ = L + C * k_l;
                    float m_ = L + C * k_m;
                    float s_ = L + C * k_s;

                    float l = l_ * l_ * l_;
                    float m = m_ * m_ * m_;
                    float s = s_ * s_ * s_;

                    float ldt = 3 * l_dt * l_ * l_;
                    float mdt = 3 * m_dt * m_ * m_;
                    float sdt = 3 * s_dt * s_ * s_;

                    float ldt2 = 6 * l_dt * l_dt * l_;
                    float mdt2 = 6 * m_dt * m_dt * m_;
                    float sdt2 = 6 * s_dt * s_dt * s_;

                    float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s - 1;
                    float r1 = 4.0767416621f * ldt - 3.3077115913f * mdt + 0.2309699292f * sdt;
                    float r2 = 4.0767416621f * ldt2 - 3.3077115913f * mdt2 + 0.2309699292f * sdt2;

                    float u_r = r1 / (r1 * r1 - 0.5f * r * r2);
                    float t_r = -r * u_r;

                    float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s - 1;
                    float g1 = -1.2684380046f * ldt + 2.6097574011f * mdt - 0.3413193965f * sdt;
                    float g2 = -1.2684380046f * ldt2 + 2.6097574011f * mdt2 - 0.3413193965f * sdt2;

                    float u_g = g1 / (g1 * g1 - 0.5f * g * g2);
                    float t_g = -g * u_g;

                    float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s - 1;
                    float b1 = -0.0041960863f * ldt - 0.7034186147f * mdt + 1.7076147010f * sdt;
                    float b2 = -0.0041960863f * ldt2 - 0.7034186147f * mdt2 + 1.7076147010f * sdt2;

                    float u_b = b1 / (b1 * b1 - 0.5f * b * b2);
                    float t_b = -b * u_b;

                    t_r = u_r >= 0.f ? t_r : FLT_MAX;
                    t_g = u_g >= 0.f ? t_g : FLT_MAX;
                    t_b = u_b >= 0.f ? t_b : FLT_MAX;

                    t += fmin(t_r, fmin(t_g, t_b));
                }
            }
        }

        return t;
    }

    static inline float find_gamut_intersection(float a, float b, float L1, float C1, float L0)
    {
        // Find the cusp of the gamut triangle
        LC cusp = find_cusp(a, b);

        return find_gamut_intersection(a, b, L1, C1, L0, cusp);
    }

    static inline RGB gamut_clip_preserve_chroma(RGB rgb)
    {
        if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
            return rgb;

        Lab lab = linear_srgb_to_oklab(rgb);

        float L = lab.L;
        float eps = 0.00001f;
        float C = fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float L0 = clamp(L, 0, 1);

        float t = find_gamut_intersection(a_, b_, L, C, L0);
        float L_clipped = L0 * (1 - t) + t * L;
        float C_clipped = t * C;

        return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
    }

    static inline RGB gamut_clip_project_to_0_5(RGB rgb)
    {
        if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
            return rgb;

        Lab lab = linear_srgb_to_oklab(rgb);

        float L = lab.L;
        float eps = 0.00001f;
        float C = fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float L0 = 0.5;

        float t = find_gamut_intersection(a_, b_, L, C, L0);
        float L_clipped = L0 * (1 - t) + t * L;
        float C_clipped = t * C;

        return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
    }

    static inline RGB gamut_clip_project_to_L_cusp(RGB rgb)
    {
        if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
            return rgb;

        Lab lab = linear_srgb_to_oklab(rgb);

        float L = lab.L;
        float eps = 0.00001f;
        float C = fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        // The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
        LC cusp = find_cusp(a_, b_);

        float L0 = cusp.L;

        float t = find_gamut_intersection(a_, b_, L, C, L0);

        float L_clipped = L0 * (1 - t) + t * L;
        float C_clipped = t * C;

        return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
    }

    static inline RGB gamut_clip_adaptive_L0_0_5(RGB rgb, float alpha = 0.05f)
    {
        if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
            return rgb;

        Lab lab = linear_srgb_to_oklab(rgb);

        float L = lab.L;
        float eps = 0.00001f;
        float C = fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float Ld = L - 0.5f;
        float e1 = 0.5f + fabs(Ld) + alpha * C;
        float L0 = 0.5f * (1.f + sgn(Ld) * (e1 - sqrtf(e1 * e1 - 2.f * fabs(Ld))));

        float t = find_gamut_intersection(a_, b_, L, C, L0);
        float L_clipped = L0 * (1.f - t) + t * L;
        float C_clipped = t * C;

        return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
    }

    static inline RGB gamut_clip_adaptive_L0_L_cusp(RGB rgb, float alpha = 0.05f)
    {
        if (rgb.r < 1 && rgb.g < 1 && rgb.b < 1 && rgb.r > 0 && rgb.g > 0 && rgb.b > 0)
            return rgb;

        Lab lab = linear_srgb_to_oklab(rgb);

        float L = lab.L;
        float eps = 0.00001f;
        float C = fmax(eps, sqrtf(lab.a * lab.a + lab.b * lab.b));
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        // The cusp is computed here and in find_gamut_intersection, an optimized solution would only compute it once.
        LC cusp = find_cusp(a_, b_);

        float Ld = L - cusp.L;
        float k = 2.f * (Ld > 0 ? 1.f - cusp.L : cusp.L);

        float e1 = 0.5f * k + fabs(Ld) + alpha * C / k;
        float L0 = cusp.L + 0.5f * (sgn(Ld) * (e1 - sqrtf(e1 * e1 - 2.f * k * fabs(Ld))));

        float t = find_gamut_intersection(a_, b_, L, C, L0);
        float L_clipped = L0 * (1.f - t) + t * L;
        float C_clipped = t * C;

        return oklab_to_linear_srgb({ L_clipped, C_clipped * a_, C_clipped * b_ });
    }

    static inline float toe(float x)
    {
        constexpr float k_1 = 0.206f;
        constexpr float k_2 = 0.03f;
        constexpr float k_3 = (1.f + k_1) / (1.f + k_2);
        return 0.5f * (k_3 * x - k_1 + sqrtf((k_3 * x - k_1) * (k_3 * x - k_1) + 4 * k_2 * k_3 * x));
    }

    static inline float toe_inv(float x)
    {
        constexpr float k_1 = 0.206f;
        constexpr float k_2 = 0.03f;
        constexpr float k_3 = (1.f + k_1) / (1.f + k_2);
        return (x * x + k_1 * x) / (k_3 * (x + k_2));
    }

    static inline ST to_ST(LC cusp)
    {
        float L = cusp.L;
        float C = cusp.C;
        return { C / L, C / (1 - L) };
    }

    // Returns a smooth approximation of the location of the cusp
    // This polynomial was created by an optimization process
    // It has been designed so that S_mid < S_max and T_mid < T_max
    static inline ST get_ST_mid(float a_, float b_)
    {
        float S = 0.11516993f + 1.f / (+7.44778970f + 4.15901240f * b_ +
                                       a_ * (-2.19557347f + 1.75198401f * b_ + a_ * (-2.13704948f - 10.02301043f * b_ + a_ * (-4.24894561f + 5.38770819f * b_ + 4.69891013f * a_))));

        float T = 0.11239642f +
                  1.f / (+1.61320320f - 0.68124379f * b_ + a_ * (+0.40370612f + 0.90148123f * b_ + a_ * (-0.27087943f + 0.61223990f * b_ + a_ * (+0.00299215f - 0.45399568f * b_ - 0.14661872f * a_))));

        return { S, T };
    }

    struct Cs
    {
        float C_0;
        float C_mid;
        float C_max;
    };
    static inline Cs get_Cs(float L, float a_, float b_)
    {
        LC cusp = find_cusp(a_, b_);

        float C_max = find_gamut_intersection(a_, b_, L, 1, L, cusp);
        ST ST_max = to_ST(cusp);

        // Scale factor to compensate for the curved part of gamut shape:
        float k = C_max / fmin((L * ST_max.S), (1 - L) * ST_max.T);

        float C_mid;
        {
            ST ST_mid = get_ST_mid(a_, b_);

            // Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
            float C_a = L * ST_mid.S;
            float C_b = (1.f - L) * ST_mid.T;
            C_mid = 0.9f * k * sqrtf(sqrtf(1.f / (1.f / (C_a * C_a * C_a * C_a) + 1.f / (C_b * C_b * C_b * C_b))));
        }

        float C_0;
        {
            // for C_0, the shape is independent of hue, so ST are constant. Values picked to roughly be the average values of ST.
            float C_a = L * 0.4f;
            float C_b = (1.f - L) * 0.8f;

            // Use a soft minimum function, instead of a sharp triangle shape to get a smooth value for chroma.
            C_0 = sqrtf(1.f / (1.f / (C_a * C_a) + 1.f / (C_b * C_b)));
        }

        return { C_0, C_mid, C_max };
    }

    static inline RGB okhsl_to_srgb(HSL hsl)
    {
        float h = hsl.h;
        float s = hsl.s;
        float l = hsl.l;

        if (l == 1.0f)
        {
            return { 1.f, 1.f, 1.f };
        }

        else if (l == 0.f)
        {
            return { 0.f, 0.f, 0.f };
        }

        float a_ = cosf(2.f * pi * h);
        float b_ = sinf(2.f * pi * h);
        float L = toe_inv(l);

        Cs cs = get_Cs(L, a_, b_);
        float C_0 = cs.C_0;
        float C_mid = cs.C_mid;
        float C_max = cs.C_max;

        float mid = 0.8f;
        float mid_inv = 1.25f;

        float C, t, k_0, k_1, k_2;

        if (s < mid)
        {
            t = mid_inv * s;

            k_1 = mid * C_0;
            k_2 = (1.f - k_1 / C_mid);

            C = t * k_1 / (1.f - k_2 * t);
        }
        else
        {
            t = (s - mid) / (1 - mid);

            k_0 = C_mid;
            k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
            k_2 = (1.f - (k_1) / (C_max - C_mid));

            C = k_0 + t * k_1 / (1.f - k_2 * t);
        }

        RGB rgb = oklab_to_linear_srgb({ L, C * a_, C * b_ });
        return {
            srgb_transfer_function(rgb.r),
            srgb_transfer_function(rgb.g),
            srgb_transfer_function(rgb.b),
        };
    }

    static inline HSL srgb_to_okhsl(RGB rgb)
    {
        Lab lab = linear_srgb_to_oklab({ srgb_transfer_function_inv(rgb.r), srgb_transfer_function_inv(rgb.g), srgb_transfer_function_inv(rgb.b) });

        float C = sqrtf(lab.a * lab.a + lab.b * lab.b);
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float L = lab.L;
        float h = 0.5f + 0.5f * atan2f(-lab.b, -lab.a) / pi;

        Cs cs = get_Cs(L, a_, b_);
        float C_0 = cs.C_0;
        float C_mid = cs.C_mid;
        float C_max = cs.C_max;

        // Inverse of the interpolation in okhsl_to_srgb:

        float mid = 0.8f;
        float mid_inv = 1.25f;

        float s;
        if (C < C_mid)
        {
            float k_1 = mid * C_0;
            float k_2 = (1.f - k_1 / C_mid);

            float t = C / (k_1 + k_2 * C);
            s = t * mid;
        }
        else
        {
            float k_0 = C_mid;
            float k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
            float k_2 = (1.f - (k_1) / (C_max - C_mid));

            float t = (C - k_0) / (k_1 + k_2 * (C - k_0));
            s = mid + (1.f - mid) * t;
        }

        float l = toe(L);
        return { h, s, l };
    }

    static inline RGB okhsv_to_srgb(HSV hsv)
    {
        float h = hsv.h;
        float s = hsv.s;
        float v = hsv.v;

        float a_ = cosf(2.f * pi * h);
        float b_ = sinf(2.f * pi * h);

        LC cusp = find_cusp(a_, b_);
        ST ST_max = to_ST(cusp);
        float S_max = ST_max.S;
        float T_max = ST_max.T;
        float S_0 = 0.5f;
        float k = 1 - S_0 / S_max;

        // first we compute L and V as if the gamut is a perfect triangle:

        // L, C when v==1:
        float L_v = 1 - s * S_0 / (S_0 + T_max - T_max * k * s);
        float C_v = s * T_max * S_0 / (S_0 + T_max - T_max * k * s);

        float L = v * L_v;
        float C = v * C_v;

        // then we compensate for both toe and the curved top part of the triangle:
        float L_vt = toe_inv(L_v);
        float C_vt = C_v * L_vt / L_v;

        float L_new = toe_inv(L);
        C = C * L_new / L;
        L = L_new;

        RGB rgb_scale = oklab_to_linear_srgb({ L_vt, a_ * C_vt, b_ * C_vt });
        float scale_L = cbrtf(1.f / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.f)));

        L = L * scale_L;
        C = C * scale_L;

        RGB rgb = oklab_to_linear_srgb({ L, C * a_, C * b_ });
        return {
            srgb_transfer_function(rgb.r),
            srgb_transfer_function(rgb.g),
            srgb_transfer_function(rgb.b),
        };
    }

    static inline HSV srgb_to_okhsv(RGB rgb)
    {
        Lab lab = linear_srgb_to_oklab({ srgb_transfer_function_inv(rgb.r), srgb_transfer_function_inv(rgb.g), srgb_transfer_function_inv(rgb.b) });

        float C = sqrtf(lab.a * lab.a + lab.b * lab.b);
        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float L = lab.L;
        float h = 0.5f + 0.5f * atan2f(-lab.b, -lab.a) / pi;

        LC cusp = find_cusp(a_, b_);
        ST ST_max = to_ST(cusp);
        float S_max = ST_max.S;
        float T_max = ST_max.T;
        float S_0 = 0.5f;
        float k = 1 - S_0 / S_max;

        // first we find L_v, C_v, L_vt and C_vt

        float t = T_max / (C + L * T_max);
        float L_v = t * L;
        float C_v = t * C;

        float L_vt = toe_inv(L_v);
        float C_vt = C_v * L_vt / L_v;

        // we can then use these to invert the step that compensates for the toe and the curved top part of the triangle:
        RGB rgb_scale = oklab_to_linear_srgb({ L_vt, a_ * C_vt, b_ * C_vt });
        float scale_L = cbrtf(1.f / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.f)));

        L = L / scale_L;
        C = C / scale_L;

        C = C * toe(L) / L;
        L = toe(L);

        // we can now compute v and s:

        float v = L / L_v;
        float s = (S_0 + T_max) * C_v / ((T_max * S_0) + T_max * k * C_v);

        return { h, s, v };
    }

    static inline Lab okhsl_to_oklab(HSL hsl)
    {
        float h = hsl.h;
        float s = hsl.s;
        float l = hsl.l;

        // 边界情况：黑色或白色
        if (l < 1e-8f)
        {
            return { 0.f, 0.f, 0.f };
        }
        if (l > (1.f - 1e-3f))
        {
            return { 1.f, 0.f, 0.f }; // 白色：L=1, a=0, b=0
        }

        float a_ = cosf(2.f * pi * h);
        float b_ = sinf(2.f * pi * h);
        float L = toe_inv(l);

        Cs cs = get_Cs(L, a_, b_);
        float C_0 = cs.C_0;
        float C_mid = cs.C_mid;
        float C_max = cs.C_max;

        float mid = 0.8f;
        float mid_inv = 1.25f;

        float C, t, k_0, k_1, k_2;

        if (s < mid)
        {
            t = mid_inv * s;

            k_1 = mid * C_0;
            k_2 = (1.f - k_1 / C_mid);

            C = t * k_1 / (1.f - k_2 * t);
        }
        else
        {
            t = (s - mid) / (1 - mid);

            k_0 = C_mid;
            k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
            k_2 = (1.f - (k_1) / (C_max - C_mid));

            C = k_0 + t * k_1 / (1.f - k_2 * t);
        }

        return { L, C * a_, C * b_ };
    }
    static inline RGB oklab_to_srgb(Lab lab)
    {
        RGB rgb = oklab_to_linear_srgb(lab);
        return {
            srgb_transfer_function(rgb.r),
            srgb_transfer_function(rgb.g),
            srgb_transfer_function(rgb.b),
        };
    }
    static inline Lab srgb_to_oklab(RGB rgb)
    {
        return linear_srgb_to_oklab({ srgb_transfer_function_inv(rgb.r), srgb_transfer_function_inv(rgb.g), srgb_transfer_function_inv(rgb.b) });
    }

    // 直接转换函数（无 sRGB 中转，避免精度损失）
    static inline HSL oklab_to_okhsl(Lab lab)
    {
        float C = sqrtf(lab.a * lab.a + lab.b * lab.b);
        float L = lab.L;

        // 边界情况：无色彩（黑/白/灰色）
        if (C == 0.0f || L == 0.0f || L == 1.0f)
        {
            return { 0.f, 0.f, toe(L) };
        }

        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float h = 0.5f + 0.5f * atan2f(-lab.b, -lab.a) / pi;

        Cs cs = get_Cs(L, a_, b_);
        float C_0 = cs.C_0;
        float C_mid = cs.C_mid;
        float C_max = cs.C_max;

        float mid = 0.8f;
        float mid_inv = 1.25f;

        float s;
        if (C < C_mid)
        {
            float k_1 = mid * C_0;
            float k_2 = (1.f - k_1 / C_mid);

            float t = C / (k_1 + k_2 * C);
            s = t * mid;
        }
        else
        {
            float k_0 = C_mid;
            float k_1 = (1.f - mid) * C_mid * C_mid * mid_inv * mid_inv / C_0;
            float k_2 = (1.f - (k_1) / (C_max - C_mid));

            float t = (C - k_0) / (k_1 + k_2 * (C - k_0));
            s = mid + (1.f - mid) * t;
        }

        float l = toe(L);
        return { h, s, l };
    }

    static inline HSV oklab_to_okhsv(Lab lab)
    {
        float C = sqrtf(lab.a * lab.a + lab.b * lab.b);
        float L = lab.L;

        // 边界情况：无色彩或黑色
        if (C == 0.0f || L == 0.0f || L == 1.0f)
        {
            return { 0.f, 0.f, toe(L) };
        }

        float a_ = lab.a / C;
        float b_ = lab.b / C;

        float h = 0.5f + 0.5f * atan2f(-lab.b, -lab.a) / pi;

        LC cusp = find_cusp(a_, b_);
        ST ST_max = to_ST(cusp);
        float S_max = ST_max.S;
        float T_max = ST_max.T;
        float S_0 = 0.5f;
        float k = 1 - S_0 / S_max;

        float t = T_max / (C + L * T_max);
        float L_v = t * L;
        float C_v = t * C;

        // 防止 L_v 为零时除以零
        if (L_v == 0.0f)
        {
            return { h, 0.f, 0.f };
        }

        float L_vt = toe_inv(L_v);
        float C_vt = C_v * L_vt / L_v;

        RGB rgb_scale = oklab_to_linear_srgb({ L_vt, a_ * C_vt, b_ * C_vt });
        float scale_L = cbrtf(1.f / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.f)));

        L = L / scale_L;
        C = C / scale_L;

        // 防止 L 为零时除以零
        if (L == 0.0f)
        {
            return { h, 0.f, 0.f };
        }

        C = C * toe(L) / L;
        L = toe(L);

        float v = L / L_v;
        float s = (S_0 + T_max) * C_v / ((T_max * S_0) + T_max * k * C_v);

        return { h, s, v };
    }

    static inline Lab okhsv_to_oklab(HSV hsv)
    {
        float h = hsv.h;
        float s = hsv.s;
        float v = hsv.v;

        // 边界情况：黑色
        if (v == 0.0f)
        {
            return { 0.f, 0.f, 0.f };
        }

        float a_ = cosf(2.f * pi * h);
        float b_ = sinf(2.f * pi * h);

        LC cusp = find_cusp(a_, b_);
        ST ST_max = to_ST(cusp);
        float S_max = ST_max.S;
        float T_max = ST_max.T;
        float S_0 = 0.5f;
        float k = 1 - S_0 / S_max;

        float L_v = 1 - s * S_0 / (S_0 + T_max - T_max * k * s);
        float C_v = s * T_max * S_0 / (S_0 + T_max - T_max * k * s);

        float L = v * L_v;
        float C = v * C_v;

        // 防止 L 为零时除以零
        if (L == 0.0f)
        {
            return { 0.f, 0.f, 0.f };
        }

        float L_vt = toe_inv(L_v);
        float C_vt = C_v * L_vt / L_v;

        float L_new = toe_inv(L);
        C = C * L_new / L;
        L = L_new;

        RGB rgb_scale = oklab_to_linear_srgb({ L_vt, a_ * C_vt, b_ * C_vt });
        float scale_L = cbrtf(1.f / fmax(fmax(rgb_scale.r, rgb_scale.g), fmax(rgb_scale.b, 0.f)));

        L = L * scale_L;
        C = C * scale_L;

        return { L, C * a_, C * b_ };
    }

    static inline HSL okhsv_to_okhsl(HSV hsv)
    {
        return oklab_to_okhsl(okhsv_to_oklab(hsv));
    }

    static inline HSV okhsl_to_okhsv(HSL hsl)
    {
        return oklab_to_okhsv(okhsl_to_oklab(hsl));
    }
} // namespace ok_color
