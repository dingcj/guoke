#ifndef _DRAW_H_
#define _DRAW_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    void draw_plate_window(uint8_t *gray_img, int32_t img_w, int32_t img_h,
                           int32_t left, int32_t right, int32_t top, int32_t down);

#ifdef __cplusplus
};
#endif

#endif  // _DRAW_H_

