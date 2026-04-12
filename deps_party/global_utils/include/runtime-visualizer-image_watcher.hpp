#pragma once
#include "runtime-visualizer-signal.hpp"

#include <imgui.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>

class image_watcher
{
    // 全局视图设置
    struct view_settings
    {
        bool link_views = false;
        bool auto_maximize_contrast = true;
        bool pseudo_color_1ch = false;
        bool ignore_alpha_4ch = true;
        bool hexadecimal_display = false;
    };
    using shared_settings_ptr = std::shared_ptr<view_settings>;
    shared_settings_ptr settings = std::make_shared<view_settings>();

    // 按图像尺寸存储的链接视图状态
    struct linked_view_state
    {
        float zoom = 1.0f;
        ImVec2 offset = { 0, 0 };
    };
    using linked_views_ptr = std::shared_ptr<std::map<std::pair<int, int>, linked_view_state>>;
    linked_views_ptr linked_views = std::make_shared<std::map<std::pair<int, int>, linked_view_state>>();

    class image_viewer
    {
        std::string name;
        std::reference_wrapper<const cv::Mat> image;
        std::optional<cv::Mat> cached_image;
        shared_settings_ptr ref_settings = nullptr;
        linked_views_ptr ref_linked_views = nullptr;

        bool empty = true;
        bool changed = true;
        bool expanded = true;

        GLuint texture_id = 0;
        GLuint texture_width = 0;
        GLuint texture_height = 0;
        GLuint thumb_texture_width = 0;
        GLuint thumb_texture_height = 0;
        std::string type_info;

    public:
        signal_ptr<> on_updated = signal<>::create();
        signal_ptr<cv::Rect> on_middle_selection_changed = signal<cv::Rect>::create();
        signal_ptr<cv::Rect> on_right_selection_changed = signal<cv::Rect>::create();

    public:
        struct viewer_state
        {
            float zoom = 1.0f;
            ImVec2 offset = { 0, 0 };
            std::string pixel_info_text;
            int hover_x = -1;
            int hover_y = -1;
            bool need_fit = true;

            // 窗宽窗位
            bool use_window_level = false;
            double window_center = 0.0;
            double window_width = 1.0;

            // 中键框选状态
            bool is_selecting = false;
            ImVec2 selection_start = { 0, 0 }; // 图像坐标
            ImVec2 selection_end = { 0, 0 };   // 图像坐标

            // 框选区域统计
            bool has_roi_stats = false;
            double roi_min = 0.0;
            double roi_max = 0.0;
            double roi_mean = 0.0;
            double roi_stddev = 0.0;
            std::vector<float> roi_histogram;
            float roi_hist_max = 0.0f;

            // 右键框选映射
            bool is_right_selecting = false;
            ImVec2 right_selection_start = { 0, 0 };
            ImVec2 right_selection_end = { 0, 0 };
            bool has_right_selection = false;
            cv::Rect right_selection_roi = {};

        } view;

