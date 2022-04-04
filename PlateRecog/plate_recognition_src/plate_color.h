#ifndef _PLATE_COLOR_H_
#define _PLATE_COLOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

// ³µÅÆµ×É«ÅÐ±ðº¯Êý
// ·µ»ØÖµ£º
// 0£ºÀ¶µ×
// 1£º»Æµ×
// 2£º°×µ×
// 3£ººÚµ×
    uint8_t plate_yellow_or_white(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h);


#ifdef __cplusplus
};
#endif

#endif

