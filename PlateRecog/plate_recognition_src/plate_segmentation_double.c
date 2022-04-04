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

// ɾ��������ǰ���У����ٶ�ͶӰ�ַ��ָ�ĸ���
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

// ���ݴ�ֱͶӰֱ��ͼ��λ�ַ������ұ߽�
static void character_left_and_right_locating_double(int32_t *restrict hist, int32_t len, Rects *restrict ch, int32_t *ch_num1)
{
    int32_t k;
    int32_t ch_num;

    // ��λ�ַ������ұ߽�
    ch_num = 0;

    // 2012.6.8����len - 1�޸�Ϊlen�� len - 1������£����λ�����ַ��ұ߽������ǰ�Ļ�������һ������
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

// �����ַ���ƽ�����
static int32_t get_character_average_width(Rects *restrict ch, int32_t ch_num, int32_t plate_h)
{
    int32_t k;
    int32_t num = 0;
    int32_t avg_w = 0;
    int32_t ch_w, ch_h;
    uint8_t *restrict is_1 = NULL;

    CHECKED_MALLOC(is_1, sizeof(uint8_t) * ch_num, CACHE_SIZE);

    // �����ж��ַ�1
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
                // �����ǰ�ַ���ǰһ���ַ�����࣬��ǰ�ַ���Ȳ�����ͳ��
                if (ch_w > (ch[k - 1].x1 - ch[k - 1].x0 + 1) * 3 / 2)
                {
                    continue;
                }
            }

            if ((k + 1 < ch_num) && (!is_1[k + 1]))
            {
                // �����ǰ�ַ��Ⱥ�һ���ַ�����࣬��ǰ�ַ���Ȳ�����ͳ��
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
        // Ĭ��ƽ�����
        avg_w = plate_h * 2 / 3;
    }

    CHECKED_FREE(is_1, sizeof(uint8_t) * ch_num);

    return avg_w;
}

// �����ַ���ƽ���߶�
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
            // ��ǰ�ַ��߶�Ҫ�������ַ��߶ȵ�һ����Χ�ڲż���ͳ��
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


// ͳ��ճ���ַ��ĸ���
static int32_t character_merge_numbers(uint8_t *restrict merge_mask, Rects *ch, int32_t ch_num,
                                       int32_t plate_w, int32_t plate_h)
{
    int32_t ch_w;
    int32_t avg_w;
    int32_t k;
    int32_t merge = 0;

    memset(merge_mask, 1, plate_w);

    avg_w = get_character_average_width(ch, ch_num, plate_h);  // ����ƽ�����

    for (k = 0; k < ch_num; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;

        if (ch_w > avg_w * 8 / 3)   // ���������ַ�ճ��
        {
            merge += 3;
            memset(merge_mask + ch[k].x0, 0, ch_w);
        }
        else if (ch_w > avg_w * 3 / 2)  // �����ַ�ճ��
        {
            merge += 2;
            memset(merge_mask + ch[k].x0, 0, ch_w);
        }
    }

    return merge;
}

// ͨ����ͨ���ǵķ���ȥ��˫�����ϲ������еĸ���С����
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
            // �ж����ص��Ƿ�����С��80����ͨ����
            if ((label_img[img_w * h + w] > (uint16_t)0) && (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 80))
            {
                // ���ص�������ͨ����С��36��ֱ��ɾ��
                if (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 36)
                {
                    bina_img[img_w * h + w] = (uint8_t)0;
                }
                else
                {
                    int32_t flag0, flag1;
                    Rects rect_label;

                    rect_label = obj_pos[label_img[img_w * h + w] - (uint16_t)1];

                    // ȥ���ϱߵ��ݶ�
                    flag0 = /*(rect_label.y0 <= 5) && */(rect_label.y1 <= 10);

                    // ȥ���߶ȹ�խ�����򣬸߶ȹ�խ��С����Ϊ���ѵı߿�
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

// ˫�����ϲ��������±߽�ľ�ȷ��λ
static int32_t plate_upper_and_lower_locating_exact_up(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
                                                       int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t change;
    int32_t change_thresh;
    uint8_t *restrict ptr = NULL;

    // ͳ���м�9�е���������Ϊ��ֵ
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

    // ���м�����ɨ��õ��ϱ߽�
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

        // ������С����ֵ���ҵ��˳��Ƶ��ϱ߽�
        if (change < change_thresh)
        {
            *y0 = h;
            break;
        }
    }

    // ���м�����ɨ��õ��±߽�
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

        // ������С����ֵ���ҵ��˳��Ƶ��±߽�
        if (change < change_thresh)
        {
            *y1 = h;
            break;
        }
    }

    return (change_thresh * 2);
}

// ȥ��˫�������ϱ߿�ĸ���
static void remove_upper_and_lower_border_up(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t len;
    int32_t start, end;
    const int32_t thresh = plate_h * 5 / 3;  // �ϲ������߱�Ϊ4:3, ȡ��ֵʱ��չһ�㵽5:3

    // �����ϱ߿����ֻ��5��
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
                    // ��ɾ��һ�У����������Ӱ��
                    memset(bina_img + plate_w * (h + 0) + start, 0, len);
                    memset(bina_img + plate_w * (h + 1) + start, 0, len);
                }
            }
        }
    }

    return;
}

