#ifndef _DESKEWING_H_
#define _DESKEWING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    void plate_deskewing_single(uint8_t *restrict plate_img, int32_t cmp_y0, int32_t cmp_y1,
                                int32_t plate_w, int32_t plate_h, int32_t x0, int32_t x1);
    void plate_deskewing_double(uint8_t *restrict plate_img, int32_t cmp_y0, int32_t cmp_y1,
                                int32_t plate_w, int32_t plate_h, int32_t plate_h_up, int32_t x0, int32_t x1);

#ifdef __cplusplus
};
#endif

#endif
