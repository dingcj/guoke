#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "image.h"

#define NSHIFT 15

/********************************************************************
Function:       image_resize_using_interpolation
Description:    采样方式实现灰度图像缩放(灰度图像和二值图像都可以适用)

  1.  Param:
    src_img:    源图像
    dst_img:    目标图像
    src_w：     源图像宽度
    src_h：     源图像高度
    dst_w:      目标图像宽度
    dst_h:      目标图像高度

    2.  Return:
    void
********************************************************************/
#define SHIFT_NUM (16)
void image_resize_using_interpolation(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                                      int32_t src_w, int32_t src_h,
                                      int32_t dst_w, int32_t dst_h)
{
    int32_t w, h;
    int32_t kx, ky;
    int32_t x16, y16;
    uint8_t *restrict p_src = src_img;
    uint8_t *restrict p_dst = dst_img;

#if 0
    kx = ((src_w - 1) << SHIFT_NUM) / dst_w;
    ky = ((src_h - 1) << SHIFT_NUM) / dst_h;
#else
    kx = (src_w << SHIFT_NUM) / dst_w;
    ky = (src_h << SHIFT_NUM) / dst_h;
#endif

    y16 = 0;

    for (h = 0; h < dst_h; h++)
    {
        x16 = 0;
        p_src = src_img + src_w * (y16 >> SHIFT_NUM);

        for (w = 0; w < dst_w; w++)
        {
            p_dst[w] = p_src[(x16 >> SHIFT_NUM)];
            x16 += kx;
        }

        y16 += ky;
        p_dst += dst_w;
    }

    return;
}
#undef SHIFT_NUM

#if 0
static void gray_image_resize_linear(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                                     int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h)
{
    int32_t A, B, C, D, x, y, index, gray;
    float x_ratio = ((float)(src_w - 1)) / dst_w;
    float y_ratio = ((float)(src_h - 1)) / dst_h;
    float x_diff, y_diff;
    int32_t offset = 0;
    int32_t h = 0;
    int32_t w = 0;

    for (h = 0; h < dst_h; h++)
    {
        for (w = 0; w < dst_w; w++)
        {
            x = (int32_t)(x_ratio * w);
            y = (int32_t)(y_ratio * h);

            x_diff = (x_ratio * w) - x;
            y_diff = (y_ratio * h) - y;

            index = y * src_w + x;

            A = src_img[index + 0];
            B = src_img[index + 1];
            C = src_img[index + src_w + 0];
            D = src_img[index + src_w + 1];

            // Y = A(1-src_w)(1-src_h) + B(src_w)(1-src_h) + C(src_h)(1-src_w) + Dwh
            gray = (int32_t)(A * (1 - x_diff) * (1 - y_diff) +  B * (x_diff) * (1 - y_diff) +
                             C * (y_diff) * (1 - x_diff) +  D * (x_diff * y_diff));

            dst_img[offset++] = (uint8_t)gray;
        }
    }

    return;
}
#endif

/********************************************************************
Function:       gray_image_resize_linear
Description:    双线性插值方式实现图像缩放(针对灰度图像)

  1.  Param:
    src_img:    源图像
    dst_img:    目标图像
    src_w：     源图像宽度
    src_h：     源图像高度
    dst_w:      目标图像宽度
    dst_h:      目标图像高度

    2.  Return:
    void
********************************************************************/
void gray_image_resize_linear(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                              int32_t src_w, int32_t src_h,
                              int32_t dst_w, int32_t dst_h)
{
    int32_t w, h;
    int32_t tmp;
    int32_t ix, iy;
    int32_t sum;
    int32_t kx, ky;
    int32_t fx, fy, fxy;
    uint8_t *restrict p_src = src_img;
    uint8_t *restrict p_dst = dst_img;

#if 1
    kx = ((src_w - 1) << NSHIFT) / dst_w;
    ky = ((src_h - 1) << NSHIFT) / dst_h;
#else
    kx = (src_w << NSHIFT) / dst_w;
    ky = (src_h << NSHIFT) / dst_h;
#endif

    for (h = 0; h < dst_h; h++)
    {
        tmp = h * ky;
        iy = tmp >> NSHIFT;
        fy = tmp - (iy << NSHIFT);

        for (w = 0; w < dst_w; w++)
        {
            tmp = w * kx;
            ix = tmp >> NSHIFT;
            fx = tmp - (ix << NSHIFT);
            fxy = (fx * fy) >> NSHIFT;

            p_src = src_img + src_w * iy + ix;

            // 这样处理会使很多字符的边界信息丢失（从而导致8和B很难区分）
//             if((ix == 0) || (iy == 0) || (ix >= src_w - 1) || (iy >= src_h - 1))
//             {
//                 p_dst[w] = src_img[src_w * iy + ix];
//             }
//             else
            {
                sum = (int32_t)(((1 << NSHIFT) - fx - fy + fxy) * (int16_t)p_src[0]
                                + (fx - fxy) * (int16_t)p_src[1]
                                + (fy - fxy) * (int16_t)p_src[src_w + 0]
                                + (fxy) * (int16_t)p_src[src_w + 1]);

                p_dst[w] = (uint8_t)(sum >> NSHIFT);
            }
        }

        p_dst += dst_w;
    }

    return;
}

