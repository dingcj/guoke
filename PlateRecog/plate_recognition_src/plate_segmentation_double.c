#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "plate_segmentation.h"
#include "../malloc_aligned.h"
#include "plate_segmentation_double.h"
#include "../common/threshold.h"
#include "../common/label.h"
#include "../common/image.h"
#include "lsvm.h"
#include "feature.h"
#include "../debug_show_image.h"

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#ifdef WIN321
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#endif

#ifdef WIN32
#define DEBUG_SEG_DOUBLE_SHOW 0
#define DEBUG_SEGMENT_PRINT   0
#define DEBUG_SEGMENT_HIST    0
#define DEBUG_SEGMENT_LR      0
#endif

#ifdef _TMS320C6X_
#pragma CODE_SECTION(character_segmentation_double_up, ".text:character_segmentation_double_up")
#pragma CODE_SECTION(plate_upper_and_lower_locating_exact_up, ".text:plate_upper_and_lower_locating_exact_up")
#pragma CODE_SECTION(remove_small_region_by_labelling_up, ".text:remove_small_region_by_labelling_up")
#pragma CODE_SECTION(remove_upper_and_lower_border_up, ".text:remove_upper_and_lower_border_up")
#pragma CODE_SECTION(remove_isolated_lines, ".text:remove_isolated_lines")
#pragma CODE_SECTION(character_left_and_right_locating_iteration_up, ".text:character_left_and_right_locating_iteration_up")
#pragma CODE_SECTION(character_left_and_right_locating_double, ".text:character_left_and_right_locating_double")
#pragma CODE_SECTION(character_up_decide_white_back, ".text:character_up_decide_white_back")
#pragma CODE_SECTION(find_interval_mid, ".text:find_interval_mid")
#pragma CODE_SECTION(best_character_adjust_white_back, ".text:best_character_adjust_white_back")
#pragma CODE_SECTION(character_up_decide_black_back, ".text:character_up_decide_black_back")
#pragma CODE_SECTION(best_character_adjust_black_back, ".text:best_character_adjust_black_back")
#pragma CODE_SECTION(character_upper_and_lower_locating_up, ".text:character_upper_and_lower_locating_up")
#pragma CODE_SECTION(double_plate_up_character_recognition, ".text:double_plate_up_character_recognition")
#pragma CODE_SECTION(character_segmentation_double_down, ".text:character_segmentation_double_down")
#pragma CODE_SECTION(remove_small_region_by_labelling_down, ".text:remove_small_region_by_labelling_down")
#pragma CODE_SECTION(plate_upper_and_lower_locating_exact_down, ".text:plate_upper_and_lower_locating_exact_down")
#pragma CODE_SECTION(remove_upper_and_lower_border_down, ".text:remove_upper_and_lower_border_down")
#pragma CODE_SECTION(character_left_and_right_locating_iteration_down, ".text:character_left_and_right_locating_iteration_down")
#pragma CODE_SECTION(character_upper_and_lower_locating_down, ".text:character_upper_and_lower_locating_down")
#pragma CODE_SECTION(remove_too_small_region_down, ".text:remove_too_small_region_down")
#pragma CODE_SECTION(character_spliting_down, ".text:character_spliting_down")
#pragma CODE_SECTION(character_merging_down, ".text:character_merging_down")
#pragma CODE_SECTION(character_correct_down, ".text:character_correct_down")
#pragma CODE_SECTION(remove_non_character_region_down, ".text:remove_non_character_region_down")
#pragma CODE_SECTION(double_plate_down_character_recognition, ".text:double_plate_down_character_recognition")
#endif

// 删除孤立的前景行，减少对投影字符分割的干扰
static void remove_isolated_lines(uint8_t *restrict bina_img, int32_t h0, int32_t h1, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t hh0, hh1;
    uint8_t *restrict ptru = NULL;
    uint8_t *restrict ptr0 = NULL;
    uint8_t *restrict ptr1 = NULL;

    hh0 = h0;
    hh1 = h1;

    if (h0 == 0)
    {
        h = 0;
        hh0 = 1;

        ptr0 = bina_img + img_w * (h + 0);
        ptr1 = bina_img + img_w * (h + 1);

        for (w = 0; w < img_w; w++)
        {
            *ptr0 &= *ptr1;
            ptr0++;
            ptr1++;
        }
    }

    if (h1 == img_h - 1)
    {
        h = img_h - 1;
        hh1 = img_h - 2;

        ptru = bina_img + img_w * (h - 1);
        ptr0 = bina_img + img_w * (h - 0);

        for (w = 0; w < img_w; w++)
        {
            *ptr0 &= *ptru;
            ptr0++;
            ptru++;
        }
    }

    for (h = hh0; h <= hh1; h++)
    {
        ptru = bina_img + img_w * (h - 1);
        ptr0 = bina_img + img_w * (h + 0);
        ptr1 = bina_img + img_w * (h + 1);

        for (w = 0; w < img_w; w++)
        {
            *ptr0 &= (*ptru | *ptr1);
            ptr0++;
            ptru++;
            ptr1++;
        }
    }

    return;
}

// 根据垂直投影直方图定位字符的左右边界
static void character_left_and_right_locating_double(int32_t *restrict hist, int32_t len, Rects *restrict ch, int32_t *ch_num1)
{
    int32_t k;
    int32_t ch_num;

    // 定位字符的左右边界
    ch_num = 0;

    // 2012.6.8，将len - 1修改为len， len - 1的情况下，最后定位出的字符右边界会在以前的基础上少一个像素
    for (k = 0; k < len/* - 1*/; k++)
    {
        if (hist[k])
        {
            ch[ch_num].x0 = (int16_t)k;

            while ((k < len/* - 1*/) && hist[k])
            {
                k++;
            }

            k--;

            if ((k - ch[ch_num].x0 + 1) >= 2)
            {
                ch[ch_num].x1 = (int16_t)k;
                ch_num++;

                if (ch_num >= CH_MAX)
                {
                    break;
                }
            }
        }
    }

    for (k = 0; k < ch_num; k++)
    {
        ch[k].y0 = 0;
        ch[k].y1 = 0;
    }

    *ch_num1 = ch_num;

    return;
}

// 计算字符的平均宽度
static int32_t get_character_average_width(Rects *restrict ch, int32_t ch_num, int32_t plate_h)
{
    int32_t k;
    int32_t num = 0;
    int32_t avg_w = 0;
    int32_t ch_w, ch_h;
    uint8_t *restrict is_1 = NULL;

    CHECKED_MALLOC(is_1, sizeof(uint8_t) * ch_num, CACHE_SIZE);

    // 首先判断字符1
    for (k = 0; k < ch_num; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;
        ch_h = ch[k].y1 - ch[k].y0 + 1;

        if ((ch_w < 6) || (ch_h > 3 * ch_w))
        {
            is_1[k] = (uint8_t)1;
        }
        else
        {
            is_1[k] = (uint8_t)0;
        }
    }

    num = 0;
    avg_w = 0;

    for (k = 0; k < ch_num; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;
        ch_h = ch[k].y1 - ch[k].y0 + 1;

        if ((ch_w < plate_h) && (!is_1[k]))
        {
            if ((k - 1 >= 0) && (!is_1[k - 1]))
            {
                // 如果当前字符比前一个字符宽许多，则当前字符宽度不加入统计
                if (ch_w > (ch[k - 1].x1 - ch[k - 1].x0 + 1) * 3 / 2)
                {
                    continue;
                }
            }

            if ((k + 1 < ch_num) && (!is_1[k + 1]))
            {
                // 如果当前字符比后一个字符宽许多，则当前字符宽度不加入统计
                if (ch_w > (ch[k + 1].x1 - ch[k + 1].x0 + 1) * 3 / 2)
                {
                    continue;
                }
            }

            num++;
            avg_w += ch_w;
        }
    }

    if (num > 0)
    {
        avg_w /= num;
    }
    else
    {
        // 默认平均宽度
        avg_w = plate_h * 2 / 3;
    }

    CHECKED_FREE(is_1, sizeof(uint8_t) * ch_num);

    return avg_w;
}

// 计算字符的平均高度
static int32_t get_character_average_height(Rects *restrict ch, int32_t ch_num, int32_t plate_h)
{
    int32_t k;
    int32_t num;
    int32_t avg_h;
    int32_t h0, h1, h2;

    num = 0;
    avg_h = 0;

    for (k = 1; k < ch_num - 1; k++)
    {
        h0 = ch[k - 1].y1 - ch[k - 1].y0 + 1;
        h1 = ch[k - 0].y1 - ch[k - 0].y0 + 1;
        h2 = ch[k + 1].y1 - ch[k + 1].y0 + 1;

        if ((h0 > plate_h / 2) && (h1 > plate_h / 2) && (h2 > plate_h / 2))
        {
            // 当前字符高度要在相邻字符高度的一定范围内才加入统计
            if ((h1 > h0 * 4 / 5) && (h1 < h0 * 6 / 5) &&
                (h1 > h2 * 4 / 5) && (h1 < h2 * 6 / 5))
            {
                num++;
                avg_h += h1;
            }
        }
    }

    if (num > 0)
    {
        avg_h /= num;
    }
    else
    {
        avg_h = plate_h * 4 / 5;
    }

    return avg_h;
}


// 统计粘连字符的个数
static int32_t character_merge_numbers(uint8_t *restrict merge_mask, Rects *ch, int32_t ch_num,
                                       int32_t plate_w, int32_t plate_h)
{
    int32_t ch_w;
    int32_t avg_w;
    int32_t k;
    int32_t merge = 0;

    memset(merge_mask, 1, plate_w);

    avg_w = get_character_average_width(ch, ch_num, plate_h);  // 计算平均宽度

    for (k = 0; k < ch_num; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;

        if (ch_w > avg_w * 8 / 3)   // 三个以上字符粘连
        {
            merge += 3;
            memset(merge_mask + ch[k].x0, 0, ch_w);
        }
        else if (ch_w > avg_w * 3 / 2)  // 两个字符粘连
        {
            merge += 2;
            memset(merge_mask + ch[k].x0, 0, ch_w);
        }
    }

    return merge;
}

// 通过连通域标记的方法去除双层牌上层区域中的干扰小区域
static void remove_small_region_by_labelling_up(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    //  int32_t k;
    int32_t w, h;
    //    int32_t ch_num;
    Rects *restrict obj_pos = NULL;
    int32_t *restrict obj_area = NULL;
    uint16_t *restrict label_img = NULL;
    //    uint8_t *restrict ptr = NULL;
    //  int32_t obj_w;
    //  int32_t obj_h;

#ifdef _TMS320C6X_1
    obj_pos = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * LABEL_MAX, 128);
    obj_area = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * LABEL_MAX, 128);
    label_img = (uint16_t *)MEM_alloc(IRAM, sizeof(uint16_t) * img_w * img_h, 128);
#else
    CHECKED_MALLOC(obj_pos, sizeof(Rects) * LABEL_MAX, CACHE_SIZE);
    CHECKED_MALLOC(obj_area, sizeof(int32_t) * LABEL_MAX, CACHE_SIZE);
    CHECKED_MALLOC(label_img, sizeof(uint16_t) * img_w * img_h, CACHE_SIZE);
