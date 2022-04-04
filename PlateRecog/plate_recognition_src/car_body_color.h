#ifndef _CAR_BODY_COLOR_H_
#define _CAR_BODY_COLOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    void car_body_color(uint8_t *restrict yuv_data, int32_t img_w, int32_t img_h,
                        Rects *rect_real, uint8_t *car_color, uint8_t *car_color_trust);
#ifdef __cplusplus
};
#endif

#endif