// 双线性插值方式实现图像缩放(针对灰度图像)
void gray_image_resize_linear_for_center(uint8_t *restrict src_img, uint8_t *restrict dst_img,
                                         int32_t src_w, int32_t src_h,
                                         int32_t dst_w, int32_t dst_h)
{
    int32_t w, h;
    int32_t tmp;
    int32_t ix, iy;
    int32_t sum;
    int32_t kx, ky;
    int32_t fx, fy, fxy;
    uint8_t *restrict p_src = src_img;
    uint8_t *restrict p_dst = dst_img;

#if 0
    kx = ((src_w - 1) << NSHIFT) / dst_w;
    ky = ((src_h - 1) << NSHIFT) / dst_h;
#else
    kx = (src_w << NSHIFT) / dst_w;
    ky = (src_h << NSHIFT) / dst_h;
#endif

#ifdef WIN32

    for (h = 0; h < dst_h; h++)
    {
        tmp = h * ky;
        iy = tmp >> NSHIFT;
        fy = tmp - (iy << NSHIFT);

        for (w = 0; w < dst_w; w++)
        {
            tmp = w * kx;
            ix = tmp >> NSHIFT;
            fx = tmp - (ix << NSHIFT);
            fxy = (fx * fy) >> NSHIFT;

            p_src = src_img + src_w * iy + ix;

            // 对于车牌图像这样处理可以减少一些边框和字符的粘连
            if ((ix == 0) || (iy == 0) || (ix >= src_w - 1) || (iy >= src_h - 1))
            {
                p_dst[w] = src_img[src_w * iy + ix];
            }
            else
            {
                sum = (int32_t)(((1 << NSHIFT) - fx - fy + fxy) * (int16_t)p_src[0]
                                + (fx - fxy) * (int16_t)p_src[1]
                                + (fy - fxy) * (int16_t)p_src[src_w + 0]
                                + (fxy) * (int16_t)p_src[src_w + 1]);

                p_dst[w] = (uint8_t)(sum >> NSHIFT);
            }
        }

        p_dst += dst_w;
    }

#else

    for (h = 0; h < dst_h; h++)
    {
        tmp = h * ky;
        iy = tmp >> NSHIFT;
        fy = tmp - (iy << NSHIFT);

        p_src = src_img + iy * src_w;

        for (w = 0; w < dst_w; w++)
        {
            tmp = w * kx;
            ix = tmp >> NSHIFT;
            fx = tmp - (ix << NSHIFT);
            fxy = (fx * fy) >> NSHIFT;

            sum = ((1 << NSHIFT) - fx - fy + fxy) * (int16_t)p_src[ix + 0]
                  + (fx - fxy) * (int16_t)p_src[ix + 1]
                  + (fy - fxy) * (int16_t)p_src[src_w + ix + 0]
                  + (fxy) * (int16_t)p_src[src_w + ix + 1];

            p_dst[w] = (uint8_t)(sum >> NSHIFT);
        }

        p_dst += dst_w;
    }

    p_src = src_img;
    p_dst = dst_img;

    for (h = 0; h < dst_h; h++)
    {
        iy = (h * ky) >> NSHIFT;
        p_src = src_img + iy * src_w;

        for (w = 0; w < dst_w; w++)
        {
            ix = (w * kx) >> NSHIFT;

            // 对于车牌图像这样处理可以减少一些边框和字符的粘连
            if ((ix == 0) || (iy == 0) || (ix >= src_w - 1) || (iy >= src_h - 1))
            {
                p_dst[w] = src_img[iy * src_w + ix];
            }
        }

        p_dst += dst_w;
    }

#endif

    return;
}

