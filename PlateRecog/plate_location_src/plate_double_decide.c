#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//#include "../portab.h"
#include "../malloc_aligned.h"
#include "../common/threshold.h"

#ifdef  WIN321
#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include "opencv/highgui.h"
//#include "vld.h"
#endif

#define COLOR_BLUE   (0u)
#define COLOR_YELLOW (1u)
#define COLOR_WHITE  (2u)
#define COLOR_BLACK  (3u)

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

uint8_t bkgd_color_by_HSV(uint8_t *restrict yuv_img, uint8_t *bina_img, int32_t img_w, int32_t img_h, Rects *rect_plate)
{
    int32_t w, h;
    uint8_t y, u, v;
    uint8_t hh, ss, vv;
    uint8_t *y_buf = yuv_img;
    uint8_t *u_buf = y_buf + img_w * img_h;
    uint8_t *v_buf = u_buf + img_w * img_h / 4;
    int32_t yellow_num[2], blue_num[2], white_num[2], black_num[2];
    uint8_t best_color[2];
    int32_t plate_w, plate_h;
//    int32_t bkgd_num = 0;
    int32_t ftgd_num = 0;
    int32_t color;

    yellow_num[0] = 0;
    yellow_num[1] = 0;
    blue_num[0] = 0;
    blue_num[1] = 0;
    white_num[0] = 0;
    white_num[1] = 0;
    black_num[0] = 0;
    black_num[1] = 0;

    plate_w = rect_plate->x1 - rect_plate->x0 + 1;
    plate_h = rect_plate->y1 - rect_plate->y0 + 1;

    for (h = rect_plate->y0 + plate_h / 5; h < rect_plate->y0 + (plate_h - plate_h / 5); h++)
    {
        for (w = rect_plate->x0 + plate_w / 5; w < rect_plate->x0 + (plate_w - plate_w / 5); w++)
        {
            uint8_t gray_tmp = (uint8_t)(bina_img[plate_w * (h - rect_plate->y0) + w - rect_plate->x0] > (0u));

            y = y_buf[img_w * h + w];
            u = u_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
            v = v_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];

            // ͳ��ǰ����
            ftgd_num = ftgd_num + (int32_t)gray_tmp;

            yuv2hsv(y, u, v, &hh, &ss, &vv);

            if (vv < (89u))
            {
                black_num[gray_tmp]++;
            }
            else if ((ss < (50u)) && (vv > (89u)))
            {
                white_num[gray_tmp]++;
            }
            else if ((hh > (21u)) && (hh < (42u)) && (ss > (89u)) && (vv > (89u)))
            {
                yellow_num[gray_tmp]++;
            }
            else if ((hh > (141u)) && (hh < (184u)) && (ss < (89u)) && (vv > (89u)))
            {
                blue_num[gray_tmp]++;
            }
        }
    }

    for (w = 0; w < 2; w++)
    {
        if ((blue_num[w] >= yellow_num[w]) && (blue_num[w] >= white_num[w]) && (blue_num[w] >= black_num[w]))
        {
            best_color[w] = COLOR_BLUE;
        }
        else if ((yellow_num[w] >= blue_num[w]) && (yellow_num[w] >= white_num[w]) && (yellow_num[w] >= black_num[w]))
        {
            best_color[w] = COLOR_YELLOW;
        }
        else if ((white_num[w] >= blue_num[w]) && (white_num[w] >= yellow_num[w]) && (white_num[w] >= black_num[w]))
        {
            best_color[w] = COLOR_WHITE;
        }
        else
        {
            best_color[w] = COLOR_BLACK;
        }
    }

    if (best_color[0] == COLOR_BLUE)
    {
        if (best_color[1] == COLOR_BLUE)
        {
            color = 0;
        }
        else if (best_color[1] == COLOR_YELLOW)
        {
            color = 255;
        }
        else if (best_color[1] == COLOR_WHITE)
        {
            color = 0;
        }
        else
        {
            color = 255;
        }
    }
    else if (best_color[0] == COLOR_YELLOW)
    {
        if (best_color[1] == COLOR_BLUE)
        {
            color = 255;
        }
        else if (best_color[1] == COLOR_YELLOW)
        {
            color = 1;
        }
        else if (best_color[1] == COLOR_WHITE)
        {
            color = 255;
        }
        else
        {
            color = 255;
        }
    }
    else if (best_color[0] == COLOR_WHITE)
    {
        if (best_color[1] == COLOR_BLUE)
        {
            color = 255;
        }
        else if (best_color[1] == COLOR_YELLOW)
        {
            color = 255;
        }
        else if (best_color[1] == COLOR_WHITE)
        {
            color = 0;
        }
        else
        {
            color = 255;
        }
    }
    else
    {
        if (best_color[1] == COLOR_BLUE)
        {
            color = 0;
        }
        else if (best_color[1] == COLOR_YELLOW)
        {
            color = 1;
        }
        else if (best_color[1] == COLOR_WHITE)
        {
            color = 0;
        }
        else
        {
            // ���������ǰ����Ϊ��ɫ��ǰ���л�ɫ�����ﵽһ��Ҫ�����ж�Ϊ����
            if (yellow_num[1] > ftgd_num / 5)
            {
                color = 1;
            }
            else
            {
                color = 255;
            }
        }
    }

    return (uint8_t)color;
}

