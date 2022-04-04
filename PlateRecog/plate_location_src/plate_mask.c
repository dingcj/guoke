#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "../common/threshold.h"
//#include "../dma_interface.h"
#include "../debug_show_image.h"

#ifdef  WIN32
#define DEBUG_DELETES 0
#define SHOW_HIST 0
#define SHOW_HIST_BG 0
#endif

#define CAND_MIN_W (50)
#define CAND_MIN_H (10)

#define MAX_CAND_NUM (20)   // 候选区域限制数量
#define NOMAL_CAND_NUM (20) // 保留数量

#ifdef  WIN32
static int deletes = 0;
#endif

// 上下边界的粗定位
static int32_t plate_upper_and_lower_locating_new(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
                                                  int32_t plate_w, int32_t plate_h,
                                                  int32_t *restrict change_w, int32_t *restrict change_h)
{
    int32_t w, h;
    int32_t change_thresh;
    uint8_t *restrict ptr = NULL;
    uint16_t cnt;

    // 统计中间9行的跳变数作为阈值
    (*change_w) = 0;

    for (h = plate_h / 2 - 4; h <= plate_h / 2 + 4; h++)
    {
        ptr = bina_img + plate_w * h;

        for (w = 0; w < plate_w - 1; w++)
        {
            if (*(ptr + w) != *(ptr + w + 1))
            {
                (*change_w)++;
            }
        }
    }

    change_thresh = ((*change_w) / 9) / 2;

    //  printf("-------------------------------------%d\n", change_thresh);

    // 从中间往上扫描得到上边界
    *y0 = 0;

    for (h = plate_h / 2 - 3; h >= 0; h--)
    {
        (*change_w) = 0;
        ptr = bina_img + plate_w * h;

        for (w = 0; w < plate_w - 1; w++)
        {
            if (*(ptr + w) != *(ptr + w + 1))
            {
                (*change_w)++;
            }
        }

        // 跳变数小于阈值则找到了车牌的上边界
        if ((*change_w) < change_thresh)
        {
            *y0 = h;
            break;
        }
    }

    // 从中间往下扫描得到下边界
    *y1 = plate_h - 1;

    for (h = plate_h / 2 + 3; h < plate_h; h++)
    {
        (*change_w) = 0;
        ptr = bina_img + plate_w * h;

        for (w = 0; w < plate_w - 1; w++)
        {
            if (*(ptr + w) != *(ptr + w + 1))
            {
                (*change_w)++;
            }
        }

        // 跳变数小于阈值则找到了车牌的下边界
        if ((*change_w) < change_thresh)
        {
            *y1 = h;
            break;
        }
    }

    if (plate_h < 10)
    {
        *y0 = 0;
        *y1 = plate_h - 1;
    }

    cnt = 0u;
    *change_h = 0;

    for (w = plate_w >> 3; w <= plate_w - (plate_w >> 3); w++)
    {
        for (h = *y0; h < *y1 - 1; h++)
        {
            ptr = bina_img + plate_w * h + w;

            if (*(ptr) != *(ptr + plate_h))
            {
                (*change_h)++;
            }
        }

        cnt++;
    }

    if (cnt == 0u)
    {
        *change_h = 0;
    }
    else
    {
        *change_h = (*change_h) / cnt;
    }

    (*change_w) = (change_thresh * 2);

    return (change_thresh * 2);
}

