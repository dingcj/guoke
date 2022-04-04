#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../portab.h"
#include "../malloc_aligned.h"
#include "plate_runs.h"
#include "plate_mask.h"
#include "plate_double_decide.h"
#include "plate_location.h"
#include "../common/threshold.h"
#include "../common/image.h"
#include "../time_test.h"
#include "../debug_show_image.h"

#ifdef  WIN32
#include "..\ti64_c\cintrins.h"
#include "..\ti64_c\c64intrins.h"
#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include "opencv/highgui.h"
#endif

#define DEBUG_DELET_PLATE 0

// 创建车牌检测句柄
void lc_plate_location_create(Loc_handle *loc_handle, int32_t img_w, int32_t img_h)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    CHECKED_MALLOC(loc, sizeof(PlateLocation), CACHE_SIZE);
    memset(loc, 0, sizeof(PlateLocation));

    loc->img_w = img_w;
    loc->img_h = img_h;

#ifdef USE_HAAR_DETECTION
    CHECKED_MALLOC(loc->cascade, sizeof(Cascade), CACHE_SIZE);
    load_classifier(loc->cascade, haar_param);
#endif

    CHECKED_MALLOC(loc->edge_img, sizeof(uint8_t) * img_h / 2 * img_w, CACHE_SIZE);
    CHECKED_MALLOC(loc->bina_img, sizeof(uint8_t) * img_h / 2 * img_w, CACHE_SIZE);

    *loc_handle = (Loc_handle)loc;

    return;
}

void lc_plate_location_recreate(Loc_handle loc_handle, int32_t img_w, int32_t img_h)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        CHECKED_FREE(loc->edge_img, sizeof(uint8_t) * img_h / 2 * img_w);
        CHECKED_FREE(loc->bina_img, sizeof(uint8_t) * img_h / 2 * img_w);

        loc->img_w = img_w;
        loc->img_h = img_h;

        loc->rect_detect.x0 = (int16_t)0;
        loc->rect_detect.x1 = (int16_t)(img_w - 1);
        loc->rect_detect.y0 = (int16_t)((img_h * 1) / 10);
        loc->rect_detect.y1 = (int16_t)((img_h * 9) / 10);

        CHECKED_MALLOC(loc->edge_img, sizeof(uint8_t) * img_h / 2 * img_w, CACHE_SIZE);
        CHECKED_MALLOC(loc->bina_img, sizeof(uint8_t) * img_h / 2 * img_w, CACHE_SIZE);
    }

    return;
}

// 撤销车牌检测句柄
void lc_plate_location_destroy(Loc_handle loc_handle, int32_t img_w, int32_t img_h)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
#ifdef USE_HAAR_DETECTION
        destroy_classifier(loc->cascade);
        CHECKED_FREE(loc->cascade);
#endif
        CHECKED_FREE(loc->edge_img, sizeof(uint8_t) * img_h / 2 * img_w);
        CHECKED_FREE(loc->bina_img, sizeof(uint8_t) * img_h / 2 * img_w);
        CHECKED_FREE(loc, sizeof(PlateLocation));
        loc_handle = NULL;
    }

    return;
}

// 车牌检测默认参数设置
void lc_plate_location_param_default(Loc_handle loc_handle)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    loc->haar_enable = (uint8_t)0;
    loc->video_flag  = (uint8_t)1;

#ifdef USE_HAAR_DETECTION

    if (loc->haar_enable)
    {
        loc->scale = 1.1f;
        loc->min_neighbors = 1;
    }

#endif

//  if(loc->video_flag)
//  {
    loc->plate_w_min = 60;
    loc->plate_w_max = 180;
    loc->plate_h_min = 10;
    loc->plate_h_max = 60;
