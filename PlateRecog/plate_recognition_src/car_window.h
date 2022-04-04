#ifndef _CAR_WINDOW_H_
#define _CAR_WINDOW_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    int32_t car_window_area_recog(uint8_t *yuv_data, int32_t img_w, /*int32_t img_h,*/
                                  Rects *plate_real, Rects *window_rect);

#ifdef __cplusplus
};
#endif

#endif