    public:
        image_viewer(const std::string& name, const cv::Mat& image, shared_settings_ptr settings, linked_views_ptr linked)
            : name(name), image(std::ref(image)), ref_settings(settings), ref_linked_views(linked)
        {
        }
        ~image_viewer() { destroy_texture(); }
        void persist()
        {
            cached_image = image.get().clone();
            image = std::cref(cached_image.value());
        }
        void update() { changed = true; }
        void sync_state()
        {
            if (!changed)
                return;
            auto type_to_string = [](int type) {
                int depth = type & CV_MAT_DEPTH_MASK;
                int channels = 1 + (type >> CV_CN_SHIFT);

                const char* depth_str = nullptr;
                switch (depth)
                {
                    case CV_8U: depth_str = "uint8"; break;
                    case CV_8S: depth_str = "int8"; break;
                    case CV_16U: depth_str = "uint16"; break;
                    case CV_16S: depth_str = "int16"; break;
                    case CV_32S: depth_str = "int32"; break;
                    case CV_32F: depth_str = "float32"; break;
                    case CV_64F: depth_str = "float64"; break;
                    default: depth_str = "unknown"; break;
                }
                return std::to_string(channels) + " x " + depth_str;
            };
            auto apply_pseudo_color = [](const cv::Mat& gray) {
                cv::Mat colored;
                cv::applyColorMap(gray, colored, cv::COLORMAP_JET);
                cv::Mat rgba;
                cv::cvtColor(colored, rgba, cv::COLOR_BGR2RGBA);
                return rgba;
            };
            auto convert_to_rgba = [this, &apply_pseudo_color](const cv::Mat& src) {
                cv::Mat rgba;
                bool auto_contrast = ref_settings ? ref_settings->auto_maximize_contrast : true;
                bool pseudo_color = ref_settings ? ref_settings->pseudo_color_1ch : false;
                bool ignore_alpha = ref_settings ? ref_settings->ignore_alpha_4ch : true;
                // 如果启用了窗宽窗位且是灰度图像
                if (view.use_window_level && (src.channels() == 1))
                {
                    cv::Mat norm;
                    double wl_min = view.window_center - view.window_width / 2.0;
                    double wl_max = view.window_center + view.window_width / 2.0;
                    double scale = 255.0 / (wl_max - wl_min + 1e-6);
                    double offset = -wl_min * scale;
                    src.convertTo(norm, CV_8UC1, scale, offset);
                    cv::cvtColor(norm, rgba, cv::COLOR_GRAY2RGBA);
                    return rgba;
                }

                switch (src.type())
                {
                    case CV_8UC1:
                        if (auto_contrast)
                        {
                            cv::Mat norm;
                            cv::normalize(src, norm, 0, 255, cv::NORM_MINMAX);
                            if (pseudo_color)
                                rgba = apply_pseudo_color(norm);
                            else
                                cv::cvtColor(norm, rgba, cv::COLOR_GRAY2RGBA);
                        }
                        else
                        {
                            if (pseudo_color)
                                rgba = apply_pseudo_color(src);
                            else
                                cv::cvtColor(src, rgba, cv::COLOR_GRAY2RGBA);
                        }
                        break;
                    case CV_8UC3: cv::cvtColor(src, rgba, cv::COLOR_BGR2RGBA); break;
                    case CV_8UC4:
                        if (ignore_alpha)
                        {
                            cv::Mat bgr;
                            cv::cvtColor(src, bgr, cv::COLOR_BGRA2BGR);
                            cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);
                        }
                        else
                        {
                            cv::cvtColor(src, rgba, cv::COLOR_BGRA2RGBA);
                        }
                        break;
                    case CV_16UC1:
                        {
                            cv::Mat norm;
                            if (auto_contrast)
                                cv::normalize(src, norm, 0, 255, cv::NORM_MINMAX, CV_8UC1);
                            else
                                src.convertTo(norm, CV_8UC1, 255.0 / 65535.0);
                            if (pseudo_color)
                                rgba = apply_pseudo_color(norm);
                            else
                                cv::cvtColor(norm, rgba, cv::COLOR_GRAY2RGBA);
                            break;
                        }
                    case CV_32FC1:
                        {
                            cv::Mat norm;
                            if (auto_contrast)
                                cv::normalize(src, norm, 0, 255, cv::NORM_MINMAX, CV_8UC1);
                            else
                            {
                                cv::Mat clamped;
                                src.convertTo(clamped, CV_8UC1, 255.0);
                                norm = clamped;
                            }
                            if (pseudo_color)
                                rgba = apply_pseudo_color(norm);
                            else
                                cv::cvtColor(norm, rgba, cv::COLOR_GRAY2RGBA);
                            break;
                        }
                    default:
                        src.convertTo(rgba, CV_8UC3);
                        cv::cvtColor(rgba, rgba, cv::COLOR_BGR2RGB);
                        break;
                }
                return rgba;
            };
            const cv::Mat& img = image.get();
            empty = img.empty();
            if (!empty)
            {
                type_info = type_to_string(img.type());
                update_texture(convert_to_rgba(img), texture_id, texture_width, texture_height);
                float thumb_max_dim = 64.0f;
                float scale = std::min(thumb_max_dim / img.cols, thumb_max_dim / img.rows);
                thumb_texture_width = static_cast<int>(img.cols * scale);
                thumb_texture_height = static_cast<int>(img.rows * scale);
            }