//  }
//  else
//  {
//      loc->plate_w_min = 60;
//      loc->plate_w_max = 180;
//      loc->plate_h_min = 10;
//      loc->plate_h_max = 80;
//  }

    loc->rect_detect.x0 = (int16_t)0;
    loc->rect_detect.x1 = (int16_t)(loc->img_w - 1);
    loc->rect_detect.y0 = (int16_t)((loc->img_h * 1) / 10);
    loc->rect_detect.y1 = (int16_t)((loc->img_h * 9) / 10);

    return;
}

void lc_plate_location_get_video_flag(Loc_handle loc_handle, uint8_t *video_flag)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        *video_flag = loc->video_flag;
    }

    return;
}

void lc_plate_location_set_video_flag(Loc_handle loc_handle, uint8_t video_flag)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        loc->video_flag = video_flag;
    }

    return;
}

void lc_plate_location_get_plate_width(Loc_handle loc_handle, int32_t *plate_w_min, int32_t *plate_w_max)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        *plate_w_max = loc->plate_w_max;
        *plate_w_min = loc->plate_w_min;
    }

    return;
}

void lc_plate_location_set_plate_width(Loc_handle loc_handle, int32_t plate_w_min, int32_t plate_w_max)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        loc->plate_w_min = plate_w_min;
        loc->plate_w_max = plate_w_max;
        loc->plate_h_min = plate_w_min / 7;
        loc->plate_h_max = plate_w_max / 3;
    }

    return;
}

// 获取车牌识别的检测区域
void lc_plate_location_get_rect_detect(Loc_handle loc_handle, Rects *rect_detect)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        *rect_detect = loc->rect_detect;
    }

    return;
}

// 设置车牌识别的检测区域
void lc_plate_location_set_rect_detect(Loc_handle loc_handle, Rects rect_detect)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;

    if (loc)
    {
        loc->rect_detect = rect_detect;
    }

    return;
}

