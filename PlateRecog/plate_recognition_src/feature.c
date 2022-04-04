#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "feature.h"

// 提取二值图像的网格特征 水平4等分 垂直8等分 48维
#define STRIDE (4)
int32_t grid_feature_4x4(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t h, w, m, n;
    int32_t cnt;
    feature_type *restrict feat = feature;

    for (h = 0; h < (img_h / STRIDE); h++)
    {
        for (w = 0; w < (img_w / STRIDE); w++)
        {
            cnt = 0;

            for (m = h * STRIDE; m < (h + 1) * STRIDE; m++)
            {
                for (n = w * STRIDE; n < (w + 1) * STRIDE; n++)
                {
                    cnt += (bina_img[img_w * m + n] == CHARACTER_PIXEL);  // 白底黑字
                }
            }

            *feat++ = (feature_type)cnt;
        }
    }

    return (feat - feature);
}
#undef STRIDE

#define STRIDE_W (6)
#define STRIDE_H (8)
int32_t grid_feature_6x8(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t h, w, m, n;
    int32_t cnt;
    feature_type *restrict feat = feature;

    for (h = 0; h < (img_h / STRIDE_H); h++)
    {
        for (w = 0; w < (img_w / STRIDE_W); w++)
        {
            cnt = 0;

            for (m = h * STRIDE_H; m < (h + 1) * STRIDE_H; m++)
            {
                for (n = w * STRIDE_W; n < (w + 1) * STRIDE_W; n++)
                {
                    cnt += (bina_img[img_w * m + n] == CHARACTER_PIXEL);  // 白底黑字
                }
            }

            *feat++ = (feature_type)cnt;
        }
    }

    return (feat - feature);
}
#undef STRIDE_W
#undef STRIDE_H

#define STRIDE_W (3)
#define STRIDE_H (4)
int32_t grid_feature_projection(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t cnt;
    feature_type *restrict feat = feature;

    for (h = 0; h < img_h; h += STRIDE_H)
    {
        cnt = 0;

        for (w = 0; w < img_w; w++)
        {
            cnt += (bina_img[img_w * h + w] == CHARACTER_PIXEL);  // 白底黑字
        }

        *feat++ = (feature_type)cnt;
    }

    for (w = 0; w < img_w; w += STRIDE_W)
    {
        cnt = 0;

        for (h = 0; h < img_h; h++)
        {
            cnt += (bina_img[img_w * h + w] == CHARACTER_PIXEL);  // 白底黑字
        }

        *feat++ = (feature_type)cnt;
    }

    return (feat - feature);
}
#undef STRIDE_W
#undef STRIDE_H

int32_t grid_feature_pixels(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix = 0;
    feature_type *restrict feat = feature;

    for (pix = 0; pix < img_w * img_h; pix++)
    {
        *feat++ = (feature_type)((bina_img[pix] == CHARACTER_PIXEL) ? 1 : 0); // 白底黑字
    }

    return (feat - feature);
}

// 上下左右的外围特征 总共(w + h) * 2 = (24 + 32) * 2 = 112维
// 左右外围取(0 - img_w - 1)，当该行没有字符点时取img_w
// 上下外围取(0 - img_h - 1)，当该列没有字符点时取img_h
int32_t peripheral_feature(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                           int32_t step)
{
    int32_t w, h;
    feature_type *restrict feat = feature;

    // 左外围
    for (h = 0; h < img_h; h += step)
    {
        for (w = 0; w < img_w; w++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                break;
            }
        }

        *feat++ = (feature_type)w;
    }

    // 右外围
    for (h = 0; h < img_h; h += step)
    {
        for (w = img_w - 1; w >= 0; w--)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                break;
            }
        }

        *feat++ = (feature_type)(img_w - 1 - w);
    }

    // 上外围
    for (w = 0; w < img_w; w += step)
    {
        for (h = 0; h < img_h; h++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                break;
            }
        }

        *feat++ = (feature_type)h;
    }

    // 下外围
    for (w = 0; w < img_w; w += step)
    {
        for (h = img_h - 1; h >= 0; h--)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                break;
            }
        }

        *feat++ = (feature_type)(img_h - 1 - h);
    }

    return (feat - feature);
}

