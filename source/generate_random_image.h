#pragma once
#include <opencv2/core.hpp>

// @brief 生成一张包含随机几何图形（矩形、圆形、直线）的图像
// @param width     图像宽度
// @param height    图像高度
// @param numShapes 生成的形状数量（矩形/圆形/直线总和）
// @param bgColor   背景灰度值（0~255），默认255（白色）
// @return          生成的彩色图像（CV_8UC3）
cv::Mat generate_random_image(int width, int height, int numShapes = 20, int bgColor = 255)
{
    // 创建背景图像
    cv::Mat img(height, width, CV_8UC3, cv::Scalar(bgColor, bgColor, bgColor));
    cv::RNG rng; // 随机数生成器（基于系统时间）

    for (int i = 0; i < numShapes; ++i)
    {
        // 随机决定形状类型：0-矩形，1-圆形，2-直线
        int shapeType = rng.uniform(0, 3);
        // 随机颜色（BGR顺序）
        cv::Scalar color(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));

        if (shapeType == 0)
        { // 矩形
            int minSize = 10;
            int maxW = std::max(minSize, width / 4);
            int maxH = std::max(minSize, height / 4);
            int rectW = rng.uniform(minSize, maxW);
            int rectH = rng.uniform(minSize, maxH);
            int x = rng.uniform(0, width - rectW);
            int y = rng.uniform(0, height - rectH);
            cv::Rect rect(x, y, rectW, rectH);

            // 随机决定是填充矩形还是空心矩形
            if (rng.uniform(0, 2) == 0)
            {
                cv::rectangle(img, rect, color, cv::FILLED);
            }
            else
            {
                int thickness = rng.uniform(1, 5);
                cv::rectangle(img, rect, color, thickness);
            }
        }
        else if (shapeType == 1)
        { // 圆形
            int minR = 5;
            int maxR = std::max(minR, std::min(width, height) / 5);
            int radius = rng.uniform(minR, maxR);
            int cx = rng.uniform(radius, width - radius);
            int cy = rng.uniform(radius, height - radius);
            cv::Point center(cx, cy);

            if (rng.uniform(0, 2) == 0)
            {
                cv::circle(img, center, radius, color, cv::FILLED);
            }
            else
            {
                int thickness = rng.uniform(1, 5);
                cv::circle(img, center, radius, color, thickness);
            }
        }
        else
        { // 直线
            cv::Point p1(rng.uniform(0, width), rng.uniform(0, height));
            cv::Point p2(rng.uniform(0, width), rng.uniform(0, height));
            int thickness = rng.uniform(1, 5);
            cv::line(img, p1, p2, color, thickness);
        }
    }
    return img;
}