#pragma once
#include <imgui.h>
#include <opencv2/core.hpp>

#include <opencv2/imgproc.hpp>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>

class image_watcher
{
    class image_viewer
    {
        std::reference_wrapper<cv::Mat> image;
        std::function<void()> callback;

        bool empty = true;
        bool changed = true;
        bool expanded = true;

        GLuint texture_id = 0;
        GLuint texture_width = 0;
        GLuint texture_height = 0;
        GLuint thumb_texture_id = 0;
        GLuint thumb_texture_width = 0;
        GLuint thumb_texture_height = 0;
        std::string type_info;

        struct viewer_state
        {
            float zoom = 1.0f;
            ImVec2 offset = { 0, 0 };
            std::string pixel_info_text;
            bool need_fit = true;
        } view;

    public:
        image_viewer(cv::Mat& image, std::function<void()> callback) : image(std::ref(image)), callback(callback) {};
        void update()
        {
            changed = true;
            if (callback)
                callback();
        }
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
            auto convert_to_rgba = [](const cv::Mat& src) {
                cv::Mat rgba;
                switch (src.type())
                {
                    case CV_8UC1: cv::cvtColor(src, rgba, cv::COLOR_GRAY2RGBA); break;
                    case CV_8UC3: cv::cvtColor(src, rgba, cv::COLOR_BGR2RGBA); break;
                    case CV_8UC4: cv::cvtColor(src, rgba, cv::COLOR_BGRA2RGBA); break;
                    case CV_16UC1:
                        {
                            cv::Mat norm;
                            double minVal, maxVal;
                            cv::minMaxLoc(src, &minVal, &maxVal);
                            src.convertTo(norm, CV_8UC1, 255.0 / (maxVal - minVal + 1), -minVal * 255.0 / (maxVal - minVal + 1));
                            cv::cvtColor(norm, rgba, cv::COLOR_GRAY2RGBA);
                            break;
                        }
                    case CV_32FC1:
                        {
                            cv::Mat norm;
                            cv::normalize(src, norm, 0, 255, cv::NORM_MINMAX, CV_8UC1);
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
            cv::Mat& img = image.get();
            empty = img.empty();
            if (!empty)
            {
                type_info = type_to_string(img.type());
                update_texture(convert_to_rgba(img), texture_id, texture_width, texture_height);
                float thumb_max_dim = 64.0f;
                float scale = std::min(thumb_max_dim / img.cols, thumb_max_dim / img.rows);
                thumb_texture_width = static_cast<int>(img.cols * scale);
                thumb_texture_height = static_cast<int>(img.rows * scale);
                thumb_texture_id = texture_id;
            }

            changed = false;
        }
        void render_thumbnail(std::string_view name, bool selected)
        {
            expanded = ImGui::TreeNodeEx(name.data(), ImGuiTreeNodeFlags_AllowOverlap | (expanded ? ImGuiTreeNodeFlags_DefaultOpen : 0) | (selected ? ImGuiTreeNodeFlags_Selected : 0));
            if (!expanded)
                return;
            if (empty || thumb_texture_id == 0)
                return ImGui::TreePop();
            ImGui::Image(static_cast<ImTextureID>(static_cast<intptr_t>(thumb_texture_id)), ImVec2(thumb_texture_width, thumb_texture_height));
            ImGui::SameLine();
            ImGui::TextDisabled("%d x %d\n%s\ncv::Mat", texture_width, texture_height, type_info.c_str());
            ImGui::TreePop();
        }
        void render_preview(std::string_view selected_name)
        {
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
                view.offset = calc_center_offset(view.zoom, { avail.x, avail.y - 30 });
            }
            ImGui::SameLine();
            if (ImGui::Button("1:1"))
            {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                view.zoom = 1.0f;
                view.offset = calc_center_offset(view.zoom, { avail.x, avail.y - 30 });
            }
            ImGui::SameLine();
            if (ImGui::Button("+"))
                view.zoom *= 1.2f;
            ImGui::SameLine();
            if (ImGui::Button("-"))
                view.zoom /= 1.2f;
            ImGui::SameLine();
            if (ImGui::Button("刷新"))
                update();

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
            {
                ImGui::EndChild();
                return;
            }

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

            ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
            bool is_hovered = ImGui::IsItemHovered();
            bool is_active = ImGui::IsItemActive();

            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImVec2 mouse_canvas = { mouse_pos.x - canvas_pos.x, mouse_pos.y - canvas_pos.y };

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
                }
            }

            // 左键拖拽
            if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
            {
                ImVec2 delta = ImGui::GetIO().MouseDelta;
                view.offset.x += delta.x;
                view.offset.y += delta.y;
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

            int img_x = static_cast<int>((mouse_canvas.x - view.offset.x) / view.zoom);
            int img_y = static_cast<int>((mouse_canvas.y - view.offset.y) / view.zoom);

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

            auto get_pixel_info = [](const cv::Mat& mat, int x, int y) -> std::string {
                if (x < 0 || y < 0 || x >= mat.cols || y >= mat.rows)
                    return "越界";

                char buf[128];
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
                return buf;
            };
            // 更新像素信息
            if (is_hovered && img_x >= 0 && img_y >= 0 && img_x < texture_width && img_y < texture_height)
            {
                char buf[256];
                snprintf(buf, sizeof(buf), "(%d, %d) %s", img_x, img_y, get_pixel_info(image, img_x, img_y).c_str());
                view.pixel_info_text = buf;
            }
            else
            {
                view.pixel_info_text = "";
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
    };
    std::map<std::string, std::unique_ptr<image_viewer>> viewers;
    std::string selected_name;

public:
    void destroy() { viewers.clear(); }
    void watch_image(const std::string& var_name, cv::Mat& image, std::function<void()> callback = {}) { viewers[var_name] = std::move(std::make_unique<image_viewer>(image, callback)); }
    void remove_watcher(const std::string& var_name) { viewers.erase(var_name); }
    void update_image(const std::string& var_name)
    {
        if (auto it = viewers.find(var_name); it != viewers.end())
            it->second->update();
    }
    void render()
    {
        ImGui::Begin("图像监视器", nullptr, ImGuiWindowFlags_NoScrollbar);
        render_list_viewer();
        ImGui::SameLine();
        render_splitter();
        ImGui::SameLine();
        render_viewer_preview();
        ImGui::End();
    }

private:
    float left_panel_width = 200.0f;
    void render_list_viewer()
    {
        ImGui::BeginChild("List", ImVec2(left_panel_width, 0), true);
        for (auto& [name, viewer] : viewers)
        {
            viewer->sync_state();
            viewer->render_thumbnail(name, (name == selected_name));
            if (ImGui::IsItemClicked())
                selected_name = name;
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
    void render_viewer_preview()
    {
        ImGui::BeginChild("Preview", ImVec2(0, 0), true);
        if (selected_name.empty() || viewers.find(selected_name) == viewers.end())
        {
            ImGui::EndChild();
            return;
        }

        auto& viewer = viewers[selected_name];
        viewer->render_preview(selected_name);
        ImGui::EndChild();
    }
};
