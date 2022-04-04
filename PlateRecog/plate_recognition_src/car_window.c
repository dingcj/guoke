#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../malloc_aligned.h"
#include "car_window.h"
#include "plate_color.h"
#include "../common/threshold.h"

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#ifdef  WIN321
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "../color_space_src/rgb2yuv.h"
//调试用标志，用于显示原图像上车窗的框定和sobel以及二值化后的效果图
#define CAR_WINDOW_DEBUG 0
#endif

#ifdef _TMS320C6X_
#pragma CODE_SECTION(horizontal_sobel_window, ".text:horizontal_sobel_window")
#pragma CODE_SECTION(vertical_sobel_window, ".text:vertical_sobel_window")
#pragma CODE_SECTION(car_window_area_recog, ".text:car_window_area_recog")
#endif

#if CAR_WINDOW_DEBUG
//截取预选检测框图像
void get_choose_area(uint8_t *restrict yuv_data, uint8_t *restrict bodyimg, int32_t img_w, int32_t img_h,
                     int32_t body_w, int32_t body_h, Rects *restrict rect_body)
{
    int32_t w, h;
    uint8_t *restrict yuv_y = yuv_data;
    uint8_t *restrict yuv_u = yuv_data + img_w * img_h;
    uint8_t *restrict yuv_v = yuv_data + img_w * img_h * 5 / 4;
    uint8_t *restrict bodyimg_y = bodyimg;
    uint8_t *restrict bodyimg_u = bodyimg + body_w * body_h;
    uint8_t *restrict bodyimg_v = bodyimg + body_w * body_h * 5 / 4;

    for (h = 0; h < body_h; h++)
    {
        for (w = 0; w < body_w; w++)
        {
            bodyimg_y[body_w * h + w] = yuv_y[img_w * (h + rect_body->y0) + rect_body->x0 + w];
        }
    }

    for (h = 0; h < body_h / 2; h++)
    {
        for (w = 0; w < body_w / 2; w++)
        {
            bodyimg_u[body_w / 2 * h + w] = yuv_u[img_w / 2 * (h + rect_body->y0 / 2) + rect_body->x0 / 2 + w];
            bodyimg_v[body_w / 2 * h + w] = yuv_v[img_w / 2 * (h + rect_body->y0 / 2) + rect_body->x0 / 2 + w];
        }
    }

    return;
}
#endif

//sobel水平边缘检测函数
static void horizontal_sobel_window(uint8_t *restrict in, uint8_t *restrict out,
                                    int32_t img_w, int32_t img_h)
{
    int32_t h, w;
    int32_t i00, i02;
    int32_t i20, i22;
#ifdef _TMS320C6X_
    int32_t tmp0, tmp1, tmp2, tmp3;
    int32_t tmp4, tmp5, tmp6, tmp7;
    int32_t mp0 = 0x00FFFEFF;   // 0  -1  -2  -1
    int32_t mp1 = 0x00010201;   // 0   1   2   1
    int32_t mp2 = 0xFFFEFF00;   //-1  -2  -1   0
    int32_t mp3 = 0x01020100;   // 1   2   1   0

    for (h = 0; h < img_h - 2; h++)
    {
        for (w = img_w / 4 - 4; w < img_w - img_w / 4; w += 4)
        {
            i00 = _mem4(in + img_w * h + w + 0);
            i02 = _mem4(in + img_w * h + w + 2);
            i20 = _mem4(in + img_w * (h + 2) + w + 0);
            i22 = _mem4(in + img_w * (h + 2) + w + 2);

            tmp0 = _dotpsu4(mp0, i00);
            tmp1 = _dotpsu4(mp2, i00);
            tmp0 = _pack2(tmp1, tmp0);

            tmp2 = _dotpsu4(mp1, i20);
            tmp3 = _dotpsu4(mp3, i20);
            tmp2 = _pack2(tmp3, tmp2);

            tmp0 = _add2(tmp0, tmp2);

            tmp4 = _dotpsu4(mp0, i02);
            tmp5 = _dotpsu4(mp2, i02);
            tmp4 = _pack2(tmp5, tmp4);

            tmp6 = _dotpsu4(mp1, i22);
            tmp7 = _dotpsu4(mp3, i22);
            tmp6 = _pack2(tmp7, tmp6);

            tmp4 = _add2(tmp4, tmp6);

            tmp0 = _abs2(tmp0);
            tmp4 = _abs2(tmp4);

            tmp0 = _spacku4(tmp4, tmp0);

            _mem4(out + img_w * (h + 1) + w + 1) = tmp0;
        }
    }

#else
    int32_t O, H;
    int32_t i01, i21;

    for (h = 0; h < img_h - 2; h++)
    {
        for (w = img_w / 4 - 1; w < img_w - img_w / 4 - 2; w++)
        {
            i00 = (int32_t)(in[img_w * (h + 0) + w + 0]);
            i01 = (int32_t)(in[img_w * (h + 0) + w + 1]);
            i02 = (int32_t)(in[img_w * (h + 2) + w + 2]);
            i20 = (int32_t)(in[img_w * (h + 2) + w + 0]);
            i21 = (int32_t)(in[img_w * (h + 2) + w + 1]);
            i22 = (int32_t)(in[img_w * (h + 2) + w + 2]);

            H = -   i00         - 2 * i01         -   i02
                +   i20         + 2 * i21         +   i22;

            O = abs(H);

            if (O > 255)
            {
                O = 255;
            }

            out[img_w * (h + 1) + w + 1] = (uint8_t)O;
        }
    }

#endif
    return;
}