#endif

    image_label(bina_img, label_img, obj_pos, obj_area, img_w, img_h, LABEL_MAX);

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            // 判断像素点是否属于小于80的连通区域
            if ((label_img[img_w * h + w] > (uint16_t)0) && (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 80))
            {
                // 像素点所属连通区域小于36，直接删除
                if (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 36)
                {
                    bina_img[img_w * h + w] = (uint8_t)0;
                }
                else
                {
                    int32_t flag0, flag1;
                    Rects rect_label;

                    rect_label = obj_pos[label_img[img_w * h + w] - (uint16_t)1];

                    // 去除上边的螺钉
                    flag0 = /*(rect_label.y0 <= 5) && */(rect_label.y1 <= 10);

                    // 去除高度过窄的区域，高度过窄的小区域为断裂的边框
                    flag1 = (rect_label.y1 - rect_label.y0 + 1 < 4);

                    if (flag0 || flag1)
                    {
                        bina_img[img_w * h + w] = (uint8_t)0;
                    }
                }
            }
        }
    }

#ifdef _TMS320C6X_1
    MEM_free(IRAM, obj_pos, sizeof(Rects) * LABEL_MAX, 128);
    MEM_free(IRAM, obj_area, sizeof(int32_t) * LABEL_MAX, 128);
    MEM_free(IRAM, label_img, sizeof(uint16_t) * img_w * img_h, 128);
#else
    CHECKED_FREE(label_img, sizeof(uint16_t) * img_w * img_h);
    CHECKED_FREE(obj_area, sizeof(int32_t) * LABEL_MAX);
    CHECKED_FREE(obj_pos, sizeof(Rects) * LABEL_MAX);
#endif

    return;
}