// ǰ���ͱ�������ȷ����ɫ
// static int32_t bkgd_color(uint8_t *bina_img, int32_t img_w, int32_t img_h)
// {
//     int32_t w, h;
//     int32_t num_black, num_white;
//     int32_t r;
//
//     num_black = num_white = 0;
//
//     for (h = img_h / 5; h < img_h - img_h / 5; h++)
//     {
//         for (w = img_w / 5; w < img_w - img_w / 5; w++)
//         {
//             num_black += bina_img[img_w * h + w] > (0u);
//         }
//     }
//
//     r = num_black * 100 / ((img_w * 3 / 5) * (img_h * 3 / 5));
//
//     if (r > max_r)
//     {
//         max_r = r;
//     }
//     if (r < min_r)
//     {
//         min_r = r;
//     }
//
//     if (r > 66)
//     {
//         return 1;
//     }
//     else if (r < 49)
//     {
//         return 0;
//     }
//     else
//     {
//         return 255;
//     }
// }

#define SHOW_HIST_BG 0
// ȷ�������Ǻڵװ��ֻ��ǰ׵׺���(0:�ڵװ��� 1:�׵׺��� 255:��ȷ��)
static int32_t get_plate_background_color(uint8_t *bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t peak;
    int32_t threshold;
    int32_t black_cnt;
    int32_t white_cnt;
    int32_t r; // ��������
    int32_t *restrict hist_hor = NULL;

#if SHOW_HIST_BG
    IplImage *hist = cvCreateImage(cvSize(img_w, 50), IPL_DEPTH_8U, 3);
    IplImage *image = cvCreateImage(cvSize(img_w, img_h), IPL_DEPTH_8U, 1);

    cvZero(hist);
#endif

    if (img_h < 5 || img_h > 255)
    {
        return -1;
    }

    CHECKED_MALLOC(hist_hor, sizeof(int32_t) * img_w, CACHE_SIZE);

#if SHOW_HIST_BG

    for (h = 0; h < img_h; h++)
    {
        memcpy(image->imageData + image->widthStep * h, bina_img + img_w * h, img_w);
    }

#endif

    peak = 0;

    for (w = 0; w < img_w; w++)
    {
        hist_hor[w] = 0;

        for (h = img_h / 5; h < img_h - img_h / 5; h++)
        {
            hist_hor[w] += (bina_img[img_w * h + w] > (uint8_t)0);
        }

        peak = hist_hor[w] > peak ? hist_hor[w] : peak;
    }

    threshold = peak / 2;

    for (w = 0; w < img_w; w++)
    {
        if (hist_hor[w] > threshold)
        {
            hist_hor[w] = img_h / 2;
        }
        else
        {
            hist_hor[w] = 0;
        }
    }

    black_cnt = 0;
    white_cnt = 0;

    for (w = img_w / 5; w < img_w - img_w / 5; w++)
    {
        if (hist_hor[w] > 0)
        {
            white_cnt++;
        }
        else
        {
            black_cnt++;
        }
    }

    // �������ͼ��㣬����PC��DSP������㾫���ϵĲ���
    r = white_cnt * 100 / (white_cnt + black_cnt);

//  printf("--------------rrr=%f\n", r);

#if SHOW_HIST_BG

    for (w = 0; w < img_w; w++)
    {
        for (h = 0; h < hist_hor[w] * 50 / img_h; h++)
        {
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 0] = (uint8_t)0;
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 1] = (uint8_t)255;
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 2] = (uint8_t)0;
        }
    }

    cvNamedWindow("hist", CV_WINDOW_AUTOSIZE);
    cvNamedWindow("image", CV_WINDOW_AUTOSIZE);

    cvShowImage("hist", hist);
    cvShowImage("image", image);

    cvWaitKey(0);
    cvReleaseImage(&hist);
    cvReleaseImage(&image);