//sobel垂直边缘检测函数
static void vertical_sobel_window(const uint8_t *restrict in, uint8_t *restrict out, int32_t img_w, int32_t img_h)
{
    int32_t O, V;
    int32_t w, h;
    int32_t i00, i02;
    int32_t i10, i12;
    int32_t i20, i22;
#ifdef _TMS320C6X11_
    int32_t tmp0, tmp1, tmp2, tmp3, tmp4;
    int32_t mp0 = 0x000100FF; // 0   1   0   -1
    int32_t mp1 = 0x000200FE; // 0   2   0   -2
    int32_t mp2 = 0x0100FF00; // 1   0   -1  0
    int32_t mp3 = 0x0200FE00; // 2   0   -2  0

    for (h = 0; h < img_h - 2; h++)
    {
        for (w = 0; w < (img_w - 2) / 4 * 4; w += 4)
        {
            i00 = _amem4(in + img_w * h + w);
            i10 = _amem4(in + img_w * h + w + img_w);
            i20 = _amem4(in + img_w * h + w + img_w * 2);
            i02 = _mem4(in + img_w * h + w + 2);
            i12 = _mem4(in + img_w * h + w + img_w + 2);
            i22 = _mem4(in + img_w * h + w + img_w * 2 + 2);

            tmp0 = _dotpsu4(mp0, i00);
            tmp1 = _dotpsu4(mp1, i10);
            tmp2 = _dotpsu4(mp0, i20);
            tmp3 = tmp0 + tmp1 + tmp2;

            tmp0 = _dotpsu4(mp2, i00);
            tmp1 = _dotpsu4(mp3, i10);
            tmp2 = _dotpsu4(mp2, i20);
            tmp4 = tmp0 + tmp1 + tmp2;
            tmp3 = _pack2(tmp4, tmp3);
            tmp3 = _abs2(tmp3);

            tmp0 = _dotpsu4(mp0, i02);
            tmp1 = _dotpsu4(mp1, i12);
            tmp2 = _dotpsu4(mp0, i22);
            tmp4 = tmp0 + tmp1 + tmp2;

            tmp0 = _dotpsu4(mp2, i02);
            tmp1 = _dotpsu4(mp3, i12);
            tmp2 = _dotpsu4(mp2, i22);
            tmp0 = tmp0 + tmp1 + tmp2;
            tmp4 = _pack2(tmp0, tmp4);
            tmp4 = _abs2(tmp4);

            tmp3 = _spacku4(tmp4, tmp3);

            _mem4(out + img_w * (h + 1) + w + 1) = tmp3;
        }
    }

#else

    for (h = 0; h < img_h - 2; h++)
    {
        for (w = 0; w < img_w - 2; w++)
        {
            i00 = (int32_t)in[img_w * (h + 0) + w + 0];
            i02 = (int32_t)in[img_w * (h + 0) + w + 2];
            i10 = (int32_t)in[img_w * (h + 1) + w + 0];
            i12 = (int32_t)in[img_w * (h + 1) + w + 2];
            i20 = (int32_t)in[img_w * (h + 2) + w + 0];
            i22 = (int32_t)in[img_w * (h + 2) + w + 2];

            V = -   i00   +   i02
                - 2 * i10 + 2 * i12
                -   i20   +   i22;

            O = abs(V);

            if (O > 255)
            {
                O = 255;
            }

            out[img_w * (h + 1) + w + 1] = (uint8_t)O;
        }
    }

#endif
    return;
}

