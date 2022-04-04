#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "plate_mask.h"
#include "plate_runs.h"
#include "plate_location.h"
//#include "../dma_interface.h"
#include "../malloc_aligned.h"
#include "../time_test.h"
#include "../debug_show_image.h"

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#ifdef  WIN32
#include "..\ti64_c\cintrins.h"
#include "..\ti64_c\c64intrins.h"
#endif

#ifdef WIN32
#define DEBUG_RUNS_SHOW  0
#endif

#define DEBUG_RUNS_OUT   0

#if DEBUG_RUNS_SHOW
#include <string.h>
#endif

// 寻找水平方向所有的车牌连线
#ifndef PLATE_RUN_BIT
static void find_plate_runs(uint8_t *restrict edge_img, PlateRunVecs *restrict run, PlateRun *restrict p_run,
                            int32_t *restrict run_num, int32_t lamda,
                            int32_t plate_w_min, int32_t plate_w_max, int32_t img_w, int32_t img_h)
{
    uint64_t tmp_pix;
    int32_t w, h;
    int32_t find;
    int32_t width;
    PlateRun runs;
    int32_t lamda_tmp = lamda;
    uint8_t *restrict pimg = NULL;
    int32_t idx = 0;

    width = img_w - plate_w_min - 1;

    for (h = 0; h < img_h; h++)
    {
        run[h].row = h << 1;
        run[h].p_num = 0;

        pimg = edge_img + img_w * h;

        // 采用预取数据方式可以提高速度
#ifdef _TMS320C6X_3
        touch((const void *)(pimg + img_w), img_w);
#else
        w = (int32_t)(pimg[img_w - 1]);
#endif

        for (w = 1; w < width; w++)
        {
            // 图像中存在大量空白区域，因此采用以下方式会有很大的速度提升
            // 只要plate_w_min的值大于等于7，则tmp_pix的取值不会越界
#ifdef _TMS320C6X_
            // 在DSP中，强制类型转换时，右侧会字节对齐后再赋值给左边，所以会造成赋值错误
            tmp_pix = (uint64_t)_mem8(pimg + w);
#else
            tmp_pix = *((uint64_t *)(pimg + w));
#endif

            if (tmp_pix == (uint64_t)0)
            {
                w += 7;
                continue;
            }

            if (*(pimg + w) > 0)
            {
                int dec = 0;
                int decidx = 0;

                runs.start = (int16_t)w;
                runs.end = (int16_t)w;

                find = 1;

                // 当车牌连线超过车牌宽度最大值时停止车牌连线的扩展
                for (dec = w + lamda; ((runs.end - (int16_t)w + (int16_t)1) < plate_w_max) && find && (dec < img_w - 1);)
                {
                    // 前面1/3宽度内有最大间隔，可能由于最大间隔之间的字符相差的距离超过了尺度，因此不能构成连线，
                    // 故在前面最大间隔位置区域进行连线时，尺度适当扩展
                    if ((runs.end - runs.start > plate_w_min / 6) && (runs.end - runs.start < plate_w_max / 3))
                    {
                        lamda = lamda_tmp + 10;
                    }
                    else
                    {
                        lamda = lamda_tmp;
                    }

                    decidx = dec;
                    find = 0;

                    while (decidx > runs.end)
                    {
                        if (*(pimg + decidx))
                        {
                            runs.end = (int16_t)decidx;
                            find = 1;
                            dec = runs.end + lamda;
                            break;
                        }

                        decidx--;
                    }
                }

                if (((runs.end - runs.start + (int16_t)1) >= (int16_t)plate_w_max))
                {
                    runs.end = (int16_t)(runs.start + (int16_t)plate_w_max - (int16_t)1);
                    w = runs.end;

                    p_run[idx].start = runs.start;
                    p_run[idx].end = runs.end;
                    p_run[idx].row = h << 1;
                    idx++;
                    run[h].p_num++;
                }
                else
                {
                    w = runs.end + lamda;

                    // 车牌连线大于车牌宽度最小值时保留该车牌连线
                    if (((runs.end - runs.start + 1) > plate_w_min))
                    {
                        p_run[idx].start = runs.start;
                        p_run[idx].end = runs.end;
                        p_run[idx].row = h << 1;
                        idx++;
                        run[h].p_num++;
                    }
                }
            }
        }
    }

    *run_num = idx;

    return;
}
#else
static void find_plate_runs_lmbd(uint8_t *restrict edge_img, PlateRunVecs *restrict run, PlateRun *restrict p_run,
                                 int32_t *restrict run_num, int32_t lamda,
                                 int32_t plate_w_min, int32_t plate_w_max, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    PlateRun runs;
    int32_t lamda_tmp = lamda;
//    uint8_t *restrict pimg = NULL;
    int32_t num_bit = img_w / 20;
    int32_t idx = 0;
    uint32_t bit32_0 = 0u;
//    uint32_t bit32_1 = 0u;
    int32_t st0, st1;
    int32_t ed0, ed1;
    uint32_t *restrict edge_bit = (uint32_t*)edge_img;

    for (h = 0; h < img_h; h++)
    {
        run[h].row = h << 1;
        run[h].p_num = 0;

        for (w = 0; w < num_bit; w++)
        {
            bit32_0 = edge_bit[num_bit * h + w];

            if (bit32_0 != 0u)
            {
                break;
            }
        }

        // 找到本行的第一个点
        st0 = (int32_t)_lmbd(1u, _bitr(bit32_0));
        ed0 = 31 - (int32_t)_lmbd(1u, bit32_0);

        runs.start = w * 20 + st0;
        runs.end = w * 20 + ed0;

        w += 1;

        for (; w < num_bit; w++)
        {
            if ((runs.end - runs.start > plate_w_min / 6) && (runs.end - runs.start < plate_w_max / 3))
            {
                lamda = lamda_tmp + 10;
            }
            else
            {
                lamda = lamda_tmp;
            }

            bit32_0 = edge_bit[num_bit * h + w];

            st1 = (int32_t)_lmbd(1u, _bitr(bit32_0));
            ed1 = 31 - (int32_t)_lmbd(1u, bit32_0);

            st1 = st1 > 19 ? 19 : st1;

            // 线段之间的距离不大于20, 连接起来
            if (w * 20 + st1 - runs.end <= lamda)
            {
                if (ed1 >= 0)
                {
                    runs.end = w * 20 + ed1;
                }

                // 线段长度大于最大车牌长度, 截断
                if (runs.end - runs.start + 1 >= plate_w_max)
                {
                    runs.end = runs.start + plate_w_max - 1;

                    p_run[idx].start = runs.start;
                    p_run[idx].end = runs.end;
                    p_run[idx].row = h << 1;
                    idx++;
                    run[h].p_num++;

//                    runs.start = runs.end + 1;
                    runs.start = w * 20 + st1;
                    runs.end = w * 20 + ed1;
                }

                if (runs.end > (num_bit - 1) * 20 && (runs.end - runs.start + 1 >= plate_w_min))
                {
                    p_run[idx].start = runs.start;
                    p_run[idx].end = runs.end;
                    p_run[idx].row = h << 1;
                    idx++;
                    run[h].p_num++;
                }
            }
            else
            {
                // 如果线段长度大于最小宽度, 保存
                if (runs.end - runs.start + 1 >= plate_w_min)
                {
                    p_run[idx].start = runs.start;
                    p_run[idx].end = runs.end;
                    p_run[idx].row = h << 1;
                    idx++;
                    run[h].p_num++;
                }

                if (bit32_0 != 0u)
                {
                    runs.start = w * 20 + st1;
                    runs.end = w * 20 + ed1;
                }
                else
                {
                    w += 1;

                    for (; w < num_bit; w++)
                    {
                        bit32_0 = edge_bit[num_bit * h + w];

                        if (bit32_0 != 0u)
                        {
                            st1 = (int32_t)_lmbd(1u, _bitr(bit32_0));
                            ed1 = 31 - (int32_t)_lmbd(1u, bit32_0);

                            runs.start = w * 20 + st1;
                            runs.end = w * 20 + ed1;

                            break;
                        }
                    }
                }
            }
        }
    }

    *run_num = idx;

    return;
}
#endif