// 上下边界的粗定位
static int32_t plate_upper_and_lower_locating(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
                                              int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t change;
    int32_t change_thresh;
    uint8_t *restrict ptr = NULL;

    // 统计中间5行的跳变数作为阈值
    change = 0;

    if (plate_h < 10)
    {
        *y0 = 0;
        *y1 = plate_h - 1;

        return 0;
    }

    for (h = plate_h / 2 - 4; h <= plate_h / 2 + 4; h++)
    {
        ptr = bina_img + plate_w * h;

        for (w = 0; w < plate_w - 1; w++)
        {
            if (*(ptr + w) != *(ptr + w + 1))
            {
                change++;
            }
        }
    }

    change_thresh = (change / 9) / 2;

//  printf("-------------------------------------%d\n", change_thresh);

    // 从中间往上扫描得到上边界
    *y0 = 0;

    for (h = plate_h / 2 - 3; h >= 0; h--)
    {
        change = 0;
        ptr = bina_img + plate_w * h;

        for (w = 0; w < plate_w - 1; w++)
        {
            if (*(ptr + w) != *(ptr + w + 1))
            {
                change++;
            }
        }

        // 跳变数小于阈值则找到了车牌的上边界
        if (change < change_thresh)
        {
            *y0 = h;
            break;
        }
    }

    // 从中间往下扫描得到下边界
    *y1 = plate_h - 1;

    for (h = plate_h / 2 + 3; h < plate_h; h++)
    {
        change = 0;
        ptr = bina_img + plate_w * h;

        for (w = 0; w < plate_w - 1; w++)
        {
            if (*(ptr + w) != *(ptr + w + 1))
            {
                change++;
            }
        }

        // 跳变数小于阈值则找到了车牌的下边界
        if (change < change_thresh)
        {
            *y1 = h;
            break;
        }
    }

    return (change_thresh * 2);
}