int32_t get_run_length(feature_type *restrict feature, uint8_t *restrict bina_img,
                       int32_t img_w, int32_t img_h, int32_t x, int32_t y)
{
    int w, h;
    int L1 = 1;
    int L2 = 1;
    int L3 = 1;
    int L4 = 1;
    int L5 = 1;
    int L6 = 1;
    int L7 = 1;
    int L8 = 1;

    // 水平方向
    w = x + 1;
    h = y;

    while ((w < img_w) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L1++;
        w++;
    }

    w = x - 1;
    h = y;

    while ((w > 0) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L2++;
        w--;
    }

    // 垂直方向
    w = x;
    h = y - 1;

    while ((h > 0) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L3++;
        h--;
    }

    w = x;
    h = y + 1;

    while ((h < img_h) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L4++;
        h++;
    }

    // 正45度方向
    w = x + 1;
    h = y - 1;

    while ((w < img_w) && (h > 0) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L5++;
        w++;
        h--;
    }

    w = x - 1;
    h = y + 1;

    while ((w > 0) && (h < img_h) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L6++;
        w--;
        h++;
    }

    // 负45度方向
    w = x + 1;
    h = y + 1;

    while ((w < img_w) && (h < img_h) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L7++;
        w++;
        h++;
    }

    w = x - 1;
    h = y - 1;

    while ((w > 0) && (h > 0) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
    {
        L8++;
        w--;
        h--;
    }

    *feature++ = (feature_type)(L1 / 4);
    *feature++ = (feature_type)(L2 / 4);
    *feature++ = (feature_type)(L3 / 4);
    *feature++ = (feature_type)(L4 / 4);
    *feature++ = (feature_type)(L5 / 4);
    *feature++ = (feature_type)(L6 / 4);
    *feature++ = (feature_type)(L7 / 4);
    *feature++ = (feature_type)(L8 / 4);

    return 8;
}

int32_t peripheral_direction_feature(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                     int32_t step, int32_t order_set)
{
    int32_t w, h;
    feature_type *restrict feat = feature;
    int32_t order_idx = 0;

    // 左外围
    for (h = 0; h < img_h; h += step)
    {
        order_idx = 0;

        w = 0;

        if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
        {
            order_idx++;
        }

        for (w = 1; w < img_w; w++)
        {
            if ((bina_img[img_w * h + w - 1] != CHARACTER_PIXEL) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

//      *feat++ = w;
        get_run_length(feat, bina_img, img_w, img_h, w, h);
        feat += 8;
    }

    // 右外围
    for (h = 0; h < img_h; h += step)
    {
        order_idx = 0;

        w = img_w - 1;

        if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
        {
            order_idx++;
        }

        for (w = img_w - 2; w >= 0; w--)
        {
            if ((bina_img[img_w * h + w + 1] != CHARACTER_PIXEL) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

//      *feat++ = img_w - 1 - w;
        get_run_length(feat, bina_img, img_w, img_h, w, h);
        feat += 8;
    }

    // 上外围
    for (w = 0; w < img_w; w += step)
    {
        order_idx = 0;

        h = 0;

        if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
        {
            order_idx++;
        }

        for (h = 1; h < img_h; h++)
        {
            if ((bina_img[img_w * (h - 1) + w] != CHARACTER_PIXEL) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

//        *feat++ = h;
        get_run_length(feat, bina_img, img_w, img_h, w, h);
        feat += 8;
    }

    // 下外围
    for (w = 0; w < img_w; w += step)
    {
        order_idx = 0;

        h = img_h - 1;

        if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
        {
            order_idx++;
        }

        for (h = img_h - 2; h >= 0; h--)
        {
            if ((bina_img[img_w * (h + 1) + w] != CHARACTER_PIXEL) && (bina_img[img_w * h + w] == CHARACTER_PIXEL))
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

//        *feat++ = img_h - 1 - h;
        get_run_length(feat, bina_img, img_w, img_h, w, h);
        feat += 8;
    }

    return (feat - feature);
}

#define STEP (1)
int32_t peripheral_direction_feature_x(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                       int32_t order_set)
{
    int32_t x = 0;
    int32_t b = 8;
    int32_t y0 = 0;
    int32_t y1 = 0;
    int32_t pix0 = 0;
    int32_t line_st = 0;
    int32_t line_ed = 0;
    int32_t order_idx = 0;
    feature_type *restrict feat = feature;
#if DEBUG_FEARTURE
    FILE *txt = fopen("cross_count_feature.txt", "w");

    if (txt == NULL)
    {
        printf("can't open output file for cross_count_feature()!\n");

        while (1);
    }

#endif

    // 正45度方向 y = -x + b (0 <= x < img_w, 0 < b < img_w + img_h)
    for (b = 8; b < img_w + img_h - 8; b += STEP)
    {
        for (x = 0; x < img_w - 1; x++)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_st = x;
                break;
            }
        }

        for (x = img_w - 2; x >= 0; x--)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_ed = x;
                break;
            }
        }

#if DEBUG_FEARTURE
        fprintf(txt, "\nb = %2d line_st = %2d, line_ed = %2d\n", b, line_st, line_ed);
#endif

        *feat = 0;
        order_idx = 0;

        for (x = line_st; x <= line_ed; x++)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = bina_img[img_w * y0 + (x + 0)] > (uint8_t)0;
//            pix1 = bina_img[img_w * y1 + (x + 1)] > (uint8_t)0;

            if (pix0 == 0)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

        get_run_length(feat, bina_img, img_w, img_h, x, y0);
        feat += 8;

        *feat = 0;
        order_idx = 0;

        for (x = line_ed; x >= line_st; x--)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = bina_img[img_w * y0 + (x + 0)] > (uint8_t)0;
//            pix1 = bina_img[img_w * y1 + (x + 1)] > (uint8_t)0;

            if (pix0 == 0)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

        get_run_length(feat, bina_img, img_w, img_h, x, y0);
        feat += 8;
    }

    // 负45度方向 y = x + b (0 <= x < img_w, -img_w < b < img_h)
    for (b = -img_w + 8; b < img_h - 8; b += STEP)
    {
        for (x = 0; x < img_w - 1; x++)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_st = x;
                break;
            }
        }

        for (x = img_w - 2; x >= 0; x--)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_ed = x;
                break;
            }
        }

#if DEBUG_FEARTURE
        fprintf(txt, "\nb = %2d line_st = %2d, line_ed = %2d\n", b, line_st, line_ed);
#endif

        *feat = 0;
        order_idx = 0;

        for (x = line_st; x <= line_ed; x++)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = bina_img[img_w * y0 + (x + 0)] > (uint8_t)0;
//            pix1 = bina_img[img_w * y1 + (x + 1)] > (uint8_t)0;

            if (pix0 == 0)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

        get_run_length(feat, bina_img, img_w, img_h, x, y0);
        feat += 8;

        *feat = 0;
        order_idx = 0;

        for (x = line_ed; x >= line_st; x--)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = bina_img[img_w * y0 + (x + 0)] > (uint8_t)0;
//            pix1 = bina_img[img_w * y1 + (x + 1)] > (uint8_t)0;

            if (pix0 == 0)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                break;
            }
        }

        get_run_length(feat, bina_img, img_w, img_h, x, y0);
        feat += 8;
    }

