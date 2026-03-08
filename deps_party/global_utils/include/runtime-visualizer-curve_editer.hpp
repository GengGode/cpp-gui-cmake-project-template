#pragma once
#include <imgui.h>
#include <implot.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <string>
#include <vector>

// ============================================================================
// Enums
// ============================================================================

enum class curve_interpolation_mode
{
    linear,
    cubic_bezier,
    catmull_rom,
    cubic_b_spline,
    hermite,
};

enum class curve_handle_mode
{
    symmetric,
    smooth,
    free,
    auto_smooth,
};

// ============================================================================
// curve_node
// ============================================================================

struct curve_node
{
    ImVec2 position = { 0.0f, 0.0f };
    ImVec2 handle_in = { -0.1f, 0.0f };
    ImVec2 handle_out = { 0.1f, 0.0f };
    curve_handle_mode handle_mode = curve_handle_mode::smooth;

    void enforce_handle_constraint(bool in_changed)
    {
        switch (handle_mode)
        {
            case curve_handle_mode::symmetric:
                if (in_changed)
                {
                    handle_out = { -handle_in.x, -handle_in.y };
                }
                else
                {
                    handle_in = { -handle_out.x, -handle_out.y };
                }
                break;

            case curve_handle_mode::smooth:
                {
                    if (in_changed)
                    {
                        float len_in = sqrtf(handle_in.x * handle_in.x + handle_in.y * handle_in.y);
                        float len_out = sqrtf(handle_out.x * handle_out.x + handle_out.y * handle_out.y);
                        if (len_in > 1e-6f)
                        {
                            handle_out.x = -handle_in.x / len_in * len_out;
                            handle_out.y = -handle_in.y / len_in * len_out;
                        }
                    }
                    else
                    {
                        float len_in = sqrtf(handle_in.x * handle_in.x + handle_in.y * handle_in.y);
                        float len_out = sqrtf(handle_out.x * handle_out.x + handle_out.y * handle_out.y);
                        if (len_out > 1e-6f)
                        {
                            handle_in.x = -handle_out.x / len_out * len_in;
                            handle_in.y = -handle_out.y / len_out * len_in;
                        }
                    }
                    break;
                }

            case curve_handle_mode::free:
            case curve_handle_mode::auto_smooth: break;
        }
    }
};

// ============================================================================
// curve_path
// ============================================================================

struct curve_path
{
    std::vector<curve_node> nodes;
    std::vector<curve_interpolation_mode> segment_modes;
    curve_interpolation_mode default_mode = curve_interpolation_mode::cubic_bezier;

    ImVec2 range_min = { 0.0f, 0.0f };
    ImVec2 range_max = { 1.0f, 1.0f };
    bool function_mode = false;

    // -- node management --

    void sync_segment_modes()
    {
        int n = (int)nodes.size() > 1 ? (int)nodes.size() - 1 : 0;
        while ((int)segment_modes.size() < n)
            segment_modes.push_back(default_mode);
        if ((int)segment_modes.size() > n)
            segment_modes.resize(n);
    }

    void add_node(const curve_node& nd, int insert_idx = -1)
    {
        if (insert_idx < 0 || insert_idx >= (int)nodes.size())
            nodes.push_back(nd);
        else
            nodes.insert(nodes.begin() + insert_idx, nd);
        sync_segment_modes();
    }

    void remove_node(int idx)
    {
        if (idx < 0 || idx >= (int)nodes.size())
            return;
        nodes.erase(nodes.begin() + idx);
        if (!segment_modes.empty())
        {
            int seg = std::min(idx, (int)segment_modes.size() - 1);
            if (seg >= 0)
                segment_modes.erase(segment_modes.begin() + seg);
        }
        sync_segment_modes();
    }

    void sort_nodes_by_x()
    {
        if (!function_mode || nodes.size() < 2)
            return;
        std::vector<int> idx(nodes.size());
        for (int i = 0; i < (int)idx.size(); ++i)
            idx[i] = i;
        std::sort(idx.begin(), idx.end(), [&](int a, int b) { return nodes[a].position.x < nodes[b].position.x; });
        std::vector<curve_node> sorted(nodes.size());
        for (int i = 0; i < (int)idx.size(); ++i)
            sorted[i] = nodes[idx[i]];
        nodes = std::move(sorted);
        sync_segment_modes();
    }

