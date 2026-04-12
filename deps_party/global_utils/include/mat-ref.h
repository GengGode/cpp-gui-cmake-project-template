#pragma once

#ifndef MAT_REF
    #define MAT_REF
struct mat_ref
{
    int rows;     // 图像高度
    int cols;     // 图像宽度
    int channels; // 通道数
    int depth;    // 位深度 (CV_8U: 1, CV_16U: 2, CV_32F: 4, CV_64F: 8)
    void* data;   // 数据指针
    size_t step;  // 行步长（字节数）
};
#endif
#pragma once

#ifndef ROW_REF
    #define ROW_REF
struct row_ref
{
    int rows;
    int cols;     // 图像宽度
    int channels; // 通道数
    int depth;    // 位深度 (CV_8U: 1, CV_16U: 2, CV_32F: 4, CV_64F: 8)
    void* data;   // 数据指针
    size_t step;  // 行步长（字节数）
};
#endif

#ifndef IS_CONTINUOUS
    #define IS_CONTINUOUS
    #define is_continuous(ref) ((ref).step == (size_t)((ref).cols * (ref).channels * (ref).depth))
#endif // IS_CONTINUOUS

#ifndef ROW_RANGE
    #define ROW_RANGE
    #define row_range(ref, start, end) mat_ref(((end) - (start)), (ref).cols, (ref).channels, (ref).depth, (void*)((char*)((ref).data) + (start) * (ref).step), (size_t)(ref).step)
#endif // ROW_RANGE

#ifndef REF_ROW
    #define REF_ROW
    #define ref_row(ref, row) row_ref(1, (ref).cols, (ref).channels, (ref).depth, (void*)((char*)((ref).data) + (row) * (ref).step), (size_t)(ref).step)
#endif // REF_ROW

#ifdef OPENCV_CORE_MAT_HPP
    #ifndef TO_REF
        #define TO_REF
        #define to_ref(mat) mat_ref((mat).rows, (mat).cols, (mat).channels(), (mat).elemSize(), (void*)(mat).data, (mat).step)
    #endif // TO_REF

    #ifndef TO_MAT
        #define TO_MAT
        #define to_mat(ref) cv::Mat((ref).rows, (ref).cols, CV_MAKETYPE((ref).depth, (ref).channels), (ref).data, (ref).step)
    #endif // TO_MAT
#endif     // OPENCV_CORE_MAT_HPP