#if DEBUG_FEARTURE
    fclose(txt);
#endif

    return (feat - feature);
}
#undef STEP

#define STEP (1)
int32_t peripheral_feature_x(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                             int32_t order_set)
{
    int32_t x = 0;
    int32_t b = 8;
    int32_t y0 = 0;
    int32_t y1 = 0;
    int32_t pix0 = 0;
    int32_t pix1 = 0;
    int32_t line_st = 0;
    int32_t line_ed = 0;
    int32_t order_idx = 0;
    feature_type *restrict feat = feature;
#if DEBUG_FEARTURE
    FILE *txt = fopen("cross_count_feature.txt", "w");

    if (txt == NULL)
    {
        printf("can't open output file for cross_count_feature()!\n");

        while (1);
    }

#endif

    // 正45度方向 y = -x + b (0 <= x < img_w, 0 < b < img_w + img_h)
    for (b = 8; b < img_w + img_h - 8; b += STEP)
    {
        for (x = 0; x < img_w - 1; x++)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_st = x;
                break;
            }
        }

        for (x = img_w - 2; x >= 0; x--)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_ed = x;
                break;
            }
        }

#if DEBUG_FEARTURE
        fprintf(txt, "\nb = %2d line_st = %2d, line_ed = %2d\n", b, line_st, line_ed);