    void compute_auto_smooth_handles()
    {
        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            if (nodes[i].handle_mode != curve_handle_mode::auto_smooth)
                continue;
            ImVec2 prev = (i > 0) ? nodes[i - 1].position : nodes[i].position;
            ImVec2 next = (i < (int)nodes.size() - 1) ? nodes[i + 1].position : nodes[i].position;
            ImVec2 tang = { 0.5f * (next.x - prev.x), 0.5f * (next.y - prev.y) };
            float scale = 0.33f;
            nodes[i].handle_out = { tang.x * scale, tang.y * scale };
            nodes[i].handle_in = { -tang.x * scale, -tang.y * scale };
        }
    }

    // -- interpolation --

    const curve_node& node_clamped(int i) const { return nodes[std::clamp(i, 0, (int)nodes.size() - 1)]; }

    ImVec2 evaluate_segment(int seg, float t) const
    {
        if (seg < 0 || seg >= (int)segment_modes.size() || nodes.size() < 2)
            return { 0, 0 };

        const auto& p0 = nodes[seg];
        const auto& p1 = nodes[seg + 1];
        auto mode = segment_modes[seg];

        switch (mode)
        {
            case curve_interpolation_mode::linear:
                {
                    return {
                        p0.position.x + t * (p1.position.x - p0.position.x),
                        p0.position.y + t * (p1.position.y - p0.position.y),
                    };
                }

            case curve_interpolation_mode::cubic_bezier:
                {
                    float c0x = p0.position.x + p0.handle_out.x;
                    float c0y = p0.position.y + p0.handle_out.y;
                    float c1x = p1.position.x + p1.handle_in.x;
                    float c1y = p1.position.y + p1.handle_in.y;
                    float u = 1.0f - t, u2 = u * u, u3 = u2 * u;
                    float t2 = t * t, t3 = t2 * t;
                    return {
                        u3 * p0.position.x + 3 * u2 * t * c0x + 3 * u * t2 * c1x + t3 * p1.position.x,
                        u3 * p0.position.y + 3 * u2 * t * c0y + 3 * u * t2 * c1y + t3 * p1.position.y,
                    };
                }

            case curve_interpolation_mode::catmull_rom:
                {
                    const auto& pm = node_clamped(seg - 1);
                    const auto& pp = node_clamped(seg + 2);
                    float t2 = t * t, t3 = t2 * t;
                    return {
                        0.5f * (2 * p0.position.x + (-pm.position.x + p1.position.x) * t + (2 * pm.position.x - 5 * p0.position.x + 4 * p1.position.x - pp.position.x) * t2 +
                                (-pm.position.x + 3 * p0.position.x - 3 * p1.position.x + pp.position.x) * t3),
                        0.5f * (2 * p0.position.y + (-pm.position.y + p1.position.y) * t + (2 * pm.position.y - 5 * p0.position.y + 4 * p1.position.y - pp.position.y) * t2 +
                                (-pm.position.y + 3 * p0.position.y - 3 * p1.position.y + pp.position.y) * t3),
                    };
                }

            case curve_interpolation_mode::cubic_b_spline:
                {
                    const auto& pm = node_clamped(seg - 1);
                    const auto& pp = node_clamped(seg + 2);
                    float t2 = t * t, t3 = t2 * t;
                    float u = 1.0f - t;
                    float b0 = u * u * u / 6.0f;
                    float b1 = (3 * t3 - 6 * t2 + 4) / 6.0f;
                    float b2 = (-3 * t3 + 3 * t2 + 3 * t + 1) / 6.0f;
                    float b3 = t3 / 6.0f;
                    return {
                        b0 * pm.position.x + b1 * p0.position.x + b2 * p1.position.x + b3 * pp.position.x,
                        b0 * pm.position.y + b1 * p0.position.y + b2 * p1.position.y + b3 * pp.position.y,
                    };
                }

            case curve_interpolation_mode::hermite:
                {
                    float t2 = t * t, t3 = t2 * t;
                    float h00 = 2 * t3 - 3 * t2 + 1;
                    float h10 = t3 - 2 * t2 + t;
                    float h01 = -2 * t3 + 3 * t2;
                    float h11 = t3 - t2;
                    // Tangents: handle_out as forward tangent at p0, -handle_in as forward tangent at p1
                    // No 3x scale so Hermite gives a gentler curve than Bezier with the same handles
                    float m0x = p0.handle_out.x, m0y = p0.handle_out.y;
                    float m1x = -p1.handle_in.x, m1y = -p1.handle_in.y;
                    return {
                        h00 * p0.position.x + h10 * m0x + h01 * p1.position.x + h11 * m1x,
                        h00 * p0.position.y + h10 * m0y + h01 * p1.position.y + h11 * m1y,
                    };
                }
        }
        return { 0, 0 };
    }

    // -- parametric evaluation --

    ImVec2 evaluate(float global_t) const
    {
        if (nodes.empty())
            return { 0, 0 };
        if (nodes.size() == 1)
            return nodes[0].position;
        int n = (int)nodes.size() - 1;
        float scaled = global_t * n;
        int seg = std::clamp((int)scaled, 0, n - 1);
        float lt = std::clamp(scaled - seg, 0.0f, 1.0f);
        return evaluate_segment(seg, lt);
    }

    std::vector<ImVec2> sample(int num) const
    {
        std::vector<ImVec2> out;
        if (nodes.size() < 2 || num < 2)
        {
            if (!nodes.empty())
                out.push_back(nodes[0].position);
            return out;
        }
        out.reserve(num);
        for (int i = 0; i < num; ++i)
        {
            float t = (float)i / (float)(num - 1);
            out.push_back(evaluate(t));
        }
        return out;
    }

    // -- 1D function evaluation --

    float evaluate_y(float x) const
    {
        if (nodes.empty())
            return 0.0f;
        if (nodes.size() == 1)
            return nodes[0].position.y;
        if (x <= nodes.front().position.x)
            return nodes.front().position.y;
        if (x >= nodes.back().position.x)
            return nodes.back().position.y;

        for (int i = 0; i < (int)nodes.size() - 1; ++i)
        {
            float x0 = nodes[i].position.x;
            float x1 = nodes[i + 1].position.x;
            if (x >= x0 && x <= x1)
            {
                float dx = x1 - x0;
                if (dx < 1e-7f)
                    return nodes[i].position.y;

                auto mode = (i < (int)segment_modes.size()) ? segment_modes[i] : default_mode;
                if (mode == curve_interpolation_mode::linear)
                {
                    float t = (x - x0) / dx;
                    return evaluate_segment(i, t).y;
                }
                // Binary search for t such that evaluate_segment(i, t).x ≈ x
                float lo = 0.0f, hi = 1.0f;
                for (int it = 0; it < 32; ++it)
                {
                    float mid = (lo + hi) * 0.5f;
                    if (evaluate_segment(i, mid).x < x)
                        lo = mid;
                    else
                        hi = mid;
                }
                return evaluate_segment(i, (lo + hi) * 0.5f).y;
            }
        }
        return nodes.back().position.y;
    }

    std::vector<ImVec2> sample_uniform_x(int num) const
    {
        std::vector<ImVec2> out;
        if (nodes.empty() || num < 1)
            return out;
        out.reserve(num);
        for (int i = 0; i < num; ++i)
        {
            float x = (num > 1) ? range_min.x + (range_max.x - range_min.x) * (float)i / (float)(num - 1) : (range_min.x + range_max.x) * 0.5f;
            out.push_back({ x, evaluate_y(x) });
        }
        return out;
    }

    // -- search --

    int find_node_near(float x, float y, float tol) const
    {
        float best = FLT_MAX;
        int idx = -1;
        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            float dx = nodes[i].position.x - x;
            float dy = nodes[i].position.y - y;
            float d = dx * dx + dy * dy;
            if (d < best)
            {
                best = d;
                idx = i;
            }
        }
        return (best < tol * tol) ? idx : -1;
    }

    // -- curve hit testing --

    struct curve_hit
    {
        int segment = -1;
        float t = 0;
        ImVec2 position = { 0, 0 };
        float dist_sq = FLT_MAX;
    };

    curve_hit find_nearest_on_curve(float mx, float my, int spp = 48) const
    {
        curve_hit best;
        if (nodes.size() < 2)
            return best;
        int ns = (int)nodes.size() - 1;
        for (int s = 0; s < ns; ++s)
        {
            for (int k = 0; k <= spp; ++k)
            {
                float t = (float)k / spp;
                ImVec2 pt = evaluate_segment(s, t);
                float dx = pt.x - mx, dy = pt.y - my;
                float d = dx * dx + dy * dy;
                if (d < best.dist_sq)
                {
                    best.segment = s;
                    best.t = t;
                    best.position = pt;
                    best.dist_sq = d;
                }
            }
        }
        return best;
    }

    int insert_node_on_segment(int seg, float t)
    {
        if (seg < 0 || seg >= (int)nodes.size() - 1)
            return -1;
        ImVec2 pos = evaluate_segment(seg, t);
        float dt = 0.01f;
        ImVec2 before = evaluate_segment(seg, std::max(0.0f, t - dt));
        ImVec2 after = evaluate_segment(seg, std::min(1.0f, t + dt));
        ImVec2 tang = { (after.x - before.x) * 0.5f, (after.y - before.y) * 0.5f };
        float sc = 0.3f;
        curve_node nn;
        nn.position = pos;
        nn.handle_in = { -tang.x * sc, -tang.y * sc };
        nn.handle_out = { tang.x * sc, tang.y * sc };
        nn.handle_mode = curve_handle_mode::smooth;
        nodes.insert(nodes.begin() + seg + 1, nn);
        if (seg < (int)segment_modes.size())
            segment_modes.insert(segment_modes.begin() + seg + 1, segment_modes[seg]);
        else
            sync_segment_modes();
        return seg + 1;
    }
};

// ============================================================================
// curve_bg_image  —  optional background image for curve plots
// ============================================================================

struct curve_bg_image
{
    ImTextureID texture = ImTextureID();
    ImVec2 uv0 = { 0, 0 };
    ImVec2 uv1 = { 1, 1 };
    ImVec4 tint = { 1, 1, 1, 1 };
    bool custom_bounds = false;
    ImVec2 bounds_min = { 0, 0 };
    ImVec2 bounds_max = { 1, 1 };

    void set(ImTextureID tex) { texture = tex; }

    void set(ImTextureID tex, ImVec2 bmin, ImVec2 bmax)
    {
        texture = tex;
        custom_bounds = true;
        bounds_min = bmin;
        bounds_max = bmax;
    }