#endif

    CHECKED_FREE(hist_hor, sizeof(int32_t) * img_w);

    // �������ͼ��㣬����PC��DSP������㾫���ϵĲ���
    if (r < 45)
    {
        return 0;
    }
    // �������ͼ��㣬����PC��DSP������㾫���ϵĲ���
    else if (r > 65)
    {
        return 1;
    }
    else
    {
        return 255;
    }
}

// static void double_decide_by_color(uint8_t *restrict yuv_img, uint8_t *bina_img, int32_t img_w,
//                                     int32_t img_h, int32_t color, int32_t *plate_double, Rects *plate_down)
// {
//     int32_t w, h;
//     int32_t plate_w, plate_h;
//     uint8_t y, u, v;
//     uint8_t hh, ss, vv;
//     int16_t ll, aa, bb;
//     uint8_t *y_buf = yuv_img;
//     uint8_t *u_buf = y_buf + img_w * img_h;
//     uint8_t *v_buf = u_buf + img_w * img_h / 4;
//     int32_t hh_num_down, hh_num_up;
//     int32_t ss_num_down, ss_num_up;
//     int32_t vv_num_down, vv_num_up;
//     int32_t bkgd_num = 0;
//
//     plate_w = plate_down->x1 - plate_down->x0 + 1;
//     plate_h = plate_down->y1 - plate_down->y0 + 1;
//
//     hh_num_down = hh_num_up = 0;
//     ss_num_down = ss_num_up = 0;
//     vv_num_down = vv_num_up = 0;
//
//     // �ڵװ��ֻ��߲�ȷ��
//     if ((color == 0) || (color == 255))
//     {
//         // �²����򱳾���ɫͳ��
//         for (h = plate_down->y0 + plate_h / 5; h <= plate_down->y1 - plate_h / 5; h++)
//         {
//             for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
//             {
//                 if (bina_img[img_w * h + w] == 0)
//                 {
//                     y = y_buf[img_w * h + w];
//                     u = u_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//                     v = v_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//
//                     bkgd_num++;
//
//                     yuv2hsv(y, u, v, &hh, &ss, &vv);
//
//                     hh_num_down += hh;
//                     ss_num_down += ss;
//                     vv_num_down += vv;
//                 }
//             }
//         }
//
//         if (bkgd_num != 0)
//         {
//             hh_num_down /= bkgd_num;
//             ss_num_down /= bkgd_num;
//             vv_num_down /= bkgd_num;
//         }
//         else
//         {
//             hh_num_down = 0;
//             ss_num_down = 0;
//             vv_num_down = 0;
//         }
//
//         bkgd_num = 0;
//
//         // �ϲ����򱳾���ɫͳ��
//         for (h = plate_down->y0 - plate_h / 2; h < plate_down->y0; h++)
//         {
//             for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
//             {
//                 if (bina_img[img_w * h + w] == 0)
//                 {
//                     y = y_buf[img_w * h + w];
//                     u = u_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//                     v = v_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//
//                     bkgd_num++;
//
//                     yuv2hsv(y, u, v, &hh, &ss, &vv);
//
//                     hh_num_up += hh;
//                     ss_num_up += ss;
//                     vv_num_up += vv;
//                 }
//             }
//         }
//
//         if (bkgd_num != 0)
//         {
//             hh_num_up /= bkgd_num;
//             ss_num_up /= bkgd_num;
//             vv_num_up /= bkgd_num;
//         }
//         else
//         {
//             hh_num_up = 0;
//             ss_num_up = 0;
//             vv_num_up = 0;
//         }
//
//         if (plate_double[0] != 0)
//         {
//             // H��S��Vֵ�����һ����Χ�ڣ�����Ϊ���²����򱳾�����С���п���Ϊ˫����
//             if ((abs(hh_num_down - hh_num_up) < 20) && (abs(ss_num_down - ss_num_up) < 45) && (abs(vv_num_down - vv_num_up) < 35))
//             {
//                 plate_double[0] = 1;
//             }
//             else
//             {
//                 plate_double[0] = 0;
//             }
//         }
//     }
//
//     hh_num_down = hh_num_up = 0;
//     ss_num_down = ss_num_up = 0;
//     vv_num_down = vv_num_up = 0;
//     bkgd_num = 0;
//
//     // �׵׺��ֻ��߲�ȷ��
//     if ((color == 1) || (color == 255))
//     {
//         // �²����򱳾���ɫͳ��
//         for (h = plate_down->y0 + plate_h / 5; h <= plate_down->y1 - plate_h / 5; h++)
//         {
//             for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
//             {
//                 if (bina_img[img_w * h + w] != 0)
//                 {
//                     y = y_buf[img_w * h + w];
//                     u = u_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//                     v = v_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//
//                     bkgd_num++;
//
//                     yuv2hsv(y, u, v, &hh, &ss, &vv);
//
//                     hh_num_down += hh;
//                     ss_num_down += ss;
//                     vv_num_down += vv;
//                 }
//             }
//         }
//
//         if (bkgd_num != 0)
//         {
//             hh_num_down /= bkgd_num;
//             ss_num_down /= bkgd_num;
//             vv_num_down /= bkgd_num;
//         }
//         else
//         {
//             hh_num_down = 0;
//             ss_num_down = 0;
//             vv_num_down = 0;
//         }
//
//         bkgd_num = 0;
//
//         // �ϲ����򱳾���ɫͳ��
//         for (h = plate_down->y0 - plate_h / 2; h < plate_down->y0; h++)
//         {
//             for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
//             {
//                 if (bina_img[img_w * h + w] != 0)
//                 {
//                     y = y_buf[img_w * h + w];
//                     u = u_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//                     v = v_buf[(img_w >> 1) * (h >> 1) + (w >> 1)];
//
//                     bkgd_num++;
//
//                     yuv2hsv(y, u, v, &hh, &ss, &vv);
//
//                     hh_num_up += hh;
//                     ss_num_up += ss;
//                     vv_num_up += vv;
//                 }
//             }
//         }
//
//         if (bkgd_num != 0)
//         {
//             hh_num_up /= bkgd_num;
//             ss_num_up /= bkgd_num;
//             vv_num_up /= bkgd_num;
//         }
//         else
//         {
//             hh_num_up = 0;
//             ss_num_up = 0;
//             vv_num_up = 0;
//         }
//
//         if (plate_double[1] != 0)
//         {
//             // H��S��Vֵ�����һ����Χ�ڣ�����Ϊ���²����򱳾�����С���п���Ϊ˫����
//             if ((abs(hh_num_down - hh_num_up) < 20) && (abs(ss_num_down - ss_num_up) < 60) && (abs(vv_num_down - vv_num_up) < 35))
//             {
//                 plate_double[1] = 1;
//             }
//             else
//             {
//                 plate_double[1] = 0;
//             }
//         }
//     }
// }