#endif

        *feat = 0;
        order_idx = 0;

        for (x = line_st; x <= line_ed; x++)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = (int32_t)(bina_img[img_w * y0 + (x + 0)] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * y1 + (x + 1)] > (uint8_t)0);

            if (pix0 != pix1)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                *feat = (uint8_t)sqrt(x * x + (img_h - y0) * (img_h - y0));
                break;
            }
        }

        feat++;

        *feat = 0;
        order_idx = 0;

        for (x = line_ed; x >= line_st; x--)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = (int32_t)(bina_img[img_w * y0 + (x + 0)] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * y1 + (x + 1)] > (uint8_t)0);

            if (pix0 != pix1)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                *feat = (uint8_t)sqrt((img_w - x) * (img_w - x) + y0 * y0);
                break;
            }
        }

        feat++;
    }

    // 负45度方向 y = x + b (0 <= x < img_w, -img_w < b < img_h)
    for (b = -img_w + 8; b < img_h - 8; b += STEP)
    {
        for (x = 0; x < img_w - 1; x++)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_st = x;
                break;
            }
        }

        for (x = img_w - 2; x >= 0; x--)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_ed = x;
                break;
            }
        }

#if DEBUG_FEARTURE
        fprintf(txt, "\nb = %2d line_st = %2d, line_ed = %2d\n", b, line_st, line_ed);
#endif

        *feat = 0;
        order_idx = 0;

        for (x = line_st; x <= line_ed; x++)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = (int32_t)(bina_img[img_w * y0 + (x + 0)] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * y1 + (x + 1)] > (uint8_t)0);

            if (pix0 != pix1)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                *feat = (uint8_t)sqrt(x * x + y0 * y0);
                break;
            }
        }

        feat++;

        *feat = 0;
        order_idx = 0;

        for (x = line_ed; x >= line_st; x--)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = (int32_t)(bina_img[img_w * y0 + (x + 0)] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * y1 + (x + 1)] > (uint8_t)0);

            if (pix0 != pix1)
            {
                order_idx++;
            }

            if (order_idx == order_set)
            {
                *feat = (uint8_t)sqrt((img_w - x) * (img_w - x) + (img_h - y0) * (img_h - y0));
                break;
            }
        }

        feat++;
    }

#if DEBUG_FEARTURE
    fclose(txt);
#endif

    return (feat - feature);
}
#undef STEP

