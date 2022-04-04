/********************************************************************
FileName:       draw.c
Description:    完成矩形框和车牌信息的叠加

********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <assert.h>
#include "draw.h"
#include "../common/image.h"

#ifdef _TMS320C6X_
#pragma CODE_SECTION(draw_plate_window, ".text:draw_plate_window")
#endif

#define RGB2YUV(r, g, b, y, u, v)   \
    y = (( (9798) * (r) + (19235) * (g) + (3736) * (b)) >> 15);       \
    u = (((-4784) * (r) -  (9437) * (g) + (14221) * (b)) >> 15) + 128; \
    v = (((20218) * (r) - (16941) * (g) -  (3277) * (b)) >> 15) + 128; \
    y = y < 0 ? 0 : y;  \
    u = u < 0 ? 0 : u;  \
    v = v < 0 ? 0 : v;  \
    y = y > 255 ? 255 : y;  \
    u = u > 255 ? 255 : u;  \
    v = v > 255 ? 255 : v

void draw_plate_window(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                       int32_t left, int32_t right, int32_t top, int32_t down)
{
    int32_t m;
    int32_t pix_y;
    int32_t pix_u;
    int32_t pix_v;
    int32_t wid_uv = img_w / 2;
    uint8_t *restrict img_y = gray_img;
    uint8_t *restrict img_u = gray_img + img_w * img_h;
    uint8_t *restrict img_v = gray_img + img_w * img_h * 5 / 4;

    if (right - left <= 10) // 车牌宽度不可能小于10
    {
        return;
    }

    left  = MAX2(left, 0);
    right = MIN2(right, img_w - 1);
    top   = MAX2(top, 0);
    down  = MIN2(down, img_h - 1);

    RGB2YUV(255, 0, 0, pix_y, pix_u, pix_v);

    memset(img_y +  top * img_w + left, pix_y, right - left);
    memset(img_y + down * img_w + left, pix_y, right - left);

    for (m = top; m <= down; m++)
    {
        img_y[m * img_w +  left] = (uint8_t)pix_y;
        img_y[m * img_w + right] = (uint8_t)pix_y;
    }

    left /= 2;
    right /= 2;
    top  /= 2;
    down /= 2;

    memset(img_u +  top * wid_uv + left, pix_u, right - left);
    memset(img_v +  top * wid_uv + left, pix_v, right - left);

    memset(img_u + down * wid_uv + left, pix_u, right - left);
    memset(img_v + down * wid_uv + left, pix_v, right - left);

    for (m = top; m <= down; m++)
    {
        img_u[m * wid_uv +  left] = (uint8_t)pix_u;
        img_v[m * wid_uv +  left] = (uint8_t)pix_v;
        img_u[m * wid_uv + right] = (uint8_t)pix_u;
        img_v[m * wid_uv + right] = (uint8_t)pix_v;
    }

    return;
}
