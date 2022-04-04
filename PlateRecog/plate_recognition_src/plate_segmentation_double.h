#ifndef _PLATE_SEGMENTATION_DOUBLE_H_
#define _PLATE_SEGMENTATION_DOUBLE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "plate_recognition.h"
#include "../portab.h"


    uint8_t character_segmentation_double_down(uint8_t *restrict src_img, int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                                               int32_t *ch_num1, uint8_t *restrict chs, uint8_t *trust, uint8_t *trust_avg,
                                               uint8_t *restrict ch_buf,
                                               SvmModel *restrict model_english,
                                               SvmModel *restrict model_numbers,
                                               SvmModel *restrict model_chinese,
                                               uint8_t *not_ch_cnt, uint8_t bkgd_color, int32_t center_rate);
    uint8_t character_segmentation_double_up(uint8_t *restrict src_img, int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                                             int32_t *ch_num1, uint8_t *restrict chs, uint8_t *trust, uint8_t *trust_avg,
                                             uint8_t *restrict ch_buf,
                                             SvmModel *restrict model_english,
                                             SvmModel *restrict model_numbers,
                                             SvmModel *restrict model_chinese,
                                             uint8_t *not_ch_cnt, uint8_t bkgd_color);


#ifdef __cplusplus
};
#endif

#endif