// 笔划密度特征
#define STEP (2)
#define DEBUG_FEARTURE (0)
int32_t cross_count_feature(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t x = 0;
    int32_t b = 8;
    int32_t y0 = 0;
    int32_t y1 = 0;
    int32_t pix0 = 0;
    int32_t pix1 = 0;
    int32_t len = 0;
    int32_t sum = 0;
    int32_t line_st = 0;
    int32_t line_ed = 0;
    feature_type *restrict feat = feature;
    uint8_t *cc = NULL;

    CHECKED_MALLOC(cc, sizeof(uint8_t) * img_h * 3, CACHE_SIZE);

#if DEBUG_FEARTURE
    FILE *txt = fopen("cross_count_feature.txt", "w");

    if (txt == NULL)
    {
        printf("can't open output file for cross_count_feature()!\n");

        while (1);
    }

#endif

    // 水平方向 32 / 2 = 16
    for (h = 0; h < img_h; h += STEP)
    {
        cc[0] = (uint8_t)0;

        for (w = 0; w < img_w - 1; w++)
        {
            pix0 = (int32_t)(bina_img[img_w * h + w + 0] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * h + w + 1] > (uint8_t)0);

            if (pix0 != pix1)
            {
                cc[w + 1] = (uint8_t)10;
            }
            else
            {
                cc[w + 1] = (uint8_t)0;
            }
        }

        *feat = 0;

        for (w = 2; w < img_w - 2; w++)
        {
            if (cc[w])
            {
                sum = (cc[w - 2] << 1) + (cc[w - 1] << 2) + (cc[w] << 3) + (cc[w + 1] << 2) + (cc[w + 2] << 1);
                *feat += (sum / 20);
            }
        }

        feat++;
    }

    // 垂直方向 24 / 2 = 12
    for (w = 0; w < img_w; w += STEP)
    {
        cc[0] = 0u;

        for (h = 0; h < img_h - 1; h++)
        {
            pix0 = (int32_t)(bina_img[img_w * (h + 0) + w] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * (h + 1) + w] > (uint8_t)0);

            if (pix0 != pix1)
            {
                cc[h + 1] = (uint8_t)10;
            }
            else
            {
                cc[h + 1] = (uint8_t)0;
            }
        }

        *feat = 0;

        for (h = 2; h < img_h - 2; h++)
        {
            if (cc[h])
            {
                sum = (cc[h - 2] << 1) + (cc[h - 1] << 2) + (cc[h] << 3) + (cc[h + 1] << 2) + (cc[h + 2] << 1);
                *feat += (sum / 20);
            }
        }

        feat++;
    }

    // 负45度方向 y = -x + b (0 <= x < img_w, 0 < b < img_w + img_h)
    for (b = 8; b < img_w + img_h - 8; b += STEP)
    {
        for (x = 0; x < img_w - 1; x++)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_st = x;
                break;
            }
        }

        for (x = img_w - 2; x >= 0; x--)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_ed = x;
                break;
            }
        }

#if DEBUG_FEARTURE
        fprintf(txt, "\nb = %2d line_st = %2d, line_ed = %2d\n", b, line_st, line_ed);
#endif

        cc[0] = (uint8_t)0;
        len = line_ed - line_st + 1;

        for (x = line_st; x <= line_ed; x++)
        {
            y0 = b - (x + 0);
            y1 = b - (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = (int32_t)(bina_img[img_w * y0 + (x + 0)] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * y1 + (x + 1)] > (uint8_t)0);

            if (pix0 != pix1)
            {
                cc[x - line_st + 1] = (uint8_t)10;
            }
            else
            {
                cc[x - line_st + 1] = (uint8_t)0;
            }
        }

        *feat = 0;

        for (h = 2; h < len - 2; h++)
        {
            sum = (cc[h - 2] << 1) + (cc[h - 1] << 2) + (cc[h] << 3) + (cc[h + 1] << 2) + (cc[h + 2] << 1);
            *feat += (sum / 20);
        }

        feat++;
    }

    // 正45度方向 y = x + b (0 <= x < img_w, -img_w < b < img_h)
    for (b = -img_w + 8; b < img_h - 8; b += STEP)
    {
        for (x = 0; x < img_w - 1; x++)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_st = x;
                break;
            }
        }

        for (x = img_w - 2; x >= 0; x--)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

            if ((y0 >= 0) && (y0 < img_h) && (y1 >= 0) && (y1 < img_h))
            {
                line_ed = x;
                break;
            }
        }

#if DEBUG_FEARTURE
        fprintf(txt, "\nb = %2d line_st = %2d, line_ed = %2d\n", b, line_st, line_ed);