/********************************************************************
Function:       multi_channels_image_resize
Description:    双线性插值方式实现图像缩放(针对灰度图像和RGB彩色图像)

  1.  Param:
    src_img:    源图像
    dst_img:    目标图像
    nchannels:  通道数目(1：灰度图像或3：彩色图像)
    src_w：     源图像宽度
    src_h：     源图像高度
    dst_w:      目标图像宽度
    dst_h:      目标图像高度

    2.  Return:
    void
********************************************************************/
#if 0
void multi_channels_image_resize(uint8_t *restrict src_img, uint8_t *restrict dst_img, int32_t nchannels,
                                 int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h)
{
    int32_t h, w, k;
    int32_t ix, iy;
    int32_t sum;
    int32_t kx, ky;
    int32_t fx, fy, fxy;
    uint8_t *restrict p_src = src_img;
    uint8_t *restrict p_dst = dst_img;

    kx = (src_w << NSHIFT) / dst_w;
    ky = (src_h << NSHIFT) / dst_h;

    for (h = 0; h < dst_h; h++)
    {
        k = h * ky;
        iy = k >> NSHIFT;
        fy = k - (iy << NSHIFT);

        for (w = 0; w < dst_w; w++)
        {
            k = w * kx;
            ix = k >> NSHIFT;
            fx = k - (ix << NSHIFT);
            fxy = (fx * fy) >> NSHIFT;

            p_src = src_img + src_w * iy * nchannels;

            for (k = 0; k < nchannels; k++)
            {
                // 这样处理会使很多字符的边界信息丢失（从而导致8和B很难区分）
                if ((ix == 0) || (iy == 0) || (ix >= src_w - 1) || (iy >= src_h - 1))
                {
                    p_dst[w * nchannels + k] = src_img[(src_w * iy + ix) * nchannels + k];
                }
                else
                {
                    sum = ((1 << NSHIFT) - fx - fy + fxy) * p_src[(ix + 0) * nchannels + k]
                          + (fx - fxy) * p_src[(ix + 1) * nchannels + k]
                          + (fy - fxy) * p_src[(src_w + ix + 0) * nchannels + k]
                          + (fxy) * p_src[(src_w + ix + 1) * nchannels + k];

                    p_dst[w * nchannels + k] = sum >> NSHIFT;
                }
            }
        }

        p_dst += dst_w * nchannels;
    }

    return;
}

// 判断两个矩形框是否包含(recti包含rectj)
int32_t two_rectanges_inclusive_or_not(Rects recti, Rects rectj)
{
    int32_t inclusive_flg = 0;

    if (recti.x0 >= rectj.x0 && recti.x1 <= rectj.x1
        && recti.y0 >= rectj.y0 && recti.y1 <= rectj.y1)
    {
        inclusive_flg = 1;
    }

    return inclusive_flg;
}

// 判断两个矩形框是否重叠
int32_t two_rectanges_overlapped_or_not(Rects recti, Rects rectj, float ratio)
{
    int32_t xx0, xx1;
    int32_t yy0, yy1;
    int32_t dist;
    int32_t r1, r2;
    int32_t w1, h1;
    int32_t w2, h2;
    int32_t thresh;

    xx0 = (recti.x0 + recti.x1) >> 1;
    yy0 = (recti.y0 + recti.y1) >> 1;
    xx1 = (rectj.x0 + rectj.x1) >> 1;
    yy1 = (rectj.y0 + rectj.y1) >> 1;

    w1 = recti.x1 - recti.x0 + 1;
    h1 = recti.y1 - recti.y0 + 1;
    w2 = rectj.x1 - rectj.x0 + 1;
    h2 = rectj.y1 - rectj.y0 + 1;

    r1 = (int32_t)sqrt(w1 * w1 + h1 * h1);
    r2 = (int32_t)sqrt(w2 * w2 + h2 * h2);
    thresh = (int32_t)(MIN2(r1, r2) * ratio / 2);
    dist = (int32_t)sqrt((xx1 - xx0) * (xx1 - xx0) + (yy1 - yy0) * (yy1 - yy0));

    if (dist < thresh)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#endif

/********************************************************************
Function:       two_rectanges_intersect_or_not
Date:           2009/06/04
Description:    判断矩形框是否相交(包括包含)

  1.    Param:
  rc_src    矩形框1
  rc_dst    矩形框2

    2.  Return:
    1: 相交 0: 不相交
********************************************************************/
int32_t two_rectanges_intersect_or_not(Rects rc_src, Rects rc_dst)
{
    int32_t b_Result = 1;

    if ((rc_src.x0 > rc_dst.x1) || (rc_src.x1 < rc_dst.x0)
        || (rc_src.y0 > rc_dst.y1) || (rc_src.y1 < rc_dst.y0))
    {
        b_Result = 0;
    }

    return b_Result;
}

#ifdef WIN32
void write_pgm(unsigned char *img, int32_t img_w, int32_t img_h, int32_t id)
{
    FILE *fp = NULL;
    char buf[10];

    memset(buf, 0, sizeof(buf));
    sprintf(buf, "%d.pgm", id);

    fp = fopen(buf, "wb+");

    if (fp == NULL)
    {
        return;
    }

    memset(buf, 0, sizeof(buf));
    fprintf(fp, "P5");
    fprintf(fp, "\n");
    sprintf(buf, "%d %d", img_w, img_h);
    fprintf(fp, buf);
    fprintf(fp, "\n255\n");
    fwrite(img, (uint32_t)1, img_w * img_h, fp);
    fclose(fp);

    return;
}
#endif

#undef NSHIFT