// 用车牌连线初始化矩形区域
static void init_rectangle_by_plate_runs(Rects *restrict region, int32_t row, PlateRun *restrict run)
{
    region->x0 = (int16_t)run->start;
    region->x1 = (int16_t)run->end;
    region->y0 = (int16_t)row;
    region->y1 = (int16_t)row;

    return;
}

// 是否需要将车牌连线融合进矩形区域
static int32_t merge_to_rectangle_or_not(Rects *restrict region, int32_t row, PlateRun *restrict run,
                                         int32_t lamda, int32_t thresh_h)
{
    int32_t k0, k1, k2;

    k0 = abs(region->x0 - run->start);
    k1 = abs(region->x1 - run->end);
    k2 = abs(region->y1 - row);

    if ((k0 > lamda) || (k1 > lamda) || k2 > thresh_h)
    {
        return 0;
    }
    else
    {
        region->x0 = (int16_t)MIN2(region->x0, run->start);
        region->x1 = (int16_t)MAX2(region->x1, run->end);
        region->y1 = (int16_t)row;

        return 1;
    }
}

// 将车牌连线融合成车牌矩形区域
static void merge_plate_runs_to_rectangle(Rects *restrict rect_cand, int32_t *rect_num, PlateRunVecs *restrict run,
                                          PlateRun *p_run, int32_t run_num, /*int32_t lamda, */
                                          int32_t plate_h_min, int32_t plate_h_max, int32_t img_w, int32_t img_h)
{
    int32_t n = 0;
    int32_t k;
    int32_t h;
    int32_t row;
    int32_t index;
    int32_t find;
    int32_t region_num = 0;
    int32_t thresh_w;
    int32_t thresh_h = 4;
    int32_t idx = 0;
    Rects *restrict regions = NULL;
    int32_t rect_h2 = img_h / 2;
    int32_t run_num_tmp = 0;

    if (run_num == 0)
    {
        *rect_num = 0;
        return;
    }

    // 暂时将最大车牌高度在此处调整为80，后续考虑从整体算法角度进行赋值
    plate_h_max = 80;

    CHECKED_MALLOC(regions, sizeof(Rects) * run_num, CACHE_SIZE);

    memset(regions, 0, sizeof(Rects) * run_num);

    region_num = 0;

    for (index = 0; index < rect_h2; index++)
    {
        int32_t j;
        row = run[index].row;

        for (j = run_num_tmp; j < run_num_tmp + run[index].p_num; j++)
        {
            find = 0;

            for (k = 0; k < region_num; k++) // 根据车牌连线融合矩形区域
            {
                if (regions[k].y1 != row)
                {
                    thresh_w = (regions[k].x1 - regions[k].x0 + 1) >> 1;

                    if (merge_to_rectangle_or_not(&regions[k], row, &p_run[j], thresh_w, thresh_h))  // 是否需要合并
                    {
                        find = 1;
                    }
                }
            }

            if (!find)  // 该车牌连线没有被融合，用该车牌连线初始化新的车牌矩形区域
            {
                init_rectangle_by_plate_runs(&regions[region_num++], row, &p_run[j]);
            }
        }

        run_num_tmp += run[index].p_num;
    }

    // 下边界的特殊处理，判断矩形区域是否融合完成
    for (n = 0; n < region_num; n++)
    {
        h = regions[n].y1 - regions[n].y0 + 1;

        if (h > plate_h_min && h < plate_h_max)
        {
            rect_cand[idx] = regions[n];
            rect_cand[idx].x0 = (int16_t)regions[n].x0;
            rect_cand[idx].x1 = (int16_t)MIN2(rect_cand[idx].x1 + 16, img_w - 1);
            rect_cand[idx].y0 = (int16_t)MAX2(rect_cand[idx].y0 - 16, 0);
            rect_cand[idx].y1 = (int16_t)MIN2(rect_cand[idx].y1 + 16, img_h - 1);

            if (++idx >= RECT_MAX)
            {
                break;
            }
        }
    }

    *rect_num = idx;

    CHECKED_FREE(regions, sizeof(Rects) * run_num);

    return;
}

