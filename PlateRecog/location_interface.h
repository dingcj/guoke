#ifndef _LOCATION_INTERFACE_H_
#define _LOCATION_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"

typedef void* Loc_handle;   // 车牌定位句柄

#define USE_DOUBLE_PLATE_RECOG  1   // 双层牌识别开关，1: 进行双层牌识别 0: 关闭双层牌识别
                                    // 户线停车场场景，暂时关闭双层牌识别

#define RECT_MAX (80)       // 车牌的最大数目, 高清视频里车牌候选区域较多, 因此将上限调整为80

    typedef struct
    {
        int32_t cand_num;               // 车牌候选区域的数目
        int32_t real_num;               // 实际检测车牌的数目
        Rects rect_cand[RECT_MAX];      // 车牌候选区域
        Rects rect_real[RECT_MAX];      // 实际检测的车牌区域
        uint8_t plate_double_flag[RECT_MAX]; // 双层牌标志：0:单层牌 1:双层牌
        Rects rect_double_up[RECT_MAX];      // 双层牌上层区域，单层牌无效
        uint8_t ch_buf[RECT_MAX][24 * 32 * 10]; // 车牌字符图像
        uint8_t ch_num[RECT_MAX];       //识别出来的车牌字符数目
        uint8_t chs[RECT_MAX][7 + 1];   // 识别出来的车牌字符
        uint8_t chs_trust[RECT_MAX][8]; // 单个车牌字符的置信度，由7个调整为8个，以满足绿牌
        uint8_t trust[RECT_MAX];        // 整个车牌字符的置信度
        uint8_t color[RECT_MAX];        // 车牌颜色
        uint8_t brightness[RECT_MAX];   // 车牌亮度值1 ~ 255
        uint8_t car_color[RECT_MAX];    // 车身颜色 0:白色 1:银灰 2:黄色 3:粉色 4:红色 5:紫色 6:绿色 7:蓝色 8:棕色 9:黑色 99:未知 255:未识别
        uint8_t car_color_trust[RECT_MAX];  //车身颜色置信度
        uint8_t save_flag[RECT_MAX];    // 当前帧检测得到的车牌是否需要用户保存(在视频流中提取车牌时，同一个车牌只提取一次)
        // 0:不需保存 1:需要保存
    } LocationInfo;

// 创建车牌定位句柄
    void lc_plate_location_create(Loc_handle *loc_handle, int32_t img_w, int32_t img_h);

// 图像大小改变后重新申请内存
    void lc_plate_location_recreate(Loc_handle loc_handle, int32_t img_w, int32_t img_h);

// 车牌定位
    void lc_plate_location(Loc_handle loc_handle, uint8_t *restrict yuv_img, LocationInfo *plate);

// 撤销车牌定位句柄
void lc_plate_location_destroy(Loc_handle loc_handle, int32_t img_w, int32_t img_h);


// 车牌定位默认参数设置
    void lc_plate_location_param_default(Loc_handle loc_handle);

// 车牌定位参数设置
    void lc_plate_location_set_plate_width(Loc_handle loc_handle, int32_t plate_w_min, int32_t plate_w_max);
    void lc_plate_location_get_plate_width(Loc_handle loc_handle, int32_t *plate_w_min, int32_t *plate_w_max);

    void lc_plate_location_set_rect_detect(Loc_handle loc_handle, Rects rect_detect);
    void lc_plate_location_get_rect_detect(Loc_handle loc_handle, Rects *rect_detect);

    void lc_plate_location_get_video_flag(Loc_handle loc_handle, uint8_t *video_flag);
    void lc_plate_location_set_video_flag(Loc_handle loc_handle, uint8_t video_flag);

#ifdef __cplusplus
};
#endif

#endif  // _LOCATION_INTERFACE_H_