#endif

        cc[0] = (uint8_t)0;
        len = line_ed - line_st + 1;

        for (x = line_st; x <= line_ed; x++)
        {
            y0 = b + (x + 0);
            y1 = b + (x + 1);

#if DEBUG_FEARTURE
            fprintf(txt, "x+0 = %2d  y0 = %2d  ", x + 0, y0);
            fprintf(txt, "x+1 = %2d  y1 = %2d\n", x + 1, y1);
#endif
            pix0 = (int32_t)(bina_img[img_w * y0 + (x + 0)] > (uint8_t)0);
            pix1 = (int32_t)(bina_img[img_w * y1 + (x + 1)] > (uint8_t)0);

            if (pix0 != pix1)
            {
                cc[x - line_st + 1] = (uint8_t)10;
            }
            else
            {
                cc[x - line_st + 1] = (uint8_t)0;
            }
        }

        *feat = (feature_type)0;

        for (h = 2; h < len - 2; h++)
        {
            sum = ((int32_t)cc[h - 2] << 1) + ((int32_t)cc[h - 1] << 2) + ((int32_t)cc[h] << 3)
                  + ((int32_t)cc[h + 1] << 2) + ((int32_t)cc[h + 2] << 1);

            *feat += (feature_type)(sum / 20);
        }

        feat++;
    }

    CHECKED_FREE(cc, sizeof(uint8_t) * img_h * 3);

#if DEBUG_FEARTURE
    fclose(txt);
#endif

    return 68;
}
#undef STEP
#undef DEBUG_FEARTURE

int32_t grid_feature_8andB0(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t st = 0;
    int32_t ed = 0;
    int32_t x0 = 0;
//  int32_t x1, y1;
    int32_t longest_vertical_line_x = img_w - 1;
    int32_t longest_vertical_line = 0;
    int32_t vertical_line_y0 = 0;
    int32_t vertical_line_y1 = img_h - 1;
    int32_t cnt = 0;
    feature_type *restrict feat = feature;

    // 寻找图像左半部分最长的一竖对应的位置
    longest_vertical_line_x = 0;
    longest_vertical_line = 0;

    for (w = 0; w < img_w / 2; w++)
    {
        for (h = 0; h < img_h; h++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                st = h;
                ed = img_h - 1;

                for (; h < img_h; h++)
                {
                    if (bina_img[img_w * h + w] != CHARACTER_PIXEL)
                    {
                        ed = h - 1;
                        break;
                    }
                }

                if (ed - st + 1 > longest_vertical_line)
                {
                    longest_vertical_line = ed - st + 1;
                    longest_vertical_line_x = w;
                    vertical_line_y0 = st;
                    vertical_line_y1 = ed;
                }
            }
        }
    }

    cnt = 0;    // 存放拐点数目
    x0 = longest_vertical_line_x;
//    y0 = vertical_line_y0;

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < x0; w++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                cnt++;
            }
        }
    }

    *feat++ = (feature_type)x0;
    *feat++ = (feature_type)longest_vertical_line;
    *feat++ = (feature_type)vertical_line_y0;
    *feat++ = (feature_type)vertical_line_y1;
    *feat++ = (feature_type)cnt;

    longest_vertical_line_x = img_w - 1;
    longest_vertical_line = 0;

    for (w = img_w - 1; w > img_w / 2; w--)
    {
        for (h = 0; h < img_h; h++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                st = h;
                ed = img_h - 1;

                for (; h < img_h; h++)
                {
                    if (bina_img[img_w * h + w] != (uint8_t)0)
                    {
                        ed = h - 1;
                        break;
                    }
                }

                if (ed - st + 1 > longest_vertical_line)
                {
                    longest_vertical_line = ed - st + 1;
                    longest_vertical_line_x = w;
                    vertical_line_y0 = st;
                    vertical_line_y1 = ed;
                }
            }
        }
    }

    cnt = 0;    // 存放拐点数目
    x0 = longest_vertical_line_x;