            changed = false;
        }
        void render_thumbnail(bool selected)
        {
            expanded = ImGui::TreeNodeEx(name.data(), ImGuiTreeNodeFlags_AllowOverlap | (expanded ? ImGuiTreeNodeFlags_DefaultOpen : 0) | (selected ? ImGuiTreeNodeFlags_Selected : 0));
            if (!expanded)
                return;
            if (empty || texture_id == 0)
                return ImGui::TreePop();
            ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(texture_id)), ImVec2(thumb_texture_width, thumb_texture_height));
            ImGui::SameLine();
            ImGui::TextDisabled("%d x %d\n%s\ncv::Mat", texture_width, texture_height, type_info.c_str());
            ImGui::TreePop();
        }
        void render_preview()
        {
            // 链接视图：加载当前图像尺寸对应的视图状态
            auto size_key = std::make_pair(static_cast<int>(texture_width), static_cast<int>(texture_height));
            bool link_views = ref_settings && ref_settings->link_views;
            if (link_views && ref_linked_views)
            {
                auto it = ref_linked_views->find(size_key);
                if (it != ref_linked_views->end())
                {
                    view.zoom = it->second.zoom;
                    view.offset = it->second.offset;
                }
            }

            // 用于同步链接视图状态的 lambda (定义在早期以便工具栏使用)
            auto sync_linked_view = [&]() {
                if (link_views && ref_linked_views)
                {
                    (*ref_linked_views)[size_key] = { view.zoom, view.offset };
                }
            };

            // 工具栏
            ImGui::Text("缩放: %.1f%%", view.zoom * 100);
            auto calc_center_offset = [&](float zoom, ImVec2 canvas_size) -> ImVec2 {
                float img_w = texture_width * zoom;
                float img_h = texture_height * zoom;
                return { (canvas_size.x - img_w) / 2, (canvas_size.y - img_h) / 2 };
            };

            ImGui::SameLine();
            if (ImGui::Button("适应"))
            {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                float scale_x = avail.x / texture_width;
                float scale_y = (avail.y - 30) / texture_height;
                view.zoom = std::min(scale_x, scale_y) * 0.95f;
                view.offset = { 0, 0 };
                sync_linked_view();
            }
            ImGui::SameLine();
            if (ImGui::Button("1:1"))
            {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                view.zoom = 1.0f;
                view.offset = { 0, 0 };
                sync_linked_view();
            }
            ImGui::SameLine();
            if (ImGui::Button("+"))
            {
                view.zoom *= 1.2f;
                sync_linked_view();
            }
            ImGui::SameLine();
            if (ImGui::Button("-"))
            {
                view.zoom /= 1.2f;
                sync_linked_view();
            }
            ImGui::SameLine();
            if (view.use_window_level)
            {
                if (ImGui::Button("重置WL"))
                {
                    view.use_window_level = false;
                    view.has_roi_stats = false;
                    changed = true;
                }
                ImGui::SameLine();
                ImGui::Text("WC:%.1f WW:%.1f", view.window_center, view.window_width);
                if (view.has_roi_stats)
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("|");
                    ImGui::SameLine();
                    ImGui::Text("均值:%.2f 标准差:%.2f [%.1f~%.1f]", view.roi_mean, view.roi_stddev, view.roi_min, view.roi_max);
                    if (ImGui::IsItemHovered() && !view.roi_histogram.empty())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("ROI 直方图  [%.1f ~ %.1f]", view.roi_min, view.roi_max);
                        char overlay[64];
                        snprintf(overlay, sizeof(overlay), "max: %.0f", view.roi_hist_max);
                        ImGui::PlotHistogram("##roi_hist", view.roi_histogram.data(), static_cast<int>(view.roi_histogram.size()), 0, overlay, 0.0f, view.roi_hist_max, ImVec2(300, 150));
                        ImGui::EndTooltip();
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("中键框选设置窗宽窗位");
            }

            // 像素信息显示在工具栏
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            ImGui::Text("像素: %s", view.pixel_info_text.c_str());

            view.zoom = std::clamp(view.zoom, 0.01f, 50.0f);

            // 图像区域
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();

            // 确保canvas尺寸有效
            if (canvas_size.x < 1.0f || canvas_size.y < 1.0f)
                return;

            // 初始化时自动适应并居中
            if (view.need_fit && texture_width > 0 && texture_height > 0)
            {
                float scale_x = canvas_size.x / texture_width;
                float scale_y = canvas_size.y / texture_height;
                view.zoom = std::min(scale_x, scale_y) * 0.95f;
                float img_w = texture_width * view.zoom;
                float img_h = texture_height * view.zoom;
                view.offset = { (canvas_size.x - img_w) / 2, (canvas_size.y - img_h) / 2 };
                view.need_fit = false;
            }

            ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
            bool is_hovered = ImGui::IsItemHovered();
            bool is_active = ImGui::IsItemActive();

            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 mouse_canvas = { mouse_pos.x - canvas_pos.x, mouse_pos.y - canvas_pos.y };

            // 计算鼠标在图像上的坐标
            int img_x = static_cast<int>((mouse_canvas.x - view.offset.x) / view.zoom);
            int img_y = static_cast<int>((mouse_canvas.y - view.offset.y) / view.zoom);

            // 滚轮缩放
            if (is_hovered)
            {
                float wheel = ImGui::GetIO().MouseWheel;
                if (wheel != 0)
                {
                    float old_zoom = view.zoom;
                    view.zoom *= (wheel > 0) ? 1.15f : (1.0f / 1.15f);
                    view.zoom = std::clamp(view.zoom, 0.01f, 50.0f);
                    float zoom_ratio = view.zoom / old_zoom;
                    view.offset.x = mouse_canvas.x - (mouse_canvas.x - view.offset.x) * zoom_ratio;
                    view.offset.y = mouse_canvas.y - (mouse_canvas.y - view.offset.y) * zoom_ratio;
                    sync_linked_view();
                }
            }

            // 左键拖拽
            if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                view.offset.x += delta.x;
                view.offset.y += delta.y;
                sync_linked_view();
            }

            // 中键框选窗宽窗位
            if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            {
                view.is_selecting = true;
                view.selection_start = { static_cast<float>(img_x), static_cast<float>(img_y) };
                view.selection_end = view.selection_start;
            }
            if (view.is_selecting)
            {
                view.selection_end = { static_cast<float>(img_x), static_cast<float>(img_y) };
                if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
                {
                    view.is_selecting = false;
                    // 计算选区的窗宽窗位
                    int x1 = static_cast<int>(std::min(view.selection_start.x, view.selection_end.x));
                    int y1 = static_cast<int>(std::min(view.selection_start.y, view.selection_end.y));
                    int x2 = static_cast<int>(std::max(view.selection_start.x, view.selection_end.x));
                    int y2 = static_cast<int>(std::max(view.selection_start.y, view.selection_end.y));
                    x1 = std::clamp(x1, 0, static_cast<int>(texture_width) - 1);
                    y1 = std::clamp(y1, 0, static_cast<int>(texture_height) - 1);
                    x2 = std::clamp(x2, 0, static_cast<int>(texture_width) - 1);
                    y2 = std::clamp(y2, 0, static_cast<int>(texture_height) - 1);
                    if (x2 >= x1 && y2 >= y1)
                    {
                        const cv::Mat& img = image.get();
                        cv::Rect roi(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
                        cv::Mat region = img(roi);
                        double minVal, maxVal;
                        cv::minMaxLoc(region, &minVal, &maxVal);
                        cv::Scalar mean_val, stddev_val;
                        cv::meanStdDev(region, mean_val, stddev_val);

                        view.window_center = (minVal + maxVal) / 2.0;
                        view.window_width = maxVal - minVal;
                        if (view.window_width < 1.0)
                            view.window_width = 1.0;
                        view.use_window_level = true;

                        // 保存统计信息
                        view.has_roi_stats = true;
                        view.roi_min = minVal;
                        view.roi_max = maxVal;
                        view.roi_mean = mean_val[0];
                        view.roi_stddev = stddev_val[0];

                        // 计算直方图
                        {
                            cv::Mat gray;
                            if (region.channels() > 1)
                                cv::cvtColor(region, gray, cv::COLOR_BGR2GRAY);
                            else
                                region.convertTo(gray, CV_32F);

                            if (gray.type() != CV_32F)
                                gray.convertTo(gray, CV_32F);

                            const int hist_bins = 256;
                            float range[] = { static_cast<float>(minVal), static_cast<float>(maxVal + 1e-6) };
                            const float* hist_range = range;
                            cv::Mat hist;
                            cv::calcHist(&gray, 1, nullptr, cv::Mat(), hist, 1, &hist_bins, &hist_range);

                            view.roi_histogram.resize(hist_bins);
                            view.roi_hist_max = 0.0f;
                            for (int i = 0; i < hist_bins; ++i)
                            {
                                view.roi_histogram[i] = hist.at<float>(i);
                                if (view.roi_histogram[i] > view.roi_hist_max)
                                    view.roi_hist_max = view.roi_histogram[i];
                            }
                        }

                        changed = true;
                    }
                }
            }

            // 右键框选映射
            if (on_right_selection_changed->empty() == false)
            {
                if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    view.is_right_selecting = true;
                    view.right_selection_start = { static_cast<float>(img_x), static_cast<float>(img_y) };
                    view.right_selection_end = view.right_selection_start;
                }
                if (view.is_right_selecting)
                {
                    view.right_selection_end = { static_cast<float>(img_x), static_cast<float>(img_y) };
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
                    {
                        view.is_right_selecting = false;
                        int x1 = static_cast<int>(std::min(view.right_selection_start.x, view.right_selection_end.x));
                        int y1 = static_cast<int>(std::min(view.right_selection_start.y, view.right_selection_end.y));
                        int x2 = static_cast<int>(std::max(view.right_selection_start.x, view.right_selection_end.x));
                        int y2 = static_cast<int>(std::max(view.right_selection_start.y, view.right_selection_end.y));
                        x1 = std::clamp(x1, 0, static_cast<int>(texture_width) - 1);
                        y1 = std::clamp(y1, 0, static_cast<int>(texture_height) - 1);
                        x2 = std::clamp(x2, 0, static_cast<int>(texture_width) - 1);
                        y2 = std::clamp(y2, 0, static_cast<int>(texture_height) - 1);
                        if (x2 >= x1 && y2 >= y1)
                        {
                            view.has_right_selection = true;
                            view.right_selection_roi = cv::Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
                            on_right_selection_changed->emit(view.right_selection_roi);
                        }
                    }
                }
            }

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_pos, { canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y }, IM_COL32(128, 128, 128, 255));
            draw_list->PushClipRect(canvas_pos, { canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y }, true);

            float img_w = texture_width * view.zoom;
            float img_h = texture_height * view.zoom;
            ImVec2 img_min = { canvas_pos.x + view.offset.x, canvas_pos.y + view.offset.y };
            ImVec2 img_max = { img_min.x + img_w, img_min.y + img_h };

            draw_list->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(texture_id)), img_min, img_max);
            draw_list->AddRect(img_min, img_max, IM_COL32(80, 80, 80, 255));

            // 绘制框选矩形
            if (view.is_selecting)
            {
                float sel_x1 = canvas_pos.x + view.offset.x + std::min(view.selection_start.x, view.selection_end.x) * view.zoom;
                float sel_y1 = canvas_pos.y + view.offset.y + std::min(view.selection_start.y, view.selection_end.y) * view.zoom;
                float sel_x2 = canvas_pos.x + view.offset.x + (std::max(view.selection_start.x, view.selection_end.x) + 1) * view.zoom;
                float sel_y2 = canvas_pos.y + view.offset.y + (std::max(view.selection_start.y, view.selection_end.y) + 1) * view.zoom;
                draw_list->AddRectFilled({ sel_x1, sel_y1 }, { sel_x2, sel_y2 }, IM_COL32(255, 255, 0, 50));
                draw_list->AddRect({ sel_x1, sel_y1 }, { sel_x2, sel_y2 }, IM_COL32(255, 255, 0, 200), 0.0f, 0, 2.0f);
            }

            // 绘制右键框选矩形
            if (view.is_right_selecting)
            {
                float sel_x1 = canvas_pos.x + view.offset.x + std::min(view.right_selection_start.x, view.right_selection_end.x) * view.zoom;
                float sel_y1 = canvas_pos.y + view.offset.y + std::min(view.right_selection_start.y, view.right_selection_end.y) * view.zoom;
                float sel_x2 = canvas_pos.x + view.offset.x + (std::max(view.right_selection_start.x, view.right_selection_end.x) + 1) * view.zoom;
                float sel_y2 = canvas_pos.y + view.offset.y + (std::max(view.right_selection_start.y, view.right_selection_end.y) + 1) * view.zoom;
                draw_list->AddRectFilled({ sel_x1, sel_y1 }, { sel_x2, sel_y2 }, IM_COL32(0, 255, 0, 50));
                draw_list->AddRect({ sel_x1, sel_y1 }, { sel_x2, sel_y2 }, IM_COL32(0, 255, 0, 200), 0.0f, 0, 2.0f);
            }
            else if (view.has_right_selection)
            {
                auto& roi = view.right_selection_roi;
                float sel_x1 = canvas_pos.x + view.offset.x + roi.x * view.zoom;
                float sel_y1 = canvas_pos.y + view.offset.y + roi.y * view.zoom;
                float sel_x2 = canvas_pos.x + view.offset.x + (roi.x + roi.width) * view.zoom;
                float sel_y2 = canvas_pos.y + view.offset.y + (roi.y + roi.height) * view.zoom;
                draw_list->AddRect({ sel_x1, sel_y1 }, { sel_x2, sel_y2 }, IM_COL32(0, 255, 0, 150), 0.0f, 0, 1.5f);
            }

            // 高缩放时绘制网格
            if (view.zoom >= 8.0f)
            {
                int start_x = std::max(0, static_cast<int>(-view.offset.x / view.zoom));
                int start_y = std::max(0, static_cast<int>(-view.offset.y / view.zoom));
                int end_x = std::min(texture_width, static_cast<uint32_t>((canvas_size.x - view.offset.x) / view.zoom) + 1);
                int end_y = std::min(texture_height, static_cast<uint32_t>((canvas_size.y - view.offset.y) / view.zoom) + 1);

                ImU32 grid_color = IM_COL32(60, 60, 60, 100);
                for (int gx = start_x; gx <= end_x; ++gx)
                {
                    float x = canvas_pos.x + view.offset.x + gx * view.zoom;
                    draw_list->AddLine({ x, img_min.y }, { x, img_max.y }, grid_color);
                }
                for (int gy = start_y; gy <= end_y; ++gy)
                {
                    float y = canvas_pos.y + view.offset.y + gy * view.zoom;
                    draw_list->AddLine({ img_min.x, y }, { img_max.x, y }, grid_color);
                }
            }
            draw_list->PopClipRect();

            bool hex_display = ref_settings ? ref_settings->hexadecimal_display : false;
            auto get_pixel_info = [hex_display](const cv::Mat& mat, int x, int y) -> std::string {
                if (x < 0 || y < 0 || x >= mat.cols || y >= mat.rows)
                    return "越界";

                char buf[128];
                if (hex_display)
                {
                    switch (mat.type())
                    {
                        case CV_8UC1: snprintf(buf, sizeof(buf), "0x%02X", mat.at<uchar>(y, x)); break;
                        case CV_8UC3:
                            {
                                auto v = mat.at<cv::Vec3b>(y, x);
                                snprintf(buf, sizeof(buf), "B:0x%02X G:0x%02X R:0x%02X", v[0], v[1], v[2]);
                                break;
                            }
                        case CV_8UC4:
                            {
                                auto v = mat.at<cv::Vec4b>(y, x);
                                snprintf(buf, sizeof(buf), "B:0x%02X G:0x%02X R:0x%02X A:0x%02X", v[0], v[1], v[2], v[3]);
                                break;
                            }
                        case CV_16UC1: snprintf(buf, sizeof(buf), "0x%04X", mat.at<ushort>(y, x)); break;
                        case CV_16UC2:
                            {
                                auto v = mat.at<cv::Vec2w>(y, x);
                                snprintf(buf, sizeof(buf), "C0:0x%04X C1:0x%04X", v[0], v[1]);
                                break;
                            }
                        case CV_32FC1: snprintf(buf, sizeof(buf), "%.4f", mat.at<float>(y, x)); break;
                        case CV_32FC3:
                            {
                                auto v = mat.at<cv::Vec3f>(y, x);
                                snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f", v[0], v[1], v[2]);
                                break;
                            }
                        default: snprintf(buf, sizeof(buf), "类型: %d", mat.type()); break;
                    }
                }
                else
                {
                    switch (mat.type())
                    {
                        case CV_8UC1: snprintf(buf, sizeof(buf), "%d", mat.at<uchar>(y, x)); break;
                        case CV_8UC3:
                            {
                                auto v = mat.at<cv::Vec3b>(y, x);
                                snprintf(buf, sizeof(buf), "B:%d G:%d R:%d", v[0], v[1], v[2]);
                                break;
                            }
                        case CV_8UC4:
                            {
                                auto v = mat.at<cv::Vec4b>(y, x);
                                snprintf(buf, sizeof(buf), "B:%d G:%d R:%d A:%d", v[0], v[1], v[2], v[3]);
                                break;
                            }
                        case CV_16UC1: snprintf(buf, sizeof(buf), "%d", mat.at<ushort>(y, x)); break;
                        case CV_16UC2:
                            {
                                auto v = mat.at<cv::Vec2w>(y, x);
                                snprintf(buf, sizeof(buf), "C0:%d C1:%d", v[0], v[1]);
                                break;
                            }
                        case CV_32FC1: snprintf(buf, sizeof(buf), "%.4f", mat.at<float>(y, x)); break;
                        case CV_32FC3:
                            {
                                auto v = mat.at<cv::Vec3f>(y, x);
                                snprintf(buf, sizeof(buf), "%.2f, %.2f, %.2f", v[0], v[1], v[2]);
                                break;
                            }
                        default: snprintf(buf, sizeof(buf), "类型: %d", mat.type()); break;
                    }
                }
                return buf;
            };
            // 更新像素信息
            if (is_hovered && img_x >= 0 && img_y >= 0 && img_x < texture_width && img_y < texture_height)
            {
                char buf[256];
                snprintf(buf, sizeof(buf), "(%d, %d) %s", img_x, img_y, get_pixel_info(image, img_x, img_y).c_str());
                view.pixel_info_text = buf;
                view.hover_x = img_x;
                view.hover_y = img_y;
            }
            else
            {
                view.pixel_info_text = "";
                view.hover_x = -1;
                view.hover_y = -1;
            }

            // 右键菜单
            if (ImGui::BeginPopupContextItem("preview_context_menu"))
            {
                if (ImGui::MenuItem("适应大小"))
                {
                    float scale_x = canvas_size.x / texture_width;
                    float scale_y = canvas_size.y / texture_height;
                    view.zoom = std::min(scale_x, scale_y) * 0.95f;
                    view.offset = { 0, 0 };
                    sync_linked_view();
                }
                if (ImGui::MenuItem("原始尺寸"))
                {
                    view.zoom = 1.0f;
                    view.offset = { 0, 0 };
                    sync_linked_view();
                }
                ImGui::Separator();
                if (ref_settings)
                {
                    if (ImGui::MenuItem("链接视图", nullptr, &ref_settings->link_views))
                    {
                        // 启用链接视图时，保存当前视图状态
                        if (ref_settings->link_views)
                            sync_linked_view();
                    }
                    ImGui::Separator();
                    bool settings_changed = false;
                    if (ImGui::MenuItem("自动归一化", nullptr, &ref_settings->auto_maximize_contrast))
                        settings_changed = true;
                    if (ImGui::MenuItem("单通道伪彩色", nullptr, &ref_settings->pseudo_color_1ch))
                        settings_changed = true;
                    if (ImGui::MenuItem("四通道忽略Alpha", nullptr, &ref_settings->ignore_alpha_4ch))
                        settings_changed = true;
                    if (settings_changed)
                        changed = true; // 触发重新渲染
                    ImGui::Separator();
                    ImGui::MenuItem("十六进制显示", nullptr, &ref_settings->hexadecimal_display);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("复制像素地址", nullptr, false, view.hover_x >= 0 && view.hover_y >= 0))
                {
                    char addr_buf[64];
                    snprintf(addr_buf, sizeof(addr_buf), "(%d, %d)", view.hover_x, view.hover_y);
                    ImGui::SetClipboardText(addr_buf);
                }
                ImGui::EndPopup();
            }
        }

    private:
        void update_texture(const cv::Mat& rgba, GLuint& texture_id, GLuint& texture_width, GLuint& texture_height)
        {
            if (rgba.empty())
                return;

            texture_width = static_cast<GLuint>(rgba.cols);
            texture_height = static_cast<GLuint>(rgba.rows);

            if (texture_id == 0)
                glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data);
        }
        void destroy_texture()
        {
            if (texture_id)
                glDeleteTextures(1, &texture_id);
            texture_id = 0;
        }
    };
    std::map<std::string, std::unique_ptr<image_viewer>> viewers;
    std::string selected_name;
    std::set<std::string> pinned_names;
    bool dock_needs_init = false;
    bool dock_first_init = true;
    ImGuiID preview_dock_id = 0;