// 通过跳变数去除一部分伪车牌区域
void remove_non_plate_region(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                             Rects *restrict rect_cand, int32_t *rect_num)
{
    int32_t n;
    int32_t num;
    int32_t area;
    int32_t w, h;
    int32_t ch0;
    int32_t ch1;
    int32_t ch2;
    int32_t xextend;
    int32_t yextend;
    int32_t cand_w, cand_h;
    int32_t top, bottom;
    int32_t ptr_line_h0;
    int32_t ptr_line_h1;
    int32_t ptr_line_h2;
    int32_t x0, x1, y0, y1;
    int32_t chg_w, chg_h;
    int32_t area_max;
    int32_t cnt_cand;
    int32_t change_w[80];/*RECT_MAX*/
    int32_t change_h[80];/*RECT_MAX*/
    int32_t change_w_min;
    int32_t change_w_max;
    int32_t change_h_min;
    int32_t change_h_max;
    uint8_t *restrict cand_img = NULL;
    uint8_t *restrict bina_img = NULL;

    num = 0;
    area_max = 0;

    for (n = 0; n < *rect_num; n++)
    {
        x0 = rect_cand[n].x0;
        x1 = rect_cand[n].x1;
        y0 = rect_cand[n].y0;
        y1 = rect_cand[n].y1;

        cand_w = x1 - x0 + 1;
        cand_h = y1 - y0 + 1;

        area = cand_w * cand_h;

        if (area < 100)
        {
            continue;
        }

        if (cand_w > cand_h * 10)
        {
            continue;
        }

        area_max = (area > area_max) ? area : area_max;

        rect_cand[num].x0 = (int16_t)x0;
        rect_cand[num].x1 = (int16_t)x1;
        rect_cand[num].y0 = (int16_t)y0;
        rect_cand[num].y1 = (int16_t)y1;
        num++;
    }

    cnt_cand = num;

    CHECKED_MALLOC(cand_img, sizeof(uint8_t) * area_max, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * area_max, CACHE_SIZE);

    num = 0;

    for (n = 0; n < cnt_cand; n++)
    {
        x0 = rect_cand[n].x0;
        x1 = rect_cand[n].x1;
        y0 = rect_cand[n].y0;
        y1 = rect_cand[n].y1;

        cand_w = x1 - x0 + 1;
        cand_h = y1 - y0 + 1;

        area = cand_w * cand_h;

        for (h = y0; h <= y1; h++)
        {
            memcpy(cand_img + cand_w * (h - y0), gray_img + img_w * h + x0, cand_w);
        }

        thresholding_percent(cand_img, bina_img, cand_w, cand_h,
                             cand_w / 3, cand_w - cand_w / 3, cand_h / 3, cand_h - cand_h / 3, 50);

        // 通过统计二值图像中h/3，h/2，2h/3处水平方向跳变次数来排除一部分候选区域
        ch0 = 0;
        ch1 = 0;
        ch2 = 0;
        ptr_line_h0 = cand_w * ((int32_t)(cand_h * 0.444));
        ptr_line_h1 = cand_w * ((int32_t)(cand_h * 0.500));
        ptr_line_h2 = cand_w * ((int32_t)(cand_h * 0.555));

        for (w = 1; w < cand_w; w++)
        {
            ch0 += (bina_img[ptr_line_h0 + w] != bina_img[ptr_line_h0 + w - 1]);
            ch1 += (bina_img[ptr_line_h1 + w] != bina_img[ptr_line_h1 + w - 1]);
            ch2 += (bina_img[ptr_line_h2 + w] != bina_img[ptr_line_h2 + w - 1]);
        }

        if ((ch0 > 10) + (ch1 > 10) + (ch2 > 10) < 2)
        {
            continue;
        }

        // 通过统计二值图像中间5行的跳变数来排除一部分候选区域
        plate_upper_and_lower_locating_new(bina_img, &top, &bottom, cand_w, cand_h, &chg_w, &chg_h);

        //         if (chg_w < 13 || chg_w > 35 || chg_h > 25 || chg_h < 5)
        //         {
        //             continue;
        //         }

        if ((bottom - top + 1) > 90)
        {
            continue;
        }

        change_w[num] = chg_w;
        change_h[num] = chg_h;

        xextend = 10;
        yextend = 8;

        rect_cand[num].x0 = (int16_t)MAX2((x0 - xextend), 0);
        rect_cand[num].x1 = (int16_t)MIN2((x1 + xextend), (img_w - 1));
        rect_cand[num].y0 = (int16_t)MAX2((y0 + top - yextend), 0);
        rect_cand[num].y1 = (int16_t)MIN2((y0 + bottom + yextend), (img_h - 1));
        num++;
    }

    cnt_cand = num;

    CHECKED_FREE(cand_img, sizeof(uint8_t) * area_max);
    CHECKED_FREE(bina_img, sizeof(uint8_t) * area_max);

    cand_img = NULL;
    bina_img = NULL;

    // 当区域数量超过MAX_CAND_NUM个时，保留至多NOMAL_CAND_NUM个区域
    change_w_min =  8;
    change_w_max =  40;
    change_h_min =  1;
    change_h_max =  30;

    if (cnt_cand >= MAX_CAND_NUM)
    {
        // 统计
        while (num > NOMAL_CAND_NUM)
        {
            change_w_min++;
            change_w_max--;
            change_h_min++;
            change_h_max--;

            num = 0;

            for (n = 0; n < cnt_cand; n++)
            {
                num += (change_w[n] >= change_w_min) && (change_w[n] <= change_w_max)
                       && (change_h[n] >= change_h_min) && (change_h[n] <= change_h_max);
            }
        }

        // 删除
        num = 0;

        for (n = 0; n < cnt_cand; n++)
        {
            if ((change_w[n] >= change_w_min) && (change_w[n] <= change_w_max)
                && (change_h[n] >= change_h_min) && (change_h[n] <= change_h_max))
            {
                rect_cand[num].x0 = rect_cand[n].x0;
                rect_cand[num].x1 = rect_cand[n].x1;
                rect_cand[num].y0 = rect_cand[n].y0;
                rect_cand[num].y1 = rect_cand[n].y1;
                num++;
            }
        }

        cnt_cand = num;
    }

    *rect_num = cnt_cand;

    //printf(" !!!cnt_cand = %4d\n ", cnt_cand);

    return;
}