    void clear() { texture = ImTextureID(); }
    explicit operator bool() const { return texture != ImTextureID(); }
};

// ============================================================================
// curve_editor
// ============================================================================

struct curve_editor
{
    curve_path path;
    curve_bg_image bg_image;
    int selected_node = -1;
    bool is_dragging = false;

    curve_path::curve_hit rc_hit_;
    bool rc_open_node_menu_ = false;
    bool rc_open_add_menu_ = false;
    bool rc_open_blank_menu_ = false;

    void set_range(ImVec2 mn, ImVec2 mx)
    {
        path.range_min = mn;
        path.range_max = mx;
    }

    void set_function_mode(bool on)
    {
        path.function_mode = on;
        if (on)
            path.sort_nodes_by_x();
    }

    std::vector<ImVec2> sample(int n) const { return path.sample(n); }
    std::vector<ImVec2> sample_uniform_x(int n) const { return path.sample_uniform_x(n); }
    float evaluate_y(float x) const { return path.evaluate_y(x); }

    bool render(const char* label, ImVec2 canvas_size = { -1, 0 }, ImU32 curve_color = IM_COL32(0, 200, 0, 255));

private:
    bool node_has_handle_segments(int idx) const
    {
        if (idx > 0 && idx - 1 < (int)path.segment_modes.size())
        {
            auto m = path.segment_modes[idx - 1];
            if (m == curve_interpolation_mode::cubic_bezier || m == curve_interpolation_mode::hermite)
                return true;
        }
        if (idx < (int)path.segment_modes.size())
        {
            auto m = path.segment_modes[idx];
            if (m == curve_interpolation_mode::cubic_bezier || m == curve_interpolation_mode::hermite)
                return true;
        }
        return false;
    }

    static const char* interp_name(curve_interpolation_mode m)
    {
        switch (m)
        {
            case curve_interpolation_mode::linear: return "线性";
            case curve_interpolation_mode::cubic_bezier: return "三次贝塞尔";
            case curve_interpolation_mode::catmull_rom: return "Catmull-Rom";
            case curve_interpolation_mode::cubic_b_spline: return "B 样条";
            case curve_interpolation_mode::hermite: return "Hermite";
        }
        return "?";
    }

    static const char* handle_mode_name(curve_handle_mode m)
    {
        switch (m)
        {
            case curve_handle_mode::symmetric: return "对称";
            case curve_handle_mode::smooth: return "平滑";
            case curve_handle_mode::free: return "自由";
            case curve_handle_mode::auto_smooth: return "自动平滑";
        }
        return "?";
    }
};

// ============================================================================
// curve_editor::render
// ============================================================================

