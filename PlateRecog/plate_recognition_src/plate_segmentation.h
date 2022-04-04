#ifndef _CH_SEG_H_
#define _CH_SEG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "plate_recognition.h"
#include "../portab.h"


    uint8_t character_segmentation(uint8_t *restrict src_img, int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                                   int32_t *ch_num1, uint8_t *restrict chs, uint8_t *trust, uint8_t *trust_avg,
                                   uint8_t *restrict ch_buf,
                                   SvmModel *restrict model_english,
                                   SvmModel *restrict model_numbers,
                                   SvmModel *restrict model_chinese,
                                   uint8_t *not_ch_cnt, uint8_t bkgd_color, float tan_angle, int nChNum);

    void single_character_recognition(uint8_t *restrict plate_img, int32_t plate_w, /*int32_t plate_h, */Rects pos,
                                      uint8_t *restrict chs, uint8_t *restrict trust,
                                      uint8_t *restrict ch_buf, SvmModel *model,
                                      uint8_t noise_suppress, uint8_t is_chinese, uint8_t *is_ch);

    void chinese_locating_by_least_square(Rects *restrict ch, int32_t ch_num, int32_t avg_w, int32_t avg_h,
                                          /*int32_t plate_w, */int32_t plate_h, int32_t type);

    void chinese_left_right_locating_by_projection(uint8_t *restrict gray_img, int32_t plate_w, int32_t plate_h,
                                                   Rects *restrict ch, int32_t ch_num, int32_t jing_flag);

    void character_standardization_by_regional_growth(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

#ifdef __cplusplus
};
#endif

#endif