// 通过垂直边缘密度定位左右边界
static int32_t plate_left_and_right_locating_by_edge_density(uint8_t *restrict cand_img,
                                                             int32_t cand_w, int32_t cand_h, int32_t *x0, int32_t *x1)
{
    int32_t w, h;
    int32_t max, k;
    int32_t sum;
    int32_t n;
    int32_t plate_w; // 通过车牌高度估算的车牌宽度
    int32_t *restrict proj_img = NULL;
    uint8_t *restrict edge_img = NULL;
    uint8_t *restrict bina_img = NULL;

//  if (cand_w < (int32_t)(cand_h * 5.5f))
    if (cand_w < (cand_h * 5 + cand_h / 2))
    {
        *x0 = 0;
        *x1 = cand_w - 1;

        return 0;
    }

    CHECKED_MALLOC(edge_img, sizeof(uint8_t) * cand_w * cand_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * cand_w * cand_h, CACHE_SIZE);
    CHECKED_MALLOC(proj_img, sizeof(int32_t) * cand_w, CACHE_SIZE);

    vertical_sobel_simple(cand_img, edge_img, cand_w, cand_h);

//  thresholding_percent(edge_img, bina_img, cand_w, cand_h, 0, cand_w - 1, 0, cand_h - 1, 20);
    thresholding_percent_all(edge_img, bina_img, cand_w, cand_h, 20);

    // 边缘图像向水平方向投影
    max = 0;

    for (w = 0; w < cand_w; w++)
    {
        proj_img[w] = 0;

        for (h = 0; h < cand_h; h++)
        {
            proj_img[w] += (bina_img[cand_w * h + w] > (uint8_t)0);
        }

        max = MAX2(max, proj_img[w]);
    }

    // 通过车牌高度估算出车牌的大致宽度
//  plate_w = (int32_t)(cand_h * 4.5f);
    plate_w = (cand_h << 2) + (cand_h >> 1);

    // 寻找垂直边缘最多的区域作为车牌所在的区域
    sum = 0;

    for (n = 0; n < plate_w; n++)
    {
        sum += proj_img[n];
    }

    max = sum;
    k = 0;

    for (n = 1; n < cand_w - plate_w; n++)
    {
        sum += proj_img[n + plate_w - 1];
        sum -= proj_img[n - 1];

        if (sum > max)
        {
            max = sum;
            k = n;
        }
    }

    *x0 = k;
    *x1 = k + plate_w - 1;

    *x0 = MAX2(*x0 - cand_h, 0);
    *x1 = MIN2(*x1 + cand_h, cand_w - 1);

    CHECKED_FREE(proj_img, sizeof(int32_t) * cand_w);
    CHECKED_FREE(bina_img, sizeof(uint8_t) * cand_w * cand_h);
    CHECKED_FREE(edge_img, sizeof(uint8_t) * cand_w * cand_h);

    return 1;
}

// 通过灰度图像投影定位左右边界
static void plate_left_and_right_locating_by_projection(uint8_t *restrict gray,
                                                        int32_t img_w, int32_t img_h, int32_t *x0, int32_t *x1)
{
    int32_t w, h;
    int32_t i, j;
    int32_t k;
    int32_t n;
    uint8_t *restrict gray_img = NULL;
    int32_t *restrict proj_hist_orig = NULL;
    int32_t *restrict proj_hist_sort = NULL;
    int32_t left, right;
    int32_t peak, valley;
    int32_t thresh;

#if SHOW_HIST
    IplImage* hist = cvCreateImage(cvSize(img_w, 50), IPL_DEPTH_8U, 3);
    cvZero(hist);
#endif

    CHECKED_MALLOC(gray_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(proj_hist_orig, sizeof(int32_t) * img_w, CACHE_SIZE);
    CHECKED_MALLOC(proj_hist_sort, sizeof(int32_t) * img_w, CACHE_SIZE);

    // 对比度拉伸
    contrast_extend_for_gary_image(gray_img, gray, img_w, img_h);

    // 灰度值投影
    for (w = 0; w < img_w; w++)
    {
        proj_hist_orig[w] = 0;

        for (h = 0; h < img_h; h++)
        {
            proj_hist_orig[w] += (int32_t)gray_img[img_w * h + w];
        }

        proj_hist_orig[w] = proj_hist_orig[w] / img_h;
    }

#if SHOW_HIST

    for (w = 0; w < img_w; w++)
    {
        for (h = 0; h < proj_hist_orig[w] * 50 / 255; h++)
        {
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 0] = (uint8_t)0;
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 1] = (uint8_t)255;
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 2] = (uint8_t)0;
        }
    }

