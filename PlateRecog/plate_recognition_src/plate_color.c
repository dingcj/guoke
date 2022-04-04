#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "../common/threshold.h"

#ifdef _TMS320C6X_
#pragma CODE_SECTION(plate_yellow_or_white, ".text:plate_yellow_or_white")
#pragma CODE_SECTION(pixel_yellow_or_white, ".text:pixel_yellow_or_white")
#pragma CODE_SECTION(yuv2hsv, ".text:yuv2hsv")
#endif

static void yuv2hsv(uint8_t y, uint8_t u, uint8_t v,
                    uint8_t *restrict h, uint8_t *restrict s, uint8_t *restrict vv)
{
    int32_t cb, cr;
    int32_t r, g, b;
    int32_t min, max, delta;
    int32_t hue;

    cb = (int32_t)u - 128;
    cr = (int32_t)v - 128;

    r = (int32_t)((int32_t)y + cr * 1.4075f);
    g = (int32_t)((int32_t)y - cb * 0.39f - cr * 0.58f);
    b = (int32_t)((int32_t)y + cb * 2.03f);

    r = CLIP3(r, 0, 255);
    g = CLIP3(g, 0, 255);
    b = CLIP3(b, 0, 255);

    min = MIN3(r, g, b);
    max = MAX3(r, g, b);

    *vv = (uint8_t)max;
    delta = max - min;

    if (delta == 0)
    {
        *s = 0;
        *h = 220;
        return;
    }

    if (max != 0)
    {
        *s = (uint8_t)(delta * 255 / max);
    }
    else
    {
        *s = 0;
    }

    if (*s == 0)
    {
        *h = 255 * 120 / 360;
    }
    else
    {
        if (r == max) // between yellow & magenta
        {
            hue = 42 * (g - b) / delta;
        }
        else if (g == max) // between cyan & yellow
        {
            hue = 84 + 42 * (b - r) / delta;
        }
        else // between magenta & cyan
        {
            hue = 168 + 42 * (r - g) / delta;
        }

        if (hue < 0)
        {
            hue += 255;
        }

        *h = (uint8_t)hue;
    }

    return;
}

static uint8_t pixel_yellow_or_white(uint8_t y, uint8_t u, uint8_t v)
{
    uint8_t h, s, vv;

    yuv2hsv(y, u, v, &h, &s, &vv);

    if (h > (uint8_t)10 && h < (uint8_t)50 && s > (uint8_t)85 && v > (uint8_t)40) // yellow
    {
        return (uint8_t)0;
    }

    if (s < (110u) && v > (80u)) // white
    {
        return (uint8_t)1;
    }

    return (uint8_t)255;
}

// 判断车牌底色是黄色还是白色
uint8_t plate_yellow_or_white(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t yellow_cnt = 0;
    int32_t white_cnt = 0;
    int32_t result = 0;
    uint8_t y, u, v;
    uint8_t color;
    uint8_t *restrict mask_img = NULL;
    uint8_t *restrict y_buf = yuv_img;
    uint8_t *restrict u_buf = yuv_img + img_w * img_h;
    uint8_t *restrict v_buf = yuv_img + img_w * img_h + (img_w * img_h >> 2);

    CHECKED_MALLOC(mask_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);

    thresholding_avg_all(yuv_img, mask_img, img_w, img_h);

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            // 分析、统计非字符区域的像素
            if (mask_img[img_w * h + w] != CHARACTER_PIXEL)
            {
                y = y_buf[img_w * h + w];
                u = u_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
                v = v_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];

                color = pixel_yellow_or_white(y, u, v);

                if (color == (uint8_t)0)
                {
                    yellow_cnt++;
                }
                else if (color == (uint8_t)1)
                {
                    white_cnt++;
                }
            }
        }
    }

    CHECKED_FREE(mask_img, sizeof(uint8_t) * img_w * img_h);

    if (yellow_cnt > white_cnt / 3)
    {
        result = 1; // 黄牌
    }
    else
    {
        result = 2; // 白底军牌
    }

    return (uint8_t)result;
}