//    y0 = vertical_line_y0;

    for (h = 0; h < img_h; h++)
    {
        for (w = x0; w < img_w; w++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                cnt++;
            }
        }
    }

    *feat++ = (feature_type)x0;
    *feat++ = (feature_type)longest_vertical_line;
    *feat++ = (feature_type)vertical_line_y0;
    *feat++ = (feature_type)vertical_line_y1;
    *feat++ = (feature_type)cnt;

    return (feat - feature);
}

int32_t grid_feature_8andB(feature_type *restrict feature, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t cnt;
    feature_type *restrict feat = feature;

//    for (h = 0; h < img_h; h++)
//    {
//        cnt = 0;
//        for (w = 0; w < img_w; w++)
//        {
//            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
//            {
//                cnt++;
//            }
//        }
//        *feat++ = (feature_type)cnt;
//    }

    for (w = 0; w < 4; w++)
    {
        cnt = 0;

        for (h = 0; h < img_h; h++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                cnt++;
            }
        }

        *feat++ = (feature_type)cnt;
    }

    for (w = img_w - 5; w < img_w - 1; w++)
    {
        cnt = 0;

        for (h = 0; h < img_h; h++)
        {
            if (bina_img[img_w * h + w] == CHARACTER_PIXEL)
            {
                cnt++;
            }
        }

        *feat++ = (feature_type)cnt;
    }

    return (feat - feature);
}

int32_t feature_extract(feature_type *restrict feature, uint8_t *restrict bina_img,
                        int32_t img_w, int32_t img_h, int32_t character_type)
{
//    int32_t area = img_w * img_h;
    int32_t count_total = 0;
    feature_type *restrict feat = feature;

    // 省份汉字和军牌汉字 48 + 112 + 68 = 228
    // （包括学、警、领、港、澳、台、挂、试、超、使）
    if (character_type == 0 || character_type == 1)
    {
        count_total += grid_feature_4x4(feat, bina_img, img_w, img_h);
        count_total += peripheral_feature(feat + count_total, bina_img, img_w, img_h, 1);
        count_total += cross_count_feature(feat + count_total, bina_img, img_w, img_h);
    }
    else if (character_type == 2) // 除'O'和'P'之外的24个英文字母（A~Z）
    {
        count_total += grid_feature_4x4(feat, bina_img, img_w, img_h);
        count_total += grid_feature_projection(feat + count_total, bina_img, img_w, img_h);
    }
    else if (character_type == 3) // 字母、数字 48
    {
        count_total += grid_feature_4x4(feat, bina_img, img_w, img_h);
//        count_total += grid_feature_projection(feat + count_total, bina_img, img_w, img_h);
    }
    else if (character_type == 4) // 字母、数字以及最后一个汉字字符（学警领港澳台挂试超使）
    {
        count_total += grid_feature_4x4(feat, bina_img, img_w, img_h);
//        count_total += grid_feature_projection(feat + count_total, bina_img, img_w, img_h);
    }
    else if (character_type == 5) // 相似字符 48 + 56 + 160 = 264
    {
        count_total += grid_feature_4x4(feat, bina_img, img_w, img_h);
        count_total += peripheral_feature(feat + count_total, bina_img, img_w, img_h, 2);
        count_total += peripheral_feature_x(feat + count_total, bina_img, img_w, img_h, 2);
    }
    else if (character_type == 6) // 相似字符 24 * 32 = 768
    {
        count_total += grid_feature_pixels(feat + count_total, bina_img, img_w, img_h);
    }
    else if (character_type == 7) // 相似字符 112 * 8 = 896
    {
        count_total += peripheral_direction_feature(feat + count_total, bina_img, img_w, img_h, 1, 1);
//      count_total += peripheral_direction_feature_x(feat + count_total, bina_img, img_w, img_h, 1);
//      count_total += grid_feature_8andB0(feat + count_total, bina_img, img_w, img_h);
//      count_total += grid_feature_8andB(feat + count_total, bina_img, img_w, img_h);
//      count_total += grid_feature_projection(feat + count_total, bina_img, img_w, img_h);
    }

    return count_total;
}