// ���õ����ķ�����λ�ַ����ұ߽�
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

    // ��ֵͼ����ˮƽ����ͶӰ
    for (j = 0; j < plate_w; j++)
    {
        hist[j] = 0;

        for (i = 0; i < plate_h; i++)
        {
            hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    // ͳ��ճ���ַ��ĸ���
    merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

    // ����ַ���Ŀ̫�ٻ���ճ���ַ�̫�࣬��Ϊ���ϱ߿�����ճ��
    iteration = 1;
    p = 0;

    while ((ch_num <= 3 || merge >= 3) && (iteration < 5) && (p < 5))
    {
        k = ch_num;

        // һ��һ�е�ɾ���ϱ߿�ֱ���ָ�õ����ַ�����ch_num + 1
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

        // ����ͳ��ճ���ַ��ĸ���
        merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

        iteration++;
    }

    // �ų�����������������Ĺ���ǰ���ж�ͶӰ��Ӱ��
    remove_isolated_lines(bina_img, 0, plate_h / 3 - 1, plate_w, plate_h);
    remove_isolated_lines(bina_img, plate_h * 2 / 3, plate_h - 1, plate_w, plate_h);

    // ����ͳ��ֱ��ͼ
    for (j = 0; j < plate_w; j++)
    {
        hist[j] = 0;

        for (i = 0; i < plate_h; i++)
        {
            hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    // ���ն�λ��ÿ���ַ��ı߽�
    character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    *ch_num1 = ch_num;

    CHECKED_FREE(merge_mask, sizeof(uint8_t) * plate_w);
    CHECKED_FREE(hist, sizeof(int32_t) * plate_w);

    return;
}

// ���м俪ʼ�Ҽ��ȷ���ϲ������ַ�����λ��
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

    // �ұ��ַ�̫խ
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

    // �ұ��ַ�̫��,��ȴ��ڸ߶ȵ�5/3,��Ϊ��ճ��
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

    // ����ַ�̫խ
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

    // ����ַ�̫��,��ȴ��ڸ߶ȵ�5/3,��Ϊ��ճ��
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

    // ����ַ�̫խ
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

    // ����ַ�̫��,��ȴ��ڸ߶ȵ�5/3,��Ϊ��ճ��
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

    // �ұ��ַ�̫խ
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

// �׵׺���˫�����ϲ������ַ���ȷ��λ, ��Ժ��ƻ��ƺͰ��Ƶ�,�ϲ�����Ϊ����+��ĸ
static int32_t character_up_decide_white_back(uint8_t *bina_img, Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t interval[CH_MAX];
    int32_t interval_num = 0;
    int32_t ch_num = *ch_num1;
    int32_t min_dist = plate_w;
    int32_t best_place = 0;

    // ���뱣֤�ַ�����С��2
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
        // �Ҳ������ʵļ����ֱ���ҵ��м�λ�ý����ַ���λ
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

    // ���������ַ���λ��
    best_character_adjust_white_back(bina_img, ch, &ch_num, plate_w, plate_h, best_place);

    ch[0] = ch[best_place];
    ch[1] = ch[best_place + 1];

    // �׵׺���˫�����ϲ�����ֻ����2���ַ�
    *ch_num1 = 2;

    return 1;
}

// �ڵװ���˫�����ϲ������ַ���ȷ��λ,���ũ��վ����,�ϲ�����Ϊ����+2������
static int32_t character_up_decide_black_back(uint8_t *bina_img, Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t interval[CH_MAX];
    int32_t interval_num = 0;
    int32_t ch_num = *ch_num1;
    int32_t min_dist = plate_w;
    int32_t best_place = 0;

    // ���뱣֤�ַ�����С��3
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

    // �ַ���С��3�����ߵõ����ַ������������2���ַ�
    if ((ch_num < 3) || (ch_num - 1 - best_place < 2))
    {
        return 0;
    }

    ch[0] = ch[best_place];
    ch[1] = ch[best_place + 1];
    ch[2] = ch[best_place + 2];

    // �ڵװ���˫�����ϲ�����ֻ����3���ַ�
    *ch_num1 = 3;

    return 1;
}

// ͶӰ����λ˫�����ϲ������ַ������±߽�
static void character_upper_and_lower_locating_up(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
                                                  int32_t plate_w, int32_t plate_h, uint8_t bkgd_color)
{
    int32_t w, h;
    int32_t k, p;
    int32_t sum;
    int32_t ch_num = *ch_num1;
    int32_t max_h = 0;

    // ��λ�ַ������±߽�
    for (k = 0; k < ch_num; k++)
    {
        ch[k].y0 = (int16_t)0;
        ch[k].y1 = (int16_t)(plate_h - 1);

        // �ϱ߽�
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

        // �±߽�
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

    // ��ũ��վ˫���Ƶķָ��Ѷȸ���Ϊ�˸�׼ȷ��ȷ���ַ��߿򣬽����±߽�ͳһ�涨Ϊ��ַ��ı߽�
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

// ��չ�ַ��߽�
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

// ˫�����ϲ������ַ�ʶ��
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

    // �Ժ��ֽ���ʶ��
    // ��ʱ�޷��õ������жԺ��ֵľ�ȷ��λ
    //    chinese_left_right_locating_by_projection(gray_img, plate_w, plate_h, ch, *ch_num);

    // �Ժ����ַ�����ʶ��
    single_character_recognition(gray_img, plate_w, ch[0], &chs[0],
                                 &trust[0], ch_buf, model_chinese, (0u), (1u), &is_ch);

    if (is_ch == (0u))
    {
        not_ch_number++;
    }

    // ���ַ�����ʶ��
    if (bkgd_color == (0u))    // �������ϲ������2�����ֽ���ʶ��
    {
        // ������ĸ
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
    else                   // �Ի��ƻ����˫�����ϲ��������ĸ����ʶ��
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

// ˫�����ϲ������ַ��ָ�ʶ��
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
    // �Աȶ�����
    contrast_extend_for_gary_image(plate_img, src_img, plate_w, plate_h);

#if DEBUG_SEG_DOUBLE_SHOW
    _IMAGE3_(plate, plate_img, plate_w, plate_h, plate_h)
#endif

    // ��ֵ��ͼ��
    // ͼ���м�4/7������2/3������Ϊͳ������
    // �м�4/7Ϊ��׼ģ�����ϲ��ַ����������²��ַ������ȱȣ�����2/3��Ϊ��ȥ������߿����
    thresholding_percent(plate_img, bina_img, plate_w, plate_h, plate_w * 3 / 14, plate_w - plate_w * 3 / 14,
                         plate_h / 3, plate_h - 1/*plate_h / 5*/, 20);

#if DEBUG_SEG_DOUBLE_SHOW
    _IMAGE3_(plate, bina_img, plate_w, plate_h, plate_h * 2)
#endif

    // ˫�����ϲ��������±߽羫ȷ��λ
    plate_upper_and_lower_locating_exact_up(bina_img, &y0, &y1, plate_w, plate_h);

    // ɾ����������±߽���Ϣ
    memset(bina_img, 0, plate_w * y0);
    memset(bina_img + plate_w * y1, 0, plate_w * (plate_h - y1));

    // ���Ҹ�������ȥ�������ҷֱ�ɾ��1/7,��֤�ϲ������ַ�������ɾ��
    x0 = plate_w / 7;
    x1 = plate_w - plate_w / 7;

    for (h = 0; h < plate_h; h++)
    {
        memset(bina_img + plate_w * h, 0, x0);
        memset(bina_img + plate_w * h + x1, 0, plate_w - x1);
    }

// ��ʾ�����ϲ��������±߽綨λЧ��
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 3;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)

    cvRectangle(plate, cvPoint(x0, y0 + offset), cvPoint(x1, y1 + offset),
                cvScalar(0, 255, 0, 0), 1, 1, 0);
#endif

    // ��ͨ��ȥ���ݶ���Բ��ȸ�������
    remove_small_region_by_labelling_up(bina_img, plate_w, plate_h);

    ch_h = y1 - y0 + 1;
    memcpy(bina_img, bina_img + plate_w * y0, plate_w * ch_h);

    // ȥ���ϱ߿�
    remove_upper_and_lower_border_up(bina_img, plate_w, ch_h);

    // ȥ������ǰ����
    remove_isolated_lines(bina_img, 0, ch_h - 1, plate_w, ch_h);

// ��ʾС���򼰱߿�ȸ�������ȥ��Ч��
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 4 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset)
#endif

    // ����ͶӰ����λ�ַ����ұ߽�
    character_left_and_right_locating_iteration_up(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    if (bkgd_color == (1u))
    {
        // �׵׺���˫�����ϲ������ַ���ȷ��λ
        err_ret = character_up_decide_white_back(bina_img, ch_tmp, &ch_num, plate_w, ch_h);
    }
    else
    {
        // �ڵװ���˫�����ϲ������ַ���ȷ��λ
        err_ret = character_up_decide_black_back(bina_img, ch_tmp, &ch_num, plate_w, ch_h);
    }

    if (err_ret == 0)
    {
        ch_num = 0;
    }

    character_upper_and_lower_locating_up(bina_img, ch_tmp, &ch_num, plate_w, ch_h, bkgd_color);

// ��ʾ�ַ��������ұ߽綨λЧ��
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

// ��ʾ�ַ��ָ�Ч��
#if DEBUG_SEG_DOUBLE_SHOW
    cvResize(plate, resize, CV_INTER_LINEAR);

    // ͼƬ���濪��
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

// ͨ����ͨ���ǵķ���ȥ��˫�����²������еĸ���С����
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
            // �ж����ص��Ƿ�����С��64����ͨ����
            if ((label_img[img_w * h + w] > (uint16_t)0) && (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 64))
            {
                // ���ص�������ͨ����С��36��ֱ��ɾ��
                if (obj_area[label_img[img_w * h + w] - (uint16_t)1] < 36)
                {
                    bina_img[img_w * h + w] = (uint8_t)0;
                }
                else
                {
                    int32_t flag0, flag1;
                    Rects rect_label;

                    rect_label = obj_pos[label_img[img_w * h + w] - (uint16_t)1];

                    // ȥ���±ߵ��ݶ�
                    flag0 = (rect_label.y0 >= img_h - 8) && (rect_label.y1 >= img_h - 5);

                    // ȥ���߶ȹ�խ�����򣬸߶ȹ�խ��С����Ϊ���ѵı߿�
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

// �������±߽�ľ�ȷ��λ
static int32_t plate_upper_and_lower_locating_exact_down(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
                                                         int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t change;
    int32_t change_thresh;
    uint8_t *restrict ptr = NULL;

    // ͳ���м�9�е���������Ϊ��ֵ
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

    // ���м�����ɨ��õ��ϱ߽�
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

        // ������С����ֵ���ҵ��˳��Ƶ��ϱ߽�
        if (change < change_thresh)
        {
            *y0 = h;
            break;
        }
    }

    // ���м�����ɨ��õ��±߽�
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

        // ������С����ֵ���ҵ��˳��Ƶ��±߽�
        if (change < change_thresh)
        {
            *y1 = h;
            break;
        }
    }

    // �Գ����±߽���������������߿�ճ���������
//    if (1)
    {
        int32_t *hist = NULL;
        int32_t new_w = plate_w - plate_w / 3;

        CHECKED_MALLOC(hist, sizeof(int32_t) * plate_h, CACHE_SIZE);
        memset(hist, 0, sizeof(int32_t) * plate_h);

        // ͳ���°벿�ֵ�ͶӰ
        for (h = plate_h - 1; h >= plate_h / 2; h--)
        {
            for (w = plate_w / 6; w < plate_w - plate_w / 6; w++)
            {
                hist[h] += bina_img[plate_w * h + w] > (uint8_t)0;
            }
        }

        for (h = plate_h - 1; h >= plate_h / 2; h--)
        {
            // ���һ����ǰ��ռ�˿�ȵ�3/4����Ϊ�ҵ��˱߿�
            if (hist[h] > new_w * 3 / 4)
            {
                break;
            }
        }

        // ɾ���߿�
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

// ȥ��˫�㳵���±߿�ĸ���
static void remove_upper_and_lower_border_down(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t len;
    int32_t start, end;
    const int32_t thresh = plate_h * 3 / 2;

    // �����±߿����ֻ��7��
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
                    // ��ɾ��һ�У����������Ӱ��
                    memset(bina_img + plate_w * (h + 0) + start, 0, len);
                    memset(bina_img + plate_w * (h - 1) + start, 0, len);
                }
            }
        }
    }

    return;
}

// ��λ˫�����²������ַ������ұ߽�
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

    // ��ֵͼ����ˮƽ����ͶӰ
    for (j = 0; j < plate_w; j++)
    {
        hist[j] = 0;

        for (i = 0; i < plate_h; i++)
        {
            hist[j] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    // ͳ��ճ���ַ��ĸ���
    merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

    // �ָ�������ַ���Ŀ̫�ٻ���ճ���ַ�̫�࣬��Ϊ���±߿�����ճ��
    iteration = 1;
    p = plate_h - 1;

    while ((ch_num <= 3 || merge >= 3) && (iteration < 5) && (p > plate_h - 4))
    {
        k = ch_num;

        // һ��һ�е�ɾ���±߿�ֱ���ָ�õ����ַ�����ch_num + 1
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

        // ����ͳ��ճ���ַ��ĸ���
        merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

        iteration++;
    }

    //     // ����ͳ��ֱ��ͼ
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
    //     // ���ն�λ��ÿ���ַ��ı߽�
    //     character_left_and_right_locating_double(hist, plate_w, ch, &ch_num);

    *ch_num1 = ch_num;

    CHECKED_FREE(merge_mask, sizeof(uint8_t) * plate_w);
    CHECKED_FREE(hist, sizeof(int32_t) * plate_w);

    return;
}

// ͶӰ����λ�ַ����±߽�
static void character_upper_and_lower_locating_down(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
                                                    int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t k, p;
    int32_t sum;
    int32_t ch_num = *ch_num1;

    // ��λ�ַ������±߽�
    for (k = 0; k < ch_num; k++)
    {
        ch[k].y0 = (int16_t)0;
        ch[k].y1 = (int16_t)(plate_h - 1);

        // �ϱ߽�
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

        // �±߽�
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

    // ɾ�����߶�̫С��α�ַ�����
    for (k = 0; k < ch_num; k++)
    {
        // �ַ��߶�С�ڳ��Ƹ߶ȵ�1/4��ȷ��Ϊ����
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

// ɾ����С������
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

//     // ɾ����Ⱥ�С������
//     for (k = 0; k < ch_num; k++)
//     {
//         avg_w = get_character_average_width(ch, ch_num, plate_h);
//
//         // ����ַ����С��ƽ����ȵ�1/6��ȷ��Ϊ����
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

    // ɾ���߶Ƚ�С������
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

        // ����ַ�ͶӰ�����ֵС�ڸ߶ȵ�1/4�������ַ��߶�С��ƽ���߶ȵ�1/2��ȷ��Ϊ����
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

// ����ͶӰֱ��ͼѰ����ѷָ��
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

    // ���Ȳ���ͶӰ��Ѱ����ѷָ��
    for (w = x0; w <= x1; w++)
    {
        hist[w - x0] = 0;

        for (h = y0; h <= y1; h++)
        {
            hist[w - x0] += bina_img[plate_w * h + w] > (uint8_t)0;
        }
    }

    // ȷ��������Χ
    left  = MAX2(init_point - x0 - 5, 0);
    right = MIN2(init_point - x0 + 5, ch_w - 1);

    // Ѱ�Ҿֲ���Сֵ
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

    // ����������ߵ�ֵ���ȼ�Сֵ����Ѹü�Сֵ����Ϊ��ѷָ��
    if ((hist[left] - min > 3) && (hist[right] - min > 3))
    {
        *best_point = x0 + idx;
    }
    else // ���ͶӰ��û���ҵ���ѷָ����Ĭ�Ϸָ��Ϊ��������
    {
        *best_point = init_point;
    }

    CHECKED_FREE(hist, ch_w * sizeof(int32_t));

    return;
}

// ͨ��ͶӰֱ��ͼ������ճ���ַ�
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

        if (num_tmp > 1) // �ָ�ɹ�
        {
            // �ָ�����ӵ��ַ��������ڷ��ѳ������ַ�������ȥԭ���Ǹ��ַ�
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

            break; // �ָ�ɹ�������
        }

        // �������ַ����Ҵ��ڸ�����Ϣʱ��ͨ��ͶӰ��Ҳ���԰����ҵĸ�����Ϣȥ������
        // ������¶�λ���ַ��������Ҫ��ҲӦ�������ɹ�
        // �ⲽ������ʶ������0.3%������
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

// ͨ��Ѱ�Ҳ��ȵķ�ʽ����ճ���ַ�
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

        if (ch_w < avg_w * 27 / 10) // �����ַ�ճ��
        {
            init_point = ch[k].x0 + ch_w / 2;
        }
        else // ����ַ�ճ��
        {
            if (ch_w / 3 < avg_w * 3 / 2) // �����ַ�ճ��
            {
                init_point = ch[k].x0 + ch_w / 3;
            }
            else // ����ַ�ճ��
            {
                init_point = ch[k].x0 + avg_w;
            }
        }

        // Ѱ�Ҳ��ȵ�
        find_valley_point(bina_img, &ch[k], init_point, &best_point, plate_w/*, plate_h*/);

        // |0 |1 |2 |3 |k==4|5 |6 |7 |8 |9 | �ַ����
        // |c0|c1|c2|c3|ck01|c5|c6|c7|c8|  | ԭʼ�ַ�����
        //               |\
        //               | \
        // |c0|c1|c2|c3|ck0|ck1|c5|c6|c7|c8| ����ճ���ַ�����ַ�����

        // ch[k+0]����ԭ���ַ�ch[k]��ǰ�벿��
        // ch[k+1]����ԭ���ַ�ch[k]�ĺ�벿��

        // �������ַ�����ch
//      for (n = ch_num + 1; n > k + 1; n--)
        for (n = ch_num; n > k + 1; n--)
        {
            ch[n] = ch[n - 1];
        }

        ch_num++;

        // ch[k]����ԭ���ַ�ch[k]��ǰ�벿��
        ch[k].x1 = (int16_t)(best_point - 1);

        if (k > 0)  // �ų���һ���ַ�(�����Ǻ���)
        {
            // �ٴ�ȷ�Ϸָ�λ���Ƿ����
            if (ch[k].x1 - ch[k].x0 + 1 < 4)
            {
                // ��������ʣ���ѡ���ʼ�ָ��
                ch[k].x1 = (int16_t)(init_point - 1);
            }
        }

        // ch[k+1]����ԭ���ַ�ch[k]�ĺ�벿��
        ch[k + 1].x0 = (int16_t)(MIN2(ch[k + 0].x1 + 2, plate_w - 1));
        ch[k + 1].x0 = (int16_t)(MAX2(ch[k + 1].x0, ch[k + 0].x1 + 1));
        ch[k + 1].x1 = (int16_t)(MIN2(ch[k + 0].x0 + ch_w - 1, plate_w - 1));

        // ��֤��ǰ�ַ����ұ߽粻������һ���ַ�����߽�
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

// ճ���ַ�����
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
            // ǰ���ַ���ƽ�����
            flag = ch_w > (((ch[k - 1].x1 - ch[k - 1].x0 + 1) + (ch[k + 1].x1 - ch[k + 1].x0 + 1)) / 2 * 5 / 4);
        }

        /***********************************************************************************************
         *����˫�����²������ַ������٣�������ճ��ʱ���������ƽ���ַ���Ⱥ��ѷ�ӳ�������ַ���ȣ�
         *���ֱ�������ַ��ĸ߶����õ�ƽ����ȣ�˫�����²������ַ���߱�Ϊ13:22��ԼΪ0.6
         ***********************************************************************************************/
//        avg_w = get_character_average_width(ch, ch_num, plate_h);
        avg_w = ch_h * 3 / 5;

        if ((ch_w > avg_w * 4 / 3) && flag) // �ַ�������ճ��
        {
            // ͨ��ͶӰֱ��ͼ������ճ���ַ�
            spliting_flag = spliting_by_projection_histogram(bina_img, plate_w, /*plate_h, */ch, k, &ch_num, avg_w);

            // ���ͶӰֱ��ͼ�ķ���û�з���ɹ������Ѱ�Ҳ��ȵķ���������
            if (spliting_flag == 0)
            {
                spliting_flag = spliting_by_find_valley(bina_img, plate_w/*, plate_h*/, ch, k, &ch_num, avg_w);
            }
            else
            {
                k--; // ��ֹͨ��ͶӰֱ��ͼ�������ַ�����ճ���ַ�
            }
        }
    }

    *ch_num1 = ch_num;

    // ��鳵�Ƶ��ұ߽��Ƿ����߽��
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

// �����ַ��ϲ�
static void character_merging_down(/*uint8_t *restrict bina_img, */Rects *restrict ch, int32_t *ch_num1,
                                                                   /*int32_t plate_w, */int32_t plate_h)
{
    int32_t k, n;
    int32_t ch_w;
    int32_t merge_w;     // �ַ��ϲ���Ŀ��
    int32_t interval;    // �ַ����
    int32_t avg_w;
    int32_t ch_num = *ch_num1;

    if (ch_num < 5)
    {
        return;
    }

    //    avg_w = get_character_average_width(ch, ch_num, plate_h);  // ����ƽ�����

    // |0   |1 |2 |3 |4 |5 |6 |7 |8 |9 | �ַ����
    // |c0  |c1|c2|c3|c4|c5|c6|c7|c8|c9| ԭʼ�ַ�����
    // |c01 |c2|c3|c4|c5|c6|c7|c8|c9|  | ��1�κϲ���(k = 0)
    // |c012|c3|c4|c5|c6|c7|c8|c9|  |  | ��2�κϲ���(k = 0)
    //    |
    // ��3�κϲ�������Ҫ�ӵ�1���ַ�(c012)��ʼ����ע�⣡

    // ֻ��Ҫ��������һֱ���Һϲ��Ϳ�����
    for (k = 0; k < ch_num - 1; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;

        // ÿ�κϲ���Ҫ���¼���ƽ�����
        avg_w = get_character_average_width(ch, ch_num, plate_h);

        if (ch_w < (avg_w * 2 + 2) / 3) // �����Ƕ����ַ�
        {
            // ����ұ�
            merge_w  = ch[k + 1].x1 - ch[k].x0 + 1;
            interval = ch[k + 1].x0 - ch[k].x1 + 1;

            // ���Һϲ�
            if ((merge_w < avg_w) && (interval <= 4))
            {
                ch[k].x1 = (int16_t)(ch[k + 1].x1);
                ch[k].y0 = (int16_t)(MIN2(ch[k].y0, ch[k + 1].y0));
                ch[k].y1 = (int16_t)(MAX2(ch[k].y1, ch[k + 1].y1));

                for (n = k + 1; n < ch_num - 1; n++)
                {
                    ch[n] = ch[n + 1];
                }

                // �ϲ����ַ�������1��ͬʱk--���˵��ϲ�����ַ����¿�ʼ
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

    // �ַ���������ַ������һ���ַ������С���������ַ���Ƚ�С��˵���ַ��ָ��խ����΢��չһЩ��
    for (k = 1; k < ch_num - 1; k++)
    {
        // �����һ���ַ����С���ַ��߶ȵ�1/4��˵���ַ���Ϊ1��
        // �ַ����С��ƽ����ȵ�2/3��
        // ��ǰһ���ַ��ļ�������ַ��߶ȵ�1/4
        // ��������3��Ҫ����Ϊ�ַ����и���ˣ�������϶�����2������
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

// ȥ�����ұ߽�ķ��ַ�����
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



    ch = chs + *ch_num - 1; // �ұߵ�һ���ַ�

    // ��ߵ�һ���ַ�������'1', ��˲���������ж�
    // ����ұߵ�һ���ַ��ǲ����������ַ�����
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

// �����Ϊ������㣬DSP�ϻ�����������1013245 cycles ���͵�45441 cycles�����Ƚ���û�ж����յ�ʶ�����������Ӱ��
#define EPSITION (100)   // Ϊ�˽���������Ϊ���㣬���м��һЩ����ֵ����100.
static uint8_t character_position_standardization(Rects *restrict ch, int32_t ch_num, int32_t plate_w)
{
    int32_t i, j, k;
    int32_t avg_w;
    int32_t x0, x1;
    int32_t std_len;
    int32_t std_ch_w;
    float std_ch_center_x_7[7] = {22.5f, 79.5f, 158.5f, 215.5f, 272.5f, 329.5f, 386.5f};// ��������7���ַ�������λ��X����
    float std_ch_center_x_5[5] = {32.5f, 112.5f, 192.5f, 272.5f, 352.5f};               // ˫������5���ַ�������λ��X����
    int32_t std_ch_center_x[7];  // ��ű�׼�����ַ����ĵ��X����
    int32_t now_ch_center_x[7];  // ��ŵ�ǰ�����ַ����ĵ��X����
    uint8_t similarity = (uint8_t)0;      // ���ƶ�
    int32_t c;           // ��������
    int32_t c_min;
    int32_t error;       // λ�����
    int32_t error_min;   // ��Сλ�����
    int32_t r;           // ƥ�������뾶
    int32_t r_min;
    Rects std_ch[7];
    float *std_ch_center_x_ptr = NULL;

//  const int32_t std_ch_center_x1_x = 45/2;                                     // ��һ���ַ�������λ��x����
//  const int32_t std_ch_center_x2_x = 45+12+45/2;                               // �ڶ����ַ�������λ��x����
//  const int32_t std_ch_center_x3_x = 45+12+45+34+45/2;                         // �������ַ�������λ��x����
//  const int32_t std_ch_center_x4_x = 45+12+45+34+45+12+45/2;                   // ���ĸ��ַ�������λ��x����
//  const int32_t std_ch_center_x5_x = 45+12+45+34+45+12+45+12+45/2;             // ������ַ�������λ��x����
//  const int32_t std_ch_center_x6_x = 45+12+45+34+45+12+45+12+45+12+45/2;       // �������ַ�������λ��x����
//  const int32_t std_ch_center_x7_x = 45+12+45+34+45+12+45+12+45+12+45+12+45/2; // ���߸��ַ�������λ��x����

    if ((ch_num == 7) || (ch_num == 5))
    {
        if (ch_num == 7)
        {
            std_len = 409;  // �����ƣ�7�ַ�����׼���Ƴ���409mm
            std_ch_w = 45;  // �����ƣ�7�ַ�����׼�����ַ����45mm
            std_ch_center_x_ptr = std_ch_center_x_7;

            // ����õ���������c
            c = (ch[6].x1 - ch[0].x0 + 1) * 100 / std_len;

            for (k = 0; k < 7; k++)
            {
                now_ch_center_x[k] = (ch[k].x1 + ch[k].x0 + 1) * EPSITION / 2 - (ch[0].x0 + 1) * EPSITION;
            }
        }
        else
        {
            std_len = 385;  // ˫���ƣ�5�ַ�����׼���Ƴ���385mm
            std_ch_w = 60;  // ˫���ƣ�5�ַ�����׼�����ַ����60mm
            std_ch_center_x_ptr = std_ch_center_x_5;

            // ����õ���������c
            c = (ch[4].x1 - ch[0].x0 + 1) * EPSITION / std_len;

            for (k = 0; k < 5; k++)
            {
                now_ch_center_x[k] = (ch[k].x1 + ch[k].x0 + 1) * EPSITION / 2 - (ch[0].x0 + 1) * EPSITION;
            }
        }

        // ��ָ�õ����ַ�λ�ý��бȽϣ��������
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

        // ���ݱ�׼���Ƴߴ����õ���ǰ�������ַ���λ��
        for (k = 0; k < ch_num; k++)
        {
            std_ch[k].x0 = (int16_t)MAX2(ch[0].x0 + ((std_ch_center_x_ptr[k] * c_min / EPSITION) - r_min) - avg_w / 2, 0);
            std_ch[k].x1 = (int16_t)MIN2(ch[0].x0 + ((std_ch_center_x_ptr[k] * c_min / EPSITION) - r_min) + avg_w / 2, plate_w - 1);
        }

        // ��׼��
        for (k = 0; k < ch_num; k++)
        {
            x0 = ch[k].x0;
            x1 = ch[k].x1;

            // �Կ�Ƚ�С���ַ�'1'��������
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

        // ����һ�������ƿ��
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

// ��դ������ʶ��ģ����м�飬���1�϶࣬���ַ������ֺ�С�ģ���Ϊ����ʶ��
static void more_chs_1_delete(uint8_t *chs, Rects *ch, int32_t ch_num, uint8_t *not_ch_cnt)
{
    int32_t chs_1 = 0;
    int32_t k;

    for (k = ch_num - 5; k < ch_num - 1; k++)
    {
        if (chs[k] == (uint8_t)'1' || chs[k + 1] == (uint8_t)'1') // ���ַ�1�����ַ����С
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
            chs_1 += (chs[k] == (uint8_t)'1') ? 1 : 0;   // ͳ���ַ�1�ĸ���
        }

        if (chs_1 >= 4) // ��С��4������Ϊ��ʶ�𵽷ǳ���
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

    // ���²��ַ����������λ�ñ���ȽϽӽ�
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

            // ͬʱ�������²���������λ�þ�����ַ���׼��
            if ((tmp_stand > max_stand) && (tmp_stand > 90))
            {
                max_stand = tmp_stand;
                best_place = k;
            }
        }
    }

    // û���ʺϵ����
    if (best_place == 255)
    {
        *ch_num1 = 0;

        return;
    }

    for (k = best_place; k < best_place + 5; k++)
    {
        ch[k - best_place] = ch[k];
    }

    if (bkgd_color == (0u)) // ���Ƴ���������3���ַ�
    {
        for (k = 0; k < 5; k++)
        {
            int32_t ch_w = ch[k].x1 - ch[k].x0 + 1;
            int32_t ch_h = ch[k].y1 - ch[k].y0 + 1;

            // ���̫խ��ֱ��ȷ��Ϊ����1
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
    else                // ���Ƴ������Ƴ���������2���ַ�
    {
        for (k = 0; k < 5; k++)
        {
            int32_t ch_w = ch[k].x1 - ch[k].x0 + 1;
            int32_t ch_h = ch[k].y1 - ch[k].y0 + 1;

            // ���̫խ��ֱ��ȷ��Ϊ����1
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

// ˫�����²������ַ��ָ�ʶ��
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
    int32_t x0 = 0;             //˫�����²����򲻽������ұ߽綨λ
    int32_t x1 = plate_w - 1;
    int32_t w, h;
    int32_t offset = 0;
    IplImage *plate = cvCreateImage(cvSize(plate_w, plate_h * 8), IPL_DEPTH_8U, 3);
    IplImage *resize = cvCreateImage(cvSize(plate_w * 2, plate_h * 8 * 2), IPL_DEPTH_8U, 3);

    memset(plate->imageData, 0, plate->widthStep * plate->height);

    // ��ʾԭʼ�Ҷ�ͼ��
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

// ��ʾ�Ҷ�ͼ����ǿЧ��
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h;
    _IMAGE3_(plate, plate_img, plate_w, plate_h, offset)
#endif

    thresholding_percent(plate_img, bina_img, plate_w, plate_h, plate_w / 4, plate_w - plate_w / 4,
                         plate_h / 5, plate_h - plate_h / 5, 30);

// ��ʾ��ֵ��Ч��
#if DEBUG_SEG_DOUBLE_SHOW
    offset  = plate_h * 2;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)
#endif

    // ����ȥ��
    remove_small_region_by_labelling_down(bina_img, plate_w, plate_h);

// ��ʾ����ȥ��Ч��
#if DEBUG_SHOW12
    offset  = plate_h * 3;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset)
#endif

    // ��ȷ��λ�������±߽�
    plate_upper_and_lower_locating_exact_down(bina_img, &y0, &y1, plate_w, plate_h);

// ��ʾ�����������ұ߽綨λЧ��
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

    // ��ȷ��λ�������ұ߽�
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

    // ȥ���±߿�ĸ���
    remove_upper_and_lower_border_down(bina_img, plate_w, ch_h);

    // ȥ��������ǰ���У�����ǰ��ֻ��������1/3��Ϊ����������ͼ����������ʶ������0.2%����
    remove_isolated_lines(bina_img, 0, ch_h - 1, plate_w, ch_h);

    // ����ͶӰ����λ�ַ������ұ߽�
    character_left_and_right_locating_iteration_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // ����ͶӰ����λ�²������ַ������±߽�
    character_upper_and_lower_locating_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// ��ʾ�ַ��������ұ߽綨λЧ��
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 4 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    // ɾ���²�������̫С���ַ�����
    remove_too_small_region_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // �²�����ճ���ַ�����
    character_spliting_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // ճ���ַ��������Ҫ�ٴ����¶�λ�ַ����±߽�
    character_upper_and_lower_locating_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// ��ʾճ���ַ�����Ч��
#if DEBUG_SEG_DOUBLE_SHOW
    offset = plate_h * 5 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset)

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif
    // �����ַ��ϲ�
    character_merging_down(/*bina_img, */ch_tmp, &ch_num, /*plate_w, */ch_h);

    // �ַ�����
    character_correct_down(bina_img, ch_tmp, &ch_num, plate_w, plate_h);

    // �ٴ�ȥ��̫С������
    remove_too_small_region_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// ��ʾ�����ַ��ϲ�Ч��
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

    // ȥ�����ұ߽�ķ��ַ�����
    remove_non_character_region_down(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// ��ʾ�����ַ��ָ�Ч��
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

// ��ʾʶ�����ַ���λЧ��(���ַ�ʶ���л��ж��ַ��߽�ĵ���)
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

    // ͼƬ���濪��
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