public:
    void destroy() { viewers.clear(); }
    void watch_image(const std::string& var_name, const cv::Mat& image)
    {
        auto viewer = std::make_unique<image_viewer>(var_name, image, settings, linked_views);
        if (auto find_viewer = viewers.find(var_name); find_viewer != viewers.end())
        {
            viewer->view = find_viewer->second->view;
            viewer->on_right_selection_changed.swap(find_viewer->second->on_right_selection_changed);
        }
        viewers[var_name] = std::move(viewer);
    }
    void remove_watcher(const std::string& var_name) { viewers.erase(var_name); }
    void persist_watcher(const std::string& var_name)
    {
        if (auto it = viewers.find(var_name); it != viewers.end())
            it->second->persist();
    }
    void update_watcher(const std::string& var_name)
    {
        if (auto it = viewers.find(var_name); it != viewers.end())
            it->second->update();
    }

public:
    class unique_visitor
    {
        image_watcher& watcher;
        std::string var_name;
        bool auto_persist = false;
        using conn_t = scoped_connection<signal<>::connection>;
        std::optional<conn_t> conn;

    public:
        unique_visitor(image_watcher& watcher, const std::string& var_name, const cv::Mat& image, bool auto_persist) : watcher(watcher), var_name(var_name), auto_persist(auto_persist)
        { watcher.watch_image(var_name, image); }
        ~unique_visitor() { auto_persist ? watcher.persist_watcher(var_name) : watcher.remove_watcher(var_name); }
    };
    unique_visitor visitor(const std::string& var_name, const cv::Mat& image, bool auto_persist = false) { return unique_visitor(*this, var_name, image, auto_persist); }