// 双层牌上层区域上下边界的精确定位
static int32_t plate_upper_and_lower_locating_exact_up(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
                                                       int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t change;
    int32_t change_thresh;
    uint8_t *restrict ptr = NULL;

    // 统计中间9行的跳变数作为阈值
    change = 0;

    for (h = plate_h / 2 - 4; h <= plate_h / 2 + 4; h++)
    {
        ptr = bina_img + plate_w * h;

        for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
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

    for (h = plate_h / 2 - 5; h >= 0; h--)
    {
        change = 0;
        ptr = bina_img + plate_w * h;

        for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
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

    for (h = plate_h / 2 + 5; h < plate_h; h++)
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

// 去除双层牌牌上边框的干扰
static void remove_upper_and_lower_border_up(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t len;
    int32_t start, end;
    const int32_t thresh = plate_h * 5 / 3;  // 上层区域宽高比为4:3, 取阈值时扩展一点到5:3

    // 假设上边框最多只有5行
    for (h = 0; h < 5; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            if (bina_img[plate_w * h + w])
            {
                start = w;

                while (bina_img[plate_w * h + w] && w < plate_w)
                {
                    w++;
                }

                end = w - 1;
                len = end - start + 1;

                if (len > thresh)
                {
#if DEBUG_SEGMENT_PRINT
                    printf("delete top line ......\n");
#endif
                    // 多删除一行，消除反光的影响
                    memset(bina_img + plate_w * (h + 0) + start, 0, len);
                    memset(bina_img + plate_w * (h + 1) + start, 0, len);
                }
            }
        }
    }

    return;
}

// 采用迭代的方法定位字符左右边界
static void character_left_and_right_locating_iteration_up(uint8_t *restrict bina_img, Rects *ch, int32_t *ch_num1,
                                                           int32_t plate_w, int32_t plate_h)
{
    int32_t i, j;
    int32_t m, n, k;
    int32_t ch_num;
    int32_t p;
    int32_t merge;
    int32_t iteration;
    int32_t *restrict hist = NULL;
    uint8_t *restrict merge_mask = NULL;

    CHECKED_MALLOC(hist, sizeof(int32_t) * plate_w, CACHE_SIZE);
    CHECKED_MALLOC(merge_mask, sizeof(uint8_t) * plate_w, CACHE_SIZE);

    // 二值图像向水平方向投影
    for (j = 0; j < plate_w; j++)
    {
        hist[j] = 0;

        for (i = 0; i < plate_h; i++)
        {
            hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    // 统计粘连字符的个数
    merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

    // 如果字符数目太少或者粘连字符太多，认为和上边框发生了粘连
    iteration = 1;
    p = 0;

    while ((ch_num <= 3 || merge >= 3) && (iteration < 5) && (p < 5))
    {
        k = ch_num;

        // 一行一行的删除上边框，直到分割得到的字符大于ch_num + 1
        for (; p < 5; p++)
        {
            for (j = 0; j < plate_w; j++)
            {
                if (bina_img[plate_w * p + j] && (merge_mask[j] == (0u)))
                {
                    hist[j]--;
                }
            }

            character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

            if (ch_num >= k + 1)
            {
                for (m = 0; m <= p; m++)
                {
                    for (n = 0; n < plate_w; n++)
                    {
                        if (merge_mask[n] == (0u))
                        {
                            bina_img[m * plate_w + n] = (0u);
                        }
                    }
                }

                break;
            }
        }

        // 重新统计粘连字符的个数
        merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

        iteration++;
    }

    // 排除由于上述操作引起的孤立前景行对投影的影响
    remove_isolated_lines(bina_img, 0, plate_h / 3 - 1, plate_w, plate_h);
    remove_isolated_lines(bina_img, plate_h * 2 / 3, plate_h - 1, plate_w, plate_h);

    // 重新统计直方图
    for (j = 0; j < plate_w; j++)
    {
        hist[j] = 0;

        for (i = 0; i < plate_h; i++)
        {
            hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    // 最终定位出每个字符的边界
    character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    *ch_num1 = ch_num;

    CHECKED_FREE(merge_mask, sizeof(uint8_t) * plate_w);
    CHECKED_FREE(hist, sizeof(int32_t) * plate_w);

    return;
}

// 从中间开始找间隔确定上层区域字符真正位置
static void find_interval_mid(Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h, int32_t *best_place)
{
    int32_t w;
    int32_t ch_num = *ch_num1;
    int32_t mid_num = 0;
    int32_t ch_left = 0;
    int32_t ch_right = *ch_num1 - 1;
    int32_t min_dist = plate_w;

    for (w = 0; w < ch_num - 1; w++)
    {
        int32_t dist = abs((ch[w].x1 + ch[w + 1].x0) / 2 - plate_w / 2);

        if (dist < min_dist)
        {
            min_dist = dist;
            mid_num = w;
        }
    }

    w = mid_num + 1;

    if (w != 0)
    {
        for (mid_num = w; mid_num > 0; mid_num--)
        {
            if ((ch[mid_num].x0 - ch[mid_num - 1].x1 - 1) > plate_h * 3 / 4)
            {
                ch_right = mid_num;
                break;
            }
        }
    }

    if (mid_num == 0)
    {
        ch_right = w;
    }

    for (mid_num = w; mid_num < ch_num - 1; mid_num++)
    {
        if ((ch[mid_num + 1].x0 - ch[mid_num].x1 - 1) > plate_h * 3 / 4)
        {
            ch_left = mid_num;
            break;
        }
    }

    if (mid_num == ch_num - 1)
    {
        ch_left = ch_right - 1;
    }


    if (ch_left != ch_right - 1)
    {
        int32_t ch_w_left, ch_w_right;

        ch_w_left = ch[ch_left + 1].x1 - ch[ch_left + 1].x0 + 1;
        ch_w_right = ch[ch_right].x1 - ch[ch_right].x0 + 1;

        if (ch_w_left > ch_w_right)
        {
            ch_right = ch_left + 1;
        }
        else
        {
            ch_left = ch_right - 1;
        }
    }

    *best_place = ch_left;

    return;
}

static void best_character_adjust_white_back(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
                                             int32_t plate_w, int32_t plate_h, int32_t best_place)
{
    int32_t w, h;
    Rects ch_left;
    Rects ch_right;
    int32_t tmp_num;
    int32_t ch_num = *ch_num1;
    int32_t *hist = NULL;
    int32_t ch_w;

    CHECKED_MALLOC(hist, plate_w * sizeof(int32_t), CACHE_SIZE);

    ch_left = ch[best_place];
    ch_right = ch[best_place + 1];

    ch_w = ch_right.x1 - ch_right.x0 + 1;

    // 右边字符太窄
    if (ch_w < plate_h * 3 / 4)
    {
        tmp_num = best_place + 1;

        while (ch_right.x1 - ch_right.x0 + 1 < plate_h)
        {
            if (tmp_num < ch_num - 1)
            {
                ch_right.x1 = ch[tmp_num + 1].x1;
                tmp_num++;
            }
            else
            {
                break;
            }
        }

        if (ch_right.x1 - ch_right.x0 + 1 < plate_h)
        {
            ch_right.x1 = MIN2(plate_w - 1, ch_right.x0 + plate_h * 4 / 3);
        }
    }

    ch_w = ch_right.x1 - ch_right.x0 + 1;

    // 右边字符太宽,宽度大于高度的5/3,认为有粘连
    if (ch_w > plate_h * 4 / 3)
    {
        memset(hist + ch_right.x0, 0, ch_w * sizeof(int32_t));

        for (w = ch_right.x0; w <= ch_right.x1; w++)
        {
            for (h = plate_h / 3; h < plate_h - 1; h++)
            {
                hist[w] += bina_img[plate_w * h + w] > (0u);
            }
        }

        for (w = ch_right.x1; w >= ch_right.x0; w--)
        {
            if (hist[w] > 0)
            {
                ch_right.x1 = w;
                break;
            }
        }
    }

    ch_w = ch_left.x1 - ch_left.x0 + 1;

    // 左边字符太窄
    if (ch_w < plate_h)
    {
        tmp_num = best_place;

        while (ch_left.x1 - ch_left.x0 + 1 < plate_h)
        {
            if (tmp_num > 0)
            {
                ch_left.x0 = ch[tmp_num - 1].x0;
                tmp_num--;
            }
            else
            {
                break;
            }
        }

        if (ch_left.x1 - ch_left.x0 + 1 < plate_h)
        {
            ch_left.x0 = MAX2(0, ch_left.x1 - plate_h * 4 / 3);
        }
    }

    ch_w = ch_left.x1 - ch_left.x0 + 1;

    // 左边字符太宽,宽度大于高度的5/3,认为有粘连
    if (ch_w > plate_h * 5 / 4)
    {
        memset(hist + ch_left.x0, 0, ch_w * sizeof(int32_t));

        for (w = ch_left.x0; w <= ch_left.x1; w++)
        {
            for (h = plate_h / 3; h < plate_h - 1; h++)
            {
                hist[w] += bina_img[plate_w * h + w] > (0u);
            }
        }

        for (w = ch_left.x0; w <= ch_left.x1; w++)
        {
            if (hist[w] > 0)
            {
                ch_left.x0 = w;
                break;
            }
        }
    }

    ch[best_place].x0 = ch_left.x0;
    ch[best_place + 1].x1 = ch_right.x1;

    CHECKED_FREE(hist, plate_w * sizeof(int32_t));
    return;
}

static void best_character_adjust_black_back(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
                                             int32_t plate_w, int32_t plate_h, int32_t best_place)
{
    int32_t w, h, k;
    Rects ch_left;
//    Rects ch_right;
    int32_t tmp_num;
    int32_t ch_num = *ch_num1;
    int32_t *hist = NULL;
    int32_t ch_w;

    CHECKED_MALLOC(hist, plate_w * sizeof(int32_t), CACHE_SIZE);

    ch_left = ch[best_place];
//    ch_right = ch[best_place + 1];

    ch_w = ch_left.x1 - ch_left.x0 + 1;

    // 左边字符太窄
    if (ch_w < plate_h * 2 / 3)
    {
        tmp_num = best_place;

        while (ch_left.x1 - ch_left.x0 + 1 < plate_h)
        {
            if (tmp_num > 0)
            {
                ch_left.x0 = ch[tmp_num - 1].x0;
                tmp_num--;
            }
            else
            {
                break;
            }
        }

        if (ch_left.x1 - ch_left.x0 + 1 < plate_h)
        {
            ch_left.x0 = MAX2(0, ch_left.x1 - plate_h * 4 / 3);
        }
    }

    ch_w = ch_left.x1 - ch_left.x0 + 1;

    // 左边字符太宽,宽度大于高度的5/3,认为有粘连
    if (ch_w > plate_h * 5 / 4)
    {
        memset(hist + ch_left.x0, 0, ch_w * sizeof(int32_t));

        for (w = ch_left.x0; w <= ch_left.x1; w++)
        {
            for (h = plate_h / 3; h < plate_h - 1; h++)
            {
                hist[w] += bina_img[plate_w * h + w] > (0u);
            }
        }

        for (w = ch_left.x0; w <= ch_left.x1; w++)
        {
            if (hist[w] > 0)
            {
                ch_left.x0 = w;
                break;
            }
        }
    }

    ch[best_place].x0 = ch_left.x0;

    // 右边字符太窄
    for (k = best_place + 1; k < ch_num - 1; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;

        if (/*(ch_w < plate_h / 2) && */(ch[k + 1].x1 - ch[k].x0 + 1) < plate_h)
        {
            ch[k].x1 = ch[k + 1].x1;

            for (w = k + 1; w < ch_num - 1; w++)
            {
                ch[w] = ch[w + 1];
            }

            ch_num--;
            k--;
        }
    }

    *ch_num1 = ch_num;

    CHECKED_FREE(hist, plate_w * sizeof(int32_t));

    return;
}

// 白底黑字双层牌上层区域字符精确定位, 针对后牌黄牌和白牌等,上层区域为汉字+字母
static int32_t character_up_decide_white_back(uint8_t *bina_img, Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t interval[CH_MAX];
    int32_t interval_num = 0;
    int32_t ch_num = *ch_num1;
    int32_t min_dist = plate_w;
    int32_t best_place = 0;

    // 必须保证字符数不小于2
    if (ch_num < 2)
    {
        return 0;
    }

    memset(interval, 0, CH_MAX * sizeof(int32_t));

    for (k = 0; k < ch_num - 1; k++)
    {
        int32_t ch_interval = ch[k + 1].x0 - ch[k].x1 - 1;
        int32_t ch_w_right = ch[k + 1].x1 - ch[k + 1].x0 + 1;

        if ((ch_interval > plate_h / 2) && (ch_w_right > plate_h / 2))
        {
            interval[k] = 1;
            interval_num++;
        }
    }

    if (interval_num == 0)
    {
        // 找不到合适的间隔，直接找到中间位置进行字符定位
        find_interval_mid(ch, &ch_num, plate_w, plate_h, &best_place);
    }
    else
    {
        for (k = 0; k < ch_num - 1; k++)
        {
            int32_t dist;

            if (interval[k] == 1)
            {
                dist = abs((ch[k].x1 + ch[k + 1].x0) / 2 - plate_w / 2);

                if (dist < min_dist)
                {
                    min_dist = dist;
                    best_place = k;
                }
            }
        }
    }

    // 调整左右字符的位置
    best_character_adjust_white_back(bina_img, ch, &ch_num, plate_w, plate_h, best_place);

    ch[0] = ch[best_place];
    ch[1] = ch[best_place + 1];

    // 白底黑字双层牌上层区域只会有2个字符
    *ch_num1 = 2;

    return 1;
}

// 黑底白字双层牌上层区域字符精确定位,针对农机站绿牌,上层区域为汉字+2个数字
static int32_t character_up_decide_black_back(uint8_t *bina_img, Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t interval[CH_MAX];
    int32_t interval_num = 0;
    int32_t ch_num = *ch_num1;
    int32_t min_dist = plate_w;
    int32_t best_place = 0;

    // 必须保证字符数不小于3
    if (ch_num < 3)
    {
        return 0;
    }

    memset(interval, 0, CH_MAX * sizeof(int32_t));

    for (k = 0; k < ch_num - 2; k++)
    {
        if ((ch[k + 1].x0 - ch[k].x1 > plate_h / 2) && (ch[k + 1].x0 - ch[k].x1 < plate_h * 2))
        {
            interval[k] = 1;
            interval_num++;
        }
    }

    if (interval_num == 0)
    {
        for (k = 0; k < ch_num - 2; k++)
        {
            int32_t dist;
            dist = abs((ch[k].x1 + ch[k + 1].x0) / 2 - plate_w / 2);

            if (dist < min_dist)
            {
                min_dist = dist;
                best_place = k;
            }
        }
    }
    else
    {
        for (k = 0; k < ch_num - 2; k++)
        {
            int32_t dist;

            if (interval[k] == 1)
            {
                dist = abs((ch[k].x1 + ch[k + 1].x0) / 2 - plate_w / 2);

                if (dist < min_dist)
                {
                    min_dist = dist;
                    best_place = k;
                }
            }
        }
    }

    best_character_adjust_black_back(bina_img, ch, &ch_num, plate_w, plate_h, best_place);

    // 字符数小于3，或者得到的字符间隔后面少于2个字符
    if ((ch_num < 3) || (ch_num - 1 - best_place < 2))
    {
        return 0;
    }

    ch[0] = ch[best_place];
    ch[1] = ch[best_place + 1];
    ch[2] = ch[best_place + 2];

    // 黑底白字双层牌上层区域只会有3个字符
    *ch_num1 = 3;

    return 1;
}

// 投影法定位双层牌上层区域字符的上下边界
static void character_upper_and_lower_locating_up(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
                                                  int32_t plate_w, int32_t plate_h, uint8_t bkgd_color)
{
    int32_t w, h;
    int32_t k, p;
    int32_t sum;
    int32_t ch_num = *ch_num1;
    int32_t max_h = 0;

    // 定位字符的上下边界
    for (k = 0; k < ch_num; k++)
    {
        ch[k].y0 = (int16_t)0;
        ch[k].y1 = (int16_t)(plate_h - 1);

        // 上边界
        p = -1;

        for (h = 0; h < plate_h; h++)
        {
            sum = 0;

            for (w = ch[k].x0; w <= ch[k].x1; w++)
            {
                sum += (int32_t)bina_img[plate_w * h + w]/* > 0*/;
            }

            if (sum > 0)
            {
                p = h;
                break;
            }
        }

        if (p != -1)
        {
            ch[k].y0 = (int16_t)p;
        }

        // 下边界
        p = -1;

        for (h = plate_h - 1; h >= 0; h--)
        {
            sum = 0;

            for (w = ch[k].x0; w <= ch[k].x1; w++)
            {
                sum += (int32_t)bina_img[plate_w * h + w]/* > 0*/;
            }

            if (sum > 0)
            {
                p = h;
                break;
            }
        }

        if (p != -1)
        {
            ch[k].y1 = (int16_t)p;
        }

        if (ch[k].y1 <= ch[k].y0)
        {
            ch[k].y0 = (int16_t)0;
            ch[k].y1 = (int16_t)(plate_h - 1);
        }
    }

    // 对农机站双层牌的分割难度更大，为了更准确地确定字符边框，将上下边界统一规定为最长字符的边界
    if (bkgd_color == (0u))
    {
        p = -1;

        for (k = 0; k < ch_num; k++)
        {
            int32_t ch_h;
            ch_h = ch[k].y1 - ch[k].y0 + 1;

            if (ch_h > max_h)
            {
                p = k;
                max_h = ch_h;
            }
        }

        for (k = 0; k < ch_num; k++)
        {
            ch[k].y0 = ch[p].y0;
            ch[k].y1 = ch[p].y1;
        }
    }

    return;
}

// 扩展字符边界
static void character_border_extend(Rects *ch, int32_t *ch_num, int32_t ext, int32_t plate_w, int32_t plate_h)
{
    int32_t k;

    for (k = 0; k < *ch_num; k++)
    {
        ch[k].x0 = (int16_t)MAX2(ch[k].x0 - ext, 0);
        ch[k].x1 = (int16_t)MIN2(ch[k].x1 + ext, plate_w - 1);
        ch[k].y0 = (int16_t)MAX2(ch[k].y0 - ext, 0);
        ch[k].y1 = (int16_t)MIN2(ch[k].y1 + ext, plate_h - 1);
    }

    return;
}

// 双层牌上层区域字符识别
static void double_plate_up_character_recognition(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                                  int32_t plate_w, int32_t plate_h, Rects *restrict ch, uint8_t *restrict chs,
                                                  uint8_t *trust, uint8_t *restrict ch_buf, int32_t *ch_num,
                                                  SvmModel *restrict model_english,
                                                  SvmModel *restrict model_numbers,
                                                  SvmModel *restrict model_chinese,
                                                  uint8_t *not_ch_cnt, uint8_t bkgd_color)
{
    int32_t k;
    int32_t ch_w, ch_h;
    uint8_t is_ch = (0u);
    uint8_t not_ch_number = (0u);

    character_border_extend(ch, ch_num, 1, plate_w, plate_h);

    // 对汉字进行识别
    // 暂时无法用单层牌中对汉字的精确定位
    //    chinese_left_right_locating_by_projection(gray_img, plate_w, plate_h, ch, *ch_num);

    // 对汉字字符进行识别
    single_character_recognition(gray_img, plate_w, ch[0], &chs[0],
                                 &trust[0], ch_buf, model_chinese, (0u), (1u), &is_ch);

    if (is_ch == (0u))
    {
        not_ch_number++;
    }

    // 对字符进行识别
    if (bkgd_color == (0u))    // 对绿牌上层区域的2个数字进行识别
    {
        // 屏蔽字母
//        memset(&model_numbers->mask[10], 1, (model_numbers->nr_class - 10) * sizeof(uint8_t));

        for (k = 1; k < 3; k++)
        {
            ch_w = ch[k].x1 - ch[k].x0 + 1;
            ch_h = ch[k].y1 - ch[k].y0 + 1;

            if (ch_w < ch_h / 2)
            {
                chs[k] = (uint8_t)('1');
                trust[k] = (90u);
                is_ch = (1u);
            }
            else
            {
                single_character_recognition(gray_img, plate_w, ch[k], &chs[k],
                                             &trust[k], ch_buf + STD_AREA * k, model_numbers, (1u), (0u), &is_ch);
            }

            if (is_ch == (0u))
            {
                not_ch_number++;
            }
        }

        memset(&model_numbers->mask[10], 0, (model_numbers->nr_class - 10) * sizeof(uint8_t));
    }
    else                   // 对黄牌或白牌双层牌上层区域的字母进行识别
    {
        single_character_recognition(gray_img, plate_w, ch[1], &chs[1],
                                     &trust[1], ch_buf + STD_AREA, model_english, (1u), (0u), &is_ch);

        if (is_ch == (0u))
        {
            not_ch_number++;
        }
    }

    *not_ch_cnt = not_ch_number;

    return;
}

// 双层牌上层区域字符分割识别
uint8_t character_segmentation_double_up(uint8_t *restrict src_img, int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                                         int32_t *ch_num1, uint8_t *restrict chs, uint8_t *trust, uint8_t *trust_avg,
                                         uint8_t *restrict ch_buf,
                                         SvmModel *restrict model_english,
                                         SvmModel *restrict model_numbers,
                                         SvmModel *restrict model_chinese,
                                         uint8_t *not_ch_cnt, uint8_t bkgd_color)
{
    int32_t err_ret = 1;
    int32_t h, k;
    int32_t x0 = 0;
    int32_t y0 = 0;
    int32_t x1 = plate_w - 1;
    int32_t y1 = plate_h - 1;
    int32_t ch_h;
    int32_t ch_num;
    uint8_t *plate_img = NULL;
    Rects *ch_tmp = NULL;
    uint8_t *bina_img = NULL;

#if DEBUG_SEG_DOUBLE_SHOW
    int32_t w;
    int32_t offset;
    IplImage *plate = cvCreateImage(cvSize(plate_w, plate_h * 7), IPL_DEPTH_8U, 3);
    IplImage *resize = cvCreateImage(cvSize(plate_w * 2, plate_h * 7 * 2), IPL_DEPTH_8U, 3);

    memset(plate->imageData, 0, plate->widthStep * plate->height);
    _IMAGE3_(plate, src_img, plate_w, plate_h, 0)
#endif

#ifdef _TMS320C6X_
    ch_tmp = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * CH_MAX, 128);
    plate_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
#else
    CHECKED_MALLOC(ch_tmp, sizeof(Rects) * CH_MAX, CACHE_SIZE);
    CHECKED_MALLOC(plate_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
#endif

//    memset(plate_img, 0, plate_w * 5);
    // 对比度拉伸
    contrast_extend_for_gary_image(plate_img, src_img, plate_w, plate_h);

#if DEBUG_SEG_DOUBLE_SHOW
    _IMAGE3_(plate, plate_img, plate_w, plate_h, plate_h)
#endif

    // 二值化图像
    // 图像中间4/7，下面2/3区域作为统计区域
    // 中间4/7为标准模板中上层字符区域宽度与下层字符区域宽度比；下面2/3是为了去除上面边框干扰
    thresholding_percent(plate_img, bina_img, plate_w, plate_h, plate_w * 3 / 14, plate_w - plate_w * 3 / 14,
                         plate_h / 3, plate_h - 1/*plate_h / 5*/, 20);

#if DEBUG_SEG_DOUBLE_SHOW
    _IMAGE3_(plate, bina_img, plate_w, plate_h, plate_h * 2)
#endif

    // 双层牌上层区域上下边界精确定位
    plate_upper_and_lower_locating_exact_up(bina_img, &y0, &y1, plate_w, plate_h);

    // 删除多余的上下边界信息
    memset(bina_img, 0, plate_w * y0);
    memset(bina_img + plate_w * y1, 0, plate_w * (plate_h - y1));

    // 左右干扰区域去除，左右分别删除1/7,保证上层区域字符不被误删除
    x0 = plate_w / 7;
    x1 = plate_w - plate_w / 7;

    for (h = 0; h < plate_h; h++)
    {
        memset(bina_img + plate_w * h, 0, x0);
        memset(bina_img + plate_w * h + x1, 0, plate_w - x1);
    }

// 显示车牌上层区域上下边界定位效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 3;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)

    cvRectangle(plate, cvPoint(x0, y0 + offset), cvPoint(x1, y1 + offset),
                cvScalar(0, 255, 0, 0), 1, 1, 0);
#endif

    // 连通法去除螺钉、圆点等干扰区域
    remove_small_region_by_labelling_up(bina_img, plate_w, plate_h);

    ch_h = y1 - y0 + 1;
    memcpy(bina_img, bina_img + plate_w * y0, plate_w * ch_h);

    // 去除上边框
    remove_upper_and_lower_border_up(bina_img, plate_w, ch_h);

    // 去除孤立前景行
    remove_isolated_lines(bina_img, 0, ch_h - 1, plate_w, ch_h);

// 显示小区域及边框等干扰区域去除效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 4 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset)
#endif

    // 迭代投影法定位字符左右边界
    character_left_and_right_locating_iteration_up(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    if (bkgd_color == (1u))
    {
        // 白底黑字双层牌上层区域字符精确定位
        err_ret = character_up_decide_white_back(bina_img, ch_tmp, &ch_num, plate_w, ch_h);
    }
    else
    {
        // 黑底白字双层牌上层区域字符精确定位
        err_ret = character_up_decide_black_back(bina_img, ch_tmp, &ch_num, plate_w, ch_h);
    }

    if (err_ret == 0)
    {
        ch_num = 0;
    }

    character_upper_and_lower_locating_up(bina_img, ch_tmp, &ch_num, plate_w, ch_h, bkgd_color);

// 显示字符上下左右边界定位效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 5 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 6;
    _IMAGE3_(plate, plate_img, plate_w, plate_h, offset);

    offset = plate_h * 6 + y0;

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 0, 255, 0), 1, 1, 0);
    }

#endif

    if (ch_num != 0)
    {
        double_plate_up_character_recognition(plate_img + plate_w * y0, bina_img, plate_w, ch_h, ch_tmp, chs, trust,
                                              ch_buf, &ch_num, model_english, model_numbers, model_chinese, not_ch_cnt, bkgd_color);

        for (k = 0; k < ch_num; k++)
        {
            ch[k].x0 = (int16_t)(ch_tmp[k].x0);
            ch[k].x1 = (int16_t)(ch_tmp[k].x1);
            ch[k].y0 = (int16_t)(ch_tmp[k].y0 + y0);
            ch[k].y1 = (int16_t)(ch_tmp[k].y1 + y0);
        }

        if (bkgd_color == (0u))
        {
            if (*not_ch_cnt >= 3)
            {
                ch_num = 0;
            }
        }
        else
        {
            if (*not_ch_cnt >= 2)
            {
                ch_num = 0;
            }
        }

        *ch_num1 = ch_num;
    }

    if (ch_num > 0)
    {
        int trust_sum = 0;

        for (k = 0; k < ch_num; k++)
        {
            trust_sum += (int32_t)trust[k];
        }

        *trust_avg = (uint8_t)(trust_sum / ch_num);
    }
    else
    {
        *trust_avg = 0;
    }

// 显示字符分割效果
#if DEBUG_SEG_DOUBLE_SHOW
    cvResize(plate, resize, CV_INTER_LINEAR);

    // 图片保存开关
    if (0)
    {
        char name_save[100];
        static int32_t cnt_plate = 0;
        memset(name_save, 0, 100);
//        sprintf(name_save, "pp\\%s", name + 20);
//        sprintf(name_save, "pp\\image%04d.bmp", cnt_plate++);
        cvSaveImage(name_save, resize, 0);
    }

    cvNamedWindow("resize", CV_WINDOW_AUTOSIZE);
    cvShowImage("resize", resize);
    cvWaitKey(0);
    cvDestroyWindow("resize");

    cvReleaseImage(&plate);
    cvReleaseImage(&resize);
#endif

#ifdef _TMS320C6X_
    MEM_free(IRAM, ch_tmp, sizeof(Rects) * CH_MAX);
    MEM_free(IRAM, plate_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
#else
    CHECKED_FREE(bina_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(plate_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(ch_tmp, sizeof(Rects) * CH_MAX);
#endif

    return *trust_avg;
}

// 通过连通域标记的方法去除双层牌下层区域中的干扰小区域
static void remove_small_region_by_labelling_down(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    //  int32_t k;
    int32_t w, h;
    //    int32_t ch_num;
    Rects *restrict obj_pos = NULL;
    int32_t *restrict obj_area = NULL;
    uint16_t *restrict label_img = NULL;
    //    uint8_t *restrict ptr = NULL;
    //  int32_t obj_w;
    //  int32_t obj_h;

#ifdef _TMS320C6X_1
    obj_pos = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * LABEL_MAX, 128);
    obj_area = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * LABEL_MAX, 128);
    label_img = (uint16_t *)MEM_alloc(IRAM, sizeof(uint16_t) * img_w * img_h, 128);
#else
    CHECKED_MALLOC(obj_pos, sizeof(Rects) * LABEL_MAX, CACHE_SIZE);
    CHECKED_MALLOC(obj_area, sizeof(int32_t) * LABEL_MAX, CACHE_SIZE);
    CHECKED_MALLOC(label_img, sizeof(uint16_t) * img_w * img_h, CACHE_SIZE);
#endif

    image_label(bina_img, label_img, obj_pos, obj_area, img_w, img_h, LABEL_MAX);

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            // 判断像素点是否属于小于64的连通区域
            if ((label_img[img_w * h + w] > (uint16_t)0) && (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 64))
            {
                // 像素点所属连通区域小于36，直接删除
                if (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 36)
                {
                    bina_img[img_w * h + w] = (uint8_t)0;
                }
                else
                {
                    int32_t flag0, flag1;
                    Rects rect_label;

                    rect_label = obj_pos[label_img[img_w * h + w] - (uint16_t)1];

                    // 去除下边的螺钉
                    flag0 = (rect_label.y0 >= img_h - 8) && (rect_label.y1 >= img_h - 5);

                    // 去除高度过窄的区域，高度过窄的小区域为断裂的边框
                    flag1 = (rect_label.y1 - rect_label.y0 + 1 < 4);

                    if (flag0 || flag1)
                    {
                        bina_img[img_w * h + w] = (uint8_t)0;
                    }
                }
            }
        }
    }

#ifdef _TMS320C6X_1
    MEM_free(IRAM, obj_pos, sizeof(Rects) * LABEL_MAX, 128);
    MEM_free(IRAM, obj_area, sizeof(int32_t) * LABEL_MAX, 128);
    MEM_free(IRAM, label_img, sizeof(uint16_t) * img_w * img_h, 128);
#else
    CHECKED_FREE(label_img, sizeof(uint16_t) * img_w * img_h);
    CHECKED_FREE(obj_area, sizeof(int32_t) * LABEL_MAX);
    CHECKED_FREE(obj_pos, sizeof(Rects) * LABEL_MAX);
#endif

    return;
}

// 车牌上下边界的精确定位
static int32_t plate_upper_and_lower_locating_exact_down(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
                                                         int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t change;
    int32_t change_thresh;
    uint8_t *restrict ptr = NULL;

    // 统计中间9行的跳变数作为阈值
    change = 0;

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

    for (h = plate_h / 2 - 5; h >= 0; h--)
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

    for (h = plate_h / 2 + 5; h < plate_h; h++)
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

    // 对车牌下边界进行修正，解决与边框粘连的情况。
//    if (1)
    {
        int32_t *hist = NULL;
        int32_t new_w = plate_w - plate_w / 3;

        CHECKED_MALLOC(hist, sizeof(int32_t) * plate_h, CACHE_SIZE);
        memset(hist, 0, sizeof(int32_t) * plate_h);

        // 统计下半部分的投影
        for (h = plate_h - 1; h >= plate_h / 2; h--)
        {
            for (w = plate_w / 6; w < plate_w - plate_w / 6; w++)
            {
                hist[h] += bina_img[plate_w * h + w] > (uint8_t)0;
            }
        }

        for (h = plate_h - 1; h >= plate_h / 2; h--)
        {
            // 如果一行中前景占了宽度的3/4，认为找到了边框
            if (hist[h] > new_w * 3 / 4)
            {
                break;
            }
        }

        // 删除边框
        if ((h != plate_h / 2) && (h > plate_h * 2 / 3))
        {
            for (; h > plate_h * 2 / 3; h--)
            {
                if (hist[h] < new_w * 3 / 4)
                {
                    *y1 = h;
                    break;
                }
            }
        }

        CHECKED_FREE(hist, sizeof(int32_t) * plate_h);
    }
    return (change_thresh * 2);
}

// 去除双层车牌下边框的干扰
static void remove_upper_and_lower_border_down(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t len;
    int32_t start, end;
    const int32_t thresh = plate_h * 3 / 2;

    // 假设下边框最多只有7行
    for (h = plate_h - 7; h < plate_h; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            if (bina_img[plate_w * h + w])
            {
                start = w;

                while (bina_img[plate_w * h + w] && w < plate_w)
                {
                    w++;
                }

                end = w - 1;
                len = end - start + 1;

                if (len > thresh)
                {
#if DEBUG_SEGMENT_PRINT
                    printf("delete bottom line ......\n");
#endif
                    // 多删除一行，消除反光的影响
                    memset(bina_img + plate_w * (h + 0) + start, 0, len);
                    memset(bina_img + plate_w * (h - 1) + start, 0, len);
                }
            }
        }
    }

    return;
}

// 定位双层牌下层区域字符的左右边界
static void character_left_and_right_locating_iteration_down(uint8_t *restrict bina_img, Rects *ch, int32_t *ch_num1,
                                                             int32_t plate_w, int32_t plate_h)
{
    int32_t i, j;
//    int32_t m, n;
    int32_t k;
    int32_t ch_num;
    int32_t p;
    int32_t merge;
    int32_t iteration;
    int32_t *restrict hist = NULL;
    uint8_t *restrict merge_mask = NULL;

    CHECKED_MALLOC(hist, sizeof(int32_t) * plate_w, CACHE_SIZE);
    CHECKED_MALLOC(merge_mask, sizeof(uint8_t) * plate_w, CACHE_SIZE);

    // 二值图像向水平方向投影
    for (j = 0; j < plate_w; j++)
    {
        hist[j] = 0;

        for (i = 0; i < plate_h; i++)
        {
            hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    // 统计粘连字符的个数
    merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

    // 分割出来的字符数目太少或者粘连字符太多，认为和下边框发生了粘连
    iteration = 1;
    p = plate_h - 1;

    while ((ch_num <= 3 || merge >= 3) && (iteration < 5) && (p > plate_h - 4))
    {
        k = ch_num;

        // 一行一行的删除下边框，直到分割得到的字符大于ch_num + 1
        for (; p >= plate_h - 4; p--)
        {
            for (j = 0; j < plate_w; j++)
            {
                if (bina_img[plate_w * p + j] && (merge_mask[j] == (uint8_t)0))
                {
                    hist[j]--;
                }
            }

            character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

            if (ch_num >= k + 1)
            {
//                 for (m = p; m < plate_h; m++)
//                 {
//                     for (n = 0; n < plate_w; n++)
//                     {
//                         if (merge_mask[n] == (uint8_t)0)
//                         {
//                             bina_img[m * plate_w + n] = (uint8_t)0;
//                         }
//                     }
//                 }

                break;
            }
        }

        // 重新统计粘连字符的个数
        merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

        iteration++;
    }

    //     // 重新统计直方图
    //     for (j = 0; j < plate_w; j++)
    //     {
    //         hist[j] = 0;
    //
    //         for (i = 0; i < plate_h; i++)
    //         {
    //             hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
    //         }
    //     }
    //
    //     // 最终定位出每个字符的边界
    //     character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    *ch_num1 = ch_num;

    CHECKED_FREE(merge_mask, sizeof(uint8_t) * plate_w);
    CHECKED_FREE(hist, sizeof(int32_t) * plate_w);

    return;
}

// 投影法定位字符上下边界
static void character_upper_and_lower_locating_down(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
                                                    int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t k, p;
    int32_t sum;
    int32_t ch_num = *ch_num1;

    // 定位字符的上下边界
    for (k = 0; k < ch_num; k++)
    {
        ch[k].y0 = (int16_t)0;
        ch[k].y1 = (int16_t)(plate_h - 1);

        // 上边界
        p = -1;

        for (h = 0; h < plate_h; h++)
        {
            sum = 0;

            for (w = ch[k].x0; w <= ch[k].x1; w++)
            {
                sum += (int32_t)bina_img[plate_w * h + w]/* > 0*/;
            }

            if (sum > 0)
            {
                p = h;
                break;
            }
        }

        if (p != -1)
        {
            ch[k].y0 = (int16_t)p;
        }

        // 下边界
        p = -1;

        for (h = plate_h - 1; h >= 0; h--)
        {
            sum = 0;

            for (w = ch[k].x0; w <= ch[k].x1; w++)
            {
                sum += (int32_t)bina_img[plate_w * h + w]/* > 0*/;
            }

            if (sum > 0)
            {
                p = h;
                break;
            }
        }

        if (p != -1)
        {
            ch[k].y1 = (int16_t)p;
        }

        if (ch[k].y1 <= ch[k].y0)
        {
            ch[k].y0 = (int16_t)0;
            ch[k].y1 = (int16_t)(plate_h - 1);
        }
    }

    // 删除掉高度太小的伪字符区域
    for (k = 0; k < ch_num; k++)
    {
        // 字符高度小于车牌高度的1/4，确定为噪声
        if (ch[k].y1 - ch[k].y0 + 1 < plate_h / 4)
        {
            for (p = k; p < ch_num - 1; p++)
            {
                ch[p] = ch[p + 1];
            }

            ch_num--;
        }
    }

    *ch_num1 = ch_num;

    return;
}

// 删除较小的区域
static void remove_too_small_region_down(uint8_t *restrict bina_img, Rects *restrict ch,
                                         int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t k, n;
    int32_t ch_h;
    int32_t proj_h;
    int32_t avg_h;
//    int32_t avg_w;
    int32_t ch_num = *ch_num1;

    if (ch_num == 0)
    {
        return;
    }

//     // 删除宽度很小的区域
//     for (k = 0; k < ch_num; k++)
//     {
//         avg_w = get_character_average_width(ch, ch_num, plate_h);
//
//         // 如果字符宽度小于平均宽度的1/6，确定为噪声
//         if (ch[k].x1 - ch[k].x0 + 1 < avg_w / 6)
//         {
//             for (n = k; n < ch_num - 1; n++)
//             {
//                 ch[n] = ch[n + 1];
//             }
//
//             ch_num--;
//             k--;
//         }
//     }

    // 删除高度较小的区域
    for (k = 0; k < ch_num; k++)
    {
        proj_h = 0;

        avg_h = get_character_average_height(ch, ch_num, plate_h);

        for (w = ch[k].x0; w <= ch[k].x1; w++)
        {
            n = 0;

            for (h = ch[k].y0; h <= ch[k].y1; h++)
            {
                n += bina_img[plate_w * h + w] > (uint8_t)0;
            }

            proj_h = (n > proj_h) ? n : proj_h;
        }

        ch_h = ch[k].y1 - ch[k].y0 + 1;

        // 如果字符投影的最大值小于高度的1/4，或者字符高度小于平均高度的1/2，确定为噪声
        if ((proj_h < ch_h / 4) || (ch_h < avg_h / 2))
        {
            for (n = k; n < ch_num - 1; n++)
            {
                ch[n] = ch[n + 1];
            }

            ch_num--;
            k--;
        }
    }

    *ch_num1 = ch_num;

    return;
}

// 根据投影直方图寻找最佳分割点
static void find_valley_point(uint8_t *restrict bina_img, Rects *ch, int32_t init_point,
                              int32_t *best_point, int32_t plate_w/*, int32_t plate_h*/)
{
    int32_t w, h;
    int32_t k, idx;
    const int32_t x0 = ch->x0;
    const int32_t x1 = ch->x1;
    const int32_t y0 = ch->y0;
    const int32_t y1 = ch->y1;
    const int32_t ch_w = x1 - x0 + 1;
    int32_t left, right;
    int32_t min;
    int32_t *restrict hist = NULL;

    CHECKED_MALLOC(hist, ch_w * sizeof(int32_t), CACHE_SIZE);

    // 优先采用投影法寻找最佳分割点
    for (w = x0; w <= x1; w++)
    {
        hist[w - x0] = 0;

        for (h = y0; h <= y1; h++)
        {
            hist[w - x0] += bina_img[plate_w * h + w] > (uint8_t)0;
        }
    }

    // 确定搜索范围
    left  = MAX2(init_point - x0 - 5, 0);
    right = MIN2(init_point - x0 + 5, ch_w - 1);

    // 寻找局部极小值
    idx = 0;
    min = hist[left];

    for (k = left + 1; k <= right; k++)
    {
        if (hist[k] < min)
        {
            min = hist[k];
            idx = k;
        }
    }

    left  = MAX2(idx - 4, 0);
    right = MIN2(idx + 4, ch_w - 1);

    // 如果左右两边的值都比极小值大，则把该极小值点作为最佳分割点
    if ((hist[left] - min > 3) && (hist[right] - min > 3))
    {
        *best_point = x0 + idx;
    }
    else // 如果投影法没有找到最佳分割点则默认分割点为几何中心
    {
        *best_point = init_point;
    }

    CHECKED_FREE(hist, ch_w * sizeof(int32_t));

    return;
}

// 通过投影直方图来分离粘连字符
static int32_t spliting_by_projection_histogram(uint8_t *restrict bina_img, int32_t plate_w, /*int32_t plate_h,*/
                                                Rects *restrict ch, int32_t k, int32_t *ch_num1, int32_t avg_w)
{
    int32_t w, h;
    int32_t n;
    int32_t spliting_flag = 0;
    int32_t thresh;
    int32_t ch_num = *ch_num1;
    int32_t num_tmp = 0;
    int32_t num_add = 0;
    const int32_t ch_w = ch[k].x1 - ch[k].x0 + 1;
    const int16_t x0 = ch[k].x0;
    const int16_t x1 = ch[k].x1;
    const int16_t y0 = ch[k].y0;
    const int16_t y1 = ch[k].y1;
    int32_t *restrict hist = NULL;
    Rects *restrict ch_tmp = NULL;

    if (ch_w < 5)
    {
        return spliting_flag;
    }

    CHECKED_MALLOC(hist, sizeof(int32_t) * ch_w, CACHE_SIZE);
    CHECKED_MALLOC(ch_tmp, sizeof(Rects) * CH_MAX, CACHE_SIZE);

    for (w = x0; w <= x1; w++)
    {
        hist[w - x0] = 0;

        for (h = y0; h <= y1; h++)
        {
            hist[w - x0] += bina_img[plate_w * h + w] > (uint8_t)0;
        }
    }

    for (thresh = 0; thresh < 3; thresh++)
    {
        for (n = 0; n < ch_w; n++)
        {
            if (hist[n] > 0)
            {
                hist[n]--;
            }
        }

        character_left_and_right_locating_double(hist, ch_w, ch_tmp, &num_tmp);

        if (num_tmp > 1) // 分割成功
        {
            // 分割后增加的字符个数等于分裂出来的字符个数减去原来那个字符
            num_add = num_tmp - 1;

            if (ch_num + num_add < CH_MAX)
            {
                for (n = ch_num + num_add; n > k + num_add; n--)
                {
                    ch[n] = ch[n - num_add];
                }

                ch_num += num_add;

                for (n = k; n < k + num_tmp; n++)
                {
                    ch[n].x0 = (int16_t)(ch_tmp[n - k].x0 + x0);
                    ch[n].x1 = (int16_t)(ch_tmp[n - k].x1 - ch_tmp[n - k].x0 + ch[n].x0);
                    ch[n].y0 = (int16_t)(ch[k].y0);
                    ch[n].y1 = (int16_t)(ch[k].y1);
                }

                spliting_flag = 1;
            }

            break; // 分割成功，跳出
        }

        // 当单个字符左右存在干扰信息时，通过投影法也可以把左右的干扰信息去除掉，
        // 如果重新定位的字符宽度满足要求，也应该算分离成功
        // 这步操作对识别率有0.3%的提升
        else if ((num_tmp == 1) && (ch_tmp->x1 - ch_tmp->x0 + 1 <= avg_w * 4 / 3))
        {
            ch[k].x0 = (int16_t)(ch[k].x0 + ch_tmp->x0);
            ch[k].x1 = (int16_t)(ch_tmp->x1 - ch_tmp->x0 + ch[k].x0);
            spliting_flag = 1;
            break;
        }

    }

    CHECKED_FREE(ch_tmp, sizeof(Rects) * CH_MAX);
    CHECKED_FREE(hist, sizeof(int32_t) * ch_w);

    *ch_num1 = ch_num;

    return spliting_flag;
}

// 通过寻找波谷的方式分离粘连字符
static int32_t spliting_by_find_valley(uint8_t *restrict bina_img, int32_t plate_w, /*int32_t plate_h,*/
                                       Rects *ch, int32_t k, int32_t *ch_num1, int32_t avg_w)
{
    int32_t n;
    int32_t ch_w;
    int32_t ch_num = *ch_num1;
    int32_t init_point;
    int32_t best_point;
    int32_t spliting_flag = 0;

    if (ch_num < CH_MAX)
    {
        spliting_flag = 1;

        ch_w = ch[k].x1 - ch[k].x0 + 1;

        if (ch_w < avg_w * 27 / 10) // 两个字符粘连
        {
            init_point = ch[k].x0 + ch_w / 2;
        }
        else // 多个字符粘连
        {
            if (ch_w / 3 < avg_w * 3 / 2) // 三个字符粘连
            {
                init_point = ch[k].x0 + ch_w / 3;
            }
            else // 多个字符粘连
            {
                init_point = ch[k].x0 + avg_w;
            }
        }

        // 寻找波谷点
        find_valley_point(bina_img, &ch[k], init_point, &best_point, plate_w/*, plate_h*/);

        // |0 |1 |2 |3 |k==4|5 |6 |7 |8 |9 | 字符序号
        // |c0|c1|c2|c3|ck01|c5|c6|c7|c8|  | 原始字符序列
        //               |\
        //               | \
        // |c0|c1|c2|c3|ck0|ck1|c5|c6|c7|c8| 分离粘连字符后的字符序列

        // ch[k+0]保存原来字符ch[k]的前半部分
        // ch[k+1]保存原来字符ch[k]的后半部分

        // 分离后的字符加入ch
//      for (n = ch_num + 1; n > k + 1; n--)
        for (n = ch_num; n > k + 1; n--)
        {
            ch[n] = ch[n - 1];
        }

        ch_num++;

        // ch[k]保存原来字符ch[k]的前半部分
        ch[k].x1 = (int16_t)(best_point - 1);

        if (k > 0)  // 排除第一个字符(可能是汉字)
        {
            // 再次确认分割位置是否合适
            if (ch[k].x1 - ch[k].x0 + 1 < 4)
            {
                // 如果不合适，则选择初始分割点
                ch[k].x1 = (int16_t)(init_point - 1);
            }
        }

        // ch[k+1]保存原来字符ch[k]的后半部分
        ch[k + 1].x0 = (int16_t)(MIN2(ch[k + 0].x1 + 2, plate_w - 1));
        ch[k + 1].x0 = (int16_t)(MAX2(ch[k + 1].x0, ch[k + 0].x1 + 1));
        ch[k + 1].x1 = (int16_t)(MIN2(ch[k + 0].x0 + ch_w - 1, plate_w - 1));

        // 保证当前字符的右边界不超过下一个字符的左边界
        if (k + 2 < ch_num)
        {
            ch[k + 1].x1 = (int16_t)(MIN2(ch[k + 1].x1, ch[k + 2].x0));
        }

        ch[k + 1].y0 = (int16_t)(ch[k].y0);
        ch[k + 1].y1 = (int16_t)(ch[k].y1);
    }

    *ch_num1 = ch_num;

    return spliting_flag;
}

// 粘连字符分离
static void character_spliting_down(uint8_t *restrict bina_img, Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t ch_w;
    int32_t avg_w;
    int32_t ch_num = *ch_num1;
    int32_t spliting_flag = 0;

    for (k = 0; k < ch_num; k++)
    {
        int32_t flag = 1;
        int32_t ch_h;
        ch_w = ch[k].x1 - ch[k].x0 + 1;
        ch_h = ch[k].y1 - ch[k].y0 + 1;

//        flag = chs_width_cmp(ch, k, ch_num);

        if (k == 0)
        {
            flag = ch_w > ((ch[k + 1].x1 - ch[k + 1].x0 + 1) * 5 / 4);
        }
        else if (k == ch_num - 1)
        {
            flag = ch_w > ((ch[k - 1].x1 - ch[k - 1].x0 + 1) * 5 / 4);
        }
        else
        {
            // 前后字符的平均宽度
            flag = ch_w > (((ch[k - 1].x1 - ch[k - 1].x0 + 1) + (ch[k + 1].x1 - ch[k + 1].x0 + 1)) / 2 * 5 / 4);
        }

        /***********************************************************************************************
         *由于双层牌下层区域字符数较少，当发生粘连时，计算出的平均字符宽度很难反映真正的字符宽度，
         *因此直接利用字符的高度来得到平均宽度，双层牌下层区域字符宽高比为13:22，约为0.6
         ***********************************************************************************************/
//        avg_w = get_character_average_width(ch, ch_num, plate_h);
        avg_w = ch_h * 3 / 5;

        if ((ch_w > avg_w * 4 / 3) && flag) // 字符发生了粘连
        {
            // 通过投影直方图来分离粘连字符
            spliting_flag = spliting_by_projection_histogram(bina_img, plate_w, /*plate_h, */ch, k, &ch_num, avg_w);

            // 如果投影直方图的方法没有分离成功则采用寻找波谷的方法来分离
            if (spliting_flag == 0)
            {
                spliting_flag = spliting_by_find_valley(bina_img, plate_w/*, plate_h*/, ch, k, &ch_num, avg_w);
            }
            else
            {
                k--; // 防止通过投影直方图分离后的字符还是粘连字符
            }
        }
    }

    *ch_num1 = ch_num;

    // 检查车牌的右边界是否比左边界大
    for (k = 0; k < ch_num; k++)
    {
        if (ch[k].x1 < ch[k].x0)
        {
//          printf("left > right   error!!!!\n");
            ch[k].x0 = (int16_t)(ch[k].x1);
            ch[k].x1 = (int16_t)(ch[k].x0 + 1);
        }
    }

    return;
}

// 断裂字符合并
static void character_merging_down(/*uint8_t *restrict bina_img, */Rects *restrict ch, int32_t *ch_num1,
                                                                   /*int32_t plate_w, */int32_t plate_h)
{
    int32_t k, n;
    int32_t ch_w;
    int32_t merge_w;     // 字符合并后的宽度
    int32_t interval;    // 字符间隔
    int32_t avg_w;
    int32_t ch_num = *ch_num1;

    if (ch_num < 5)
    {
        return;
    }

    //    avg_w = get_character_average_width(ch, ch_num, plate_h);  // 计算平均宽度

    // |0   |1 |2 |3 |4 |5 |6 |7 |8 |9 | 字符序号
    // |c0  |c1|c2|c3|c4|c5|c6|c7|c8|c9| 原始字符序列
    // |c01 |c2|c3|c4|c5|c6|c7|c8|c9|  | 第1次合并后(k = 0)
    // |c012|c3|c4|c5|c6|c7|c8|c9|  |  | 第2次合并后(k = 0)
    //    |
    // 第3次合并还是需要从第1个字符(c012)开始－－注意！

    // 只需要从左往右一直向右合并就可以了
    for (k = 0; k < ch_num - 1; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;

        // 每次合并后都要重新计算平均宽度
        avg_w = get_character_average_width(ch, ch_num, plate_h);

        if (ch_w < (avg_w * 2 + 2) / 3) // 可能是断裂字符
        {
            // 检测右边
            merge_w  = ch[k + 1].x1 - ch[k].x0 + 1;
            interval = ch[k + 1].x0 - ch[k].x1 + 1;

            // 向右合并
            if ((merge_w < avg_w) && (interval <= 4))
            {
                ch[k].x1 = (int16_t)(ch[k + 1].x1);
                ch[k].y0 = (int16_t)(MIN2(ch[k].y0, ch[k + 1].y0));
                ch[k].y1 = (int16_t)(MAX2(ch[k].y1, ch[k + 1].y1));

                for (n = k + 1; n < ch_num - 1; n++)
                {
                    ch[n] = ch[n + 1];
                }

                // 合并后字符个数减1，同时k--回退到合并后的字符重新开始
                ch_num--;
                k--;
            }
        }
    }

    *ch_num1 = ch_num;

    return;
}

static void character_correct_down(uint8_t *restrict bina_img, Rects *restrict ch, int32_t *ch_num1,
                                   int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t avg_w, avg_h;
    int32_t ch_num = *ch_num1;

    avg_w = get_character_average_width(ch, ch_num, plate_h);
    avg_h = get_character_average_height(ch, ch_num, plate_h);

    // 字符补偿如果字符与后面一个字符间隔很小，且自身字符宽度较小，说明字符分割过窄，稍微扩展一些。
    for (k = 1; k < ch_num - 1; k++)
    {
        // 与后面一个字符间隔小于字符高度的1/4，说明字符不为1，
        // 字符宽度小于平均宽度的2/3，
        // 与前一个字符的间隔大于字符高度的1/4
        // 满足上面3个要求，认为字符被切割多了，往左和上都补偿2个像素
        if ((ch[k + 1].x0 - ch[k].x1 - 1 < avg_h / 4) && (ch[k].x1 - ch[k].x0 + 1 < avg_w * 2 / 3)
            && (ch[k].x0 - ch[k - 1].x1 - 1 > avg_h / 4))
        {
            ch[k].x0 = (int16_t)((ch[k].x0 - 3) > (ch[k - 1].x1 + 2) ? (ch[k].x0 - 3) : ch[k].x0);
            ch[k].y0 = (int16_t)((ch[k].y0 - 2) < 0 ? 0 : (ch[k].y0 - 2));

            avg_w = get_character_average_width(ch, ch_num, plate_h);
            avg_h = get_character_average_height(ch, ch_num, plate_h);
        }
    }

}

// 去除左右边界的非字符区域
static void remove_non_character_region_down(uint8_t *restrict bina_img, Rects *restrict chs, int32_t *ch_num,
                                             int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t m, n;
    int32_t avg_w;
    int32_t ch_w;
    int32_t area;
    int32_t cnt;
    Rects *restrict ch = NULL;

    if (*ch_num < 3)
    {
        return;
    }

    avg_w = get_character_average_width(chs, *ch_num, plate_h);



    ch = chs + *ch_num - 1; // 右边第一个字符

    // 左边第一个字符可能是'1', 因此不对其进行判断
    // 检查右边第一个字符是不是真正的字符区域
    ch_w = ch->x1 - ch->x0 + 1;

    if (ch_w > avg_w / 2)
    {
        cnt = 0;

        for (h = ch->y0; h <= ch->y1; h++)
        {
            for (w = ch->x0; w <= ch->x1; w++)
            {
                cnt += bina_img[plate_w * h + w] > (uint8_t)0;
            }
        }

        area = (ch->y1 - ch->y0 + 1) * (ch->x1 - ch->x0 + 1);

        if (cnt > area - 2 * (ch->y1 - ch->y0 + 1))
        {
            ch->x0 = 0;
            ch->x1 = 0;
        }
    }

    m = 0;

    for (n = 0; n < *ch_num; n++)
    {
        if (chs[n].x1 - chs[n].x0 > 0)
        {
            chs[m] = chs[n];
            m++;
        }
    }

    *ch_num = m;

    return;
}

// 浮点改为定点计算，DSP上花费周期数从1013245 cycles 降低到45441 cycles，精度降低没有对最终的识别率性能造成影响
#define EPSITION (100)   // 为了将浮点计算改为定点，将中间的一些计算值乘以100.
static uint8_t character_position_standardization(Rects *restrict ch, int32_t ch_num, int32_t plate_w)
{
    int32_t i, j, k;
    int32_t avg_w;
    int32_t x0, x1;
    int32_t std_len;
    int32_t std_ch_w;
    float std_ch_center_x_7[7] = {22.5f, 79.5f, 158.5f, 215.5f, 272.5f, 329.5f, 386.5f};// 单层牌中7个字符的中心位置X坐标
    float std_ch_center_x_5[5] = {32.5f, 112.5f, 192.5f, 272.5f, 352.5f};               // 双层牌中5个字符的中心位置X坐标
    int32_t std_ch_center_x[7];  // 存放标准车牌字符中心点的X坐标
    int32_t now_ch_center_x[7];  // 存放当前车牌字符中心点的X坐标
    uint8_t similarity = (uint8_t)0;      // 相似度
    int32_t c;           // 比例因子
    int32_t c_min;
    int32_t error;       // 位置误差
    int32_t error_min;   // 最小位置误差
    int32_t r;           // 匹配搜索半径
    int32_t r_min;
    Rects std_ch[7];
    float *std_ch_center_x_ptr = NULL;

//  const int32_t std_ch_center_x1_x = 45/2;                                     // 第一个字符的中心位置x坐标
//  const int32_t std_ch_center_x2_x = 45+12+45/2;                               // 第二个字符的中心位置x坐标
//  const int32_t std_ch_center_x3_x = 45+12+45+34+45/2;                         // 第三个字符的中心位置x坐标
//  const int32_t std_ch_center_x4_x = 45+12+45+34+45+12+45/2;                   // 第四个字符的中心位置x坐标
//  const int32_t std_ch_center_x5_x = 45+12+45+34+45+12+45+12+45/2;             // 第五个字符的中心位置x坐标
//  const int32_t std_ch_center_x6_x = 45+12+45+34+45+12+45+12+45+12+45/2;       // 第六个字符的中心位置x坐标
//  const int32_t std_ch_center_x7_x = 45+12+45+34+45+12+45+12+45+12+45+12+45/2; // 第七个字符的中心位置x坐标

    if ((ch_num == 7) || (ch_num == 5))
    {
        if (ch_num == 7)
        {
            std_len = 409;  // 单层牌（7字符）标准车牌长度409mm
            std_ch_w = 45;  // 单层牌（7字符）标准车牌字符宽度45mm
            std_ch_center_x_ptr = std_ch_center_x_7;

            // 计算得到比例因子c
            c = (ch[6].x1 - ch[0].x0 + 1) * 100 / std_len;

            for (k = 0; k < 7; k++)
            {
                now_ch_center_x[k] = (ch[k].x1 + ch[k].x0 + 1) * EPSITION / 2 - (ch[0].x0 + 1) * EPSITION;
            }
        }
        else
        {
            std_len = 385;  // 双层牌（5字符）标准车牌长度385mm
            std_ch_w = 60;  // 双层牌（5字符）标准车牌字符宽度60mm
            std_ch_center_x_ptr = std_ch_center_x_5;

            // 计算得到比例因子c
            c = (ch[4].x1 - ch[0].x0 + 1) * EPSITION / std_len;

            for (k = 0; k < 5; k++)
            {
                now_ch_center_x[k] = (ch[k].x1 + ch[k].x0 + 1) * EPSITION / 2 - (ch[0].x0 + 1) * EPSITION;
            }
        }

        // 与分割得到的字符位置进行比较，计算误差
        error_min = 1000000;
        r_min = 0;
        c_min = c;

        for (i = -5; i <= 5; i++)
        {
            r = std_ch_w * (c + i) / EPSITION;

            for (k = 0; k < ch_num; k++)
            {
                std_ch_center_x[k] = (int32_t)(std_ch_center_x_ptr[k] * (c + i));
            }

            for (j = -r; j <= r; j++)
            {
                error = 0;

                for (k = 0; k < ch_num; k++)
                {
                    if (ch_num == 7 && ((k == 1) || (k == 2)))
                    {
                        error += abs(now_ch_center_x[k] - (std_ch_center_x[k] - j * EPSITION)) * 2;
                    }
                    else
                    {
                        error += abs(now_ch_center_x[k] - (std_ch_center_x[k] - j * EPSITION));
                    }
                }

                if (error < error_min)
                {
                    error_min = error;
                    r_min = j;
                    c_min = c + i;
                }
            }
        }

        avg_w = (std_ch_w * c_min) / EPSITION;

        // 根据标准车牌尺寸计算得到当前车牌中字符的位置
        for (k = 0; k < ch_num; k++)
        {
            std_ch[k].x0 = (int16_t)MAX2(ch[0].x0 + ((std_ch_center_x_ptr[k] * c_min / EPSITION) - r_min) - avg_w / 2, 0);
            std_ch[k].x1 = (int16_t)MIN2(ch[0].x0 + ((std_ch_center_x_ptr[k] * c_min / EPSITION) - r_min) + avg_w / 2, plate_w - 1);
        }

        // 标准化
        for (k = 0; k < ch_num; k++)
        {
            x0 = ch[k].x0;
            x1 = ch[k].x1;

            // 对宽度较小的字符'1'不做处理
//            if ((x1 - x0) > avg_w / 2)
            {
                if (abs(std_ch[k].x0 - ch[k].x0) > avg_w / 2)
                {
                    x0 = (ch[k].x0 + std_ch[k].x0 * 3) / 4;
                }

                if (abs(std_ch[k].x1 - ch[k].x1) > avg_w / 2)
                {
                    x1 = (ch[k].x1 + std_ch[k].x1 * 3) / 4;
                }

                if (x1 > x0)
                {
                    ch[k].x0 = (int16_t)x0;
                    ch[k].x1 = (int16_t)x1;
                }
            }
        }

        // 误差归一化到车牌宽度
        error_min = error_min / (ch_num * avg_w);

        if (error_min > 100)
        {
            error_min = 100;
        }

        similarity = (uint8_t)(100 - error_min);
    }

    return similarity;
}
#undef EPSITION

// 对栅栏等误识别的，进行检查，如果1较多，且字符间间距又很小的，认为是误识别
static void more_chs_1_delete(uint8_t *chs, Rects *ch, int32_t ch_num, uint8_t *not_ch_cnt)
{
    int32_t chs_1 = 0;
    int32_t k;

    for (k = ch_num - 5; k < ch_num - 1; k++)
    {
        if (chs[k] == (uint8_t)'1' || chs[k + 1] == (uint8_t)'1') // 有字符1，且字符间距小
        {
            if (ch[k + 1].x0 - ch[k].x1 + 1 < (ch[k].y1 - ch[k].y0 + 1) / 8)
            {
                *not_ch_cnt = (uint8_t)ch_num;
                break;
            }
        }
    }

    if (ch_num != 0)
    {
        for (k = 1; k < ch_num; k++)
        {
            chs_1 += (chs[k] == (uint8_t)'1') ? 1 : 0;   // 统计字符1的个数
        }

        if (chs_1 >= 4) // 不小于4个，认为误识别到非车牌
        {
            *not_ch_cnt = (uint8_t)ch_num;
        }
    }

    return;
}

static void double_plate_down_character_recognition(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                                    int32_t plate_w, int32_t plate_h, Rects *restrict ch, uint8_t *restrict chs,
                                                    uint8_t *trust, uint8_t *restrict ch_buf, int32_t *ch_num1,
                                                    SvmModel *restrict model_english,
                                                    SvmModel *restrict model_numbers,
                                                    SvmModel *restrict model_chinese,
                                                    uint8_t *not_ch_cnt, uint8_t bkgd_color, int32_t center_rate)
{
    int32_t k;
    int32_t ch_num = *ch_num1;
    int32_t center_flag[CH_MAX];
    int32_t best_place = 255;
    int32_t max_stand = 0;
    uint8_t is_ch;

    memset(center_flag, 0, CH_MAX * sizeof(int32_t));

    character_border_extend(ch, &ch_num, 1, plate_w, plate_h);

    // 上下层字符区域的中心位置必须比较接近
    for (k = 0; k < ch_num - 4; k++)
    {
        int32_t center_rate_tmp = ((ch[k].x0 + ch[k + 4].x1) / 2) * 100 / plate_w;

        if (abs(center_rate_tmp - center_rate) < 10)
        {
            center_flag[k] = 1;
        }
    }

    for (k = 0; k < ch_num - 4; k++)
    {
        if (center_flag[k] != 0)
        {
            int32_t tmp_stand = (int32_t)character_position_standardization(&ch[k], 5, plate_w);

            // 同时满足上下层区域中心位置距离和字符标准度
            if ((tmp_stand > max_stand) && (tmp_stand > 90))
            {
                max_stand = tmp_stand;
                best_place = k;
            }
        }
    }

    // 没有适合的组合
    if (best_place == 255)
    {
        *ch_num1 = 0;

        return;
    }

    for (k = best_place; k < best_place + 5; k++)
    {
        ch[k - best_place] = ch[k];
    }

    if (bkgd_color == (0u)) // 绿牌车，上面有3个字符
    {
        for (k = 0; k < 5; k++)
        {
            int32_t ch_w = ch[k].x1 - ch[k].x0 + 1;
            int32_t ch_h = ch[k].y1 - ch[k].y0 + 1;

            // 宽度太窄，直接确认为数字1
            if (ch_w < ch_h / 3)
            {
                chs[k + 3] = (uint8_t)('1');
                trust[k + 3] = (90u);
                is_ch = (1u);
            }
            else
            {
                single_character_recognition(gray_img, plate_w, ch[k], &chs[k + 3],
                                             &trust[k + 3], ch_buf + STD_AREA * (k + 3), model_numbers, (1u), (0u), &is_ch);
            }

            if (is_ch == (0u))
            {
                *not_ch_cnt += 1;
            }
        }

        more_chs_1_delete(chs, ch, 8, not_ch_cnt);

        if (*not_ch_cnt >= 4)
        {
            *not_ch_cnt = 8;
        }
    }
    else                // 黄牌车，白牌车，上面有2个字符
    {
        for (k = 0; k < 5; k++)
        {
            int32_t ch_w = ch[k].x1 - ch[k].x0 + 1;
            int32_t ch_h = ch[k].y1 - ch[k].y0 + 1;

            // 宽度太窄，直接确认为数字1
            if (ch_w < ch_h / 3)
            {
                chs[k + 2] = (uint8_t)('1');
                trust[k + 2] = (90u);
                is_ch = (1u);
            }
            else
            {
                single_character_recognition(gray_img, plate_w, ch[k], &chs[k + 2],
                                             &trust[k + 2], ch_buf + STD_AREA * (k + 2), model_numbers, (1u), (0u), &is_ch);
            }

            if (is_ch == (0u))
            {
                *not_ch_cnt += 1;
            }
        }

        more_chs_1_delete(chs, ch, 7, not_ch_cnt);

        if (*not_ch_cnt >= 4)
        {
            *not_ch_cnt = 7;
        }
    }

    *ch_num1 = 5;

    return;
}

// 双层牌下层区域字符分割识别
uint8_t character_segmentation_double_down(uint8_t *restrict src_img, int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                                           int32_t *ch_num1, uint8_t *restrict chs, uint8_t *trust, uint8_t *trust_avg,
                                           uint8_t *restrict ch_buf,
                                           SvmModel *restrict model_english,
                                           SvmModel *restrict model_numbers,
                                           SvmModel *restrict model_chinese,
                                           uint8_t *not_ch_cnt, uint8_t bkgd_color, int32_t center_rate)
{
//  int32_t yy = 0;
    int32_t k;
    int32_t y0, y1;
    int32_t ch_num;
    int32_t ch_h;
    uint8_t result = (uint8_t)0;
    uint8_t *restrict plate_img = NULL;
    uint8_t *restrict bina_img = NULL;
    uint8_t *restrict gray_img = NULL;
    Rects *restrict ch_tmp = NULL;

#if DEBUG_SEG_DOUBLE_SHOW
    int32_t x0 = 0;             //双层牌下层区域不进行左右边界定位
    int32_t x1 = plate_w - 1;
    int32_t w, h;
    int32_t offset = 0;
    IplImage *plate = cvCreateImage(cvSize(plate_w, plate_h * 8), IPL_DEPTH_8U, 3);
    IplImage *resize = cvCreateImage(cvSize(plate_w * 2, plate_h * 8 * 2), IPL_DEPTH_8U, 3);

    memset(plate->imageData, 0, plate->widthStep * plate->height);

    // 显示原始灰度图像
    _IMAGE3_(plate, src_img, plate_w, plate_h, 0)
#endif

    if (plate_h < 10)
    {
        *ch_num1 = 0;
        return result;
    }

#ifdef _TMS320C6X_
    ch_tmp = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * CH_MAX, 128);
    plate_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
#else
    CHECKED_MALLOC(ch_tmp, sizeof(Rects) * CH_MAX, CACHE_SIZE);
    CHECKED_MALLOC(plate_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
#endif

    contrast_extend_for_gary_image(plate_img, src_img, plate_w, plate_h);

// 显示灰度图像增强效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h;
    _IMAGE3_(plate, plate_img, plate_w, plate_h, offset)
#endif

    thresholding_percent(plate_img, bina_img, plate_w, plate_h, plate_w / 4, plate_w - plate_w / 4,
                         plate_h / 5, plate_h - plate_h / 5, 30);

// 显示二值化效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset  = plate_h * 2;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)
#endif

    // 噪声去除
    remove_small_region_by_labelling_down(bina_img, plate_w, plate_h);

// 显示噪声去除效果
#if DEBUG_SHOW12
    offset  = plate_h * 3;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)
#endif

    // 精确定位车牌上下边界
    plate_upper_and_lower_locating_exact_down(bina_img, &y0, &y1, plate_w, plate_h);

// 显示车牌上下左右边界定位效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 3;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)

    cvRectangle(plate, cvPoint(x0, y0 + offset), cvPoint(x1, y1 + offset),
                cvScalar(0, 255, 0, 0), 1, 1, 0);
#endif

    ch_h = y1 - y0 + 1;

    if (ch_h < 10)
    {
        *ch_num1 = 0;

#ifdef _TMS320C6X_
        MEM_free(IRAM, ch_tmp, sizeof(Rects) * CH_MAX);
        MEM_free(IRAM, plate_img, sizeof(uint8_t) * plate_w * plate_h);
        MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
#else
        CHECKED_FREE(bina_img, sizeof(uint8_t) * plate_w * plate_h);
        CHECKED_FREE(plate_img, sizeof(uint8_t) * plate_w * plate_h);
        CHECKED_FREE(ch_tmp, sizeof(Rects) * CH_MAX);
#endif
        return result;
    }
    else
    {
#ifdef _TMS320C6X_
        gray_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * ch_h, 128);
#else
        CHECKED_MALLOC(gray_img, sizeof(uint8_t) * plate_w * ch_h, CACHE_SIZE);
#endif
    }

    // 精确定位车牌左右边界
//     if (bkgd_color == (uint8_t)1)
//     {
//         plate_left_and_right_locating_exact(bina_img, plate_w, plate_h, &x0, &x1, ch_h, bkgd_color);
//     }
//     else
//     {
//         x0 = 0;
//         x1 = plate_w - 1;
//     }
//
    ch_num = 0;

    memcpy(gray_img, plate_img + plate_w * y0, plate_w * ch_h);
    memcpy(bina_img, bina_img + plate_w * y0, plate_w * ch_h);

    // 去除下边框的干扰
    remove_upper_and_lower_border_down(bina_img, plate_w, ch_h);

    // 去除孤立的前景行，由以前的只处理上下1/3改为对整个车牌图像都作处理，对识别率有0.2%提升
    remove_isolated_lines(bina_img, 0, ch_h - 1, plate_w, ch_h);

    // 采用投影法定位字符的左右边界
    character_left_and_right_locating_iteration_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // 采用投影法定位下层区域字符的上下边界
    character_upper_and_lower_locating_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// 显示字符上下左右边界定位效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 4 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    // 删除下层区域中太小的字符区域
    remove_too_small_region_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // 下层区域粘连字符分离
    character_spliting_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // 粘连字符分离后，需要再次重新定位字符上下边界
    character_upper_and_lower_locating_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// 显示粘连字符分离效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 5 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset)

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif
    // 断裂字符合并
    character_merging_down(/*bina_img, */ch_tmp, &ch_num, /*plate_w, */ch_h);

    // 字符纠正
    character_correct_down(bina_img, ch_tmp, &ch_num, plate_w, plate_h);

    // 再次去除太小的区域
    remove_too_small_region_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// 显示断裂字符合并效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 6 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    //  find_missing_character(ch_tmp, &ch_num, plate_w, ch_h);

    // 去除左右边界的非字符区域
    remove_non_character_region_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// 显示最终字符分割效果
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 7;
    _IMAGE3_(plate, plate_img, plate_w, plate_h, offset);

    offset = plate_h * 7 + y0;

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 0, 255, 0), 1, 1, 0);
    }

