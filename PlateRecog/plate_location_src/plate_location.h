#ifndef _LOCATION_H_
#define _LOCATION_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _TMS320C6X_
#define PLATE_RUN_BIT   // 在DSP平台上用位操作方式进行处理
#endif

#include "../location_interface.h"

#define RECT_MAX    (80)    // 车牌的最大数目, 高清视频里车牌候选区域数目较多，因此将上限调整为80

//#define PLATE_SAVE 0
//#define DBUG_WRITE 0
//#define SHOW_TRUST 1

    typedef struct
    {
        uint8_t *gray_img;          // 待处理的灰度图像
        uint8_t *edge_img;          // 待处理的梯度图像
        uint8_t *bina_img;          // 待处理的二值图像

//  int32_t video_flag;         // 是否是视频流
        // 传入参数
        Rects rect_detect;          // 待检测区域
        int32_t img_w;              // 图像宽度
        int32_t img_h;              // 图像高度
        int32_t plate_w_min;        // 目标的最小宽度
        int32_t plate_w_max;        // 目标的最大宽度
        int32_t plate_h_min;        // 目标的最小高度
        int32_t plate_h_max;        // 目标的最大高度

#ifdef USE_HAAR_DETECTION
        Cascade* cascade;           // 级联分类器
        int32_t min_neighbors;      // 最小的类成员数目(用于过滤干扰)
        float scale;                // 图像缩放的尺度因子
#endif

        uint8_t haar_enable;        // 使能HAAR定位
        uint8_t video_flag;         // 视频或者图片标识

#if DEBUG_RUNS
        uint8_t runs_image[2000 * 1300];
#endif
    } PlateLocation;


#ifdef __cplusplus
};
#endif

#endif  // _LOCATION_H_
