#ifndef _RECOGNITION_INTERFACE_H_
#define _RECOGNITION_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"

    typedef void* Reg_handle;   // 车牌字符识别句柄

#define LABEL_START_OF_CHINESE (100u)    // 省份汉字的标号是从100开始的

#define CH_XUE   (143u)    // 学
#define CH_JING  (144u)    // 警
#define CH_LING  (145u)    // 领
#define CH_GANG  (146u)    // 港
#define CH_AO    (147u)    // 澳
#define CH_TAI   (148u)    // 台
#define CH_GUA   (149u)    // 挂
#define CH_SHIY  (150u)    // 试
#define CH_CHAO  (151u)    // 超
#define CH_SHIG  (152u)    // 使

// 创建车牌字符识别句柄
    void lc_plate_recognition_create(Reg_handle *reg_handle);

// 车牌字符识别
    void lc_plate_recognition_single(Reg_handle reg_handle, uint8_t *restrict pgray_img,
                                     int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t *ch_num,
                                     uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *restrict chs_trust,
                                     uint8_t *restrict brightness,
                                     uint8_t *restrict plate_trust, uint8_t trust_thresh,
                                     uint8_t *bkgd_color, uint8_t *color_new,
                                     uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                     Rects *restrict rect_old, Rects *restrict rect_new);
    void lc_plate_recognition_double(Reg_handle reg_handle, uint8_t *restrict pgray_img, int32_t plate_h_up,
                                     int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t *ch_num,
                                     uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *restrict chs_trust,
                                     uint8_t *restrict brightness,
                                     uint8_t *restrict plate_trust, uint8_t trust_thresh,
                                     uint8_t *bkgd_color, uint8_t *color_new,
                                     uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                     Rects *restrict rect_old, Rects *restrict rect_new);
//车牌底色判别函数
//返回值： 0：蓝底 1：黄底 2：白底 3：黑底
    uint8_t find_plate_color(uint8_t *yuv_img, int32_t img_w, int32_t img_h);

// 撤销车牌字符识别句柄
    void lc_plate_recognition_destroy(Reg_handle reg_handle);


// 设置汉字(省份)识别掩膜(提高字符识别的精度) 0: 开启该汉字识别 1: 屏蔽该汉字识别
    void lc_plate_recognition_set_chinese_mask(Reg_handle reg_handle, uint8_t *chinese_mask);
    void lc_plate_recognition_get_chinese_mask(Reg_handle reg_handle, uint8_t *chinese_mask);

// 设置默认的汉字(省份)，当汉字的置信度不高时(不太肯定时)，采用默认汉字
    void lc_plate_recognition_set_chinese_default(Reg_handle reg_handle, uint8_t chinese_default);
    void lc_plate_recognition_get_chinese_default(Reg_handle reg_handle, uint8_t *chinese_default);

// 设置汉字的置信度阈值，当低于该阈值时采用默认的省份汉字
    void lc_plate_recognition_set_chinese_trust_thresh(Reg_handle reg_handle, uint8_t trust_thresh);
    void lc_plate_recognition_get_chinese_trust_thresh(Reg_handle reg_handle, uint8_t *trust_thresh);

#ifdef __cplusplus
};
#endif

#endif  // _RECOGNITION_INTERFACE_H_