#endif

    memcpy(proj_hist_sort, proj_hist_orig, sizeof(int32_t) * img_w);

    // sort
    for (i = 0; i < img_w; i++)
    {
        for (j = i + 1; j < img_w; j++)
        {
            if (proj_hist_sort[i] < proj_hist_sort[j])
            {
                k = proj_hist_sort[i];
                proj_hist_sort[i] = proj_hist_sort[j];
                proj_hist_sort[j] = k;
            }
        }
    }

    // 确定阈值
    peak = 0;
    valley = 0;

    for (k = 0; k < img_w / 10; k++)
    {
        peak += proj_hist_sort[k];
    }

    peak /= (img_w / 10);

    for (k = img_w - 1; k >= img_w - img_w / 10; k--)
    {
        valley += proj_hist_sort[k];
    }

    valley /= (img_w / 10);
    thresh = (peak - valley) / 4;

    left = 0;

    // 寻找左边界拐点
    for (n = 4; n < img_w / 2; n++)
    {
        if (abs(proj_hist_orig[n] - proj_hist_orig[n - 2]) > thresh)
        {
            left += n;
            break;
        }

        if (abs(proj_hist_orig[n] - proj_hist_orig[n - 4]) > thresh)
        {
            left += n;
            break;
        }
    }

    right = img_w - 1;

    // 寻找右边界拐点
    for (n = img_w - 1; n >= 4; n--)
    {
        if (abs(proj_hist_orig[n - 2] - proj_hist_orig[n]) > thresh)
        {
            right -= ((img_w - 1) - (n - 1));
            break;
        }

        if (abs(proj_hist_orig[n - 4] - proj_hist_orig[n]) > thresh)
        {
            right -= ((img_w - 1) - (n - 1));
            break;
        }
    }

#if SHOW_HIST
    cvLine(hist,
           cvPoint(left, 0),
           cvPoint(left, hist->height - 1),
           cvScalar(0, 0, 255, 0), 2, 1, 0);

    cvLine(hist,
           cvPoint(right, 0),
           cvPoint(right, hist->height - 1),
           cvScalar(0, 0, 255, 0), 2, 1, 0);

    cvNamedWindow("hist", CV_WINDOW_AUTOSIZE);
    cvShowImage("hist", hist);

    cvWaitKey(0);

    cvReleaseImage(&hist);
    cvDestroyWindow("hist");
#endif

    if (right <= left)
    {
        *x0 = 0;
        *x1 = img_w - 1;
    }
    else
    {
        *x0 = left;
        *x1 = right;
    }

    CHECKED_FREE(proj_hist_sort, sizeof(int32_t) * img_w);
    CHECKED_FREE(proj_hist_orig, sizeof(int32_t) * img_w);
    CHECKED_FREE(gray_img, sizeof(uint8_t) * img_w * img_h);

    return;
}

// 结合边缘密度和投影两种方法定位车牌左右边界
static void plate_left_and_right_locating(uint8_t *restrict gray_img, int32_t *x0, int32_t *x1,
                                          int32_t img_w, int32_t img_h)
{
    int32_t h;
    int32_t sub_w;
    int32_t sub_h;
    int32_t xx0, xx1;
    uint8_t *restrict sub_img = NULL;

    plate_left_and_right_locating_by_edge_density(gray_img, img_w, img_h, &xx0, &xx1);

    *x0 = xx0;
    *x1 = xx1;

    sub_w = xx1 - xx0 + 1;
    sub_h = img_h;

    CHECKED_MALLOC(sub_img, sizeof(uint8_t) * sub_w * sub_h, CACHE_SIZE);

    for (h = 0; h < img_h; h++)
    {
        memcpy(sub_img + sub_w * h, gray_img + img_w * h + xx0, sub_w);
    }

    plate_left_and_right_locating_by_projection(sub_img, sub_w, sub_h, &xx0, &xx1);

    *x1 = *x0 + xx1;
    *x0 += xx0;

    CHECKED_FREE(sub_img, sizeof(uint8_t) * sub_w * sub_h);

    return;
}