#endif

    if (ch_num < 5)
    {
        ch_num = 0;
    }
    else
    {
        double_plate_down_character_recognition(gray_img, bina_img, plate_w, ch_h, ch_tmp, chs, trust, ch_buf, &ch_num,
                                                model_english, model_numbers, model_chinese, not_ch_cnt, bkgd_color, center_rate);
    }

//  printf("-------------result = %d ch_num = %d\n", result, *ch_num1);

    if ((bkgd_color == (0u)) && (ch_num == 5))
    {
        for (k = 0; k < ch_num; k++)
        {
            ch[k + 3].x0 = ch_tmp[k].x0;
            ch[k + 3].x1 = ch_tmp[k].x1;
            ch[k + 3].y0 = ch_tmp[k].y0 + y0;
            ch[k + 3].y1 = ch_tmp[k].y1 + y0;
        }

        ch_num = 8;
    }
    else if ((bkgd_color == (1u)) && (ch_num == 5))
    {
        for (k = 0; k < ch_num; k++)
        {
            ch[k + 2].x0 = ch_tmp[k].x0;
            ch[k + 2].x1 = ch_tmp[k].x1;
            ch[k + 2].y0 = ch_tmp[k].y0 + y0;
            ch[k + 2].y1 = ch_tmp[k].y1 + y0;
        }

        ch_num = 7;
    }

    if (ch_num > 0)
    {
        int trust_sum = 0;

        for (k = 0; k < ch_num; k++)
        {
            trust_sum += (int32_t)trust[k];
        }

        *trust_avg = (uint8_t)(trust_sum / ch_num);
    }
    else
    {
        *trust_avg = 0;
    }

    *ch_num1 = ch_num;

