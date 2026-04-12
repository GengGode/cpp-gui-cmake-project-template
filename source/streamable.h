#pragma once

#include <functional>
#include <memory>
#include <opencv2/opencv.hpp>
#include <optional>

struct streamable
{
    virtual std::optional<cv::Mat> feed(cv::Mat in) = 0;
    virtual std::optional<cv::Mat> flush() = 0;
    virtual ~streamable() = default;
};

using process_func = std::function<cv::Mat(const cv::Mat& extended_input)>;
using blend_func = std::function<cv::Mat(const cv::Mat& old_tail, const cv::Mat& new_head)>;
std::unique_ptr<streamable> make_delay_chunk_streaming(process_func process, blend_func blend, int context, int blend_rows);

namespace detail
{
    inline cv::Mat default_blend(const cv::Mat& old_tail, const cv::Mat& new_head)
    {
        cv::Mat result = old_tail.clone();
        for (int r = 0; r < old_tail.rows; ++r)
        {
            float alpha = static_cast<float>(r) / old_tail.rows;
            result.row(r) = old_tail.row(r) * (1.f - alpha) + new_head.row(r) * alpha;
        }
        return result;
    }
    inline cv::Mat copy_blend(const cv::Mat& old_tail, const cv::Mat& new_head)
    { return old_tail.clone(); }

    class delay_chunk_streaming : public streamable
    {
    public:
        delay_chunk_streaming(process_func process, blend_func blend, int context, int blend_rows) : process_(std::move(process)), blend_(std::move(blend)), context_(context), blend_rows_(blend_rows)
        {
            if (context_ <= 0 || blend_rows_ < 0 || blend_rows_ > context_)
                throw std::invalid_argument("Invalid context/blend_rows");
        }
        delay_chunk_streaming(process_func process, int context, int blend_rows) : delay_chunk_streaming(process, default_blend, context, blend_rows) {}

        std::optional<cv::Mat> feed(cv::Mat in) override
        {
            if (in.empty())
                return std::nullopt;

            if (first_frame_)
            {
                first_frame_ = false;
                pending_chunk_ = in.clone();
                return std::nullopt;
            }

            if (in.rows < context_)
            {
                cv::Mat merged;
                cv::vconcat(pending_chunk_, in, merged);
                pending_chunk_ = merged;
                return std::nullopt;
            }

            cv::Mat result = process_pending(in);

            prev_raw_tail_ = pending_chunk_.rowRange(std::max(0, pending_chunk_.rows - context_), pending_chunk_.rows).clone();
            pending_chunk_ = in.clone();

            return result;
        }

        std::optional<cv::Mat> flush() override
        {
            if (first_frame_)
                return std::nullopt;

            cv::Mat top_ctx = build_top_context();
            cv::Mat bottom_ctx = build_border_pad(pending_chunk_, context_, false);

            cv::Mat extended;
            cv::vconcat(top_ctx, pending_chunk_, extended);
            cv::vconcat(extended, bottom_ctx, extended);

            cv::Mat output = process_(extended);

            int valid_start = context_;
            int valid_rows = pending_chunk_.rows;
            cv::Mat block = output.rowRange(valid_start, valid_start + valid_rows).clone();

            if (!cached_overlap_.empty() && blend_rows_ > 0)
            {
                int bh = std::min(blend_rows_, valid_rows);
                cv::Mat blended = blend_(cached_overlap_, block.rowRange(0, bh));
                blended.copyTo(block.rowRange(0, bh));
            }

            first_frame_ = true;
            pending_chunk_.release();
            prev_raw_tail_.release();
            cached_overlap_.release();

            return block;
        }

    private:
        cv::Mat process_pending(const cv::Mat& next_chunk)
        {
            cv::Mat top_ctx = build_top_context();
            cv::Mat bottom_ctx = next_chunk.rowRange(0, std::min(context_, next_chunk.rows)).clone();

            cv::Mat extended;
            cv::vconcat(top_ctx, pending_chunk_, extended);
            cv::vconcat(extended, bottom_ctx, extended);

            cv::Mat output = process_(extended);

            int valid_start = context_;
            int valid_rows = pending_chunk_.rows;
            cv::Mat block = output.rowRange(valid_start, valid_start + valid_rows).clone();

            if (!cached_overlap_.empty() && blend_rows_ > 0)
            {
                int bh = std::min(blend_rows_, valid_rows);
                cv::Mat blended = blend_(cached_overlap_, block.rowRange(0, bh));
                blended.copyTo(block.rowRange(0, bh));
            }

            if (blend_rows_ > 0)
            {
                int extra_start = valid_start + valid_rows;
                int extra_avail = output.rows - extra_start;
                int extra_rows = std::min(blend_rows_, extra_avail);
                if (extra_rows > 0)
                    cached_overlap_ = output.rowRange(extra_start, extra_start + extra_rows).clone();
                else
                    cached_overlap_.release();
            }

            return block;
        }

        cv::Mat build_top_context()
        {
            if (prev_raw_tail_.empty())
            {
                return build_border_pad(pending_chunk_, context_, true);
            }
            if (prev_raw_tail_.rows >= context_)
                return prev_raw_tail_.rowRange(prev_raw_tail_.rows - context_, prev_raw_tail_.rows).clone();

            cv::Mat pad = build_border_pad(prev_raw_tail_, context_ - prev_raw_tail_.rows, true);
            cv::Mat result;
            cv::vconcat(pad, prev_raw_tail_, result);
            return result;
        }

        static cv::Mat build_border_pad(const cv::Mat& ref, int pad_rows, bool pad_top)
        {
            if (pad_rows <= 0)
                return cv::Mat();
            cv::Mat row = pad_top ? ref.row(0).clone() : ref.row(ref.rows - 1).clone();
            cv::Mat pad;
            cv::repeat(row, pad_rows, 1, pad);
            return pad;
        }

        process_func process_;
        blend_func blend_;
        int context_;
        int blend_rows_;

        bool first_frame_ = true;
        cv::Mat pending_chunk_;  // 缓存的当前原始 chunk，等下一个 chunk 到来时处理
        cv::Mat prev_raw_tail_;  // 上一个 chunk 的最后 context 行（原始数据）
        cv::Mat cached_overlap_; // 当前处理结果超出有效区域的 blend_rows 行，用于与下一块混合
    };
} // namespace detail

// @param process       全局处理函数，输入为扩展后的图像，输出同尺寸
// @param blend         混合函数，用于重叠区域的平滑过渡
// @param context       处理函数需要的单侧上下文行数（例如卷积核半径）
// @param blend_rows    混合区域的行数（须 ≤ context）
inline std::unique_ptr<streamable> make_delay_chunk_streaming(process_func process, blend_func blend, int context, int blend_rows)
{ return std::make_unique<detail::delay_chunk_streaming>(std::move(process), std::move(blend), context, blend_rows); }