public:
    void render()
    {
        ImGui::Begin("图像监视器", nullptr, ImGuiWindowFlags_NoScrollbar);
        render_list_viewer();
        ImGui::SameLine();
        render_splitter();
        ImGui::SameLine();

        preview_dock_id = ImGui::GetID("ImagePreviewDock");
        ImGui::DockSpace(preview_dock_id, ImGui::GetContentRegionAvail(), ImGuiDockNodeFlags_NoWindowMenuButton);
        ImGui::End();

        if (dock_first_init || dock_needs_init)
        {
            ImGui::DockBuilderRemoveNode(preview_dock_id);
            ImGui::DockBuilderAddNode(preview_dock_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(preview_dock_id, ImGui::GetMainViewport()->Size);
            ImGui::DockBuilderDockWindow("###temp_preview", preview_dock_id);
            for (auto& name : pinned_names)
                ImGui::DockBuilderDockWindow(name.c_str(), preview_dock_id);
            ImGui::DockBuilderFinish(preview_dock_id);
            dock_first_init = false;
            dock_needs_init = false;
        }

        render_pinned_viewers();
        render_temp_preview();
    }

private:
    float left_panel_width = 200.0f;
    void render_list_viewer()
    {
        ImGui::BeginChild("List", ImVec2(left_panel_width, 0), true);
        for (auto& [name, viewer] : viewers)
        {
            viewer->sync_state();
            bool is_pinned = pinned_names.count(name) > 0;
            viewer->render_thumbnail((name == selected_name));
            if (is_pinned)
            {
                ImGui::SameLine();
                ImGui::TextDisabled("[P]");
            }
            if (ImGui::IsItemClicked())
                selected_name = name;
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (is_pinned)
                    pinned_names.erase(name);
                else
                {
                    pinned_names.insert(name);
                    dock_needs_init = true;
                }
            }
        }

        ImGui::EndChild();
    }
    void render_splitter()
    {
        ImGui::Button("##splitter", ImVec2(3, -1));
        if (ImGui::IsItemActive())
            left_panel_width += ImGui::GetIO().MouseDelta.x;
        left_panel_width = std::clamp(left_panel_width, 10.0f, 600.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    void render_pinned_viewers()
    {
        for (auto it = pinned_names.begin(); it != pinned_names.end();)
        {
            auto vit = viewers.find(*it);
            if (vit == viewers.end())
            {
                it = pinned_names.erase(it);
                continue;
            }

            bool open = true;
            ImGui::SetNextWindowDockID(preview_dock_id, ImGuiCond_Once);
            if (ImGui::Begin(it->c_str(), &open))
                vit->second->render_preview();
            ImGui::End();

            if (!open)
                it = pinned_names.erase(it);
            else
                ++it;
        }
    }
    void render_temp_preview()
    {
        if (selected_name.empty() || viewers.find(selected_name) == viewers.end())
            return;
        if (pinned_names.count(selected_name))
            return;

        std::string title = selected_name + " (\xe9\xa2\x84\xe8\xa7\x88)###temp_preview";
        ImGui::SetNextWindowDockID(preview_dock_id, ImGuiCond_Always);
        ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoSavedSettings);
        viewers[selected_name]->render_preview();
        ImGui::End();
    }
};