// 显示识别后的字符定位效果(在字符识别中还有对字符边界的调整)
#if DEBUG_SEG_DOUBLE_SHOW0
    offset = plate_h * 7;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset)

    offset = plate_h * 7 + y0;

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

#if DEBUG_SEG_DOUBLE_SHOW
    cvResize(plate, resize, CV_INTER_LINEAR);

    // 图片保存开关
    if (0)
    {
        char name_save[100];
        static int32_t cnt_plate = 0;
        memset(name_save, 0, 100);
//        sprintf(name_save, "pp\\%s", name + 20);
//        sprintf(name_save, "pp\\image%04d.bmp", cnt_plate++);
        cvSaveImage(name_save, resize, 0);
    }

    cvNamedWindow("resize", CV_WINDOW_AUTOSIZE);
    cvShowImage("resize", resize);
    cvWaitKey(0);
    cvDestroyWindow("resize");

    cvReleaseImage(&plate);
    cvReleaseImage(&resize);

#endif

#ifdef _TMS320C6X_
    MEM_free(IRAM, ch_tmp, sizeof(Rects) * CH_MAX);
    MEM_free(IRAM, plate_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, gray_img, sizeof(uint8_t) * plate_w * ch_h);
#else
    CHECKED_FREE(gray_img, sizeof(uint8_t) * plate_w * ch_h);
    CHECKED_FREE(bina_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(plate_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(ch_tmp, sizeof(Rects) * CH_MAX);
#endif

    return *trust_avg;
}
