#include "hog_standardc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MAX_HOG_FEATURE_LEN
#define MAX_HOG_FEATURE_LEN (10000) //HOG特征
#endif

#ifndef MIN_HEAD_NUM
#define MIN_HEAD_NUM (2)    // 最小车头数
#endif

int32_t hog_detect_proc(uint8_t *src_img, int32_t img_w, int32_t img_h, 
                        Rects* plate_rect, Rects* head_rect);

#ifdef __cplusplus
};
#endif
