#ifndef _IMAGE_H_
#define _IMAGE_H_

#define LABEL_MAX   (150)

#include "../portab.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // 采样方式实现灰度图像缩放(仅针对灰度图像)
    void image_resize_using_interpolation(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                                          int32_t src_w, int32_t src_h,
                                          int32_t dst_w, int32_t dst_h);

// 双线性插值方式实现图像缩放(针对灰度图像和RGB彩色图像)
    void multi_channels_image_resize(uint8_t *restrict src_img, uint8_t *restrict dst_img, int32_t nchannels,
                                     int32_t src_w, int32_t src_h,
                                     int32_t dst_w, int32_t dst_h);

// 双线性插值方式实现图像缩放(针对灰度图像)
    void gray_image_resize_linear(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                                  int32_t src_w, int32_t src_h,
                                  int32_t dst_w, int32_t dst_h);

    void gray_image_resize_linear_for_center(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                                             int32_t src_w, int32_t src_h,
                                             int32_t dst_w, int32_t dst_h);

// 判断两个矩形框是否相交
    int32_t two_rectanges_intersect_or_not(Rects rc_src, Rects rc_dst);

    uint8_t is_yellow_or_white(uint8_t y, uint8_t u, uint8_t v);

#ifdef WIN32
    void write_pgm(unsigned char *img, int img_w, int img_h, int id);
#endif

#ifdef __cplusplus
};
#endif

#endif //  _IMAGEPROC_H_