// 根据车牌连线寻找所有的车牌候选区域
void find_plate_candidate_by_plate_runs(PlateLocation *restrict loc,
                                        uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                        int32_t img_w, int32_t img_h, int32_t rect_w, int32_t rect_h, Rects rect_detect,
                                        Rects *restrict rect_cand, int32_t *rect_num)
{
    int32_t new_flg = 0;
    int32_t i, k;
    int32_t num = 0;
    int32_t scales = 1; // 车牌连线的尺度
    int32_t idx = 0;
    int32_t run_num = 0;
    int32_t lamda = 0;
    int32_t plate_w_min = loc->plate_w_min;
    int32_t plate_w_max = loc->plate_w_max;
    int32_t plate_h_min = loc->plate_h_min;
    int32_t plate_h_max = loc->plate_h_max;
    const int32_t lamdas[] = {20, 40, 30};  // 车牌连线融合因子
    int32_t num_tmp = 0;
    int32_t img_h2 = img_h >> 1;
    PlateRunVecs *run = NULL;
    Rects *restrict rect_tmp = NULL;
    PlateRun *p_run = NULL;

#if DEBUG_RUNS_OUT
    static int frame_num_runs_test = 0;
    char runs_file[256];
    FILE *fp;
    int32_t h;
#endif

#if DEBUG_RUNS_SHOW
    static int32_t key;
    int32_t w, h;
    IplImage *image_gray = cvCreateImage(cvSize(img_w, img_h2), IPL_DEPTH_8U, 3);
    IplImage *image_bina = cvCreateImage(cvSize(rect_w, rect_h / 2), IPL_DEPTH_8U, 3);
#endif

#ifdef _TMS320C6X_
    run = (PlateRunVecs *)MEM_alloc(IRAM, sizeof(PlateRunVecs) * img_h2, 128);
    rect_tmp = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * RECT_MAX, 128);