// 根据字符的比划宽度进行二值化，然后再定位车牌的上下边界
static void plate_upper_and_lower_locating_by_stroke(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                                     int32_t *y0, int32_t *y1, int32_t plate_w, int32_t plate_h)
{
#if 0
    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别有错的车牌图片数：1438
    thresholding_stroke_width(gray_img, bina_img, plate_w, plate_h, plate_w / 7, plate_w - plate_w / 7);
#else
    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别有错的车牌图片数：1478
    thresholding_avg(gray_img, bina_img, plate_w, plate_h, plate_w / 7, plate_w - plate_w / 7, 0, plate_h - 1);

    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别写淼某蹬仆计?484
//     thresholding_avg(gray_img, bina_img, plate_w, plate_h,
//         plate_w / 7, plate_w - plate_w / 7,
//         plate_h / 7, plate_h - plate_h / 7);

    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别有错的车牌图片数：1550
//     thresholding_avg(gray_img, bina_img, plate_w, plate_h, 0, plate_w - 1, 0, plate_h - 1);

    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别有错的车牌图片数：1773
//    thresholding_percent_all(gray_img, bina_img, plate_w, plate_h, 20);
    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别有错的车牌图片数：1522
//    thresholding_percent_all(gray_img, bina_img, plate_w, plate_h, 25);
    // 在车牌连线3个尺度的情况下，车牌图片总数：23015 没有识别到或者识别有错的车牌图片数：1537
//    thresholding_percent_all(gray_img, bina_img, plate_w, plate_h, 30);
#endif

    plate_upper_and_lower_locating(bina_img, y0, y1, plate_w, plate_h);

    return;
}

