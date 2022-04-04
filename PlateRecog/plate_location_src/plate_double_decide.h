#ifndef _PLATE_DOUBLE_DECIDE_H_
#define _PLATE_DOUBLE_DECIDE_H_

#ifdef __cplusplus
extern "C"
{
#endif

//#include "../portab.h"

    void plate_double_decide_all_candidate(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                           Rects *restrict rect_real, int32_t real_num, uint8_t *restrict color,
                                           uint8_t *restrict plate_double_flag, Rects *restrict rect_double_up);

#ifdef __cplusplus
};
#endif

#endif  // _DOUBLE_DECIDE_H_