void delete_non_plate(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h,
                      Rects *restrict rect_real, uint8_t *restrict color, int32_t *restrict real_num)
{
    int32_t w, h, k;
    int32_t fit_num;
    int32_t is_plate_flag;

    for (k = 0; k < *real_num; k++)
    {
        uint8_t *restrict plate_img = NULL;
        uint8_t *restrict bina_img = NULL;
        int32_t *restrict hist = NULL;
        int32_t plate_w = rect_real[k].x1 - rect_real[k].x0 + 1;
        int32_t plate_h = rect_real[k].y1 - rect_real[k].y0 + 1;
#if DEBUG_DELET_PLATE
        IplImage *plate_show = NULL;
        plate_show = cvCreateImage(cvSize(plate_w, plate_h * 4), IPL_DEPTH_8U, 1);
        cvZero(plate_show);
#endif

        CHECKED_MALLOC(plate_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
        CHECKED_MALLOC(bina_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);
        CHECKED_MALLOC(hist, plate_w * sizeof(int32_t), CACHE_SIZE);

        for (h = 0; h < plate_h; h++)
        {
            for (w = 0; w < plate_w; w++)
            {
                plate_img[plate_w * h + w] = gray_img[img_w * (h + rect_real[k].y0) + w + rect_real[k].x0];
            }
        }

        // 车牌标志预设为1
        is_plate_flag = 1;

        contrast_extend_for_gary_image(plate_img, plate_img, plate_w, plate_h);
//        thresholding_avg_all(plate_img, bina_img, plate_w, plate_h);
//        thresholding_percent_opt(plate_img, bina_img, plate_w, plate_h, 0, plate_w - 1, 0, plate_h - 1, 50);
        thresholding_by_grads(plate_img, bina_img, plate_w, plate_h);

        memset(hist, 0, plate_w * sizeof(int32_t));

        for (w = 0; w < plate_w; w++)
        {
            for (h = plate_h / 6; h < plate_h - plate_h / 6; h++)
            {
                hist[w] += bina_img[plate_w * h + w] > 0u;
            }
        }

        fit_num = 0;

        for (w = 0; w < plate_w - 1; w++)
        {
            if ((hist[w] > plate_h / 3 && hist[w + 1] <= plate_h / 3)
                || (hist[w] <= plate_h / 3 && hist[w + 1] > plate_h / 3))
            {
                fit_num++;
            }
        }

        if (fit_num < 10)    // 垂直投影跳变数去除伪车牌
        {
            // 确定为非车牌，标志设置为0
            is_plate_flag = 0;
        }
//         else if (fit_num > 40)
//         {
//             is_plate_flag = 0;
//         }
        else                  // 垂直投影相隔列差去除伪车牌
        {
            fit_num = 0;

            for (w = 0; w < plate_w - 2; w++)
            {
                if (abs(hist[w] - hist[w + 2]) > plate_h * 2 / 9)
                {
                    fit_num++;
                }
            }

            if (fit_num < 6)
            {
                // 确定为非车牌，标志设置为0
                is_plate_flag = 0;
            }
            else              // 灰度值投影法去除伪车牌
            {
                int32_t hist_avg = 0;

                fit_num = 0;
                memset(hist, 0, plate_w * sizeof(int32_t));

                for (w = 0; w < plate_w; w++)
                {
                    for (h = 0; h < plate_h; h++)
                    {
                        hist[w] += (int32_t)plate_img[plate_w * h + w];
                    }
                }

                for (w = 0; w < plate_w; w++)
                {
                    hist_avg += hist[w];
                }

                hist_avg /= plate_w;

                for (w = 0; w < plate_w - 1; w++)
                {
                    if ((hist[w] > hist_avg && hist[w + 1] < hist_avg)
                        || (hist[w] < hist_avg && hist[w + 1] > hist_avg))
                    {
                        fit_num++;
                    }
                }

                if (fit_num < 12)
                {
                    is_plate_flag = 0;
                }
            }
        }

        // 判断为非车牌，删除此非车牌信息
        if (is_plate_flag == 0)
        {
            for (w = k + 1; w < *real_num; w++)
            {
                rect_real[w - 1] = rect_real[w];
                color[w - 1] = color[w];
            }

            (*real_num)--;
            k--;
        }

#if DEBUG_DELET_PLATE
        _IMAGE1_(plate_show, plate_img, plate_w, plate_h, 0);
        _IMAGE1_(plate_show, bina_img, plate_w, plate_h, plate_h);

        for (w = 0; w < plate_w; w++)
        {
            for (h = plate_h - hist[w] / 255; h < plate_h; h++)
            {
                plate_show->imageData[plate_show->widthStep * (h + plate_h * 2) + w] = 255u;
            }
        }

        cvNamedWindow("show", CV_WINDOW_AUTOSIZE);
        cvShowImage("show", plate_show);
        cvWaitKey(0);
        cvDestroyWindow("show");
        cvReleaseImage(&plate_show);
#endif

        CHECKED_FREE(hist, plate_w * sizeof(int32_t));
        CHECKED_FREE(bina_img, plate_w * plate_h * sizeof(uint8_t));
        CHECKED_FREE(plate_img, plate_w * plate_h * sizeof(uint8_t));
    }
}

// 水平方向腐蚀操作，用于去除噪点
#ifndef PLATE_RUN_BIT
static void image_erosion(uint8_t *restrict bina_img, uint8_t *restrict half_img, int32_t img_w, Rects rect_detect)
{
    int32_t k = 0;
    int32_t i = 0;
    uint8_t pix01, pix02, pix03, pix04, pix05, pix06, pix07, pix08, pix09;
    uint8_t pix10, pix11, pix12, pix13, pix14, pix15, pix16, pix17, pix18;
    uint8_t pix_tmp;
    uint8_t *restrict img_pos0 = NULL;
    //    uint8_t *restrict img_pos1 = NULL;
    int32_t rect_area = (rect_detect.x1 - rect_detect.x0 + 1) * (rect_detect.y1 - rect_detect.y0 + 1);

    if (rect_area > 704 * 576 * 3 / 2)
    {
        // 水平方向腐蚀操作，用于去除噪点
        k = (((rect_detect.y1 + 1) >> 1) - ((rect_detect.y0 + 1) >> 1)) * img_w - 16;

        img_pos0 = bina_img + img_w * ((rect_detect.y0 + 1) >> 1);
        //        img_pos1 = half_img + img_w * ((rect_detect.y0 + 1) >> 1);

        pix_tmp = img_pos0[0];

        for (i = 0; i < k; i += 16)
        {
            pix18 = img_pos0[i + 17];    // 预取数据可提高速度

            //            pix01 = img_pos0[i +  0];
            pix01 = pix_tmp;
            pix02 = img_pos0[i +  1];
            pix03 = img_pos0[i +  2];
            pix04 = img_pos0[i +  3];
            pix05 = img_pos0[i +  4];
            pix06 = img_pos0[i +  5];
            pix07 = img_pos0[i +  6];
            pix08 = img_pos0[i +  7];
            pix09 = img_pos0[i +  8];
            pix10 = img_pos0[i +  9];
            pix11 = img_pos0[i + 10];
            pix12 = img_pos0[i + 11];
            pix13 = img_pos0[i + 12];
            pix14 = img_pos0[i + 13];
            pix15 = img_pos0[i + 14];
            pix16 = img_pos0[i + 15];
            pix17 = img_pos0[i + 16];

            // 保留下次处理的首个字节
            pix_tmp = pix17;

            img_pos0[i +  1] = (uint8_t)(pix01 & pix02 & pix03);
            img_pos0[i +  2] = (uint8_t)(pix02 & pix03 & pix04);
            img_pos0[i +  3] = (uint8_t)(pix03 & pix04 & pix05);
            img_pos0[i +  4] = (uint8_t)(pix04 & pix05 & pix06);
            img_pos0[i +  5] = (uint8_t)(pix05 & pix06 & pix07);
            img_pos0[i +  6] = (uint8_t)(pix06 & pix07 & pix08);
            img_pos0[i +  7] = (uint8_t)(pix07 & pix08 & pix09);
            img_pos0[i +  8] = (uint8_t)(pix08 & pix09 & pix10);
            img_pos0[i +  9] = (uint8_t)(pix09 & pix10 & pix11);
            img_pos0[i + 10] = (uint8_t)(pix10 & pix11 & pix12);
            img_pos0[i + 11] = (uint8_t)(pix11 & pix12 & pix13);
            img_pos0[i + 12] = (uint8_t)(pix12 & pix13 & pix14);
            img_pos0[i + 13] = (uint8_t)(pix13 & pix14 & pix15);
            img_pos0[i + 14] = (uint8_t)(pix14 & pix15 & pix16);
            img_pos0[i + 15] = (uint8_t)(pix15 & pix16 & pix17);
            img_pos0[i + 16] = (uint8_t)(pix16 & pix17 & pix18);
        }
    }

    return;
}
#else
static void image_erosion_bit(uint8_t *restrict bina_img, uint8_t *restrict half_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    uint32_t pix01, pix02;
    uint32_t pix11, pix21;
    int32_t rect_area = img_w * img_h * 2;
    int32_t wbit_num = img_w / 20;
    uint32_t *bina_img_bit = NULL;
    uint32_t *bina_img_tmp = (uint32_t *)bina_img;

    CHECKED_MALLOC(bina_img_bit, wbit_num * img_h * sizeof(uint32_t), CACHE_SIZE);

    if (rect_area > 704 * 576 * 3 / 2)
    {
        for (h = 0; h < img_h; h++)
        {
            for (w = 0; w < wbit_num - 1; w++)
            {
                pix01 = bina_img_tmp[wbit_num * h + w];
                pix02 = bina_img_tmp[wbit_num * h + w + 1];

                pix11 = (pix01 >> 1) | (pix02 << 19);
                pix21 = (pix01 >> 2) | (pix02 << 18);

                bina_img_bit[wbit_num * h + w] = (uint32_t)(pix01 & pix11 & pix21 & (uint32_t)0x000FFFFF);
            }

            pix01 = bina_img_tmp[wbit_num * h + w];
            pix11 = pix01 >> 1;
            pix21 = pix01 >> 2;

            bina_img_bit[wbit_num * h + w] = (uint32_t)(pix01 & pix11 & pix21 & 0x0003FFFF);

            w = 0;
            bina_img_tmp[wbit_num * h + w] = bina_img_bit[wbit_num * h + w] << 1;

            for (w = 1; w < wbit_num; w++)
            {
                bina_img_tmp[wbit_num * h + w] = (bina_img_bit[wbit_num * h + w - 1] >> 19)
                    | (bina_img_bit[wbit_num * h + w] << 1);
            }
        }
    }

    CHECKED_FREE(bina_img_bit, wbit_num * img_h * sizeof(uint32_t));

    return;
}
#endif

// void find_plate_runs_by_compress_image(uint8_t *gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h, int32_t img_h2,
//                                        PlateLocation *loc, LocationInfo *restrict plate, Rects *restrict rect_cand,
//                                        Rects *restrict rect_real, int32_t cand_num)
// {
//     uint8_t *bina_img_half = NULL;
//     uint8_t *gray_img_half = NULL;
//     int32_t cand_num_tmp = 0;
//     int32_t real_num_tmp = 0;
//     Rects rect_cand_tmp[RECT_MAX];
//     Rects rect_real_tmp[RECT_MAX];
//     Rects rect_double_up_tmp[RECT_MAX];
//     uint8_t plate_color_tmp[RECT_MAX];
//     uint8_t plate_double_flag_tmp[RECT_MAX];
//     int32_t w, h, k;
// 
//     CHECKED_MALLOC(bina_img_half, (img_w / 2) * (img_h2 / 2) * sizeof(uint8_t), CACHE_SIZE);
//     CHECKED_MALLOC(gray_img_half, (img_w / 2) * (img_h / 2) * sizeof(uint8_t), CACHE_SIZE);
//     memset(rect_cand_tmp, 0, RECT_MAX * sizeof(Rects));
//     memset(rect_real_tmp, 0, RECT_MAX * sizeof(Rects));
//     memset(rect_double_up_tmp, 0, RECT_MAX * sizeof(Rects));
//     memset(plate_color_tmp, 255, RECT_MAX * sizeof(uint8_t));
//     memset(plate_double_flag_tmp, 255, RECT_MAX * sizeof(uint8_t));
// 
//     for (h = 0; h < img_h / 2; h++)
//     {
//         for (w = 0; w < img_w / 2; w++)
//         {
//             gray_img_half[img_w / 2 * h + w] = gray_img[img_w * h * 2 + w * 2];
//         }
//     }
// 
//     for (h = 0; h < img_h2 / 2; h++)
//     {
//         for (w = 0; w < img_w / 2; w++)
//         {
//             bina_img_half[img_w / 2 * h + w] = bina_img[img_w * h * 2 + w * 2];
//         }
//     }
// 
//     find_plate_candidate_by_plate_runs(loc, gray_img_half, bina_img_half,
//                                        img_w / 2, img_h / 2, rect_cand_tmp, &cand_num_tmp);
// 
//     plate_locating_for_all_candidates(gray_img_half, img_w / 2, img_h / 2, rect_cand_tmp, cand_num_tmp,
//                                       rect_real_tmp, &real_num_tmp, plate_color_tmp);
// 
//     // 车牌定位完成后伪车牌去除
//     delete_non_plate(gray_img_half, img_w / 2, img_h / 2, rect_real_tmp, plate_color_tmp, &real_num_tmp);
// 
//     // 单层牌与双层牌区分
// #if USE_DOUBLE_PLATE_RECOG
//     plate_double_decide_all_candidate(gray_img_half, img_w / 2, img_h / 2, rect_real_tmp, real_num_tmp, plate_color_tmp,
//                                       plate_double_flag_tmp, rect_double_up_tmp);
// #endif
// 
//     for (k = 0; k < cand_num_tmp; k++)
//     {
//         rect_cand_tmp[k].x0 *= 2;
//         rect_cand_tmp[k].x1 *= 2;
//         rect_cand_tmp[k].y0 *= 2;
//         rect_cand_tmp[k].y1 *= 2;
//         plate->rect_cand[plate->cand_num + k] = rect_cand_tmp[k];
//     }
// 
//     for (k = 0; k < real_num_tmp; k++)
//     {
//         rect_real_tmp[k].x0 *= 2;
//         rect_real_tmp[k].x1 *= 2;
//         rect_real_tmp[k].y0 *= 2;
//         rect_real_tmp[k].y1 *= 2;
//         rect_double_up_tmp[k].x0 *= 2;
//         rect_double_up_tmp[k].x1 *= 2;
//         rect_double_up_tmp[k].y0 *= 2;
//         rect_double_up_tmp[k].y1 *= 2;
//         plate->rect_real[plate->real_num + k] = rect_real_tmp[k];
//         plate->rect_double_up[plate->real_num + k] = rect_double_up_tmp[k];
//         plate->color[plate->real_num + k] = plate_color_tmp[k];
//         plate->plate_double_flag[plate->real_num + k] = plate_double_flag_tmp[k];
//     }
// 
//     plate->real_num += real_num_tmp;
//     plate->cand_num += cand_num_tmp;
// 
//     CHECKED_FREE(bina_img_half, (img_w / 2) * (img_h2 / 2) * sizeof(uint8_t));
//     CHECKED_FREE(gray_img_half, (img_w / 2) * (img_h / 2) * sizeof(uint8_t));
//     return;
// }

// 车牌检测主函数
void lc_plate_location(Loc_handle loc_handle, uint8_t *restrict yuv_img, LocationInfo *restrict plate)
{
    PlateLocation *loc = (PlateLocation *)loc_handle;
//    int32_t k;
    const int32_t img_w = loc->img_w;
    const int32_t img_h = loc->img_h;
//    const int32_t img_h2 = loc->img_h >> 1;
    int32_t cand_num = 0;
    int32_t type = loc->video_flag;//算法模式
    Rects rect_detect = loc->rect_detect;// 待检测区域
    Rects *restrict rect_cand = plate->rect_cand;
    Rects *restrict rect_real = plate->rect_real;
//    Rects rect_detect = loc->rect_detect;// 待检测区域
    uint8_t *restrict gray_img = yuv_img;
    int32_t rect_w, rect_h, rect_h2;     // 车牌分析区域的宽和高

    // 抓拍机电警模式处理
    uint8_t *restrict half_img = NULL;
    uint8_t *restrict edge_img = NULL;
    uint8_t *restrict bina_img = NULL;

    memset(plate->save_flag, 0, sizeof(plate->save_flag));

    cand_num = 0;

    rect_w = (rect_detect.x1 - rect_detect.x0) / 2 * 2;
    rect_h = (rect_detect.y1 - rect_detect.y0) / 2 * 2;
    rect_h2 = rect_h / 2;

    if (rect_h2 * rect_w <= 0)
    {
        return;
    }

    CHECKED_MALLOC(half_img, rect_w * rect_h2 * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(edge_img, rect_w * rect_h2 * sizeof(uint8_t), CACHE_SIZE);
    CHECKED_MALLOC(bina_img, rect_w * rect_h2 * sizeof(uint8_t), CACHE_SIZE);

    // 对图像的车牌分析区域进行水平梯度计算
    CLOCK_START(vertical_sobel_simple_rect);
    vertical_sobel_simple_rect(gray_img, edge_img, img_w, img_h, rect_detect, rect_w, rect_h, rect_h2);
    CLOCK_END("vertical_sobel_simple_rect");

    // 按百分比二值化梯度图像
    CLOCK_START(thresholding_percent_opt);
#ifdef PLATE_RUN_BIT
//    thresholding_percent_opt_bits_all(edge_img, bina_img, rect_w, rect_h2, 5);
    thresholding_percent_opt_bits(edge_img, bina_img, rect_w, rect_h2, 0, rect_w - 1, 0, rect_h2 - 1, 5);
#else
    thresholding_percent_opt(edge_img, bina_img, rect_w, rect_h2, 0, rect_w - 1, 0, rect_h2 - 1, 5);
#endif
    CLOCK_END("thresholding_percent_opt");

    // 电警模式开启腐蚀操作
    CLOCK_START(image_erosion);

    if (type == 1)/*type != PLATE_ANALYSIS_PIC*/
    {
#ifdef PLATE_RUN_BIT
        image_erosion_bit(bina_img, half_img, rect_w, rect_h2);
#else
        Rects rect_erosion;
        rect_erosion.x0 = 0;
        rect_erosion.x1 = rect_w - 1;
        rect_erosion.y0 = 0;
        rect_erosion.y1 = rect_h - 1;
        image_erosion(bina_img, half_img, rect_w, rect_erosion);
#endif
    }

    CLOCK_END("image_erosion");

    // 根据车牌连线寻找所有的车牌候选区域
    CLOCK_START(find_plate_candidate_by_plate_runs);
    find_plate_candidate_by_plate_runs(loc, gray_img, bina_img, img_w, img_h,
                                       rect_w, rect_h, rect_detect, rect_cand, &cand_num);
    CLOCK_END("find_plate_candidate_by_plate_runs");

    plate->cand_num = cand_num;
    memset(plate->color, 255, cand_num);

    // 初步去除伪车牌，粗定位车牌边界
    CLOCK_START(plate_locating_for_all_candidates);
    plate_locating_for_all_candidates(gray_img, img_w, img_h, rect_cand, cand_num,
                                      rect_real, &plate->real_num, plate->color);
    CLOCK_END("plate_locating_for_all_candidates");

    // 车牌定位完成后伪车牌去除
    CLOCK_START(delete_non_plate);
    //delete_non_plate(gray_img, img_w, img_h, rect_real, plate->color, &plate->real_num);
    CLOCK_END("delete_non_plate");

    memset(plate->plate_double_flag, 0, sizeof(plate->plate_double_flag));

    // 单层牌与双层牌区分
    CLOCK_START(plate_double_decide_all_candidate);
    plate_double_decide_all_candidate(yuv_img, img_w, img_h, rect_real, plate->real_num, plate->color,
                                      plate->plate_double_flag, plate->rect_double_up);
    CLOCK_END("plate_double_decide_all_candidate");

#if MODOULE_LOCATION_DEBUG
    {
        int32_t w, h;
        IplImage *rect_show = NULL;
        rect_show = cvCreateImage(cvSize(img_w, img_h), IPL_DEPTH_8U, 3);
        _IMAGE3_(rect_show, yuv_img, img_w, img_h, 0);

        for (h = 0; h < plate->real_num; h++)
        {
            cvRectangle(rect_show,
                        cvPoint(plate->rect_real[h].x0, plate->rect_real[h].y0),
                        cvPoint(plate->rect_real[h].x1, plate->rect_real[h].y1),
                        cvScalar(0, 255, 0, 0), 2, 2, 0);
        }

        cvNamedWindow("rect_show", 2);
        cvShowImage("rect_show", rect_show);
        cvWaitKey(0);
        cvDestroyWindow("rect_show");
        cvReleaseImage(&rect_show);
    }
#endif

    CHECKED_FREE(bina_img, rect_w * rect_h2 * sizeof(uint8_t));
    CHECKED_FREE(edge_img, rect_w * rect_h2 * sizeof(uint8_t));
    CHECKED_FREE(half_img, rect_w * rect_h2 * sizeof(uint8_t));

    return;
}