#define JUMP_SHOW 0
// ���������ֱ浥���ƺ�˫����
static void double_decide_by_jump1(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                   uint8_t *plate_double_flag, Rects *plate_up)
{
    int32_t w, h;
    int32_t plate_w, plate_h;
    int32_t stat1, stat2;
    int32_t change_avg;
    int32_t white_cnt;
    int32_t black_cnt;

    uint8_t *bina_img = NULL;
    uint8_t *gray_img = NULL;
    int32_t *change = NULL;

#if JUMP_SHOW
    IplImage *image = NULL;
#endif

    plate_w = plate_up->x1 - plate_up->x0 + 1;
    plate_h = plate_up->y1 - plate_up->y0 + 1;

    CHECKED_MALLOC(bina_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(gray_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(change, plate_h * sizeof(int32_t), CACHE_SIZE);

    memset(change, 0, plate_h * sizeof(int32_t));

    for (h = 0; h < plate_h; h++)
    {
        memcpy(gray_img + plate_w * h, yuv_img + img_w * (h + plate_up->y0) + plate_up->x0, plate_w);
    }

//    thresholding_avg(gray_img, bina_img, plate_w, plate_h, plate_w / 4, plate_w - plate_w / 4, 0, plate_h - 1);
    thresholding_by_grads(gray_img, bina_img, plate_w, plate_h);

    stat1 = MAX2(0, plate_h / 2 - 3);
    stat2 = MIN2(plate_h - 1, plate_h / 2 + 3);

    white_cnt = 0;
    black_cnt = 0;

    // ͳ���ϲ������м�7���м�1/2�е�������
    for (h = stat1; h <= stat2; h++)
    {
        for (w = plate_w / 4; w < plate_w - plate_w / 4; w++)
        {
            change[h] += bina_img[plate_w * h + w] != bina_img[plate_w * h + w + 1];
            white_cnt += bina_img[plate_w * h + w] > (0u);
            black_cnt += bina_img[plate_w * h + w] == (0u);
        }
    }

    change_avg = 0;

    for (h = stat1; h <= stat2; h++)
    {
        change_avg += change[h];
    }

    change_avg /= stat2 - stat1 + 1;

    // ƽ��������С��5����Ϊ������˫����������������ȷ��Ϊ������
    if ((change_avg >= 5 && change_avg <= 13) /*&& (white_cnt < black_cnt)*/)
    {
        *plate_double_flag = (1u);
    }
    else
    {
        *plate_double_flag = (0u);
    }

#if JUMP_SHOW
    image = cvCreateImage(cvSize(plate_w, plate_h), IPL_DEPTH_8U, 1);

    for (h = 0; h < plate_h; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            image->imageData[image->widthStep * h + w] = bina_img[plate_w * h + w];
        }
    }

    cvNamedWindow("image", CV_WINDOW_AUTOSIZE);
    cvShowImage("image", image);
    cvWaitKey(0);
    cvReleaseImage(&image);
#endif

    CHECKED_FREE(change, plate_h * sizeof(int32_t));
    CHECKED_FREE(gray_img, plate_w * plate_h * sizeof(uint8_t));
    CHECKED_FREE(bina_img, plate_w * plate_h * sizeof(uint8_t));


    return ;
}

// ��ֵͶӰ�������ֱ浥���ƺ�˫����
static void double_decide_by_jump2(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                   uint8_t *plate_double_flag, Rects *plate_up)
{
    int32_t w, h;
    int32_t white_cnt;
    int32_t black_cnt;
    int32_t plate_w, plate_h;
    uint8_t *bina_img = NULL;
    uint8_t *gray_img = NULL;
    Rects plate_up_temp;
    int32_t change_l = 0;
    int32_t change_r = 0;
    int32_t change_m = 0;
    int32_t white_cnt_l = 0;
    int32_t black_cnt_l = 0;
    int32_t white_cnt_r = 0;
    int32_t black_cnt_r = 0;
    int32_t white_cnt_m = 0;
    int32_t black_cnt_m = 0;
    int32_t cnt = 0;
    int32_t *sum_flag = NULL;

#if JUMP_SHOW
    IplImage *image = NULL;
#endif

    plate_w = plate_up->x1 - plate_up->x0 + 1;
    plate_h = plate_up->y1 - plate_up->y0 + 1;

    plate_up_temp.x0 = plate_up->x0;
    plate_up_temp.x1 = plate_up->x1;
    plate_up_temp.y0 = plate_up->y0;
    plate_up_temp.y1 = plate_up->y1;

    plate_up_temp.y0 += plate_h / 4;
    plate_h = plate_up_temp.y1 - plate_up_temp.y0;
    plate_up_temp.y0 += plate_h / 2;                 // ȥ�������ϱ߿��Ӱ��
    plate_up_temp.y1 -= plate_h / 8;                 // ȥ���ַ����������Ӱ��

//     plate_up_temp.y0 = plate_up->y0 + plate_h * 3 / 8;  // ȡ�ϲ�����3/8Ϊ�ϱ߽�
//     plate_up_temp.y1 = plate_up->y0 + plate_h * 5 / 8;  // ȡ�ϲ�����5/8Ϊ�±߽�

    // ���¶����������ĸߺͿ�
    plate_w = plate_up_temp.x1 - plate_up_temp.x0 + 1;
    plate_h = plate_up_temp.y1 - plate_up_temp.y0 + 1;

    CHECKED_MALLOC(bina_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(gray_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(sum_flag, plate_w * sizeof(int32_t), CACHE_SIZE);

    for (h = 0; h < plate_h; h++)
    {
        memcpy(gray_img + plate_w * h, yuv_img + img_w * (h + plate_up_temp.y0) + plate_up_temp.x0, plate_w);
    }

    // ��ֵ���ϲ�����
    thresholding_avg(gray_img, bina_img, plate_w, plate_h, plate_w / 4, plate_w - plate_w / 4, 0, plate_h - 1);

    // �ϲ��м����� ǰ����ͳ��
    white_cnt = 0;
    black_cnt = 0;

    for (h = 0; h < plate_h; h++)
    {
        for (w = plate_w / 4; w < plate_w - plate_w / 4; w++)
        {
//            bina_img[plate_w * h + w] = bina_img[plate_w * h + w];
            if (bina_img[plate_w * h + w] == (0u))
            {
                black_cnt++;
            }
            else
            {
                white_cnt++;
            }
        }
    }

    // ͳһΪ�׵׺���
    if (black_cnt > white_cnt)
    {
        for (h = 0; h < plate_h; h++)
        {
            for (w = plate_w / 4; w < plate_w - plate_w / 4; w++)
            {
                bina_img[plate_w * h + w] = bina_img[plate_w * h + w] == (0u) ? (255u) : (0u);
            }
        }
    }

    // ���ϲ��м�����ġ���ֵͶӰ����ֱ��ͶӰΪ��ֵ���
    for (w = plate_w / 4; w < plate_w - plate_w / 4; w++)
    {
        cnt = 0;

        for (h = 0; h < plate_h; h++)
        {
            cnt += (bina_img[plate_w * h + w] == (0u));
        }

        if (cnt * 100 / plate_h > 10) // ǰ���㳬�����е�1/10, ��ͶӰΪ��ɫ������Ϊ��ɫ
        {
            sum_flag[w] = 0;
        }
        else
        {
            sum_flag[w] = 255;
        }
    }

    // ���� ǰ��������
    // �м��ǰ������������
    for (w = plate_w / 2 - plate_w / 16; w < plate_w / 2 + plate_w / 16; w++)
    {
        if (sum_flag[w] == 0)
        {
            black_cnt_m++;
        }
        else
        {
            white_cnt_m++;
        }
    }

    // ��ߵ�ǰ������������
    for (w = plate_w / 4; w < plate_w / 2; w++)
    {
        if (sum_flag[w] == 0)
        {
            black_cnt_l++;
        }
        else
        {
            white_cnt_l++;
        }
    }

    // �ұߵ�ǰ������������
    for (w = plate_w / 2; w < plate_w - plate_w / 4; w++)
    {
        if (sum_flag[w] == 0)
        {
            black_cnt_r++;
        }
        else
        {
            white_cnt_r++;
        }
    }

    // ͶӰͼ���������
    change_l = 0;
    change_r = 0;
    change_m = 0;

    // �м�������� change_m
    for (w = plate_w / 2 - plate_w / 16; w < plate_w / 2 + plate_w / 16; w++)
    {
        if (sum_flag[w] != sum_flag[w + 1])
        {
            change_m++;
        }
    }

    // ��ߵ������� change_l
    for (w = plate_w / 4; w < plate_w / 2; w++)
    {
        if (sum_flag[w] != sum_flag[w + 1])
        {
            change_l++;
        }
    }

    // �ұߵ������� change_r
    for (w = plate_w / 2; w < plate_w - plate_w / 4 - 1; w++)
    {
        if (sum_flag[w] != sum_flag[w + 1])
        {
            change_r++;
        }
    }

    // ���������� ǰ���������������� ȷ����˫����
    if ((change_m <= 10)                                                                                 // �м���������
        && ((change_l >= 1) && (change_l < 15)) && ((change_r >= 1) && (change_r < 15))                  // �������߷ֱ���������
        && ((black_cnt_m + 1) < (white_cnt_m + 1) * 5)                                                   // �м��ǰ��������
        && (((black_cnt_l + 1) * 10 > (white_cnt_l + 1)) && ((black_cnt_l + 1) < (white_cnt_l + 1) * 5)) // ��ߵ�ǰ��������
        && (((black_cnt_r + 1) * 10 > (white_cnt_r + 1)) && ((black_cnt_r + 1) < (white_cnt_r + 1) * 5)))// �ұߵ�ǰ��������
    {
        *plate_double_flag = (1u);
    }
    else
    {
        *plate_double_flag = (0u);
    }

// ��ʾͼ��
#if JUMP_SHOW
    image = cvCreateImage(cvSize(plate_w, plate_h), IPL_DEPTH_8U, 1);

    for (h = 0; h < plate_h; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            image->imageData[image->widthStep * h + w] = bina_img[plate_w * h + w];
        }
    }

    cvNamedWindow("image", CV_WINDOW_AUTOSIZE);
    cvShowImage("image", image);
    cvWaitKey(0);
    cvReleaseImage(&image);
#endif

    CHECKED_FREE(sum_flag, plate_w * sizeof(int32_t));
    CHECKED_FREE(bina_img, plate_w * plate_h * sizeof(uint8_t));
    CHECKED_FREE(gray_img, plate_w * plate_h * sizeof(uint8_t));


    return;
}

static int32_t double_plate_up_down_location(uint8_t *bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t *change = NULL;
    int32_t min_change;
    int32_t min_line;

    CHECKED_MALLOC(change, img_h * sizeof(int32_t), CACHE_SIZE);
    memset(change, 0, img_h * sizeof(int32_t));

    for (h = 0; h < img_h; h++)
    {
        for (w = img_w / 5; w < img_w - img_w / 5; w++)
        {
            change[h] += bina_img[img_w * h + w] != bina_img[img_w * h + w + 1];
        }
    }

    min_line = img_h / 2;
    min_change = change[min_line];

    // ����²�����߶�̫С�����޷��ú��������ָ����²�����ֱ����ԭȷ��������Ϊ�²�����
    if (img_h / 2 - 12 <= 0)
    {
        min_line = img_h / 2;

        CHECKED_FREE(change, img_h * sizeof(int32_t));

        return min_line;
    }

    // �����ҷֽ���
    for (h = img_h / 2; h > img_h / 2 - 12; h--)
    {
        if (min_change > change[h])
        {
            min_change = change[h];
            min_line = h;
        }
    }

    // �����ҷֽ���
    for (h = img_h / 2; h < img_h / 2 + 12; h++)
    {
        if (min_change > change[h])
        {
            min_change = change[h];
            min_line = h;
        }
    }

    // ����ȷ�����ķֽ�������һ�����������С�ڱ��У���ѷֽ�������һ��
    if ((change[h - 1] < change[h + 1]) && (h > 0))
    {
        min_line = min_line - 1;
    }

    min_line = MAX2(0, min_line);
    min_line = MIN2(img_h - 1, min_line);

    CHECKED_FREE(change, img_h * sizeof(int32_t));

    return min_line;
}

static void plate_double_decide(uint8_t *yuv_img, int32_t img_w, int32_t img_h, uint8_t *bkgd_color,
                                uint8_t *plate_double_flag, Rects *plate_up, Rects *rect_plate)
{
    int32_t h;
    int32_t plate_w, plate_h;
    uint8_t *gray_img = NULL;
    uint8_t *bina_img = NULL;
//    uint8_t color = *bkgd_color;
    int32_t divide_line = rect_plate->y0;
    Rects plate_down;

    plate_w = rect_plate->x1 - rect_plate->x0 + 1;
    plate_h = rect_plate->y1 - rect_plate->y0 + 1;

    // ���ƺ�ѡ�����Ϸ��߶����ڳ��ƺ�ѡ����߶ȣ�ֻ�ܽ��е�����ʶ��
    if (rect_plate->y0 < plate_h)
    {
        *plate_double_flag = (0u);

//         CHECKED_MALLOC(gray_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
//         CHECKED_MALLOC(bina_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
//
//         for (h = rect_plate->y0; h <= rect_plate->y1; h++)
//         {
//             memcpy(gray_img + plate_w * (h - rect_plate->y0), yuv_img + img_w * h + rect_plate->x0, plate_w);
//         }
//
//         thresholding_avg(gray_img, bina_img, plate_w, plate_h, plate_w / 5, plate_w - plate_w / 5,
//             plate_h / 5, plate_h - plate_h / 5);
//
//         color = (uint8_t)get_plate_background_color(bina_img, plate_w, plate_h);
//
//         CHECKED_FREE(gray_img);
//         CHECKED_FREE(bina_img);

        return;
    }

    CHECKED_MALLOC(gray_img, plate_w * plate_h * 2 * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(bina_img, plate_w * plate_h * 2 * sizeof(uint8_t), CACHE_SIZE);

    for (h = rect_plate->y0 - plate_h; h <= rect_plate->y1; h++)
    {
        memcpy(gray_img + plate_w * (h - (rect_plate->y0 - plate_h)), yuv_img + img_w * h + rect_plate->x0, plate_w);
    }

    // ͳ���²�������м䲿������ĻҶȵõ���ֵ�����������������ֵ��
    thresholding_avg(gray_img, bina_img, plate_w, plate_h * 2, plate_w / 5, plate_w - plate_w / 5,
                     plate_h + plate_h / 5, plate_h * 2 - plate_h / 5);

//    color = (uint8_t)get_plate_background_color(bina_img + plate_w * plate_h, plate_w, plate_h);

    // Ϊ�˼��ٺ���ķ�֧���Ա�����ɫΪ255��������ٴν��е�ɫȷ��
    // �˴�ֻ��԰׵׺��ֵ����(�׵׺�����ǰ��ķ�������ȷ����)
//     if (color == (255u))
//     {
//         color = bkgd_color_by_HSV(yuv_img, bina_img + plate_w * plate_h, img_w, img_h, rect_plate);
//         if (color != (1u))
//         {
//             color = (255u);
//         }
//
//         *bkgd_color = color;
//     }

    // Ѱ��˫������������ֽ���
    divide_line = double_plate_up_down_location(bina_img, plate_w, plate_h * 2);

    divide_line = rect_plate->y0 - plate_h + divide_line;

    plate_down.x0 = rect_plate->x0;
    plate_down.x1 = rect_plate->x1;
    plate_down.y0 = divide_line;
    plate_down.y1 = rect_plate->y1;

    // �²�����߶�̫С���޷�����˫����ʶ��
    if (plate_down.y1 - plate_down.y0 + 1 < 10)
    {
        *plate_double_flag = 0;

        CHECKED_FREE(bina_img, plate_w * plate_h * 2 * sizeof(uint8_t));
        CHECKED_FREE(gray_img, plate_w * plate_h * 2 * sizeof(uint8_t));

        return;
    }

    plate_up->x0 = rect_plate->x0;
    plate_up->x1 = rect_plate->x1;
    plate_up->y1 = divide_line;
    plate_up->y0 = MAX2(0, plate_up->y1 + 1 - (plate_down.y1 - plate_down.y0 + 1) * 2 / 3);

    // �ϲ�����߶�̫С���޷�����˫����ʶ��
    if (plate_up->y1 - plate_up->y0 + 1 <= 7)
    {
        *plate_double_flag = 0;

        CHECKED_FREE(bina_img, plate_w * plate_h * 2 * sizeof(uint8_t));
        CHECKED_FREE(gray_img, plate_w * plate_h * 2 * sizeof(uint8_t));

        return;
    }

    // �������ֱ浥���ƺ�˫����
    double_decide_by_jump1(yuv_img, img_w, img_h, plate_double_flag, plate_up);

    // ��ȷ��Ϊ˫���Ƶĳ��ƺ�ѡ����, ��һ���ж��Ƿ�Ϊ˫����
    if (*plate_double_flag == (1u))
    {
        // ��ֵͶӰ�������ֱ浥���ƺ�˫����
        double_decide_by_jump2(yuv_img, img_w, img_h, plate_double_flag, plate_up);
    }

    if (*plate_double_flag == (1u))
    {
        rect_plate->y0 = plate_up->y0;
    }

    CHECKED_FREE(bina_img, plate_w * plate_h * 2 * sizeof(uint8_t));
    CHECKED_FREE(gray_img, plate_w * plate_h * 2 * sizeof(uint8_t));

    return;
}

void plate_double_decide_all_candidate(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                       Rects *restrict rect_real, int32_t real_num, uint8_t *restrict color,
                                       uint8_t *restrict plate_double_flag, Rects *restrict rect_double_up)
{
    int32_t k;
    Rects plate_up;

    plate_up.x0 = 0;
    plate_up.x1 = 0;
    plate_up.y0 = 0;
    plate_up.y1 = 0;

    for (k = 0; k < real_num; k++)
    {
        plate_double_decide(yuv_img, img_w, img_h, &color[k], &plate_double_flag[k], &plate_up, &rect_real[k]);

        if (plate_double_flag[k] == (1u))
        {
            rect_double_up[k] = plate_up;
        }
    }

    return;
}