#else
    CHECKED_MALLOC(run, sizeof(PlateRunVecs) * img_h2, CACHE_SIZE);
    CHECKED_MALLOC(rect_tmp, sizeof(Rects) * RECT_MAX, CACHE_SIZE);
#endif

    CHECKED_MALLOC(p_run, sizeof(PlateRun) * rect_w * rect_h / 2 / plate_w_min, CACHE_SIZE);

    memset(run, 0, sizeof(PlateRunVecs) * img_h2);

    // 暂时将最大车牌宽度在此处调整为200，后续从整体算法上进行赋值
    plate_w_max = 200;

    scales = 1;         // DSP上只做一个尺度

    *rect_num = 0;

    for (k = 0; k < scales; k++) // 通过多尺度搜索来提高精度
    {
        lamda = lamdas[k];  // 车牌连线融合因子

        {
            CLOCK_START(find_plate_runs);
#ifdef PLATE_RUN_BIT
            find_plate_runs_lmbd(bina_img, run, p_run, &run_num, lamda, plate_w_min, plate_w_max, rect_w, rect_h / 2);
#else
            find_plate_runs(bina_img, run, p_run, &run_num, lamda, plate_w_min, plate_w_max, rect_w, rect_h / 2);
#endif
            CLOCK_END("find_plate_runs");

            CLOCK_START(merge_plate_runs_to_rectangle);
            // 将车牌连线融合成车牌矩形区域
            merge_plate_runs_to_rectangle(rect_tmp, &num_tmp, run, p_run, run_num,
                                          plate_h_min, plate_h_max, rect_w, rect_h);
            CLOCK_END("merge_plate_runs_to_rectangle");

            // 将框定区域得到的车牌候选区域位置映射到整帧图像位置
            for (i = 0; i < num_tmp; i++)
            {
                rect_tmp[i].x0 = rect_tmp[i].x0 + rect_detect.x0;
                rect_tmp[i].x1 = rect_tmp[i].x1 + rect_detect.x0;
                rect_tmp[i].y0 = rect_tmp[i].y0 + rect_detect.y0;
                rect_tmp[i].y1 = rect_tmp[i].y1 + rect_detect.y0;
            }

            // 去除非车牌
            remove_non_plate_region(gray_img, img_w, img_h, rect_tmp, &num_tmp);
        }

#if DEBUG_RUNS_OUT
        sprintf(runs_file, "runs\\%04d.txt", frame_num_runs_test++);
        printf("%s\n", runs_file);
        fp = fopen(runs_file, "w");

        if (fp == NULL)
        {
            printf("can't open list.txt!\n");

            while (1);
        }

        fprintf(fp, "run_num: %d\n", run_num);

        for (idx = 0; idx < run_num; idx++)
        {
            h = p_run[idx].row;

            fprintf(fp, "h = %3d---start = %4d, end = %4d, len = %3d\n",
                    h, p_run[idx].start, p_run[idx].end, p_run[idx].end - p_run[idx].start + 1);
        }

        fprintf(fp, "\n");

        fclose(fp);
#endif

        num = *rect_num;

        for (idx = 0; idx < num_tmp; idx++)
        {
            new_flg = 1;

            // 用于判断新找到的区域是否与已找到的区域有重合，有重合则取较大的区域
            for (i = *rect_num - 1; i >= 0; i--)
            {
                if (abs(rect_cand[i].x0 - rect_tmp[idx].x0) < lamda
                    && abs(rect_cand[i].x1 - rect_tmp[idx].x1) < lamda
                    && abs(rect_cand[i].y0 - rect_tmp[idx].y0) < lamda
                    && abs(rect_cand[i].y1 - rect_tmp[idx].y1) < lamda)
                {
                    rect_cand[i].x0 = (int16_t)MIN2(rect_cand[i].x0, rect_tmp[idx].x0);
                    rect_cand[i].x1 = (int16_t)MAX2(rect_cand[i].x1, rect_tmp[idx].x1);
                    rect_cand[i].y0 = (int16_t)MIN2(rect_cand[i].y0, rect_tmp[idx].y0);
                    rect_cand[i].y1 = (int16_t)MAX2(rect_cand[i].y1, rect_tmp[idx].y1);

                    new_flg = 0;
                    break;
                }
            }

            if (new_flg)
            {
                rect_cand[num++] = rect_tmp[idx];
            }

            if (num >= RECT_MAX)
            {
                break;
            }
        }

        *rect_num = num;

#if DEBUG_RUNS_SHOW

        _IMAGE3_HALF_(image_gray, gray_img, img_w, img_h2, 0);
        _IMAGE3_(image_bina, bina_img, rect_w, rect_h / 2, 0);

        for (idx = 0; idx < run_num; idx++)
        {
            cvLine(image_gray,
                   cvPoint(p_run[idx].start + rect_detect.x0, p_run[idx].row / 2 + rect_detect.y0 / 2),
                   cvPoint(p_run[idx].end + rect_detect.x0, p_run[idx].row / 2 + rect_detect.y0 / 2),
                   cvScalar(0, 255, 0, 0), 1, 1, 0);
        }

        for (idx = 0; idx < *rect_num; idx++)
        {
            cvRectangle(image_gray,
                        cvPoint(rect_cand[idx].x0, rect_cand[idx].y0 / 2),
                        cvPoint(rect_cand[idx].x1, rect_cand[idx].y1 / 2),
                        cvScalar(0, 0, 255, 0), 2, 1, 0);
        }

        cvNamedWindow("image_gray", 2);
        cvShowImage("image_gray", image_gray);
        cvNamedWindow("image_bina", 2);
        cvShowImage("image_bina", image_bina);

        if (key == 32)
        {
            //assert(cout << "程序暂定:空格键单帧播放" << endl << "其他键继续" << endl);
            key = cvWaitKey(0);
        }
        else
        {
            key = cvWaitKey(0);
        }

#endif
    }

#if DEBUG_RUNS_SHOW
    cvReleaseImage(&image_gray);
    cvReleaseImage(&image_bina);
#endif

    CHECKED_FREE(p_run, sizeof(PlateRun) * rect_w * rect_h / 2 / plate_w_min);

#ifdef _TMS320C6X_
    MEM_free(IRAM, run, sizeof(PlateRunVecs) * img_h2);
    MEM_free(IRAM, rect_tmp, sizeof(Rects) * RECT_MAX);
#else
    CHECKED_FREE(run, sizeof(PlateRunVecs) * img_h2);
    CHECKED_FREE(rect_tmp, sizeof(Rects) * RECT_MAX);
#endif

    return;
}