// 确定车牌是黑底白字还是白底黑字(0:黑底白字 1:白底黑字 255:不确定)
static int32_t get_plate_background_color(uint8_t *restrict gray, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    uint8_t *restrict gray_img = NULL;
    uint8_t *restrict bina_img = NULL;
    int32_t *restrict hist_hor = NULL;
    int32_t peak;
    int32_t thresh;
    int32_t black_cnt;
    int32_t white_cnt;
    int32_t r; // 改用整型

#if SHOW_HIST_BG
    IplImage *hist = cvCreateImage(cvSize(img_w, 50), IPL_DEPTH_8U, 3);
    IplImage *image = cvCreateImage(cvSize(img_w, img_h), IPL_DEPTH_8U, 1);

    cvZero(hist);

    for (h = 0; h < img_h; h++)
    {
        memcpy(image->imageData + image->widthStep * h, gray + img_w * h, img_w);
    }

#endif

    if (img_h < 5 || img_h > 255)
    {
        return -1;
    }

    CHECKED_MALLOC(gray_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(hist_hor, sizeof(int32_t) * img_w, CACHE_SIZE);

    contrast_extend_for_gary_image(gray_img, gray, img_w, img_h);

    thresholding_avg(gray_img, bina_img, img_w, img_h,
                     img_w / 5, img_w - img_w / 5, img_h / 5, img_h - img_h / 5);

    peak = 0;

    for (w = 0; w < img_w; w++)
    {
        hist_hor[w] = 0;

        for (h = img_h / 5; h < img_h - img_h / 5; h++)
        {
            hist_hor[w] += (bina_img[img_w * h + w] > 0u);
        }

        peak = hist_hor[w] > peak ? hist_hor[w] : peak;
    }

    thresh = peak / 2;

    for (w = 0; w < img_w; w++)
    {
        if (hist_hor[w] > thresh)
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

    // 改用整型计算，避免PC与DSP浮点计算精度上的差异
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
    CHECKED_FREE(bina_img, sizeof(uint8_t) * img_w * img_h);
    CHECKED_FREE(gray_img, sizeof(uint8_t) * img_w * img_h);

    // 改用整型计算，避免PC与DSP浮点计算精度上的差异
    if (r < 45)
    {
        return 0;
    }
    else if (r > 65)
    {
        return 1;
    }
    else
    {
        return 255;
    }
}

// 车牌候选区域的精确定位，当候选区域满足车牌大致的尺寸要求时得到更精确的车牌位置信息并返回1，否则返回0
static int32_t plate_locating_for_single_candidate(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                                                   Rects *restrict pos, uint8_t *restrict color)
{
    int32_t h;
    int32_t sub_w, sub_h;
    int32_t y0, y1, x0, x1;
    int32_t yy0, yy1;
    uint8_t *restrict sub_img = NULL;
    uint8_t *restrict bina_img = NULL;

    if (img_w < CAND_MIN_W || img_h < CAND_MIN_H)
    {
        return 0;
    }

    pos->x0 = (int16_t)0;
    pos->x1 = (int16_t)(img_w - 1);
    pos->y0 = (int16_t)0;
    pos->y1 = (int16_t)(img_h - 1);

    CHECKED_MALLOC(sub_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);

    // 如果车牌明显宽度太大，首先粗略定位车牌的左右边界
    if (plate_left_and_right_locating_by_edge_density(gray_img, img_w, img_h, &x0, &x1))
    {
        sub_w = x1 - x0 + 1;
        sub_h = img_h;

        for (h = 0; h < img_h; h++)
        {
            memcpy(sub_img + sub_w * h, gray_img + img_w * h + x0, sub_w);
        }

        // 粗略定位车牌的上下边界
        plate_upper_and_lower_locating_by_stroke(sub_img, bina_img, &y0, &y1, sub_w, sub_h);
    }
    else
    {
        x0 = 0;
        x1 = img_w - 1;

        // 粗略定位车牌的上下边界
        plate_upper_and_lower_locating_by_stroke(gray_img, bina_img, &y0, &y1, img_w, img_h);
    }

    pos->x1 = (int16_t)(pos->x0 + x1);
    pos->x0 = (int16_t)(pos->x0 + x0);

    sub_w = x1 - x0 + 1;
    sub_h = y1 - y0 + 1;

    if (sub_w < CAND_MIN_W || sub_h < CAND_MIN_H)
    {
        CHECKED_FREE(bina_img, sizeof(uint8_t) * img_w * img_h);
        CHECKED_FREE(sub_img, sizeof(uint8_t) * img_w * img_h);
        return 0;
    }

    // 左右边界精确定位：结合车牌的固有尺寸和边缘进行左右精确定位
    for (h = 0; h < sub_h; h++)
    {
        memcpy(sub_img + sub_w * h, gray_img + img_w * (y0 + h) + x0, sub_w);
    }

    plate_left_and_right_locating(sub_img, &x0, &x1, sub_w, sub_h);

    pos->x1 = (int16_t)(pos->x0 + x1);
    pos->x0 = (int16_t)(pos->x0 + x0);
    pos->y1 = (int16_t)(pos->y0 + y1);
    pos->y0 = (int16_t)(pos->y0 + y0);

    // 车牌上下边界的精确定位
    sub_w = pos->x1 - pos->x0 + 1;
    sub_h = pos->y1 - pos->y0 + 1;

    if (sub_w < CAND_MIN_W || sub_h < CAND_MIN_H)
    {
        CHECKED_FREE(bina_img, sizeof(uint8_t) * img_w * img_h);
        CHECKED_FREE(sub_img, sizeof(uint8_t) * img_w * img_h);

        return 0;
    }

    for (h = 0; h < sub_h; h++)
    {
        memcpy(sub_img + sub_w * h, gray_img + img_w * (pos->y0 + h) + pos->x0, sub_w);
    }

    // 粗略定位车牌的上下边界
    plate_upper_and_lower_locating_by_stroke(sub_img, bina_img, &yy0, &yy1, sub_w, sub_h);

    // 判断车牌底色
    *color = (uint8_t)get_plate_background_color(sub_img, sub_w, sub_h);

    pos->y1 = (int16_t)(pos->y0 + yy1);
    pos->y0 = (int16_t)(pos->y0 + yy0);

    pos->x0 = (int16_t)MAX2(pos->x0 - sub_h / 4, 0);
    pos->x1 = (int16_t)MIN2(pos->x1 + sub_h / 4, img_w - 1);

    if (*color == 1) // 如果是白底黑字
    {
        pos->y0 = (int16_t)MAX2(pos->y0 - 1, 0);
        pos->y1 = (int16_t)MIN2(pos->y1 + 1, img_h - 1);
    }
    else
    {
        pos->y0 = (int16_t)MAX2(pos->y0 - 1, 0);
        pos->y1 = (int16_t)MIN2(pos->y1 + 1, img_h - 1);
    }

    CHECKED_FREE(bina_img, sizeof(uint8_t) * img_w * img_h);
    CHECKED_FREE(sub_img, sizeof(uint8_t) * img_w * img_h);

    // 车牌宽高比小于2，认为是伪车牌区域，测试了6000帧左右，8949个车牌候选区域中去除了693个伪车牌，未去除真正的车牌
    if ((float)sub_w / sub_h < 2.0)
    {
        return 0;
    }

    return 1;
}

#define DEBUG_MODULE_INFOMATION 0

// 精确定位候选区域，并根据车牌的大致尺寸排除一部分候选区域
void plate_locating_for_all_candidates(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                                       Rects *restrict rect_cand, int32_t cand_num,
                                       Rects *restrict rect_real, int32_t *restrict real_num,
                                       uint8_t *restrict color)
{
    int32_t c = 0; // 候选区域的循环变量
    int32_t r = 0; // 候选区域中满足车牌大致尺寸要求的区域数量
    int32_t y;
    uint8_t *restrict plate_img = NULL;
    int32_t plate_w, plate_h;
    int32_t real_flg = 0; // 候选区域是否满足车牌大致的尺寸要求
    Rects pos = {0, 0, 0, 0};

#if DEBUG_MODULE_INFOMATION
    // 打印定位模块输出信息
    {
        int32_t i;
        FILE* fmodule_info_out = fopen("module_info_out.txt", "a+");

        fprintf(fmodule_info_out, "======================================\n");
        fprintf(fmodule_info_out, "plate_locating_for_all_candidates() cand_num = %d\n", cand_num);

        for (c = 0; c < cand_num; c++)
        {
            fprintf(fmodule_info_out, "Rect%d: %d, %d, %d, %d\n", c,
                    rect_cand[c].x0, rect_cand[c].x1, rect_cand[c].y0, rect_cand[c].y1);
        }

        fprintf(fmodule_info_out, "======================================\n");

        fclose(fmodule_info_out);
    }
#endif

    for (c = 0; c < cand_num; c++)
    {
        plate_w = rect_cand[c].x1 - rect_cand[c].x0 + 1;
        plate_h = rect_cand[c].y1 - rect_cand[c].y0 + 1;

        CHECKED_MALLOC(plate_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);

        for (y = rect_cand[c].y0; y <= rect_cand[c].y1; y++)
        {
            memcpy(plate_img + plate_w * (y - rect_cand[c].y0), gray_img + img_w * y + rect_cand[c].x0, plate_w);
        }

#if 0
        {
            int32_t w, h;
            IplImage* image = cvCreateImage(cvSize(img_w, img_h), IPL_DEPTH_8U, 3);

            cvZero(image);

            _IMAGE3_(image, gray_img, img_w, img_h, 0);

            cvRectangle(image,
                        cvPoint(rect_cand[c].x0, rect_cand[c].y0),
                        cvPoint(rect_cand[c].x1, rect_cand[c].y1),
                        cvScalar(0, 0, 255, 0), 2, 1, 0);

            cvNamedWindow("image", CV_WINDOW_AUTOSIZE);
            cvShowImage("image", image);

            cvWaitKey(0);

            cvReleaseImage(&image);
            cvDestroyWindow("image");
        }
#endif

        real_flg = plate_locating_for_single_candidate(plate_img, plate_w, plate_h, &pos, &color[c]);

        if (real_flg) // 该候选区域满足车牌大致尺寸要求，将送到字符分割、识别模块进行车牌识别
        {
            rect_cand[c].x1 = (int16_t)(rect_cand[c].x0 + pos.x1);
            rect_cand[c].x0 = (int16_t)(rect_cand[c].x0 + pos.x0);
            rect_cand[c].y1 = (int16_t)(rect_cand[c].y0 + pos.y1);
            rect_cand[c].y0 = (int16_t)(rect_cand[c].y0 + pos.y0);

            if ((rect_cand[c].x1 - rect_cand[c].x0 > 0)
                && (rect_cand[c].y1 - rect_cand[c].y0 > 0))
            {
                rect_real[r] = rect_cand[c];
                color[r] = color[c];
                r++;
            }
        }

        CHECKED_FREE(plate_img, sizeof(uint8_t) * plate_w * plate_h);
    }

    *real_num = r;

    return;
}
