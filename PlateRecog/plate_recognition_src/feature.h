#ifndef _FEATURE_H_
#define _FEATURE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"

#define CH_0 (0)
#define CH_1 (1)
#define CH_2 (2)
#define CH_3 (3)
#define CH_4 (4)
#define CH_5 (5)
#define CH_6 (6)
#define CH_7 (7)
#define CH_8 (8)
#define CH_9 (9)
#define CH_A (10)
#define CH_B (11)
#define CH_C (12)
#define CH_D (13)
#define CH_E (14)
#define CH_F (15)
#define CH_G (16)
#define CH_H (17)
#define CH_I (18)
#define CH_J (19)
#define CH_K (20)
#define CH_L (21)
#define CH_M (22)
#define CH_N (23)
#define CH_O (24)
#define CH_P (25)
#define CH_Q (26)
#define CH_R (27)
#define CH_S (28)
#define CH_T (29)
#define CH_U (30)
#define CH_V (31)
#define CH_W (32)
#define CH_X (33)
#define CH_Y (34)
#define CH_Z (35)

    int32_t grid_feature_4x4(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

    int32_t grid_feature_pixels(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

    int32_t peripheral_direction_feature(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                         int32_t step, int32_t order_set);
    int32_t peripheral_feature(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                               int32_t step);

    int32_t peripheral_feature_x(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                 int32_t order_set);

    int32_t cross_count_feature(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

    int32_t feature_extract(feature_type *restrict feature, uint8_t *restrict bina_img,
                            int32_t img_w, int32_t img_h, int32_t character_type);

#ifdef __cplusplus
};
#endif

#endif