inline bool curve_editor::render(const char* label, ImVec2 canvas_size, ImU32 curve_color)
{
    bool modified = false;
    ImGui::PushID(label);

    path.compute_auto_smooth_handles();
    path.sync_segment_modes();

    ImVec4 color_vec = ImGui::ColorConvertU32ToFloat4(curve_color);
    const ImVec4 handle_col = { 0.78f, 0.39f, 1.0f, 1.0f };
    const ImVec4 handle_ln_col = { 0.78f, 0.39f, 1.0f, 0.78f };
    const ImVec4 sel_col = { 1.0f, 1.0f, 0.0f, 1.0f };

    float rx = path.range_max.x - path.range_min.x;
    float ry = path.range_max.y - path.range_min.y;
    float mx = rx * 0.05f, my = ry * 0.05f;

    ImPlotFlags pf = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText;
    bool popup_open = ImGui::IsPopupOpen("##CurveMenu") || ImGui::IsPopupOpen("##CurveAddMenu") || ImGui::IsPopupOpen("##CurveBlankMenu");

    if (ImPlot::BeginPlot(label, canvas_size, pf))
    {
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_None, ImPlotAxisFlags_None);
        ImPlot::SetupAxesLimits(path.range_min.x - mx, path.range_max.x + mx, path.range_min.y - my, path.range_max.y + my, ImPlotCond_Once);

        // ---- background image ----
        if (bg_image)
        {
            ImVec2 bmin = bg_image.custom_bounds ? bg_image.bounds_min : path.range_min;
            ImVec2 bmax = bg_image.custom_bounds ? bg_image.bounds_max : path.range_max;
            ImPlot::PlotImage("##bg", bg_image.texture, { bmin.x, bmin.y }, { bmax.x, bmax.y }, bg_image.uv0, bg_image.uv1, bg_image.tint);
        }

        // ---- range box ----
        {
            double bx[] = { path.range_min.x, path.range_max.x, path.range_max.x, path.range_min.x, path.range_min.x };
            double by[] = { path.range_min.y, path.range_min.y, path.range_max.y, path.range_max.y, path.range_min.y };
            ImPlot::SetNextLineStyle({ 0.5f, 0.5f, 0.5f, 0.6f }, 1.5f);
            ImPlot::PlotLine("##box", bx, by, 5);
        }

        // ---- drag key-points ----
        bool kp_dragged = false;
        for (int i = 0; i < (int)path.nodes.size(); ++i)
        {
            auto& nd = path.nodes[i];
            double px = nd.position.x, py = nd.position.y;
            bool sel = (i == selected_node);
            ImVec4 col = sel ? sel_col : color_vec;
            float sz = sel ? 8.0f : 6.0f;

            if (ImPlot::DragPoint(i * 3, &px, &py, col, sz, ImPlotDragToolFlags_None))
            {
                if (path.function_mode)
                {
                    if (i == 0)
                        px = path.range_min.x;
                    else if (i == (int)path.nodes.size() - 1)
                        px = path.range_max.x;
                }
                nd.position.x = std::clamp((float)px, path.range_min.x, path.range_max.x);
                nd.position.y = std::clamp((float)py, path.range_min.y, path.range_max.y);
                selected_node = i;
                kp_dragged = true;
                modified = true;
            }
        }

        // ---- click-to-select (independent of IsPlotHovered) ----
        if (!kp_dragged && !popup_open && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 pp = ImPlot::GetPlotPos();
            ImVec2 ps = ImPlot::GetPlotSize();
            ImVec2 m = ImGui::GetMousePos();
            if (m.x >= pp.x && m.x <= pp.x + ps.x && m.y >= pp.y && m.y <= pp.y + ps.y)
            {
                ImPlotPoint mp = ImPlot::GetPlotMousePos();
                auto lim = ImPlot::GetPlotLimits();
                float tol_sel = 0.025f * std::max((float)(lim.X.Max - lim.X.Min), (float)(lim.Y.Max - lim.Y.Min));
                int node_near = path.find_node_near((float)mp.x, (float)mp.y, tol_sel);
                if (node_near >= 0)
                {
                    selected_node = node_near;
                }
                else
                {
                    bool on_handle = false;
                    if (selected_node >= 0 && selected_node < (int)path.nodes.size() && node_has_handle_segments(selected_node))
                    {
                        auto& sp = path.nodes[selected_node];
                        float dx_in = sp.position.x + sp.handle_in.x - (float)mp.x;
                        float dy_in = sp.position.y + sp.handle_in.y - (float)mp.y;
                        float dx_out = sp.position.x + sp.handle_out.x - (float)mp.x;
                        float dy_out = sp.position.y + sp.handle_out.y - (float)mp.y;
                        float t2 = tol_sel * tol_sel;
                        on_handle = (dx_in * dx_in + dy_in * dy_in < t2) || (dx_out * dx_out + dy_out * dy_out < t2);
                    }
                    if (!on_handle)
                        selected_node = -1;
                }
            }
        }

        // ---- drag handles (selected node) ----
        if (selected_node >= 0 && selected_node < (int)path.nodes.size())
        {
            auto& sp = path.nodes[selected_node];
            bool show = node_has_handle_segments(selected_node);
            bool draggable = (sp.handle_mode != curve_handle_mode::auto_smooth);

            if (show && draggable)
            {
                double ix = sp.position.x + sp.handle_in.x;
                double iy = sp.position.y + sp.handle_in.y;
                if (ImPlot::DragPoint(selected_node * 3 + 1, &ix, &iy, handle_col, 5.0f, ImPlotDragToolFlags_None))
                {
                    sp.handle_in.x = (float)ix - sp.position.x;
                    sp.handle_in.y = (float)iy - sp.position.y;
                    sp.enforce_handle_constraint(true);
                    modified = true;
                }
                double ox = sp.position.x + sp.handle_out.x;
                double oy = sp.position.y + sp.handle_out.y;
                if (ImPlot::DragPoint(selected_node * 3 + 2, &ox, &oy, handle_col, 5.0f, ImPlotDragToolFlags_None))
                {
                    sp.handle_out.x = (float)ox - sp.position.x;
                    sp.handle_out.y = (float)oy - sp.position.y;
                    sp.enforce_handle_constraint(false);
                    modified = true;
                }
            }
        }

        // ---- sort after drag (function mode) ----
        if (kp_dragged)
            is_dragging = true;
        if (is_dragging && !kp_dragged)
        {
            if (path.function_mode)
            {
                float sx = -1, sy = -1;
                if (selected_node >= 0 && selected_node < (int)path.nodes.size())
                {
                    sx = path.nodes[selected_node].position.x;
                    sy = path.nodes[selected_node].position.y;
                }
                path.sort_nodes_by_x();
                selected_node = path.find_node_near(sx, sy, 0.001f);
            }
            is_dragging = false;
        }

        // ---- draw curve ----
        constexpr int SPP = 64;
        if (path.nodes.size() >= 2)
        {
            int ns = (int)path.nodes.size() - 1;
            std::vector<double> cx, cy;
            cx.reserve(ns * SPP + 1);
            cy.reserve(ns * SPP + 1);
            for (int s = 0; s < ns; ++s)
            {
                int start = (s == 0) ? 0 : 1;
                for (int k = start; k <= SPP; ++k)
                {
                    float t = (float)k / SPP;
                    ImVec2 pt = path.evaluate_segment(s, t);
                    cx.push_back(pt.x);
                    cy.push_back(pt.y);
                }
            }
            ImPlot::SetNextLineStyle(color_vec, 2.0f);
            ImPlot::PlotLine("##crv", cx.data(), cy.data(), (int)cx.size());
        }

        // ---- draw handle lines ----
        if (selected_node >= 0 && selected_node < (int)path.nodes.size())
        {
            const auto& sp = path.nodes[selected_node];
            if (node_has_handle_segments(selected_node))
            {
                double lx_in[2] = { sp.position.x, sp.position.x + sp.handle_in.x };
                double ly_in[2] = { sp.position.y, sp.position.y + sp.handle_in.y };
                double lx_out[2] = { sp.position.x, sp.position.x + sp.handle_out.x };
                double ly_out[2] = { sp.position.y, sp.position.y + sp.handle_out.y };
                ImPlot::SetNextLineStyle(handle_ln_col, 1.5f);
                ImPlot::PlotLine("##hin", lx_in, ly_in, 2);
                ImPlot::SetNextLineStyle(handle_ln_col, 1.5f);
                ImPlot::PlotLine("##hout", lx_out, ly_out, 2);

                if (sp.handle_mode == curve_handle_mode::auto_smooth)
                {
                    double dx[1] = { sp.position.x + sp.handle_in.x };
                    double dy[1] = { sp.position.y + sp.handle_in.y };
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4, handle_col, 1, handle_col);
                    ImPlot::PlotScatter("##as_in", dx, dy, 1);
                    dx[0] = sp.position.x + sp.handle_out.x;
                    dy[0] = sp.position.y + sp.handle_out.y;
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4, handle_col, 1, handle_col);
                    ImPlot::PlotScatter("##as_out", dx, dy, 1);
                }
            }
        }

        // ---- double-click add ----
        if (!popup_open && ImPlot::IsPlotHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImPlotPoint mp = ImPlot::GetPlotMousePos();
            curve_node nn;
            nn.position = {
                std::clamp((float)mp.x, path.range_min.x, path.range_max.x),
                std::clamp((float)mp.y, path.range_min.y, path.range_max.y),
            };
            nn.handle_mode = curve_handle_mode::smooth;
            path.add_node(nn);
            if (path.function_mode)
                path.sort_nodes_by_x();
            selected_node = path.find_node_near(nn.position.x, nn.position.y, 0.001f);
            modified = true;
        }

        // ---- right-click on node / curve (independent of IsPlotHovered) ----
        if (!popup_open && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImVec2 pp = ImPlot::GetPlotPos();
            ImVec2 ps = ImPlot::GetPlotSize();
            ImVec2 m = ImGui::GetMousePos();
            if (m.x >= pp.x && m.x <= pp.x + ps.x && m.y >= pp.y && m.y <= pp.y + ps.y)
            {
                ImPlotPoint mp = ImPlot::GetPlotMousePos();
                auto lim = ImPlot::GetPlotLimits();
                float tol = 0.025f * std::max((float)(lim.X.Max - lim.X.Min), (float)(lim.Y.Max - lim.Y.Min));
                int node_near = path.find_node_near((float)mp.x, (float)mp.y, tol);
                if (node_near >= 0)
                {
                    selected_node = node_near;
                    rc_open_node_menu_ = true;
                }
                else
                {
                    rc_hit_ = path.find_nearest_on_curve((float)mp.x, (float)mp.y);
                    if (sqrtf(rc_hit_.dist_sq) < tol * 2.0f)
                        rc_open_add_menu_ = true;
                    else
                        rc_open_blank_menu_ = true;
                }
            }
        }

        ImPlot::EndPlot();
    }

    // ---- right-click context menu ----
    if (rc_open_node_menu_)
    {
        ImGui::OpenPopup("##CurveMenu");
        rc_open_node_menu_ = false;
    }
    if (rc_open_add_menu_)
    {
        ImGui::OpenPopup("##CurveAddMenu");
        rc_open_add_menu_ = false;
    }
    if (rc_open_blank_menu_)
    {
        ImGui::OpenPopup("##CurveBlankMenu");
        rc_open_blank_menu_ = false;
    }

    if (ImGui::BeginPopup("##CurveMenu"))
    {
        if (selected_node >= 0 && selected_node < (int)path.nodes.size())
        {
            auto& nd = path.nodes[selected_node];

            // outgoing segment
            if (selected_node < (int)path.segment_modes.size())
            {
                auto& sm = path.segment_modes[selected_node];
                if (ImGui::BeginMenu("后一段插值"))
                {
                    for (auto m : { curve_interpolation_mode::linear, curve_interpolation_mode::cubic_bezier, curve_interpolation_mode::catmull_rom, curve_interpolation_mode::cubic_b_spline,
                                    curve_interpolation_mode::hermite })
                    {
                        if (ImGui::MenuItem(interp_name(m), nullptr, sm == m))
                        {
                            sm = m;
                            modified = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            // incoming segment
            if (selected_node > 0 && selected_node - 1 < (int)path.segment_modes.size())
            {
                auto& sm = path.segment_modes[selected_node - 1];
                if (ImGui::BeginMenu("前一段插值"))
                {
                    for (auto m : { curve_interpolation_mode::linear, curve_interpolation_mode::cubic_bezier, curve_interpolation_mode::catmull_rom, curve_interpolation_mode::cubic_b_spline,
                                    curve_interpolation_mode::hermite })
                    {
                        if (ImGui::MenuItem(interp_name(m), nullptr, sm == m))
                        {
                            sm = m;
                            modified = true;
                        }
                    }
                    ImGui::EndMenu();
                }
            }

            ImGui::Separator();

            if (ImGui::BeginMenu("控制柄模式"))
            {
                for (auto m : { curve_handle_mode::symmetric, curve_handle_mode::smooth, curve_handle_mode::free, curve_handle_mode::auto_smooth })
                {
                    if (ImGui::MenuItem(handle_mode_name(m), nullptr, nd.handle_mode == m))
                    {
                        nd.handle_mode = m;
                        if (m == curve_handle_mode::symmetric)
                            nd.enforce_handle_constraint(false);
                        modified = true;
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::Separator();

            bool can_del = (int)path.nodes.size() > 2;
            if (path.function_mode)
                can_del = can_del && selected_node > 0 && selected_node < (int)path.nodes.size() - 1;
            if (can_del && ImGui::MenuItem("删除节点"))
            {
                path.remove_node(selected_node);
                selected_node = -1;
                modified = true;
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("##CurveAddMenu"))
    {
        ImGui::Text("段 %d  t=%.2f", rc_hit_.segment, rc_hit_.t);
        ImGui::Separator();
        if (ImGui::MenuItem("在此添加节点"))
        {
            int idx = path.insert_node_on_segment(rc_hit_.segment, rc_hit_.t);
            if (idx >= 0)
            {
                selected_node = idx;
                if (path.function_mode)
                    path.sort_nodes_by_x();
                modified = true;
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("##CurveBlankMenu"))
    {
        if (ImGui::BeginMenu("所有节点控制柄模式"))
        {
            for (auto m : { curve_handle_mode::symmetric, curve_handle_mode::smooth, curve_handle_mode::free, curve_handle_mode::auto_smooth })
            {
                if (ImGui::MenuItem(handle_mode_name(m)))
                {
                    for (auto& nd : path.nodes)
                    {
                        nd.handle_mode = m;
                        if (m == curve_handle_mode::symmetric)
                            nd.enforce_handle_constraint(false);
                    }
                    modified = true;
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("所有段插值模式"))
        {
            for (auto m : { curve_interpolation_mode::linear, curve_interpolation_mode::cubic_bezier, curve_interpolation_mode::catmull_rom, curve_interpolation_mode::cubic_b_spline,
                            curve_interpolation_mode::hermite })
            {
                if (ImGui::MenuItem(interp_name(m)))
                {
                    for (auto& sm : path.segment_modes)
                        sm = m;
                    modified = true;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    // ---- node list panel ----
    if (ImGui::TreeNode("节点列表"))
    {
        for (int i = 0; i < (int)path.nodes.size(); ++i)
        {
            ImGui::PushID(i);
            auto& nd = path.nodes[i];
            bool is_sel = (i == selected_node);

            if (ImGui::Selectable("", is_sel, ImGuiSelectableFlags_None, { 0, 0 }))
                selected_node = i;
            ImGui::SameLine();

            ImGui::Text("[%d]", i);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            if (ImGui::InputFloat("##x", &nd.position.x, 0, 0, "x:%.4f", ImGuiInputTextFlags_None))
            {
                nd.position.x = std::clamp(nd.position.x, path.range_min.x, path.range_max.x);
                modified = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            if (ImGui::InputFloat("##y", &nd.position.y, 0, 0, "y:%.4f", ImGuiInputTextFlags_None))
            {
                nd.position.y = std::clamp(nd.position.y, path.range_min.y, path.range_max.y);
                modified = true;
            }

            if (is_sel && node_has_handle_segments(i))
            {
                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##ix", &nd.handle_in.x, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(true);
                    modified = true;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##iy", &nd.handle_in.y, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(true);
                    modified = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("->");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##ox", &nd.handle_out.x, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(false);
                    modified = true;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##oy", &nd.handle_out.y, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(false);
                    modified = true;
                }
            }

            ImGui::SameLine();
            ImGui::TextDisabled("%s", handle_mode_name(nd.handle_mode));

            ImGui::PopID();
        }

        if (modified && path.function_mode)
        {
            float sx = -1, sy = -1;
            if (selected_node >= 0 && selected_node < (int)path.nodes.size())
            {
                sx = path.nodes[selected_node].position.x;
                sy = path.nodes[selected_node].position.y;
            }
            path.sort_nodes_by_x();
            if (sx >= 0)
                selected_node = path.find_node_near(sx, sy, 0.001f);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
    return modified;
}

// ============================================================================
// curve_entry  —  one named curve with its own color
// ============================================================================

struct curve_entry
{
    curve_path path;
    ImU32 color = IM_COL32(0, 200, 0, 255);
    std::string name = "curve";
};

// ============================================================================
// curve_editor_multi  —  multiple 1D function curves in one plot
// ============================================================================

struct curve_editor_multi
{
    std::vector<curve_entry> curves;
    curve_bg_image bg_image;
    int active_curve = 0;
    int selected_node = -1;
    bool is_dragging = false;

    ImVec2 range_min = { 0, 0 };
    ImVec2 range_max = { 1, 1 };

    curve_path::curve_hit rc_hit_;
    bool rc_open_node_menu_ = false;
    bool rc_open_add_menu_ = false;
    bool rc_open_blank_menu_ = false;

    // -- curve management --

    int add_curve(const std::string& name, ImU32 color, curve_interpolation_mode dm = curve_interpolation_mode::cubic_bezier)
    {
        curve_entry e;
        e.name = name;
        e.color = color;
        e.path.function_mode = true;
        e.path.default_mode = dm;
        e.path.range_min = range_min;
        e.path.range_max = range_max;
        curve_node n0, n1;
        n0.position = range_min;
        n0.handle_out = { 0.15f * (range_max.x - range_min.x), 0 };
        n0.handle_in = { -n0.handle_out.x, 0 };
        n0.handle_mode = curve_handle_mode::smooth;
        n1.position = range_max;
        n1.handle_in = { -0.15f * (range_max.x - range_min.x), 0 };
        n1.handle_out = { -n1.handle_in.x, 0 };
        n1.handle_mode = curve_handle_mode::smooth;
        e.path.add_node(n0);
        e.path.add_node(n1);
        curves.push_back(std::move(e));
        return (int)curves.size() - 1;
    }

    void remove_curve(int idx)
    {
        if (idx < 0 || idx >= (int)curves.size())
            return;
        curves.erase(curves.begin() + idx);
        if (active_curve >= (int)curves.size())
            active_curve = std::max(0, (int)curves.size() - 1);
        selected_node = -1;
    }

    // -- accessors --

    curve_path& operator[](int i) { return curves[i].path; }
    const curve_path& operator[](int i) const { return curves[i].path; }
    int count() const { return (int)curves.size(); }

    std::vector<ImVec2> sample_uniform_x(int ci, int n) const
    {
        if (ci < 0 || ci >= (int)curves.size())
            return {};
        return curves[ci].path.sample_uniform_x(n);
    }

    float evaluate_y(int ci, float x) const
    {
        if (ci < 0 || ci >= (int)curves.size())
            return 0;
        return curves[ci].path.evaluate_y(x);
    }

    bool render(const char* label, ImVec2 canvas_size = { -1, 0 });

private:
    static bool has_handle_segs(const curve_path& p, int idx)
    {
        if (idx > 0 && idx - 1 < (int)p.segment_modes.size())
        {
            auto m = p.segment_modes[idx - 1];
            if (m == curve_interpolation_mode::cubic_bezier || m == curve_interpolation_mode::hermite)
                return true;
        }
        if (idx < (int)p.segment_modes.size())
        {
            auto m = p.segment_modes[idx];
            if (m == curve_interpolation_mode::cubic_bezier || m == curve_interpolation_mode::hermite)
                return true;
        }
        return false;
    }
    static const char* iname(curve_interpolation_mode m)
    {
        switch (m)
        {
            case curve_interpolation_mode::linear: return "线性";
            case curve_interpolation_mode::cubic_bezier: return "三次贝塞尔";
            case curve_interpolation_mode::catmull_rom: return "Catmull-Rom";
            case curve_interpolation_mode::cubic_b_spline: return "B 样条";
            case curve_interpolation_mode::hermite: return "Hermite";
        }
        return "?";
    }
    static const char* hname(curve_handle_mode m)
    {
        switch (m)
        {
            case curve_handle_mode::symmetric: return "对称";
            case curve_handle_mode::smooth: return "平滑";
            case curve_handle_mode::free: return "自由";
            case curve_handle_mode::auto_smooth: return "自动平滑";
        }
        return "?";
    }
};

// ============================================================================
// curve_editor_multi::render
// ============================================================================

inline bool curve_editor_multi::render(const char* label, ImVec2 canvas_size)
{
    bool modified = false;
    ImGui::PushID(label);

    for (auto& c : curves)
    {
        c.path.range_min = range_min;
        c.path.range_max = range_max;
        c.path.function_mode = true;
        c.path.compute_auto_smooth_handles();
        c.path.sync_segment_modes();
    }

    // ---- curve selector ----
    if (!curves.empty())
    {
        ImGui::SetNextItemWidth(160);
        if (ImGui::BeginCombo("##sel", curves[active_curve].name.c_str()))
        {
            for (int i = 0; i < (int)curves.size(); ++i)
            {
                ImVec4 tc = ImGui::ColorConvertU32ToFloat4(curves[i].color);
                ImGui::PushStyleColor(ImGuiCol_Text, tc);
                if (ImGui::Selectable(curves[i].name.c_str(), i == active_curve))
                {
                    active_curve = i;
                    selected_node = -1;
                }
                ImGui::PopStyleColor();
                if (i == active_curve)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
    }
    if (ImGui::SmallButton("+添加"))
    {
        static const ImU32 palette[] = {
            IM_COL32(80, 200, 80, 255), IM_COL32(220, 60, 60, 255), IM_COL32(60, 120, 255, 255), IM_COL32(255, 200, 40, 255), IM_COL32(200, 60, 220, 255), IM_COL32(40, 200, 200, 255),
        };
        int ci = (int)curves.size();
        add_curve(std::string("曲线 ") + std::to_string(ci), palette[ci % 6]);
        active_curve = ci;
        selected_node = -1;
        modified = true;
    }
    if ((int)curves.size() > 1)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("-删除"))
        {
            remove_curve(active_curve);
            modified = true;
        }
    }

    if (curves.empty())
    {
        ImGui::PopID();
        return modified;
    }

    // ---- plot ----
    float rx = range_max.x - range_min.x, ry = range_max.y - range_min.y;
    float mg_x = rx * 0.05f, mg_y = ry * 0.05f;
    ImPlotFlags pf = ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText;
    bool popup_open = ImGui::IsPopupOpen("##MNodeMenu") || ImGui::IsPopupOpen("##MAddMenu") || ImGui::IsPopupOpen("##MBlankMenu");

    if (ImPlot::BeginPlot(label, canvas_size, pf))
    {
        ImPlot::SetupAxes(nullptr, nullptr);
        ImPlot::SetupAxesLimits(range_min.x - mg_x, range_max.x + mg_x, range_min.y - mg_y, range_max.y + mg_y, ImPlotCond_Once);

        // background image
        if (bg_image)
        {
            ImVec2 bmin = bg_image.custom_bounds ? bg_image.bounds_min : range_min;
            ImVec2 bmax = bg_image.custom_bounds ? bg_image.bounds_max : range_max;
            ImPlot::PlotImage("##bg", bg_image.texture, { bmin.x, bmin.y }, { bmax.x, bmax.y }, bg_image.uv0, bg_image.uv1, bg_image.tint);
        }

        // range box
        {
            double bx[] = { range_min.x, range_max.x, range_max.x, range_min.x, range_min.x };
            double by[] = { range_min.y, range_min.y, range_max.y, range_max.y, range_min.y };
            ImPlot::SetNextLineStyle({ 0.5f, 0.5f, 0.5f, 0.6f }, 1.5f);
            ImPlot::PlotLine("##box", bx, by, 5);
        }

        constexpr int SPP = 64;

        // ---- draw ALL curves ----
        for (int ci = 0; ci < (int)curves.size(); ++ci)
        {
            auto& cp = curves[ci].path;
            ImVec4 col = ImGui::ColorConvertU32ToFloat4(curves[ci].color);
            bool is_act = (ci == active_curve);

            if (cp.nodes.size() >= 2)
            {
                int ns = (int)cp.nodes.size() - 1;
                std::vector<double> lx, ly;
                lx.reserve(ns * SPP + 1);
                ly.reserve(ns * SPP + 1);
                for (int s = 0; s < ns; ++s)
                    for (int k = (s == 0 ? 0 : 1); k <= SPP; ++k)
                    {
                        float t = (float)k / SPP;
                        ImVec2 pt = cp.evaluate_segment(s, t);
                        lx.push_back(pt.x);
                        ly.push_back(pt.y);
                    }
                ImVec4 lc = { col.x, col.y, col.z, is_act ? 1.0f : 0.45f };
                ImPlot::SetNextLineStyle(lc, is_act ? 2.5f : 1.5f);
                char id[32];
                snprintf(id, sizeof(id), "##c%d", ci);
                ImPlot::PlotLine(id, lx.data(), ly.data(), (int)lx.size());
            }

            // non-active curve: dim node markers
            if (!is_act && !cp.nodes.empty())
            {
                std::vector<double> nx(cp.nodes.size()), ny(cp.nodes.size());
                for (int i = 0; i < (int)cp.nodes.size(); ++i)
                {
                    nx[i] = cp.nodes[i].position.x;
                    ny[i] = cp.nodes[i].position.y;
                }
                ImVec4 dc = { col.x, col.y, col.z, 0.4f };
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4, dc, 1, dc);
                char mid[32];
                snprintf(mid, sizeof(mid), "##m%d", ci);
                ImPlot::PlotScatter(mid, nx.data(), ny.data(), (int)nx.size());
            }
        }

        // ---- active curve: interactive nodes ----
        auto& ap = curves[active_curve].path;
        ImVec4 ac = ImGui::ColorConvertU32ToFloat4(curves[active_curve].color);
        const ImVec4 sel_col = { 1, 1, 0, 1 };
        const ImVec4 handle_col = { 0.78f, 0.39f, 1, 1 };
        const ImVec4 handle_ln_col = { 0.78f, 0.39f, 1, 0.78f };

        bool kp_dragged = false;
        for (int i = 0; i < (int)ap.nodes.size(); ++i)
        {
            auto& nd = ap.nodes[i];
            double px = nd.position.x, py = nd.position.y;
            bool sel = (i == selected_node);
            if (ImPlot::DragPoint(i * 3, &px, &py, sel ? sel_col : ac, sel ? 8.f : 6.f, ImPlotDragToolFlags_None))
            {
                if (i == 0)
                    px = range_min.x;
                else if (i == (int)ap.nodes.size() - 1)
                    px = range_max.x;
                nd.position.x = std::clamp((float)px, range_min.x, range_max.x);
                nd.position.y = std::clamp((float)py, range_min.y, range_max.y);
                selected_node = i;
                kp_dragged = true;
                modified = true;
            }
        }

        // ---- click-to-select (independent of IsPlotHovered) ----
        if (!kp_dragged && !popup_open && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            ImVec2 pp = ImPlot::GetPlotPos();
            ImVec2 ps = ImPlot::GetPlotSize();
            ImVec2 m = ImGui::GetMousePos();
            if (m.x >= pp.x && m.x <= pp.x + ps.x && m.y >= pp.y && m.y <= pp.y + ps.y)
            {
                ImPlotPoint mp = ImPlot::GetPlotMousePos();
                auto lim = ImPlot::GetPlotLimits();
                float tol_sel = 0.025f * std::max((float)(lim.X.Max - lim.X.Min), (float)(lim.Y.Max - lim.Y.Min));
                int found = ap.find_node_near((float)mp.x, (float)mp.y, tol_sel);
                if (found >= 0)
                {
                    selected_node = found;
                }
                else
                {
                    bool on_handle = false;
                    if (selected_node >= 0 && selected_node < (int)ap.nodes.size() && has_handle_segs(ap, selected_node))
                    {
                        auto& sp = ap.nodes[selected_node];
                        float dx_in = sp.position.x + sp.handle_in.x - (float)mp.x;
                        float dy_in = sp.position.y + sp.handle_in.y - (float)mp.y;
                        float dx_out = sp.position.x + sp.handle_out.x - (float)mp.x;
                        float dy_out = sp.position.y + sp.handle_out.y - (float)mp.y;
                        float t2 = tol_sel * tol_sel;
                        on_handle = (dx_in * dx_in + dy_in * dy_in < t2) || (dx_out * dx_out + dy_out * dy_out < t2);
                    }
                    if (!on_handle)
                    {
                        bool switched = false;
                        for (int ci = 0; ci < (int)curves.size(); ++ci)
                        {
                            if (ci == active_curve)
                                continue;
                            int fi = curves[ci].path.find_node_near((float)mp.x, (float)mp.y, tol_sel);
                            if (fi >= 0)
                            {
                                active_curve = ci;
                                selected_node = fi;
                                switched = true;
                                break;
                            }
                        }
                        if (!switched)
                            selected_node = -1;
                    }
                }
            }
        }

        // handles
        if (selected_node >= 0 && selected_node < (int)ap.nodes.size())
        {
            auto& sp = ap.nodes[selected_node];
            bool show = has_handle_segs(ap, selected_node);
            bool drag = (sp.handle_mode != curve_handle_mode::auto_smooth);
            if (show && drag)
            {
                double ix = sp.position.x + sp.handle_in.x, iy = sp.position.y + sp.handle_in.y;
                if (ImPlot::DragPoint(selected_node * 3 + 1, &ix, &iy, handle_col, 5, ImPlotDragToolFlags_None))
                {
                    sp.handle_in.x = (float)ix - sp.position.x;
                    sp.handle_in.y = (float)iy - sp.position.y;
                    sp.enforce_handle_constraint(true);
                    modified = true;
                }
                double ox = sp.position.x + sp.handle_out.x, oy = sp.position.y + sp.handle_out.y;
                if (ImPlot::DragPoint(selected_node * 3 + 2, &ox, &oy, handle_col, 5, ImPlotDragToolFlags_None))
                {
                    sp.handle_out.x = (float)ox - sp.position.x;
                    sp.handle_out.y = (float)oy - sp.position.y;
                    sp.enforce_handle_constraint(false);
                    modified = true;
                }
            }
        }

        // sort after drag
        if (kp_dragged)
            is_dragging = true;
        if (is_dragging && !kp_dragged)
        {
            float sx = -1, sy = -1;
            if (selected_node >= 0 && selected_node < (int)ap.nodes.size())
            {
                sx = ap.nodes[selected_node].position.x;
                sy = ap.nodes[selected_node].position.y;
            }
            ap.sort_nodes_by_x();
            selected_node = ap.find_node_near(sx, sy, 0.001f);
            is_dragging = false;
        }

        // handle lines
        if (selected_node >= 0 && selected_node < (int)ap.nodes.size() && has_handle_segs(ap, selected_node))
        {
            const auto& sp = ap.nodes[selected_node];
            double lix[2] = { sp.position.x, sp.position.x + sp.handle_in.x };
            double liy[2] = { sp.position.y, sp.position.y + sp.handle_in.y };
            double lox[2] = { sp.position.x, sp.position.x + sp.handle_out.x };
            double loy[2] = { sp.position.y, sp.position.y + sp.handle_out.y };
            ImPlot::SetNextLineStyle(handle_ln_col, 1.5f);
            ImPlot::PlotLine("##hi", lix, liy, 2);
            ImPlot::SetNextLineStyle(handle_ln_col, 1.5f);
            ImPlot::PlotLine("##ho", lox, loy, 2);
            if (sp.handle_mode == curve_handle_mode::auto_smooth)
            {
                double dx[1], dy[1];
                dx[0] = lix[1];
                dy[0] = liy[1];
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4, handle_col, 1, handle_col);
                ImPlot::PlotScatter("##ai", dx, dy, 1);
                dx[0] = lox[1];
                dy[0] = loy[1];
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 4, handle_col, 1, handle_col);
                ImPlot::PlotScatter("##ao", dx, dy, 1);
            }
        }

        // ---- double-click add ----
        if (!popup_open && ImPlot::IsPlotHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImPlotPoint mp = ImPlot::GetPlotMousePos();
            curve_node nn;
            nn.position = { std::clamp((float)mp.x, range_min.x, range_max.x), std::clamp((float)mp.y, range_min.y, range_max.y) };
            nn.handle_mode = curve_handle_mode::smooth;
            ap.add_node(nn);
            ap.sort_nodes_by_x();
            selected_node = ap.find_node_near(nn.position.x, nn.position.y, 0.001f);
            modified = true;
        }

        // ---- right-click on node / curve (independent of IsPlotHovered) ----
        if (!popup_open && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            ImVec2 pp = ImPlot::GetPlotPos();
            ImVec2 ps = ImPlot::GetPlotSize();
            ImVec2 m = ImGui::GetMousePos();
            if (m.x >= pp.x && m.x <= pp.x + ps.x && m.y >= pp.y && m.y <= pp.y + ps.y)
            {
                ImPlotPoint mp = ImPlot::GetPlotMousePos();
                auto lim = ImPlot::GetPlotLimits();
                float tol = 0.025f * std::max((float)(lim.X.Max - lim.X.Min), (float)(lim.Y.Max - lim.Y.Min));
                int node_near = ap.find_node_near((float)mp.x, (float)mp.y, tol);
                if (node_near >= 0)
                {
                    selected_node = node_near;
                    rc_open_node_menu_ = true;
                }
                else
                {
                    rc_hit_ = ap.find_nearest_on_curve((float)mp.x, (float)mp.y);
                    if (sqrtf(rc_hit_.dist_sq) < tol * 2.0f)
                        rc_open_add_menu_ = true;
                    else
                        rc_open_blank_menu_ = true;
                }
            }
        }

        ImPlot::EndPlot();
    }

    // ---- popups ----
    if (rc_open_node_menu_)
    {
        ImGui::OpenPopup("##MNodeMenu");
        rc_open_node_menu_ = false;
    }
    if (rc_open_add_menu_)
    {
        ImGui::OpenPopup("##MAddMenu");
        rc_open_add_menu_ = false;
    }
    if (rc_open_blank_menu_)
    {
        ImGui::OpenPopup("##MBlankMenu");
        rc_open_blank_menu_ = false;
    }

    if (ImGui::BeginPopup("##MNodeMenu"))
    {
        auto& ap = curves[active_curve].path;
        if (selected_node >= 0 && selected_node < (int)ap.nodes.size())
        {
            auto& nd = ap.nodes[selected_node];

            if (selected_node < (int)ap.segment_modes.size())
            {
                auto& sm = ap.segment_modes[selected_node];
                if (ImGui::BeginMenu("后一段插值"))
                {
                    for (auto m : { curve_interpolation_mode::linear, curve_interpolation_mode::cubic_bezier, curve_interpolation_mode::catmull_rom, curve_interpolation_mode::cubic_b_spline,
                                    curve_interpolation_mode::hermite })
                        if (ImGui::MenuItem(iname(m), nullptr, sm == m))
                        {
                            sm = m;
                            modified = true;
                        }
                    ImGui::EndMenu();
                }
            }
            if (selected_node > 0 && selected_node - 1 < (int)ap.segment_modes.size())
            {
                auto& sm = ap.segment_modes[selected_node - 1];
                if (ImGui::BeginMenu("前一段插值"))
                {
                    for (auto m : { curve_interpolation_mode::linear, curve_interpolation_mode::cubic_bezier, curve_interpolation_mode::catmull_rom, curve_interpolation_mode::cubic_b_spline,
                                    curve_interpolation_mode::hermite })
                        if (ImGui::MenuItem(iname(m), nullptr, sm == m))
                        {
                            sm = m;
                            modified = true;
                        }
                    ImGui::EndMenu();
                }
            }
            ImGui::Separator();
            if (ImGui::BeginMenu("控制柄模式"))
            {
                for (auto m : { curve_handle_mode::symmetric, curve_handle_mode::smooth, curve_handle_mode::free, curve_handle_mode::auto_smooth })
                    if (ImGui::MenuItem(hname(m), nullptr, nd.handle_mode == m))
                    {
                        nd.handle_mode = m;
                        if (m == curve_handle_mode::symmetric)
                            nd.enforce_handle_constraint(false);
                        modified = true;
                    }
                ImGui::EndMenu();
            }
            ImGui::Separator();
            bool can_del = (int)ap.nodes.size() > 2 && selected_node > 0 && selected_node < (int)ap.nodes.size() - 1;
            if (can_del && ImGui::MenuItem("删除节点"))
            {
                ap.remove_node(selected_node);
                selected_node = -1;
                modified = true;
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("##MAddMenu"))
    {
        ImGui::Text("段 %d  t=%.2f", rc_hit_.segment, rc_hit_.t);
        ImGui::Separator();
        if (ImGui::MenuItem("在此添加节点"))
        {
            auto& ap = curves[active_curve].path;
            int idx = ap.insert_node_on_segment(rc_hit_.segment, rc_hit_.t);
            if (idx >= 0)
            {
                selected_node = idx;
                ap.sort_nodes_by_x();
                modified = true;
            }
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("##MBlankMenu"))
    {
        auto& ap = curves[active_curve].path;
        if (ImGui::BeginMenu("所有节点控制柄模式"))
        {
            for (auto m : { curve_handle_mode::symmetric, curve_handle_mode::smooth, curve_handle_mode::free, curve_handle_mode::auto_smooth })
            {
                if (ImGui::MenuItem(hname(m)))
                {
                    for (auto& nd : ap.nodes)
                    {
                        nd.handle_mode = m;
                        if (m == curve_handle_mode::symmetric)
                            nd.enforce_handle_constraint(false);
                    }
                    modified = true;
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("所有段插值模式"))
        {
            for (auto m : { curve_interpolation_mode::linear, curve_interpolation_mode::cubic_bezier, curve_interpolation_mode::catmull_rom, curve_interpolation_mode::cubic_b_spline,
                            curve_interpolation_mode::hermite })
            {
                if (ImGui::MenuItem(iname(m)))
                {
                    for (auto& sm : ap.segment_modes)
                        sm = m;
                    modified = true;
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    // ---- node list panel ----
    if (!curves.empty() && ImGui::TreeNode("节点列表"))
    {
        auto& ap = curves[active_curve].path;
        for (int i = 0; i < (int)ap.nodes.size(); ++i)
        {
            ImGui::PushID(i);
            auto& nd = ap.nodes[i];
            bool is_sel = (i == selected_node);

            if (ImGui::Selectable("", is_sel, ImGuiSelectableFlags_None, { 0, 0 }))
                selected_node = i;
            ImGui::SameLine();

            ImGui::Text("[%d]", i);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            if (ImGui::InputFloat("##x", &nd.position.x, 0, 0, "x:%.4f", ImGuiInputTextFlags_None))
            {
                nd.position.x = std::clamp(nd.position.x, range_min.x, range_max.x);
                modified = true;
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(70);
            if (ImGui::InputFloat("##y", &nd.position.y, 0, 0, "y:%.4f", ImGuiInputTextFlags_None))
            {
                nd.position.y = std::clamp(nd.position.y, range_min.y, range_max.y);
                modified = true;
            }

            if (is_sel && has_handle_segs(ap, i))
            {
                ImGui::SameLine();
                ImGui::TextDisabled("|");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##ix", &nd.handle_in.x, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(true);
                    modified = true;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##iy", &nd.handle_in.y, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(true);
                    modified = true;
                }
                ImGui::SameLine();
                ImGui::TextDisabled("->");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##ox", &nd.handle_out.x, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(false);
                    modified = true;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(55);
                if (ImGui::InputFloat("##oy", &nd.handle_out.y, 0, 0, "%.3f", ImGuiInputTextFlags_None))
                {
                    nd.enforce_handle_constraint(false);
                    modified = true;
                }
            }

            ImGui::SameLine();
            ImGui::TextDisabled("%s", hname(nd.handle_mode));

            ImGui::PopID();
        }

        if (modified)
        {
            float sx = -1, sy = -1;
            if (selected_node >= 0 && selected_node < (int)ap.nodes.size())
            {
                sx = ap.nodes[selected_node].position.x;
                sy = ap.nodes[selected_node].position.y;
            }
            ap.sort_nodes_by_x();
            if (sx >= 0)
                selected_node = ap.find_node_near(sx, sy, 0.001f);
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
    return modified;
}