int32_t car_window_area_recog(uint8_t *restrict yuv_data, int32_t img_w, /*int32_t img_h,*/
                              Rects *restrict plate_real, Rects *restrict window_rect)
{
    int32_t ret = 1;
    int32_t w, h, i;
    int32_t plate_w, plate_h;
    int32_t window_w, window_h;
    int32_t tmp_hist;
    int32_t num1, num2;
    int32_t new_y0, new_y1;
    int32_t top_num = 0;
    int32_t thresh_value;
    int32_t num_flag;
    int32_t top_1, top_2;
    int32_t mid_plate;
    uint8_t *restrict gray_img = NULL;
    uint8_t *restrict edge_img = NULL;
    int32_t *restrict hist_h = NULL;
    int32_t *restrict hist_w = NULL;
    int32_t *restrict top_value = NULL;
#if CAR_WINDOW_DEBUG
    IplImage *cvimg1 = NULL;
    IplImage *cvimg2 = NULL;
    uint8_t  *rgb_img = NULL;
#endif

    plate_w = plate_real->x1 - plate_real->x0;
    plate_h = plate_real->y1 - plate_real->y0;

    window_rect->x0 = (int16_t)MAX2(0, (plate_real->x0 - plate_w * 3 + 3) / 8 * 8);
    window_rect->x1 = (int16_t)MIN2(img_w, (plate_real->x1 + plate_w * 3 + 3) / 8 * 8);
    window_rect->y0 = (int16_t)MAX2(0, (plate_real->y0 - plate_h * 30 + 3) / 8 * 8);
    window_rect->y1 = (int16_t)MAX2(0, (plate_real->y0 - plate_h * 5 + 3) / 8 * 8);

    window_w = window_rect->x1 - window_rect->x0;
    window_h = window_rect->y1 - window_rect->y0;

    CHECKED_MALLOC(hist_h, window_h * sizeof(int32_t), CACHE_SIZE);
    CHECKED_MALLOC(hist_w, window_w * sizeof(int32_t), CACHE_SIZE);
    CHECKED_MALLOC(top_value, MAX2(window_w, window_h) * sizeof(int32_t), CACHE_SIZE);
#if CAR_WINDOW_DEBUG
    CHECKED_MALLOC(gray_img, window_w * window_h * 3 / 2 * sizeof(uint8_t), CACHE_SIZE);
#else
    CHECKED_MALLOC(gray_img, window_w * window_h * sizeof(uint8_t), CACHE_SIZE);
#endif
    CHECKED_MALLOC(edge_img, window_w * window_h * sizeof(uint8_t), CACHE_SIZE);

    memset(hist_h, 0, window_h * sizeof(int32_t));
    memset(hist_w, 0, window_w * sizeof(int32_t));
    memset(top_value, 0, MAX2(window_w, window_h) * sizeof(int32_t));
    memset(edge_img, 0, window_w * window_h * sizeof(uint8_t));

#if CAR_WINDOW_DEBUG
    get_choose_area(yuv_data, gray_img, img_w, img_h, window_w, window_h, window_rect);
#else

    for (h = 0; h < window_h; h++)
    {
        memcpy(gray_img + window_w * h, yuv_data + img_w * (h + window_rect->y0) + window_rect->x0,
               window_w * sizeof(uint8_t));
    }

#endif
    /*************************************************************
     *************************************************************
     *                                                           *
     *---------------------水平边界定位--------------------------*
     *                                                           *
     *************************************************************
     *************************************************************/
    //simple方式过于简单，对深色车的边缘检测很容易漏掉上界限
    horizontal_sobel_window(gray_img, edge_img, window_w, window_h);

    //生成直方图
#ifdef _TMS320C6X_

    for (h = 0; h < window_h; h += 4)
    {
        uint32_t tmp0, tmp1;
        uint32_t tmp2, tmp3;
        uint32_t CmpValue = 0x19191919;

        for (w = window_w / 4; w < window_w - window_w / 4; w += 4)
        {
            tmp0 = _amem4(edge_img + window_w * (h + 0) + w);
            tmp1 = _amem4(edge_img + window_w * (h + 1) + w);
            tmp2 = _amem4(edge_img + window_w * (h + 2) + w);
            tmp3 = _amem4(edge_img + window_w * (h + 3) + w);
            tmp0 = _cmpgtu4(tmp0, CmpValue);
            tmp1 = _cmpgtu4(tmp1, CmpValue);
            tmp2 = _cmpgtu4(tmp2, CmpValue);
            tmp3 = _cmpgtu4(tmp3, CmpValue);
            hist_h[h + 0] += _bitc4(tmp0);
            hist_h[h + 1] += _bitc4(tmp1);
            hist_h[h + 2] += _bitc4(tmp2);
            hist_h[h + 3] += _bitc4(tmp3);
        }
    }

#else

    for (h = 0; h < window_h; h++)
    {
        for (w = window_w / 4; w < window_w - window_w / 4; w++)
        {
            if (edge_img[window_w * h + w] > (uint8_t)25)
            {
                hist_h[h]++;
#if CAR_WINDOW_DEBUG
                edge_img[window_w * h + w] = (uint8_t)255;
#endif
            }

#if CAR_WINDOW_DEBUG
            else
            {
                edge_img[window_w * h + w] = (uint8_t)0;
            }

#endif
        }
    }

#endif

    //直方图峰值提取
    thresh_value = 15;

    for (h = window_h - thresh_value; h > thresh_value; h--)
    {
        for (i = thresh_value; i > -thresh_value; i--)
        {
            if (hist_h[h] < hist_h[h + i])
            {
                break;
            }
        }

        if ((i == -thresh_value) && (hist_h[h] > 50))
        {
            top_value[top_num] = h;
            top_num++;
            h -= thresh_value - 1;
        }
    }

    if (top_num == 0)
    {
        ret = 0;
        goto EXT;
    }

    //计算峰值阈值
    tmp_hist = 0;

    for (h = 0; h < top_num; h++)
    {
        tmp_hist += hist_h[top_value[h]];
    }

    tmp_hist = tmp_hist / top_num;

    if (top_num < 5)
    {
        tmp_hist = tmp_hist * 2 / 3;
    }

    //上下边界线提取
    num_flag = 0;
    num1 = 0;
    num2 = 0;
    top_1 = top_num - 1;
    top_2 = 0;

    if (top_num < 3)
    {
        if (top_num == 2)
        {
            num1 = top_value[0];
            num2 = top_value[1];

            if (abs(num2 - num1) < plate_h * 3)
            {
                num2 = MAX2(num1, num2);
                num1 = 0;
            }
        }
        else
        {
            num1 = 0;
            num2 = top_value[0];
        }
    }
    else
    {
        for (h = 0; h < top_num; h++)
        {
            if ((hist_h[top_value[h]] > tmp_hist) && (top_value[h] < window_h * 19 / 20))
            {
                if (num_flag == 0)
                {
                    num2 = top_value[h];
                    top_2 = h;
                    num_flag = 1;
                }
                else
                {
                    if ((num2 - top_value[h]) < 4 * plate_h)
                    {
                        {
                            num2 = top_value[h];
                            top_2 = h;
                        }
                    }
                    else
                    {
                        num1 = top_value[h];
                        top_1 = h;
                        break;
                    }
                }
            }
        }
    }

    //车窗过大的情况，略微调整上边界
    h = top_1;

    while (((num2 - num1) > plate_h * 8) && (h > top_2))
    {
        h--;

        if ((h > top_2) && ((num2 - top_value[h]) > plate_h * 4))
        {
            num1 = top_value[h];
            top_1 = h;
        }
    }

    //纠正不了的过大车窗，识别为车顶，车窗位置往下延伸
    if (((num2 - num1) > plate_h * 8) && (top_2 > 0))
    {
        num1 =  num2;
        num2 = top_value[top_2 - 1];
    }

    //车窗过小的情况，略微调整下边界
    h = top_2;

    while (((num2 - num1) < plate_h * 4) && (h > 0) && (num1 != 0))
    {
        h--;
        num2 = top_value[h];
        top_2 = h;
    }

    if (num1 > num2)
    {
        int32_t change_tmp = num1;
        num1 = num2;
        num2 = change_tmp;
    }

    new_y0 = window_rect->y0 + num1;
    new_y1 = window_rect->y0 + num2;

    if (num2 == num1)
    {
        ret = 0;
        goto EXT;
    }

    /*************************************************************
     *************************************************************
     *                                                           *
     *---------------------垂直边界定位--------------------------*
     *                                                           *
     *************************************************************
     *************************************************************/
//  vertical_sobel_simple(gray_img + num1 * window_w, edge_img + num1 * window_w, window_w, num2 - num1);
    vertical_sobel_window(gray_img + num1 * window_w, edge_img + num1 * window_w, window_w, num2 - num1);

    //统计倾斜直方图
    for (w = 0; w < window_w / 2 - (num2 - num1) / 4; w++)
    {
        for (h = num2 - num1 - 1; h >= 0; h--)
        {
            if (edge_img[window_w * (h + num1) + w + (num2 - num1 - h) / 4] > (uint8_t)25)
            {
                hist_w[w]++;
#if CAR_WINDOW_DEBUG
                edge_img[window_w * (h + num1) + w + (num2 - num1 - h) / 4] = (uint8_t)255;
#endif
            }

#if CAR_WINDOW_DEBUG
            else
            {
                edge_img[window_w * (h + num1) + w + (num2 - num1 - h) / 4] = (uint8_t)0;
            }

#endif
        }
    }

    for (w = window_w / 2 + (num2 - num1) / 4; w < window_w; w++)
    {
        for (h = num2 - num1 - 1; h >= 0; h--)
        {
            if (edge_img[window_w * (h + num1) + w - (num2 - num1 - h) / 4] > (uint8_t)25)
            {
                hist_w[w]++;
#if CAR_WINDOW_DEBUG
                edge_img[window_w * (h + num1) + w - (num2 - num1 - h) / 4] = (uint8_t)255;
#endif
            }

#if CAR_WINDOW_DEBUG
            else
            {
                edge_img[window_w * (h + num1) + w - (num2 - num1 - h) / 4] = (uint8_t)0;
            }

#endif
        }
    }

    //直方图峰值提取
    thresh_value = 5;
    top_num = 0;

    for (w = thresh_value; w < window_w - thresh_value; w++)
    {
        for (i = -thresh_value; i < thresh_value; i++)
        {
            if (hist_w[w] < hist_w[w + i])
            {
                break;
            }
        }

        if ((i == thresh_value) && (hist_w[w] > (num2 - num1) / 4))
        {
            top_value[top_num] = w;
            top_num++;
            w += thresh_value - 1;
        }
    }

    if (top_num == 0)
    {
        ret = 0;
        goto EXT;
    }

    num1 = 0;
    num2 = 0;

    mid_plate = (plate_real->x0 + plate_real->x1) / 2 - window_rect->x0;
    //从两边往中间收缩找边界
    top_1 = 0;
    top_2 = top_num - 1;

    //左边界定位
    for (i = 0; i < top_num; i++)
    {
        if (mid_plate - top_value[i] > plate_w * 2 / 3)
        {
            if (mid_plate - top_value[i] < plate_w * 5 / 4)
            {
                break;
            }

            num1 = top_value[i];
            top_1 = i;
        }
        else
        {
            break;
        }
    }

    if (num1 == 0)
    {
        num1 = MAX2(0, mid_plate - plate_w * 2);
    }

    //右边界定位
    for (i = top_num - 1; i >= 0; i--)
    {
        if (top_value[i] - mid_plate > plate_w * 2 / 3)
        {
            if (top_value[i] - mid_plate < plate_w * 5 / 4)
            {
                break;
            }

            num2 = top_value[i];
            top_2 = i;
        }
        else
        {
            break;
        }
    }

    if (i == top_num - 1)
    {
//      num2 = window_w - 1;
        num2 = MIN2(window_w - 1, mid_plate + plate_w * 2);
    }

#if CAR_WINDOW_DEBUG
    cvimg1 = cvCreateImage(cvSize(window_w, window_h), IPL_DEPTH_8U, 3);
    cvimg2 = cvCreateImage(cvSize(window_w, window_h), IPL_DEPTH_8U, 3);
    CHECKED_MALLOC(rgb_img, window_w * window_h * 3, CACHE_SIZE);
    yuv2bgr(gray_img, rgb_img, window_w, window_h);

    for (w = num1; w < num2 - 1; w++)
    {
        rgb_img[window_w * (new_y0 - window_rect->y0) * 3 + w * 3 + 0] = (uint8_t)0;
        rgb_img[window_w * (new_y0 - window_rect->y0) * 3 + w * 3 + 1] = (uint8_t)0;
        rgb_img[window_w * (new_y0 - window_rect->y0) * 3 + w * 3 + 2] = (uint8_t)255;
        rgb_img[window_w * (new_y1 - window_rect->y0) * 3 + w * 3 + 0] = (uint8_t)0;
        rgb_img[window_w * (new_y1 - window_rect->y0) * 3 + w * 3 + 1] = (uint8_t)0;
        rgb_img[window_w * (new_y1 - window_rect->y0) * 3 + w * 3 + 2] = (uint8_t)255;
    }

    for (h = new_y0 - window_rect->y0; h < new_y1 - window_rect->y0 - 1; h++)
    {
        rgb_img[window_w * h * 3 + num1 * 3 + 0] = (uint8_t)0;
        rgb_img[window_w * h * 3 + num1 * 3 + 1] = (uint8_t)0;
        rgb_img[window_w * h * 3 + num1 * 3 + 2] = (uint8_t)255;
        rgb_img[window_w * h * 3 + num2 * 3 + 0] = (uint8_t)0;
        rgb_img[window_w * h * 3 + num2 * 3 + 1] = (uint8_t)0;
        rgb_img[window_w * h * 3 + num2 * 3 + 2] = (uint8_t)255;
    }

    for (h = 0; h < window_h; h++)
    {
        for (w = 0; w < window_w; w++)
        {
            cvimg1->imageData[cvimg1->widthStep * h + w * 3 + 0] = edge_img[window_w * h + w];
            cvimg1->imageData[cvimg1->widthStep * h + w * 3 + 1] = edge_img[window_w * h + w];
            cvimg1->imageData[cvimg1->widthStep * h + w * 3 + 2] = edge_img[window_w * h + w];
            cvimg2->imageData[cvimg2->widthStep * h + w * 3 + 0] = rgb_img[window_w * h * 3 + w * 3 + 0];
            cvimg2->imageData[cvimg2->widthStep * h + w * 3 + 1] = rgb_img[window_w * h * 3 + w * 3 + 1];
            cvimg2->imageData[cvimg2->widthStep * h + w * 3 + 2] = rgb_img[window_w * h * 3 + w * 3 + 2];
        }
    }

    cvNamedWindow("picture", CV_WINDOW_AUTOSIZE);
    cvNamedWindow("picture2", CV_WINDOW_AUTOSIZE);
    cvShowImage("picture", cvimg1);
    cvShowImage("picture2", cvimg2);
    cvWaitKey(0);
    cvDestroyWindow("picture");
    cvDestroyWindow("picture2");
    cvReleaseImage(&cvimg1);
    cvReleaseImage(&cvimg2);
#endif

    window_rect->y0 = (int16_t)new_y0;
    window_rect->y1 = (int16_t)new_y1;
    window_rect->x1 = (int16_t)(window_rect->x0 + num2);
    window_rect->x0 = (int16_t)(window_rect->x0 + num1);
EXT:
#if CAR_WINDOW_DEBUG
    CHECKED_FREE(gray_img, window_w * window_h * 3 / 2 * sizeof(uint8_t));
#else
    CHECKED_FREE(gray_img, window_w * window_h * sizeof(uint8_t));
#endif
    CHECKED_FREE(edge_img, window_w * window_h * sizeof(uint8_t));

    CHECKED_FREE(hist_h, window_h * sizeof(int32_t));
    CHECKED_FREE(hist_w, window_w * sizeof(int32_t));
    CHECKED_FREE(top_value, MAX2(window_w, window_h) * sizeof(int32_t));

    return ret;
}
