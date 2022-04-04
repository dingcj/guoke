#ifndef _PLATE_MASK_H_
#define _PLATE_MASK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    void remove_non_plate_region(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                                 Rects *restrict rect_cand, int32_t *rect_num);

// 精确定位候选区域，并根据车牌的大致尺寸排除一部分候选区域
    void plate_locating_for_all_candidates(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                                           Rects *restrict rect_cand, int32_t cand_num,
                                           Rects *restrict rect_real, int32_t *restrict real_num,
                                           uint8_t *restrict color);

#ifdef __cplusplus
};
#endif

#endif
