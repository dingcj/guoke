#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "plate_segmentation.h"
#include "../common/threshold.h"
#include "../common/label.h"
#include "../common/image.h"
#include "lsvm.h"
#include "feature.h"
#include "../time_test.h"
#include "../debug_show_image.h"

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#ifdef WIN32
#define DEBUG_SEGMENT_SHOW  0
#define DEBUG_SEGMENT_PRINT 0
#define DEBUG_SEGMENT_HIST  0
#define DEBUG_SEGMENT_LR    0
#endif

#define INVALID_IDX  (255)  // ��Ч������ַ����
#define FEATURE_NUMS (896)  // �����ַ�����������

#ifdef _TMS320C6X_
#pragma CODE_SECTION(character_segmentation, ".text:character_segmentation")
#pragma CODE_SECTION(remove_small_region_by_labelling, ".text:remove_small_region_by_labelling")
#pragma CODE_SECTION(plate_upper_and_lower_locating_exact, ".text:plate_upper_and_lower_locating_exact")
#pragma CODE_SECTION(plate_left_and_right_locating_exact, ".text:plate_left_and_right_locating_exact")
#pragma CODE_SECTION(remove_upper_and_lower_border, ".text:remove_upper_and_lower_border")
#pragma CODE_SECTION(remove_isolated_lines, ".text:remove_isolated_lines")
#pragma CODE_SECTION(character_left_and_right_locating_iteration, ".text:character_left_and_right_locating_iteration")
#pragma CODE_SECTION(character_left_and_right_locating, ".text:character_left_and_right_locating")
#pragma CODE_SECTION(character_merge_numbers, ".text:character_merge_numbers")
#pragma CODE_SECTION(get_character_average_width, ".text:get_character_average_width")
#pragma CODE_SECTION(character_upper_and_lower_locating, ".text:character_upper_and_lower_locating")
#pragma CODE_SECTION(remove_too_small_region, ".text:remove_too_small_region")
#pragma CODE_SECTION(get_character_average_height, ".text:get_character_average_height")
#pragma CODE_SECTION(character_spliting, ".text:character_spliting")
#pragma CODE_SECTION(spliting_by_projection_histogram, ".text:spliting_by_projection_histogram")
#pragma CODE_SECTION(spliting_by_find_valley, ".text:spliting_by_find_valley")
#pragma CODE_SECTION(find_valley_point, ".text:find_valley_point")
#pragma CODE_SECTION(character_merging, ".text:character_merging")
#pragma CODE_SECTION(character_correct, ".text:character_correct")
#pragma CODE_SECTION(remove_non_character_region, ".text:remove_non_character_region")
#pragma CODE_SECTION(plate_candidate_filtering, ".text:plate_candidate_filtering")
#pragma CODE_SECTION(interval_for_single_layer_plate, ".text:interval_for_single_layer_plate")
#pragma CODE_SECTION(best_combination_for_single_layer_plate, ".text:best_combination_for_single_layer_plate")
#pragma CODE_SECTION(character_position_adjustment, ".text:character_position_adjustment")
#pragma CODE_SECTION(right_border_location_for_last_character, ".text:right_border_location_for_last_character")
#pragma CODE_SECTION(character_position_fitting, ".text:character_position_fitting")
#pragma CODE_SECTION(chinese_locating_by_least_square, ".text:chinese_locating_by_least_square")
#pragma CODE_SECTION(character_position_standardization, ".text:character_position_standardization")
#pragma CODE_SECTION(character_border_extend, ".text:character_border_extend")
#pragma CODE_SECTION(single_character_recognition, ".text:single_character_recognition")
#pragma CODE_SECTION(chinese_frame_adjust, ".text:chinese_frame_adjust")
#pragma CODE_SECTION(character_standardization_by_regional_growth, ".text:character_standardization_by_regional_growth")
#pragma CODE_SECTION(chinese_left_right_locating_by_projection, ".text:chinese_left_right_locating_by_projection")
#endif

//���б任�����ڴ�ֱ��бУ��
void shear_transform(uint8_t *restrict plate_img, float tan, int32_t img_w, int32_t img_h)
{
    int32_t h, w;
    uint8_t *restrict temp = NULL;

    CHECKED_MALLOC(temp, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);
    memcpy(temp, plate_img, img_w * img_h);

    for (h = 0; h < img_h; h++) // ��ֹ���
    {
        for (w = 0; w < img_w; w++)
        {
            if ((w + h * tan) >= 0 && (w + h * tan) < img_w)
            {
                plate_img[img_w * h + w] = temp[(int32_t)(img_w * h + w + h * tan)];
            }
            else
            {
                plate_img[img_w * h + w] = 0u;
            }
        }
    }

    CHECKED_FREE(temp, sizeof(uint8_t) * img_w * img_h);

    return;
}

// ���㴹ֱ��б�Ƕȵ�����ֵ
float get_tan(uint8_t* bina, int32_t width, int32_t height)
{
    int32_t h, w;
    int32_t x_mean, y_mean;
    int32_t sum_x, sum_y;
    int32_t cnt;
    float tan;

    sum_x = 0;
    sum_y = 0;
    cnt = 0;

    for (h = 0; h < height; h++)
    {
        for (w = 0; w < width; w++)
        {
            if (bina[h * width + w] == 255u)
            {
                cnt++;
                sum_x += w;
                sum_y += h;
            }
        }
    }

    if (!cnt)
    {
        return 0;
    }

    x_mean = sum_x / cnt;
    y_mean = sum_y / cnt;

    sum_x = 0;
    sum_y = 0;

    for (h = 0; h < height; h++)
    {
        for (w = 0; w < width; w++)
        {
            if (bina[h * width + w] == 255u)
            {
                sum_x += (w - x_mean) * (h - y_mean);
                sum_y += (h - y_mean) * (h - y_mean);
            }
        }
    }

    if (!sum_y)
    {
        return 0;
    }


    tan = (float)sum_x / sum_y;

    return tan;
}

int32_t deskewing_vertical_by_single_ch(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
    int32_t plate_w, int32_t plate_h, Rects *restrict ch, int32_t ch_num, int32_t ch_h)
{
    int32_t h, w, k;
    int32_t high, with;
    memset(bina_img, 0, sizeof(uint8_t) * plate_w * plate_h);

    for (k = 0; k < ch_num; k++)
    {
        uint8_t *restrict plate_gray_char = NULL;
        uint8_t *restrict plate_bina_char = NULL;
        high = ch[k].y1 - ch[k].y0 + 1;
        with = ch[k].x1 - ch[k].x0 + 1;
        CHECKED_MALLOC(plate_gray_char, sizeof(uint8_t) * with * high, CACHE_SIZE);
        CHECKED_MALLOC(plate_bina_char, sizeof(uint8_t) * with * high, CACHE_SIZE);
        memset(plate_gray_char, 0, sizeof(uint8_t) * with * high);
        memset(plate_bina_char, 0, sizeof(uint8_t) * with * high);

        for (h = 0; h < high; h++)
        {
            for (w = 0; w < with; w++)
            {
                plate_gray_char[h * with + w] = gray_img[(h + ch[k].y0) * plate_w + (w + ch[k].x0)];
            }
        }

        thresholding_avg_all(plate_gray_char, plate_bina_char, with, high);

        for (w = 0; w < with * high; w++)
        {
            plate_bina_char[w] = plate_bina_char[w] == 0 ? 255 : 0;
        }

        // ��ʾ�����ַ��Ĵ�ֱ��бУ��
        if (1)
        {
            float tan;
            tan = get_tan(plate_bina_char, with, high);
            shear_transform(plate_bina_char, tan, with, high);
        }

        for (h = 0; h < high; h++)
        {
            for (w = 0; w < with; w++)
            {
                bina_img[(h + ch[k].y0) * plate_w + (w + ch[k].x0)] = plate_bina_char[h * with + w];

            }
        }

        CHECKED_FREE(plate_gray_char, sizeof(uint8_t) * with * high);
        CHECKED_FREE(plate_bina_char, sizeof(uint8_t) * with * high);
    }
}

// ���ƵĴ�ֱ��бУ��
int32_t deskewing_vertical(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                           int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t ch_num, int32_t ch_h)
{
    int32_t k;
    int32_t h, w;
    int32_t high, with;
    int32_t cnt;
    float tan[24];
    float tan_avg;

    cnt = ch_num;
    tan_avg = 0;

    for (k = 0; k < ch_num; k++)
    {
        uint8_t *restrict plate_bina_char = NULL;

        high = ch_pos[k].y1 - ch_pos[k].y0 + 1;
        with = ch_pos[k].x1 - ch_pos[k].x0 + 1;

        CHECKED_MALLOC(plate_bina_char, sizeof(uint8_t) * with * high, CACHE_SIZE);

        memset(plate_bina_char, 0, sizeof(uint8_t) * with * high);

        for (h = 0; h < high; h++)
        {
            for (w = 0; w < with; w++)
            {
                plate_bina_char[h * with + w] = bina_img[(h + ch_pos[k].y0) * plate_w + (w + ch_pos[k].x0)];
            }
        }

        // ͳ����б�Ƕ�
        {
            if (with >= 2 * high)
            {
                tan[k] = 0;
                cnt--;
            }
            else
            {
                tan[k] = get_tan(plate_bina_char, with, high);
            }

            tan_avg += tan[k];
        }

        CHECKED_FREE(plate_bina_char, sizeof(uint8_t) * with * high);
    }

    if (cnt <= 0)
    {
        tan_avg = 0;
    }
    else
    {
        tan_avg /= cnt;
    }

//     for (float tan_angle = -0.5; tan_angle < 0.5; tan_angle += 0.1)
//     {
//         uint8_t* pbBina = malloc(plate_w * plate_h);
//         IplImage* binaImg = cvCreateImage(cvSize(plate_w, plate_h), 8, 1);
// 
//         memcpy(pbBina, bina_img, plate_w * plate_h);
//         shear_transform(pbBina, tan_angle, plate_w, ch_h);
// 
//         for (int h = 0; h < plate_h; h++)
//         {
//             memcpy(binaImg->imageData + h * binaImg->widthStep, pbBina + h * plate_w, plate_w);
//         }
// 
//         cvShowImage("ver", binaImg);
//         cvWaitKey(0);
//     }

    // ��ֱ��бУ��
    if (tan_avg > 0.02)
    {
//       printf("\ntan_avg = %5f\n", tan_all);
        shear_transform(bina_img, tan_avg, plate_w, ch_h);
        shear_transform(gray_img, tan_avg, plate_w, ch_h);

        return 1;
    }
    else
    {
        return 0;
    }
}

void sub_filter(uint8_t *gray_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    int32_t w, h;
    int32_t hist[256];
    int32_t hist_bar[256];
    int32_t area = img_w * img_h;
    int32_t max_gray = 0;
    int32_t i;
    uint32_t i_max = 0u;

    memset(hist, 0, sizeof(int32_t) * 256);
    memset(hist_bar, 0, sizeof(int32_t) * 256);

    // ����ֱ��ͼ
    for (h = 0; h < img_h; h++)
    {
        for (w = img_w / 6; w < img_w - img_w / 6; w++)
        {
            hist[gray_img[h * img_w + w]]++;
        }
    }

    for (i = 10; i <= 200; i++)
    {
        hist_bar[i] += hist[i - 3] + hist[i - 2] + hist[i - 1] + hist[i] + hist[i + 1] + hist[i + 2] + hist[i + 3];
    }

    for (i = 0; i < 256; i++)
    {
        if (hist_bar[i] >= max_gray)
        {
            max_gray = hist_bar[i];
            i_max = (uint32_t)i;
        }
    }

#ifdef _TMS320C6X_

    for (pix = 0; pix < area; pix++)
    {
        if (gray_img[pix] <= i_max)
        {
            gray_img[pix] = 0u;
        }
        else
        {
            gray_img[pix] -= i_max;
        }
    }

#else

    for (pix = 0; pix < area; pix++)
    {
        if (gray_img[pix] <= i_max)
        {
            gray_img[pix] = 0u;
        }
        else
        {
            gray_img[pix] -= i_max;
        }
    }

#endif

    return;
}

void thresholding_max_all(uint8_t *restrict gray_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
//     const int32_t area = img_w * img_h;
    uint8_t tmp;
    uint8_t gray_tmp[STD_W * STD_H];

    memcpy(gray_tmp, gray_img, (uint32_t)STD_AREA);

    for (h = 1; h < img_h - 1; h++)
    {
        for (w = 1; w < img_w - 1; w++)
        {
            tmp = MAX3(gray_tmp[img_w * (h - 1) + w], gray_tmp[img_w * (h + 0) + w], gray_tmp[img_w * (h + 1) + w]);
//            tmp = MAX3(gray_tmp[img_w * h + w - 1], gray_tmp[img_w * h + w + 1], tmp);

            gray_img[img_w * h + w] = tmp;
        }
    }

    return;
}


int32_t thresholding_avg_new(uint8_t *gray_img,  uint8_t *bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    int32_t area = img_w * img_h;
    uint32_t sum = 0u;
    uint32_t sum1 = 0u;
    uint32_t sum2 = 0u;
    uint32_t avg1;
    uint32_t avg2;
    uint32_t avg;
    int32_t cnt = 0;
    float flag = 0;
    uint8_t *temp_img = NULL;

    CHECKED_MALLOC(temp_img, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);
    memcpy(temp_img, gray_img, img_w * img_h);

    for (pix = 0; pix < area; pix++)
    {
        sum1 += temp_img[pix];
    }

    avg1 = sum1 / (uint32_t)(area + 1);
    //    printf("  avg1=%d\n", avg1);
    //    printf("  sum1=%d\n", sum1);

    if (1)
    {
        uint32_t temp = 0u;
        int32_t h, w;

        for (h = 1; h < img_h - 1; h++)
        {
            for (w = 1; w < img_w - 1; w++)
            {
                temp = MAX3(gray_img[img_w * h + w], gray_img[img_w * (h + 1) + w], gray_img[img_w * (h - 1) + w]);
                temp = MAX3(temp, gray_img[img_w * h + w + 1], gray_img[img_w * h + w - 1]);
//                temp = MAX3(temp, gray_img[img_w * (h - 1) + w + 1], gray_img[img_w * (h - 1) + w - 1]);
//                temp = MAX3(temp, gray_img[img_w * (h + 1) + w + 1], gray_img[img_w * (h + 1) + w - 1]);


                temp_img[img_w * h + w] = temp;
            }
        }
    }

    for (pix = 0; pix < area; pix++)
    {
        sum2 += temp_img[pix];
    }

    avg2 = sum2 / (uint32_t)(area + 1);

    for (pix = 0; pix < area; pix++)
    {
        if ((temp_img[pix] - gray_img[pix]) >= 145u)
        {
            sum ++;
        }
    }

    //    printf("  avg2=%d \n", avg2);
    //    printf("  sum2=%d\n", sum2);

    //     printf("  avg2 - avg1=%d \n", avg2 - avg1);
    //     printf("  (sum2 - sum1)=%d \n", (sum2 - sum1));
    //     printf("  change/area=%f \n", (float)sum/area);
    if (!area)
    {
        return 0;
    }

    flag = (float)sum / area;

    if (flag >= 0.015)
    {
        avg = avg2;

        for (pix = 0; pix < area; pix++)
        {
            bina_img[pix] = ((int32_t)(temp_img[pix] + gray_img[pix]) > 2 * (int32_t)avg) ? 0u : 255u;
//             bina_img[pix] = (gray_img[pix] > avg) ? 0u : 255u;
            cnt += (gray_img[pix] > avg);
        }
    }
    else
    {
        avg = avg1;

        for (pix = 0; pix < area; pix++)
        {
            bina_img[pix] = (gray_img[pix] > avg) ? 0u : 255u;
            cnt += (gray_img[pix] > avg);
        }
    }


    CHECKED_FREE(temp_img, sizeof(uint8_t) * img_w * img_h);

    if (cnt == area)
    {
        return 0;
    }

    return avg;
}

int32_t max_filter_all(uint8_t *gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h)
{
    int32_t max0;
    int32_t max1;
    int32_t max2;
    int32_t w, h;

    memcpy(bina_img, gray_img, img_w * img_h);

    for (h = 1; h < img_h - 1; h++)
    {
        for (w = 1; w < img_w - 1; w++)
        {
            max0 = MAX3(bina_img[img_w * (h - 1) + w - 1], bina_img[img_w * (h - 1) + w + 0], bina_img[img_w * (h - 1) + w + 1]);
            max1 = MAX3(bina_img[img_w * (h - 0) + w - 1], bina_img[img_w * (h - 0) + w + 0], bina_img[img_w * (h - 0) + w + 1]);
            max2 = MAX3(bina_img[img_w * (h + 1) + w - 1], bina_img[img_w * (h + 1) + w + 0], bina_img[img_w * (h + 1) + w + 1]);

            gray_img[img_w * (h - 0) + w + 0] = MAX3(max0, max1, max2);
        }
    }

    return 1;
}

int32_t thresholding_avg_all_best(uint8_t *gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    int32_t area = img_w * img_h;
    uint32_t sum = 0u;
    uint32_t avg;
    int32_t cnt = 0;
    uint32_t mean = 0u;
    int32_t variance = 0;
    int32_t variance_min = (1 << 20);
    int32_t best_idx = 0;
    uint32_t best_thr = 0u;
    int32_t k = 0;

    for (pix = 0; pix < area; pix++)
    {
        sum += gray_img[pix];
    }

    if (area == 0)
    {
        return 0;
    }

    avg = sum / (uint32_t)area;
    best_thr = avg;

    for (k = 0; k < 30; k++)
    {
        avg = MIN2(best_thr / 2 + k, 254);

        cnt = 0;
        mean = 0u;
        variance = 0;

        for (pix = 0; pix < area; pix++)
        {
            cnt += (gray_img[pix] > avg);
            mean += (gray_img[pix] > avg) ? gray_img[pix] : 0u;
        }

        if (cnt)
        {
            mean /= (uint32_t)cnt;

            for (pix = 0; pix < area; pix++)
            {
                if (gray_img[pix] > avg)
                {
                    variance += (int32_t)((gray_img[pix] - mean) * (gray_img[pix] - mean));
                }
            }

            variance /= cnt;

            if (variance <= variance_min)
            {
                variance_min = variance;
                best_idx = k;
            }
        }
        else
        {
            break;
        }

        printf("k = %2d --- mean = %3d --- variance = %6d \n", k, mean, variance);
    }

    best_thr += (uint32_t)best_idx;

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] > best_thr) ? 0u : 255u;
    }

    if (cnt == area)
    {
        return 0;
    }

    return (int32_t)avg + 1;
}

// ͨ����ͨ���ǵķ���ȥ��Բ����ݶ�
static void remove_small_region_by_labelling(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
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

// ��ÿ�����ص��жϽ�������ɾ��
#if 1

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
                    int32_t flag0, flag1, flag2;
                    Rects rect_label;

                    rect_label = obj_pos[label_img[img_w * h + w] - (uint16_t)1];

                    // ȥ���ϱߵ��ݶ�
                    flag0 = (rect_label.y0 <= 2) && (rect_label.y1 <= 5);

                    // ȥ���±ߵ��ݶ�
                    flag1 = (rect_label.y0 >= img_h - 8) && (rect_label.y1 >= img_h - 5);

                    // ȥ���߶ȹ�խ�����򣬸߶ȹ�խ��С����Ϊ���ѵı߿�
                    flag2 = (rect_label.y1 - rect_label.y0 + 1 < 4);

                    if (flag0 || flag1 || flag2)
                    {
                        bina_img[img_w * h + w] = (uint8_t)0;
                    }
                }
            }
        }
    }

// ��ÿ����������жϣ�Ȼ�������ÿ�н���ɾ��(���ַ������ܻ���ɾ�����������������)
#else

    for (k = 0; k < ch_num; k++)
    {
        if (obj_area[k] < 64)
        {
            obj_w = obj_pos[k].x1 - obj_pos[k].x0 + 1;
            obj_h = obj_pos[k].y1 - obj_pos[k].y0 + 1;

            ptr = bina_img + img_w * obj_pos[k].y0 + obj_pos[k].x0;

            // ɾ�������С������
            if (obj_area[k] < 36)
            {
                for (h = 0; h < obj_h; h++)
                {
                    memset(ptr, 0, obj_w);
                    ptr += img_w;
                }
            }
            else
            {
                // �ϱߵ��ݶ�
                int32_t flag0 = (obj_pos[k].y0 <= 2) && (obj_pos[k].y1 <= 5);
                // �±ߵ��ݶ������ѵı߿�
                int32_t flag1 = (obj_pos[k].y0 >= img_h - 8) && (obj_pos[k].y1 >= img_h - 5);
                // ɾ���ڶ����ַ��͵������ַ�֮���Բ��
                int32_t flag2 = ((obj_pos[k].y0 > img_h / 4) && (obj_pos[k].y1 < img_h * 3 / 4)
                                 && (obj_pos[k].x0 > img_h / 2) && (obj_pos[k].x1 < img_w / 2));

                if (flag0 || flag1/* || flag2*/)
                {
                    for (h = 0; h < obj_h; h++)
                    {
                        memset(ptr, 0, obj_w);
                        ptr += img_w;
                    }
                }
            }
        }
    }

#endif

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

// ����ƽ���ַ����
#if 0
static int32_t get_character_average_interval(Rects *ch, int32_t ch_num)
{
    int32_t i, k;
    int32_t num;
    int32_t avg_i;
    int32_t w0, w1;

    num = 0;
    avg_i = 0;

    for (k = ch_num - 1; k >= 2; k--)
    {
        i = ch[k + 0].x0 - ch[k - 1].x1 + 1;

        if (i > 0)
        {
            w0 = ch[k + 0].x1 - ch[k + 0].x0 + 1;
            w1 = ch[k - 1].x1 - ch[k - 1].x0 + 1;

            if ((w0 > 0) && (w1 > 0))
            {
                if (i < MIN2(w0, w1) / 2)
                {
                    num++;
                    avg_i += i;
                }
            }
        }
    }

    if (num > 0)
    {
        avg_i /= num;
    }
    else
    {
        avg_i = 0;
    }

    return avg_i;
}
#endif

// �������±߽�ľ�ȷ��λ
static int32_t plate_upper_and_lower_locating_exact(uint8_t *restrict bina_img, int32_t *y0, int32_t *y1,
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

    // ͨ����������ǰ�����ۺ��ж�
    if (1)
    {
        int32_t *chg = NULL;
        int32_t *hist = NULL;
        int32_t y_tmp;

        CHECKED_MALLOC(chg, plate_h * sizeof(int32_t), CACHE_SIZE);
        CHECKED_MALLOC(hist, plate_h * sizeof(int32_t), CACHE_SIZE);

        memset(chg, 0, plate_h * sizeof(int32_t));
        memset(hist, 0, plate_h * sizeof(int32_t));

        for (h = 0; h < plate_h; h++)
        {
            for (w = plate_w / 5; w < plate_w - plate_w / 5; w++)
            {
                chg[h] += bina_img[plate_w * h + w] != bina_img[plate_w * h + w + 1];
                hist[h] += bina_img[plate_w * h + w] > 0u;
            }
        }

        y_tmp = *y0;

        for (h = *y0 - 1; h > MAX2(*y0 - 4, 0); h--)
        {
            if (chg[h] < change_thresh && hist[h] < plate_w * 3 / 5 / 2)
            {
                y_tmp = h;
            }
            else if (hist[h] > plate_w * 3 / 5 / 2)
            {
                break;
            }
        }

        *y0 = y_tmp;

        y_tmp = *y1;

        for (h = *y1 + 1; h < MIN2(*y1 + 4, plate_h); h++)
        {
            if (chg[h] < change_thresh && hist[h] < plate_w * 3 / 5 / 2)
            {
                y_tmp = h;
            }
            else if (hist[h] > plate_w * 3 / 5 / 2)
            {
                break;
            }
        }

        *y1 = y_tmp;

        CHECKED_FREE(hist, plate_h * sizeof(int32_t));
        CHECKED_FREE(chg, plate_h * sizeof(int32_t));
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

// ��ȷ��λ�������ұ߽�
static void plate_left_and_right_locating_exact(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h,
                                                int32_t *x0, int32_t *x1, int32_t ch_h, uint8_t bkgd_color)
{
    int32_t h, w;
    int32_t k;
    int32_t m, n;
    int32_t m0, m1;
    int32_t left, right;
    int32_t peak, valley;
    int32_t thresh;
    int32_t *restrict proj_hist_orig = NULL;
    int32_t *restrict proj_hist_sort = NULL;
#if DEBUG_SEGMENT_HIST
    IplImage *hist = cvCreateImage(cvSize(plate_w, plate_h + 50), IPL_DEPTH_8U, 3);
    cvZero(hist);
#endif

    *x0 = 0;
    *x1 = plate_w - 1;

#ifdef _TMS320C6X_
    proj_hist_orig = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * plate_w, 128);
    proj_hist_sort = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * plate_w, 128);
#else
    CHECKED_MALLOC(proj_hist_orig, sizeof(int32_t) * plate_w, CACHE_SIZE);
    CHECKED_MALLOC(proj_hist_sort, sizeof(int32_t) * plate_w, CACHE_SIZE);
#endif

    // �Ҷ�ֵͶӰ
    for (w = 0; w < plate_w; w++)
    {
        proj_hist_orig[w] = 0;

        for (h = 4; h < plate_h - 4; h++)
        {
            proj_hist_orig[w] += bina_img[plate_w * h + w] > (uint8_t)0;
        }
    }

#if DEBUG_SEGMENT_HIST

    for (w = 0; w < hist->width; w++)
    {
        for (h = 0; h < hist->height; h++)
        {
            hist->imageData[hist->widthStep * h + w * 3 + 0] = (uint8_t)0;
            hist->imageData[hist->widthStep * h + w * 3 + 1] = (uint8_t)0;
            hist->imageData[hist->widthStep * h + w * 3 + 2] = (uint8_t)0;
        }
    }

    for (w = 0; w < plate_w; w++)
    {
        for (h = 0; h < proj_hist_orig[w] * 50 / (plate_h + 1); h++)
        {
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 0] = (uint8_t)0;
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 1] = (uint8_t)255;
            hist->imageData[hist->widthStep * (50 - h - 1) + w * 3 + 2] = (uint8_t)0;
        }
    }

    for (w = 0; w < plate_w; w++)
    {
        for (h = 0; h < plate_h; h++)
        {
            hist->imageData[hist->widthStep * (50 + h) + w * 3 + 0] = (uint8_t)0;
            hist->imageData[hist->widthStep * (50 + h) + w * 3 + 1] = (uint8_t)bina_img[plate_w * h + w];
            hist->imageData[hist->widthStep * (50 + h) + w * 3 + 2] = (uint8_t)0;
        }
    }

#endif
    memcpy(proj_hist_sort, proj_hist_orig, sizeof(int32_t) * plate_w);

    // �Ӵ�С����
    for (m = 0; m < plate_w - 1; m++)
    {
        for (n = m + 1; n < plate_w; n++)
        {
            if (proj_hist_sort[m] < proj_hist_sort[n])
            {
                k = proj_hist_sort[m];
                proj_hist_sort[m] = proj_hist_sort[n];
                proj_hist_sort[n] = k;
            }
        }
    }

    // ȷ����ֵ
    peak = 0;
    valley = 0;

    for (k = 0; k < plate_w / 10; k++)
    {
        peak += proj_hist_sort[k];
        valley += proj_hist_sort[plate_w - 1 - k];
    }

    thresh = (peak - valley) / ((plate_w / 10) * 4);

    left = 0;

    // Ѱ����ߵ�һ����ֵ
    for (n = 4; n < plate_w / 2; n++)
    {
        if ((proj_hist_orig[n] - proj_hist_orig[n - 2] > thresh) ||
            (proj_hist_orig[n] - proj_hist_orig[n - 4] > thresh))
        {
            left = n;
            break;
        }
    }

    // ����Ĵ����Ƿ��б�Ҫ��
// ============================================================================
    // ȥ����߿�
    k = n - 4;// MAX2(0, n - 4);
    m0 = 0;
    m1 = plate_w - 1;

    for (n = k; n < plate_w - 1; n++)
    {
        if (proj_hist_orig[n + 1] - proj_hist_orig[n] > thresh)
        {
            m0 = n + 1;
            break;
        }
    }

    for (; n < plate_w - 1; n++)
    {
        if (proj_hist_orig[n] - proj_hist_orig[n + 1] > thresh)
        {
            m1 = n;// MAX2(n, 0);
            break;
        }
    }

    if (m1 - m0 + 1 <= 3)   // ������߿�
    {
#if DEBUG_SEGMENT_PRINT
        printf("left border delete!!........\n");
#endif

        for (n = m1 + 1; n < plate_w - 2; n++)
        {
            if (proj_hist_orig[n + 1] - proj_hist_orig[n] > thresh / 3)
            {
                if (n + 1 - left < 6)
                {
                    left = n + 1;
                }

                break;
            }
        }
    }

// ============================================================================

    right = plate_w - 1;
#if 1

    // Ѱ���ұߵ�һ����ֵ
    for (n = plate_w - 1; n >= 4; n--)
    {
        if ((proj_hist_orig[n - 2] - proj_hist_orig[n] > thresh) ||
            (proj_hist_orig[n - 4] - proj_hist_orig[n] > thresh))
        {
            right = n - 1;
            break;
        }
    }

#else

    // �޷�ȷ���ұ߿���Ϊ�п���������'1'
    for (n = plate_w - 1; n >= 4; n--)
    {
        if (proj_hist_orig[n] > 0)
        {
            right = n;
            break;
        }
    }

#endif

#if DEBUG_SEGMENT_HIST
    cvRectangle(hist,
                cvPoint(0, left),
                cvPoint(hist->height - 1, left),
                cvScalar(0, 0, 255, 1), 1, 1, 0);

    cvRectangle(hist,
                cvPoint(0, right),
                cvPoint(hist->height - 1, right),
                cvScalar(0, 0, 255, 1), 1, 1, 0);

    cvNamedWindow("hist", CV_WINDOW_AUTOSIZE);
    cvShowImage("hist", hist);
    cvWaitKey(0);
    cvReleaseImage(&hist);
    cvDestroyWindow("hist");
#endif

    if (right <= left)
    {
        *x0 = 0;
        *x1 = plate_w - 1;
    }
    else
    {
        *x0 = left;
        *x1 = right;
    }

    // ���ڻ������ұ߽綨λҪ�ر�С�ģ����ұ߽��ϵ��ַ��������׺ͱ߿�ճ��
    if (bkgd_color == (uint8_t)1)
    {
        // �����λ��ĳ��ƿ��С��3.5�����ַ��߶ȣ�������������΢��չһ��
        if ((*x1 - *x0 + 1) < (int32_t)(ch_h * 3.5f))
        {
            *x0 = MAX2(0, *x0 - ch_h * 2 / 3);
            *x1 = MIN2(plate_w - 1, *x1 + ch_h * 2 / 3);
        }
    }

    for (k = 0; k < plate_h; k++)
    {
        memset(bina_img + plate_w * k, 0, *x0);
        memset(bina_img + plate_w * k + *x1, 0, plate_w - *x1);
    }

#ifdef _TMS320C6X_
    MEM_free(IRAM, proj_hist_orig, sizeof(int32_t) * plate_w);
    MEM_free(IRAM, proj_hist_sort, sizeof(int32_t) * plate_w);
#else
    CHECKED_FREE(proj_hist_sort, sizeof(int32_t) * plate_w);
    CHECKED_FREE(proj_hist_orig, sizeof(int32_t) * plate_w);
#endif

    return;
}

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

// ȥ���������±߿�ĸ���
static void remove_upper_and_lower_border(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t len;
    int32_t start, end;
    const int32_t thresh = plate_h * 3 / 2;

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

// ɾ����С������
static void remove_too_small_region(uint8_t *restrict bina_img, Rects *restrict ch,
                                    int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t k, n;
    int32_t ch_h;
    int32_t proj_h;
    int32_t avg_h;
    int32_t avg_w;
    int32_t ch_num = *ch_num1;

    if (ch_num == 0)
    {
        return;
    }

    // ɾ����Ⱥ�С������
    for (k = 0; k < ch_num; k++)
    {
        avg_w = get_character_average_width(ch, ch_num, plate_h);

        // ����ַ����С��ƽ����ȵ�1/6��ȷ��Ϊ����
        if (ch[k].x1 - ch[k].x0 + 1 < avg_w / 6)
        {
            for (n = k; n < ch_num - 1; n++)
            {
                ch[n] = ch[n + 1];
            }

            ch_num--;
            k--;
        }
    }

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

// ͶӰ����λ�ַ����±߽�
static void character_upper_and_lower_locating(uint8_t *bina_img, Rects *ch, int32_t *ch_num1,
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

// ���ݴ�ֱͶӰֱ��ͼ��λ�ַ������ұ߽�
void character_left_and_right_locating(int32_t *restrict hist, int32_t len, Rects *restrict ch, int32_t *ch_num1)
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

// ���õ����ķ�����λ�ַ����ұ߽�
static void character_left_and_right_locating_iteration(uint8_t *restrict bina_img, Rects *ch, int32_t *ch_num1,
                                                        int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t m, n, k;
    int32_t ch_num;
    int32_t p;
    int32_t merge;
    int32_t iteration;
    int32_t *restrict hist = NULL;
    uint8_t *restrict merge_mask = NULL;

#if DEBUG_SEGMENT_LR
    int32_t yt = 0;
    IplImage *plate = cvCreateImage(cvSize(plate_w, plate_h * 10), IPL_DEPTH_8U, 3);
    IplImage *resize = cvCreateImage(cvSize(300, 600), IPL_DEPTH_8U, 3);
    cvZero(plate);
#endif

    CHECKED_MALLOC(hist, sizeof(int32_t) * plate_w, CACHE_SIZE);
    CHECKED_MALLOC(merge_mask, sizeof(uint8_t) * plate_w, CACHE_SIZE);

    // ��ֵͼ����ˮƽ����ͶӰ
    for (w = 0; w < plate_w; w++)
    {
        hist[w] = 0;

        for (h = 0; h < plate_h; h++)
        {
            hist[w] += bina_img[plate_w * h + w] > (uint8_t)0;
        }
    }

    character_left_and_right_locating(hist, plate_w, ch, &ch_num);

#if DEBUG_SEGMENT_LR
    cvNamedWindow("resize", CV_WINDOW_AUTOSIZE);

    _IMAGE3_(plate, bina_img, plate_w, plate_h, 0);

    yt += plate_h + 2;

    for (w = 0; w < plate_w; w++)
    {
        for (h = plate_h - 1; h > plate_h - hist[w] - 1; h--)
        {
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 0] = (uint8_t)255;
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 1] = (uint8_t)255;
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 2] = (uint8_t)255;
        }
    }

    for (h = 0; h < ch_num; h++)
    {
        for (w = ch[h].x0; w < ch[h].x1; w++)
        {
            plate->imageData[plate->widthStep * (yt - 1) + w * 3 + 0] = (uint8_t)0;
            plate->imageData[plate->widthStep * (yt - 1) + w * 3 + 1] = (uint8_t)0;
            plate->imageData[plate->widthStep * (yt - 1) + w * 3 + 2] = (uint8_t)255;
        }
    }

#endif

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
            for (w = 0; w < plate_w; w++)
            {
                if (bina_img[plate_w * p + w] && (merge_mask[w] == (uint8_t)0))
                {
                    hist[w]--;
                }
            }

            character_left_and_right_locating(hist, plate_w, ch, &ch_num);

            if (ch_num >= k + 1)
            {
                for (m = p; m < plate_h; m++)
                {
                    for (n = 0; n < plate_w; n++)
                    {
                        if (merge_mask[n] == (uint8_t)0)
                        {
                            bina_img[m * plate_w + n] = (uint8_t)0;
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

// ���ں�����ϱ߿�ճ��ȥ�������رգ���˴˴��Ĳ������ã�Ҳ���Թر�
//    // ����ͳ�ƶ�ֵͼ����ˮƽ����ͶӰ
//    for (w = 0; w < plate_w; w++)
//    {
//        hist[w] = 0;
//        for (h = 0; h < plate_h; h++)
//        {
//            hist[w] += bina_img[plate_w * h + w] > 0;
//        }
//    }
//    character_left_and_right_locating(hist, plate_w, ch, &ch_num);
//
//    // ͳ��ճ���ַ��ĸ���
//    merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);

#if DEBUG_SEGMENT_LR
    yt += plate_h + 2;

    _IMAGE3_(plate, bina_img, plate_w, plate_h, yt);

    yt += plate_h + 2;

    for (w = 0; w < plate_w; w++)
    {
        for (h = plate_h - 1; h > plate_h - hist[w] - 1; h--)
        {
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 0] = (uint8_t)255;
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 1] = (uint8_t)255;
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 2] = (uint8_t)255;
        }
    }

    for (h = 0; h < ch_num; h++)
    {
        for (k = ch[h].x0; k < ch[h].x1; k++)
        {
            plate->imageData[plate->widthStep * (yt - 1) + k * 3 + 0] = (uint8_t)0;
            plate->imageData[plate->widthStep * (yt - 1) + k * 3 + 1] = (uint8_t)0;
            plate->imageData[plate->widthStep * (yt - 1) + k * 3 + 2] = (uint8_t)255;
        }
    }

#endif

// ����ʵ�����ϱ߿�ճ����������٣���������ַ�֮���ճ����������²����������ɾ�����ַ������沿��,
// �ر����²�����ʶ������0.2%����
//    // ����ַ���Ŀ̫�ٻ���ճ���ַ�̫�࣬��Ϊ���ϱ߿�����ճ��
//    iteration = 1;
//    p = 0;
//    while ((ch_num <= 3 || merge >= 3) && (iteration < 5) && (p < 5))
//    {
//        k = ch_num;
//        // һ��һ�е�ɾ���ϱ߿�ֱ���ָ�õ����ַ�����ch_num + 1
//        for (; p < 5; p++)
//        {
//            for (w = 0; w < plate_w; w++)
//            {
//                if (bina_img[plate_w * p + w] && (merge_mask[w] == 0))
//                {
//                    hist[w]--;
//                }
//            }
//
//            character_left_and_right_locating(hist, plate_w, ch, &ch_num);
//
//            if (ch_num >= k + 1)
//            {
//                for (m = 0; m <= p; m++)
//                {
//                    for (n = 0; n < plate_w; n++)
//                    {
//                        if (merge_mask[n] == 0)
//                        {
// //                            bina_img[m * plate_w + n] = 0;
//                        }
//                    }
//                }
//                break;
//            }
//        }
//
//        // ����ͳ��ճ���ַ��ĸ���
//        merge = character_merge_numbers(merge_mask, ch, ch_num, plate_w, plate_h);
//
//        iteration++;
//    }
//    // �ų�����������������Ĺ���ǰ���ж�ͶӰ��Ӱ��
//    remove_isolated_lines(bina_img, 0, plate_h / 3 - 1, plate_w, plate_h);
//    remove_isolated_lines(bina_img, plate_h * 2 / 3, plate_h - 1, plate_w, plate_h);

    // ����ͳ��ֱ��ͼ
    for (w = 0; w < plate_w; w++)
    {
        hist[w] = 0;

        for (h = 0; h < plate_h; h++)
        {
            hist[w] += bina_img[plate_w * h + w] > (uint8_t)0;
        }
    }

    // ���ն�λ��ÿ���ַ��ı߽�
    character_left_and_right_locating(hist, plate_w, ch, &ch_num);

#if DEBUG_SEGMENT_LR
    yt += plate_h + 2;

    _IMAGE3_(plate, bina_img, plate_w, plate_h, yt);

    yt += plate_h + 2;

    for (w = 0; w < plate_w; w++)
    {
        for (h = plate_h - 1; h > plate_h - hist[w] - 1; h--)
        {
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 0] = (uint8_t)255;
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 1] = (uint8_t)255;
            plate->imageData[plate->widthStep * (yt + h) + w * 3 + 2] = (uint8_t)255;
        }
    }

    for (h = 0; h < ch_num; h++)
    {
        for (w = ch[h].x0; w < ch[h].x1; w++)
        {
            plate->imageData[plate->widthStep * (yt - 1) + w * 3 + 0] = (uint8_t)0;
            plate->imageData[plate->widthStep * (yt - 1) + w * 3 + 1] = (uint8_t)0;
            plate->imageData[plate->widthStep * (yt - 1) + w * 3 + 2] = (uint8_t)255;
        }
    }

#endif

#if DEBUG_SEGMENT_LR
    cvResize(plate, resize, CV_INTER_LINEAR);
    cvShowImage("resize", resize);
    cvWaitKey(0);
    cvReleaseImage(&plate);
    cvReleaseImage(&resize);
#endif

    *ch_num1 = ch_num;

    CHECKED_FREE(merge_mask,  sizeof(uint8_t) * plate_w);
    CHECKED_FREE(hist,  sizeof(int32_t) * plate_w);

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

        character_left_and_right_locating(hist, ch_w, ch_tmp, &num_tmp);

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
static void character_spliting(uint8_t *restrict bina_img, Rects *ch, int32_t *ch_num1, int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t ch_w;
    int32_t avg_w;
    int32_t ch_num = *ch_num1;
    int32_t spliting_flag = 0;

    for (k = 0; k < ch_num; k++)
    {
        ch_w = ch[k].x1 - ch[k].x0 + 1;

        // ÿ�η����Ҫ���¼����ַ�ƽ�����
        avg_w = get_character_average_width(ch, ch_num, plate_h);

        if (ch_w > avg_w * 4 / 3) // �ַ�������ճ��
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
static void character_merging(/*uint8_t *restrict bina_img, */Rects *restrict ch, int32_t *ch_num1,
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

static void character_correct(uint8_t *restrict bina_img, Rects *restrict ch, int32_t *ch_num1,
                              int32_t plate_w, int32_t plate_h)
{
    int32_t k;
    int32_t avg_w, avg_h;
    int32_t ch_num = *ch_num1;

    // �ַ�1���ಿ��ȥ������1���ֶ��ಿ����������ǳ����ڵ�6���ַ�ʱ����í��ճ��
    for (k = 4; k < ch_num - 1; k++)
    {
        int32_t w, h;
        int32_t *hist = NULL;

        CHECKED_MALLOC(hist, sizeof(int32_t) * plate_w, CACHE_SIZE);

        memset(hist, 0, sizeof(int32_t) * plate_w);

        avg_h = get_character_average_height(ch, ch_num, plate_h);
        avg_w = get_character_average_width(ch, ch_num, plate_h);

        // �����һ���ַ��ļ�����ڸ߶ȵ�1/4��
        // �ַ��Ŀ�ȴ���ƽ����ȵ�1/3��
        // ����һ���ַ��Ŀ�ȴ���ƽ����ȵ�1/2(ȷ������һ���ַ���Ϊ1�������ַ�����ϴ��������Ϊ����һ���ַ���1�����)
        // ������3����������Ϊ���ַ�Ϊ��í��ճ�����ַ�1
        if ((ch[k + 1].x0 - ch[k].x1 - 1 > avg_h / 4) && (ch[k].x1 - ch[k].x0 + 1 > avg_w / 3)
            && (ch[k + 1].x1 - ch[k + 1].x0 + 1 > avg_w / 2))
        {
            // ��ֵͼ����ˮƽ����ͶӰ
            for (w = ch[k].x0; w <= ch[k].x1; w++)
            {
                hist[w] = 0;

                // ȥ������1/4�߶Ƚ���ͶӰ���¶�λ���ַ�����߽�
                for (h = ch[k].y0 + (ch[k].y1 - ch[k].y0 + 1) / 4; h <= ch[k].y1; h++)
                {
                    hist[w] += bina_img[plate_w * h + w] > (uint8_t)0;
                }
            }

            for (w = ch[k].x0; w <= ch[k].x1; w++)
            {
                if (hist[w] > 0)
                {
                    break;
                }
            }

            if (ch[k].x1 - w + 1 > avg_w / 5)
            {
                ch[k].x0 = (int16_t)w;
            }
        }

        CHECKED_FREE(hist, sizeof(int32_t) * plate_w);
    }

    // �ַ���������ַ������һ���ַ������С���������ַ���Ƚ�С��˵���ַ��ָ��խ����΢��չһЩ��
    for (k = 1; k < ch_num - 1; k++)
    {
        avg_w = get_character_average_width(ch, ch_num, plate_h);
        avg_h = get_character_average_height(ch, ch_num, plate_h);

        // �����һ���ַ����С���ַ��߶ȵ�1/4��˵���ַ���Ϊ1��
        // �ַ����С��ƽ����ȵ�2/3��
        // ��ǰһ���ַ��ļ�������ַ��߶ȵ�1/4
        // ��������3��Ҫ����Ϊ�ַ����и���ˣ�������϶�����2������
        if ((ch[k + 1].x0 - ch[k].x1 - 1 < avg_h / 4) && (ch[k].x1 - ch[k].x0 + 1 < avg_w * 2 / 3)
            && (ch[k].x0 - ch[k - 1].x1 - 1 > avg_h / 4))
        {
            ch[k].x0 = (int16_t)((ch[k].x0 - 3) > (ch[k - 1].x1 + 2) ? (ch[k].x0 - 3) : ch[k].x0);
            ch[k].y0 = (int16_t)((ch[k].y0 - 2) < 0 ? 0 : (ch[k].y0 - 2));
        }
    }

    return;
}

// ���Ҷ�ʧ���ַ�(ֻ����ȱʧһ���ַ������)
#if 0
static void find_missing_character(Rects *restrict ch, int32_t *ch_num, int32_t plate_w, int32_t plate_h)
{
    int32_t k, p;
    int32_t num = *ch_num;
    int32_t avg_w;
    int32_t avg_i;
    int32_t xx0, xx1;
    int32_t dist;
    Rects new_ch;

    if (num >= CH_MAX)
    {
        return;
    }

    avg_w = get_character_average_width(ch, num, plate_h);
    avg_i = get_character_average_interval(ch, num);

    for (k = 0; k < num - 1; k++)
    {
        xx0 = (ch[k + 0].x0 + ch[k + 0].x1) / 2;
        xx1 = (ch[k + 1].x0 + ch[k + 1].x1) / 2;

        dist = xx1 - xx0;

        // һ���ַ�ȱʧ
        if ((dist > avg_w * 2) && (dist < avg_w * 7 / 2))
        {
#if DEBUG_SEGMENT_PRINT
            printf("miss %d\n", k + 1);
#endif
            new_ch.x0 = ch[k + 0].x1 + avg_i;
            new_ch.x1 = ch[k + 1].x0 - avg_i;
            new_ch.y0 = (ch[k + 0].y0 + ch[k + 1].y0) / 2;
            new_ch.y1 = (ch[k + 0].y1 + ch[k + 1].y1) / 2;

            if (((new_ch.x1 - new_ch.x0) > avg_w / 2) && (new_ch.y1 > new_ch.y0))
            {
                num++;

                for (p = num - 2; p >= k + 1; p--)
                {
                    ch[p + 1] = ch[p];
                }

                ch[k + 1] = new_ch;
            }
        }
    }

    *ch_num = num;

    return;
}
#endif

// ȥ�����ұ߽�ķ��ַ�����
static void remove_non_character_region(uint8_t *restrict bina_img, Rects *restrict chs, int32_t *ch_num,
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

    ch = chs; // ��ߵ�һ���ַ�

    // �����ߵ�һ���ַ��ǲ����������ַ�����
    ch_w = ch->x1 - ch->x0 + 1;

    // ��ߵ�һ���ַ��϶�������1
    if (ch_w >= avg_w / 3)
    {
        // ����ռ�ձ�ɾ��α�ַ�
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
    else
    {
        ch->x0 = 0;
        ch->x1 = 0;
    }

    ch = chs + *ch_num - 1; // �ұߵ�һ���ַ�

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

// �˳�α����
static int32_t plate_candidate_filtering(Rects *restrict ch, int32_t ch_num, int32_t plate_w/*, int32_t plate_h*/)
{
    int32_t k;
    int32_t len;
    int32_t interval_max;
    int32_t interval;

    // �ָ�õ����ַ���Ŀ̫���̫��
    if (ch_num <= 4 || ch_num > 15)
    {
#if DEBUG_SEGMENT_PRINT
        printf("ch_num = %d is too small or too large!\n", ch_num);
#endif
        return 0;
    }

    // ���ƿ��̫խ
    len = ch[ch_num - 1].x1 - ch[0].x0 + 1;

    if (len < plate_w / 3)
    {
#if DEBUG_SEGMENT_PRINT
        printf("plate_w = %d is too small!\n", len);
#endif
        return 0;
    }

    interval_max = 0;

    for (k = 0; k < ch_num - 1; k++)
    {
        interval = ch[k + 1].x0 - ch[k].x1 + 1;

        if (interval > interval_max)
        {
            interval_max = interval;
        }
    }

    // �����̫խ
    if (interval_max <= 2)
    {
#if DEBUG_SEGMENT_PRINT
        printf("can't find interval_max, interval_max is too narrow!\n");
#endif
        return 0;
    }

    return 1;
}

// ����С���˷���λ���ֵı߽�
void chinese_locating_by_least_square(Rects *restrict ch, int32_t ch_num, int32_t avg_w, int32_t avg_h,
                                      /*int32_t plate_w, */int32_t plate_h, int32_t type)
{
    int32_t k;
    int32_t x, y;
    int32_t xx, yy;
    // �����ʹ��渡����
    int32_t a, b;
    int32_t denominator;
    // ��ͨ������ÿһ����׼�ַ����ĵ��X����
    int32_t std_ch_x_plain[7] = {0, 57, 136, 193, 250, 307, 364};
    // ���ó�����ÿһ����׼�ַ����ĵ��X����
    int32_t std_ch_x_plice[7] = {0, 79, 136, 193, 250, 307, 364};
    int32_t *std_ch_x = NULL;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    int32_t sum_x_x = 0;
//  int32_t sum_y_y = 0;
    int32_t sum_z_z = 0;
    int32_t sum_x_y = 0;
    int32_t sum_x_z = 0;

    if (type == 1)
    {
        std_ch_x = std_ch_x_plice;
    }
    else
    {
        std_ch_x = std_ch_x_plain;
    }

    if (ch_num < 7 || ch_num > 8)
    {
        ch[0].x1 = (int16_t)(MAX2(0, ch[1].x0 - 2));
        ch[0].x0 = (int16_t)(MAX2(0, ch[0].x1 - avg_w));
        ch[0].y1 = (int16_t)(ch[1].y1);
        ch[0].y0 = (int16_t)(ch[1].y0);

        return;
    }

    // ��ϵ�һ���ַ������ĵ�x����
    // ��ϵĲ������̣�x = a * z + b ����x��ʾ�ַ������ĵ�x����
    // z�ʾ��ǰ�ַ����ĵ㵽��һ���ַ����ĵ�ľ��?(x����)
    // =========================================
    // |��|A|-|0|1|2|3|4|
    //  0
    //  | z|
    //  |--z---|
    //  |---z----|
    //  |-----z----|
    //  |------z-----|
    //  |-------z------|
    // =========================================

    //      sum(z)*sum(x) - N*sum(z*x)
    // a = ----------------------------
    //      sum(z)*sum(z) - N*sum(z*z)

    //      sum(z*x)*sum(z) - sum(x)*sum(z*z)
    // b = -----------------------------------
    //      sum(z)*sum(z) - N*sum(z*z)

    // ���з�ĸsum(z)*sum(z) - N*sum(z*z)Ϊ��������Ϊ0
    sum_x = 0;
    sum_z = 0;
    sum_z_z = 0;
    sum_x_z = 0;

    for (k = 1; k < 7; k++)
    {
        x = (ch[k].x0 + ch[k].x1 + 0) / 2;
        y = (ch[k].y0 + ch[k].y1 + 0) / 2;

        sum_x   += x;
        sum_x_x += x * x;
        sum_z   += std_ch_x[k];
        sum_z_z += std_ch_x[k] * std_ch_x[k];
        sum_x_z += x * std_ch_x[k];
    }

    denominator = sum_z * sum_z - 6 * sum_z_z;

    // xx = a * z + b �Ե�һ���ַ������֣���z����0����xx = b
    xx = (sum_x_z * sum_z - sum_x * sum_z_z) / denominator;

    ch[0].x0 = (int16_t)MAX2((xx - avg_w / 2), 0);
    ch[0].x1 = (int16_t)MIN2((xx + avg_w / 2), ch[1].x0 - 1);

    if (ch[0].x1 <= ch[0].x0)
    {
        ch[0].x1 = (int16_t)(ch[0].x0 + 1);
    }

    // ��ϵ�һ���ַ������ĵ�y����
    // ��ϵĲ������̣�y = a * x + b ����y��ʾ�ַ������ĵ�y����
    // ����֪��һ���ַ�����x����ʱ�����Լ���õ���Ӧ��y����

    //      sum(x)*sum(y) - N*sum(x*y)
    // a = ----------------------------
    //      sum(x)*sum(x) - N*sum(x*x)

    //      sum(x*y)*sum(x) - sum(y)*sum(x*x)
    // b = -----------------------------------
    //      sum(x)*sum(x) - N*sum(x*x)

    // �ڶ����ַ��߶������ܵ��ݶ��ĸ��ţ�û�вο��ڶ����ַ��ĸ߶�
    sum_x = 0;
    sum_y = 0;
    sum_x_x = 0;
    sum_x_y = 0;

    for (k = 2; k < 7; k++)
    {
        x = (ch[k].x0 + ch[k].x1) / 2;
        y = (ch[k].y0 + ch[k].y1) / 2;

        sum_x   += x;
        sum_x_x += x * x;
        sum_y   += y;
        sum_x_y += x * y;
    }

    denominator = sum_x * sum_x - 5 * sum_x_x;

    if (denominator != 0)
    {
        // ���ǵ�����ת��������(PC��DSP��float���͵ľ����в��)����������ֱ������������õ��������
        a = sum_x * sum_y - 5 * sum_x_y;
        b = sum_x_y * sum_x - sum_y * sum_x_x;

        yy = (a * xx + b) / denominator;

        ch[0].y0 = (int16_t)MAX2((yy - avg_h / 2), 0);
        ch[0].y1 = (int16_t)MIN2((yy + avg_h / 2), plate_h - 1);

        if (ch[0].y0 >= ch[0].y1)
        {
            ch[0].y0 = (int16_t)(ch[1].y0);
            ch[0].y1 = (int16_t)(ch[1].y1);
        }
    }

    return;
}

// ���ֵ����ұ߽羫ȷ��λ(ͶӰ��)
void chinese_left_right_locating_by_projection(uint8_t *restrict gray_img, int32_t plate_w, int32_t plate_h,
                                               Rects *restrict ch, int32_t ch_num, int32_t jing_flag)
{
    int32_t w, h;
    int32_t x0, x1;
    int32_t sub_w;
    int32_t sub_h;
    int32_t avg_w;
    int32_t right_shift = 0;
    int32_t peak, thresh;
    int32_t *restrict sub_hist = NULL;
    uint8_t *restrict sub_gray = NULL;
    uint8_t *restrict sub_bina = NULL;
    Rects sub_rect;

    sub_rect.x0 = (int16_t)MAX2(0, ch[0].x0 - 4);
    sub_rect.x1 = (int16_t)MAX2(0, ch[1].x0 - 1);
    sub_rect.y0 = (int16_t)ch[0].y0;
    sub_rect.y1 = (int16_t)ch[0].y1;

    sub_w = sub_rect.x1 - sub_rect.x0 + 1;
    sub_h = sub_rect.y1 - sub_rect.y0 + 1;
    avg_w = get_character_average_width(ch, ch_num, plate_h);

    if ((sub_w < avg_w / 2) || (sub_w < 10))
    {
        return;
    }

#ifdef _TMS320C6X_
    sub_hist = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * sub_w, 128);
    sub_gray = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * sub_w * sub_h, 128);
    sub_bina = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * sub_w * sub_h, 128);
#else
    CHECKED_MALLOC(sub_hist, sizeof(int32_t) * sub_w, CACHE_SIZE);
    CHECKED_MALLOC(sub_gray, sizeof(uint8_t) * sub_w * sub_h, CACHE_SIZE);
    CHECKED_MALLOC(sub_bina, sizeof(uint8_t) * sub_w * sub_h, CACHE_SIZE);
#endif

    for (h = sub_rect.y0; h <= sub_rect.y1; h++)
    {
        memcpy(sub_gray + sub_w * (h - sub_rect.y0), gray_img + plate_w * h + sub_rect.x0, sub_w);
    }

    contrast_extend_for_gary_image(sub_gray, sub_gray, sub_w, sub_h);

    thresholding_avg_all(sub_gray, sub_bina, sub_w, sub_h);

    // ˮƽͶӰ
    peak = 0;

    for (w = 0; w < sub_w; w++)
    {
        sub_hist[w] = 0;

        for (h = 0; h < sub_h; h++)
        {
            sub_hist[w] += (sub_bina[sub_w * h + w] == CHARACTER_PIXEL);
        }

        peak = (sub_hist[w] > peak) ? sub_hist[w] : peak;
    }

    thresh = peak / 5;

    // ��λ�����ұ߽�
    x1 = sub_w - 1;

    if (jing_flag == 1)
    {
        // ���Ϊ���ƣ����ǵ������ұ������������˽��ұ߽��Ѱ�ҷ�Χ����1��
        right_shift = plate_h * 2 / 7;
    }
    else
    {
        right_shift = plate_h / 7;
    }

    for (w = sub_w - 1; w >= sub_w - right_shift; w--)
    {
        if ((sub_hist[w - 1] - sub_hist[w] > thresh)
            || (sub_hist[w - 2] - sub_hist[w] > thresh))
        {
            if (sub_hist[w - 0] != 0)
            {
                x1 = w - 0;
                break;
            }

            if (sub_hist[w - 1] != 0)
            {
                x1 = w - 1;
                break;
            }

            if (sub_hist[w - 2] != 0)
            {
                x1 = w - 2;
                break;
            }
        }
    }

    //���û���ҵ��ұ߽磬������ռ�ձȣ���������ֻҪĳһ��ռ�ձȴﵽһ�룬����Ϊ�ҵ����ұ߽�
    if (w == sub_w - right_shift - 1)
    {
        for (w = sub_w - 1; w >= sub_w - right_shift; w--)
        {
            if (sub_hist[w] > plate_h / 2)
            {
                x1 = w;
                break;
            }
        }
    }

    // �����Ȼû���ҵ��ұ߽磬���Ե�ǰ��Ϊ�ұ߽�
    if (w == sub_w - right_shift - 1)
    {
        x1 = w;
    }

    // ��λ������߽�
    if (x1 < ch[0].x1 - ch[0].x0 + 1)
    {
        x0 = 0;
    }
    else
    {
        x0 = x1 - (ch[0].x1 - ch[0].x0 + 1);

        if (sub_hist[x0] < thresh)  // ���ڹ�ֵ����ҪѰ�ҷ�ֵ
        {
            // Ѱ���ҷ�ֵ
            peak = -1;

            for (w = x0; w <= x0 + 5; w++)
            {
                if ((sub_hist[w + 1] - sub_hist[w] > thresh)
                    || (sub_hist[w + 2] - sub_hist[w] > thresh))
                {
                    if (sub_hist[w + 0] != 0)
                    {
                        peak = w + 0;
                        break;
                    }

                    if (sub_hist[w + 1] != 0)
                    {
                        peak = w + 1;
                        break;
                    }

                    if (sub_hist[w + 2] != 0)
                    {
                        peak = w + 2;
                        break;
                    }
                }
            }

            if (peak >= 0)
            {
                x0 = peak;
            }
        }
    }

    ch[0].x0 = (int16_t)(sub_rect.x0 + x0);
    ch[0].x1 = (int16_t)(sub_rect.x0 + x1);

#if 0
    {
        FILE *jpg;
        char gray_name[1024];
        char bina_name[1024];
        int32_t k;
        int32_t w, h;
        int32_t flag = 0;
        static int32_t frames = 1;
        IplImage *gray = cvCreateImage(cvSize(sub_w, sub_h), IPL_DEPTH_8U, 3);
        IplImage *bina = cvCreateImage(cvSize(sub_w, sub_h), IPL_DEPTH_8U, 3);
        IplImage *gresize = cvCreateImage(cvSize(sub_w * 4, sub_h * 4), IPL_DEPTH_8U, 3);
        IplImage *bresize = cvCreateImage(cvSize(sub_w * 4, sub_h * 4), IPL_DEPTH_8U, 3);

        for (h = 0; h < sub_h; h++)
        {
            for (w = 0; w < sub_w; w++)
            {
                bina->imageData[bina->widthStep * h + w * 3 + 0] = sub_bina[sub_w * h + w] == CHARACTER_PIXEL ? 0 : 255;
                bina->imageData[bina->widthStep * h + w * 3 + 1] = sub_bina[sub_w * h + w] == CHARACTER_PIXEL ? 0 : 255;
                bina->imageData[bina->widthStep * h + w * 3 + 2] = sub_bina[sub_w * h + w] == CHARACTER_PIXEL ? 0 : 255;
            }
        }

        for (h = 0; h < sub_h; h++)
        {
            bina->imageData[bina->widthStep * h + x0 * 3 + 0] = 0;
            bina->imageData[bina->widthStep * h + x0 * 3 + 1] = 0;
            bina->imageData[bina->widthStep * h + x0 * 3 + 2] = 255;
            bina->imageData[bina->widthStep * h + x1 * 3 + 0] = 0;
            bina->imageData[bina->widthStep * h + x1 * 3 + 1] = 0;
            bina->imageData[bina->widthStep * h + x1 * 3 + 2] = 255;
        }

        _IMAGE3_(gray, sub_gray, sub_w, sub_h, 0);

        sprintf(bina_name, "p// img%04d.bmp", frames);
        sprintf(gray_name, "p// img%04d_1.bmp", frames);

        frames++;

        cvNamedWindow("gresize", CV_WINDOW_AUTOSIZE);
        cvNamedWindow("bresize", CV_WINDOW_AUTOSIZE);

        cvResize(gray, gresize, CV_INTER_LINEAR);
        cvResize(bina, bresize, CV_INTER_LINEAR);

        cvShowImage("gresize", gresize);
        cvShowImage("bresize", bresize);

        cvWaitKey(0);

        cvDestroyWindow("gresize");
        cvDestroyWindow("bresize");

        cvSaveImage(gray_name, gresize);
        cvSaveImage(bina_name, bresize);

        cvReleaseImage(&gray);
        cvReleaseImage(&bina);
    }
#endif

#ifdef _TMS320C6X_
    MEM_free(IRAM, sub_hist, sizeof(int32_t) * sub_w);
    MEM_free(IRAM, sub_gray, sizeof(uint8_t) * sub_w * sub_h);
    MEM_free(IRAM, sub_bina, sizeof(uint8_t) * sub_w * sub_h);
#else
    CHECKED_FREE(sub_bina, sizeof(uint8_t) * sub_w * sub_h);
    CHECKED_FREE(sub_gray, sizeof(uint8_t) * sub_w * sub_h);
    CHECKED_FREE(sub_hist, sizeof(int32_t) * sub_w);
#endif

    return;
}

// ���һ���ַ����ұ߽羫ȷ��λ(����ֻ��λ�ұ߽�)
static void right_border_location_for_last_character(uint8_t *restrict bina_img,
                                                     int32_t plate_w, /*int32_t plate_h, */Rects *ch, int32_t avg_w)
{
    int32_t i, j;
    int32_t k;
    int32_t p;
    int32_t x0, x1;
    int32_t ch_w;
    int32_t len;
    int32_t left, right;
    int32_t *restrict hist = NULL;

    ch_w = ch->x1 - ch->x0 + 1;

    if (ch_w < avg_w || ch_w < 5)
    {
        return;
    }

    len = ch_w / 2;

    CHECKED_MALLOC(hist, len * sizeof(int32_t), CACHE_SIZE);

    x0 = ch->x1 - len + 1;
    x1 = ch->x1;

    // x0��x1֮��Ķ�ֵͼ�����ͶӰ��Ѱ�Ҳ���
    for (j = x0; j <= x1; j++)
    {
        hist[j - x0] = 0;

        for (i = ch->y0; i <= ch->y1; i++)
        {
            hist[j - x0] += bina_img[plate_w * i + j] > (uint8_t)0;
        }
    }

    p = -1;

    for (k = len - 3; k >= 2; k--)
    {
        left = k - 2;
        right = k + 2;

        if ((hist[left] - hist[k] > 4) && (hist[right] - hist[k] > 4))
        {
            // �ҵ����ȣ���Ϊ����ѷָ��
            p = k;
        }
    }

    if (p >= 0)     // ����ҵ������������ַ����ұ߽�
    {
        ch->x1 = (int16_t)(ch->x1 - len + p);      // �ַ��ұ߽�
        ch_w = ch->x1 - ch->x0 + 1;

        if (avg_w - ch_w > 0)
        {
            ch->x1 = (int16_t)(ch->x1 + (avg_w - ch_w) / 2 + 1);   // ��΢����һ��
        }

        ch->x1 = (int16_t)MIN2(plate_w - 1, ch->x1);
    }

    CHECKED_FREE(hist, len * sizeof(int32_t));

    return;
}

// ����ַ������±߽�
static void character_position_fitting(Rects *restrict ch, int32_t ch_num, int32_t plate_h)
{
    int32_t k;
    int32_t x, y0, y1;
    int32_t sum_x, sum_x_x;
    int32_t sum_y0, sum_x_y0;
    int32_t sum_y1, sum_x_y1;
    // �����ʹ��渡����
    int32_t a0, a1, b0, b1;
    int32_t denominator;

    // ��ϵĲ������̣�y = a * x + b

    //      sum(x)*sum(y) - N*sum(x*y)
    // a = ----------------------------
    //      sum(x)*sum(x) - N*sum(x*x)

    //      sum(x*y)*sum(x) - sum(y)*sum(x*x)
    // b = -----------------------------------
    //      sum(x)*sum(x) - N*sum(x*x)

    // ����ϱ߽���±߽�
    sum_x = 0;
    sum_x_x = 0;
    sum_y0 = 0;
    sum_x_y0 = 0;
    sum_y1 = 0;
    sum_x_y1 = 0;

    for (k = 1; k < ch_num; k++)
    {
        x = (ch[k].x1 + ch[k].x0) / 2;
        y0 = ch[k].y0;
        y1 = ch[k].y1;

        sum_x    += x;
        sum_x_x  += x * x;
        sum_y0   += y0;
        sum_x_y0 += x * y0;
        sum_y1   += y1;
        sum_x_y1 += x * y1;
    }

    denominator = sum_x * sum_x - (ch_num - 1) * sum_x_x;

    if (denominator != 0)
    {
        // �����ʹ��渡����
//      a0 = (sum_x * sum_y0 - (ch_num - 1) * sum_x_y0) / (float)denominator;
//      a1 = (sum_x * sum_y1 - (ch_num - 1) * sum_x_y1) / (float)denominator;
//      b0 = (sum_x_y0 * sum_x - sum_y0 * sum_x_x) / (float)denominator;
//      b1 = (sum_x_y1 * sum_x - sum_y1 * sum_x_x) / (float)denominator;
        a0 = sum_x * sum_y0 - (ch_num - 1) * sum_x_y0;
        a1 = sum_x * sum_y1 - (ch_num - 1) * sum_x_y1;
        b0 = sum_x_y0 * sum_x - sum_y0 * sum_x_x;
        b1 = sum_x_y1 * sum_x - sum_y1 * sum_x_x;

        for (k = 0; k < ch_num; k++)
        {
            x = (ch[k].x1 + ch[k].x0) / 2;

            // ���ǵ�����ת��������(PC��DSP��float���͵ľ����в��)����������ֱ������������õ��������
//          y0 = (int32_t)(a0 * x + b0 + 0.5f);
//          y1 = (int32_t)(a1 * x + b1 + 0.5f);
            y0 = (a0 * x + b0) / denominator;
            y1 = (a1 * x + b1) / denominator;

            y0 = CLIP3(y0, 0, plate_h - 1);
            y1 = CLIP3(y1, 0, plate_h - 1);

            if (y1 > y0)
            {
                ch[k].y0 = (int16_t)y0;
                ch[k].y1 = (int16_t)y1;
            }
        }
    }

    return;
}

// �ַ���ȷ��λ
static void character_position_adjustment(uint8_t *restrict bina_img, int32_t plate_w, int32_t plate_h,
                                          Rects *restrict ch, int32_t ch_num)
{
    int32_t k;
    int32_t x0, x1;
//  int32_t y0, y1;
    int32_t ch_w1;
    int32_t ch_w2;
    int32_t avg_w;
//  int32_t avg_h;

    // �����������ڵ��ַ������±߽�������ǰ�ַ������±߽�(ֻ�ܴ����2-��6���ַ�)
    for (k = 1; k < ch_num - 1; k++)
    {
        if ((abs(ch[k].y0 - ch[k - 1].y0) >= 4) && (abs(ch[k].y0 - ch[k + 1].y0) >= 4))
        {
            ch[k].y0 = (int16_t)((ch[k - 1].y0 + ch[k + 1].y0) / 2);
//          printf("%d-top", k);
        }

        if ((abs(ch[k].y1 - ch[k - 1].y1) >= 4) && (abs(ch[k].y1 - ch[k + 1].y1) >= 4))
        {
            ch[k].y1 = (int16_t)((ch[k - 1].y1 + ch[k + 1].y1) / 2);
//          printf("%d-bottom", k);
        }
    }

    // ���������ַ����ұ߽��ཻ�����
    for (k = 2; k < ch_num - 1; k++)
    {
        if (ch[k].x1 > ch[k + 1].x0) // ������ұ߽��ཻ
        {
            if (ch[k].x1 > ch[k + 1].x1) // �ַ�����
            {
                x1 = ch[k].x0 + (ch[k].x1 - ch[k].x0 + 1) / 2 - 1;
                x0 = x1 + 1;
                ch[k + 1].x1 = (int16_t)(x0 + (ch[k].x1 - ch[k].x0 + 1) / 2 - 1);
            }
            else
            {
                if (k == ch_num - 2)
                {
                    ch_w1 = ch[k + 0].x1 - ch[k + 0].x0 + 1;
                    ch_w2 = ch[k + 1].x1 - ch[k + 1].x0 + 1;

                    if (ch_w1 > 1.5 * ch_w2)
                    {
                        continue;
                    }
                }

                x1 = ch[k].x0 + (ch[k + 1].x1 - ch[k].x0 + 1) / 2 - 1;
                x0 = x1 + 1;
            }

            ch[k + 1].x0 = (int16_t)x0;
            ch[k + 0].x1 = (int16_t)x1;
        }
    }

    // �����ַ�ƽ�����
    avg_w = get_character_average_width(ch, ch_num, plate_h);
//  avg_h = get_character_average_height(ch, ch_num, plate_h);

// ===================================================================================
    // ��ȷ��λ���һ���ַ������ұ߽�
    right_border_location_for_last_character(bina_img, plate_w, /*plate_h, */&ch[ch_num - 1], avg_w);

    // ����ַ������±߽�
    character_position_fitting(ch, ch_num, plate_h);
// ===================================================================================

    return;
}

#if 0
#define EPSITION (0.01f)
static uint8_t character_position_standardization(Rects *restrict ch, int32_t ch_num, int32_t plate_w)
{
    int32_t i, j, k;
    int32_t avg_w;
    int32_t x0, x1;
    float std_len;
    float std_ch_w;
    float std_ch_center_x_7[7] = {22.5f, 79.5f, 158.5f, 215.5f, 272.5f, 329.5f, 386.5f};// ��������7���ַ�������λ��X����
    float std_ch_center_x_5[5] = {32.5f, 112.5f, 192.5f, 272.5f, 352.5f};               // ˫������5���ַ�������λ��X����
    float std_ch_center_x[7];  // ��ű�׼�����ַ����ĵ��X����
    float now_ch_center_x[7];  // ��ŵ�ǰ�����ַ����ĵ��X����
    uint8_t similarity = 0;    // ���ƶ�
    float c;           // ��������
    float c_min;
    float error;       // λ�����
    float error_min;   // ��Сλ�����
    int32_t r;         // ƥ�������뾶
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
            std_len = 409.0;  // �����ƣ�7�ַ�����׼���Ƴ���409mm
            std_ch_w = 45.0;  // �����ƣ�7�ַ�����׼�����ַ����45mm
            std_ch_center_x_ptr = std_ch_center_x_7;

            // ����õ���������c
            c = (ch[6].x1 - ch[0].x0 + 1) / std_len;

            for (k = 0; k < 7; k++)
            {
                now_ch_center_x[k] = (float)(ch[k].x1 + ch[k].x0 + 1) / 2 - ch[0].x0 + 1;
            }
        }
        else
        {
            std_len = 385.0;  // ˫���ƣ�5�ַ�����׼���Ƴ���385mm
            std_ch_w = 60.0;  // ˫���ƣ�5�ַ�����׼�����ַ����60mm
            std_ch_center_x_ptr = std_ch_center_x_5;

            // ����õ���������c
            c = (ch[4].x1 - ch[0].x0 + 1) / std_len;

            for (k = 0; k < 5; k++)
            {
                now_ch_center_x[k] = (float)(ch[k].x1 + ch[k].x0 + 1) / 2 - ch[0].x0 + 1;
            }
        }

        // ��ָ�õ����ַ�λ�ý��бȽϣ��������
        error_min = 10000;
        r_min = 0;
        c_min = c;

        for (i = -5; i <= 5; i++)
        {
            r = (int32_t)(std_ch_w * (c + EPSITION * i));

            for (k = 0; k < ch_num; k++)
            {
                std_ch_center_x[k] = std_ch_center_x_ptr[k] * (c + EPSITION * i);
            }

            for (j = -r; j <= r; j++)
            {
                error = 0;

                for (k = 0; k < ch_num; k++)
                {
                    if (ch_num == 7 && ((k == 1) || (k == 2)))
                    {
                        error += fabs(now_ch_center_x[k] - (std_ch_center_x[k] - j)) * 2;
                    }
                    else
                    {
                        error += fabs(now_ch_center_x[k] - (std_ch_center_x[k] - j));
                    }
                }

                if (error < error_min)
                {
                    error_min = error;
                    r_min = j;
                    c_min = c + EPSITION * i;
                }
            }
        }

        avg_w = (int32_t)(std_ch_w * c_min);

        // ���ݱ�׼���Ƴߴ����õ���ǰ�������ַ���λ��
        for (k = 0; k < ch_num; k++)
        {
            std_ch[k].x0 = (int32_t)MAX2(ch[0].x0 + ((std_ch_center_x_ptr[k] * c_min) - r_min) - avg_w / 2, 0);
            std_ch[k].x1 = (int32_t)MIN2(ch[0].x0 + ((std_ch_center_x_ptr[k] * c_min) - r_min) + avg_w / 2, plate_w - 1);
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
                    ch[k].x0 = x0;
                    ch[k].x1 = x1;
                }
            }
        }

        // ����һ�������ƿ��
        error_min = error_min / (ch_num * avg_w);

        if (error_min > 1.0f)
        {
            error_min = 1.0f;
        }

        similarity = (uint8_t)((1 - error_min) * 100);
    }

    return similarity;
}
#undef EPSITION

#else
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
#endif

// ѡȡ�������������ӵ㣬�ҵ����ʵ����ӵ�ʱ����1�����򷵻�0
static int32_t get_seed_point(uint8_t *restrict bina_img, uint8_t *restrict label_img, int32_t img_w, int32_t img_h,
                              int32_t *seedx, int32_t *seedy)
{
    int32_t w, h;
    int32_t _seedx = *seedx;
    int32_t _seedy = -1;
    uint8_t *restrict bina_ptr = NULL;
    uint8_t *restrict label_ptr = NULL;

    w = *seedx;

    h = (img_h >> 1);
    bina_ptr = bina_img + img_w * h + w;
    label_ptr = label_img + img_w * h + w;

    for (; h >= 0; h--)
    {
        if ((*bina_ptr == CHARACTER_PIXEL) && (*label_ptr == 0))
        {
            _seedy = h;
            break;
        }

        bina_ptr -= img_w;
        label_ptr -= img_w;
    }

    if (_seedy < 0)
    {
        h = (img_h >> 1) + 1;
        bina_ptr = bina_img + img_w * h + w;
        label_ptr = label_img + img_w * h + w;

        for (; h < img_h; h++)
        {
            if ((*bina_ptr == CHARACTER_PIXEL) && (*label_ptr == 0))
            {
                _seedy = h;
                break;
            }

            bina_ptr += img_w;
            label_ptr += img_w;
        }
    }

    *seedx = _seedx;
    *seedy = _seedy;

    return (*seedy >= 0);
}

// ͨ�����������ķ�ʽ����ַ�����ͬʱ����ǵõ����������쵽��׼ģ���С������������һ���̶���ȥ���߿�Ϳ��������ĸ���
// ���Ƕ��ڶ��ѳ�����2�飨���߸���飩���ַ������������ֻ�õ�������һ���ַ����򲢽������쵽��׼ģ���С
#define LABEL_TIMES (3+1)
void character_standardization_by_regional_growth(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t i;
    int32_t w, h;
    const int32_t area = img_w * img_h;
    int32_t area_lable = 0;
    int32_t xx, yy;
    int32_t label;
    int32_t seedx;
    int32_t seedy;
    int32_t start, end;
    int32_t curx;
    int32_t cury;
    int32_t resize_w;
    int32_t resize_h;
    const int8_t offsetx[8] = { -1, -1, -1,  0, 0,  1, 1, 1};
    const int8_t offsety[8] = { -1,  0,  1, -1, 1, -1, 0, 1};
    uint8_t *restrict label_img = NULL;
    uint8_t *restrict resize_img = NULL;
    uint8_t *restrict stackx = NULL;
    uint8_t *restrict stacky = NULL;
    Rects rect_label[LABEL_TIMES];
    int32_t idx = 0;
    int32_t x0 = img_w - 1;
    int32_t x1 = 0;
    int32_t y0 = img_h - 1;
    int32_t y1 = 0;
    int32_t label_sucess_flg = 0;
    int32_t seed_point_flg = 0;

    CHECKED_MALLOC(label_img, sizeof(uint8_t) * area, CACHE_SIZE);
    CHECKED_MALLOC(stackx, sizeof(uint8_t) * area, CACHE_SIZE);
    CHECKED_MALLOC(stacky, sizeof(uint8_t) * area, CACHE_SIZE);

    memset(label_img, 0, area);

    // �ڴ�ֱ��������Ѱ�����ӵ�
    seedx = img_w >> 1;
    seedy = img_h >> 1;
    seed_point_flg = get_seed_point(bina_img, label_img, img_w, img_h, &seedx, &seedy);

    if (!seed_point_flg)
    {
        CHECKED_FREE(stacky, sizeof(uint8_t) * area);
        CHECKED_FREE(stackx, sizeof(uint8_t) * area);
        CHECKED_FREE(label_img, sizeof(uint8_t) * area);

        return;
    }

    for (label = 1; label < LABEL_TIMES; label++)
    {
        label_img[img_w * seedy + seedx] = (uint8_t)label;

        start = 0;
        end = 0;
        area_lable = 0;

        stackx[end] = (uint8_t)seedx;
        stacky[end] = (uint8_t)seedy;

        rect_label[label].x0 = (int16_t)(img_w - 1);
        rect_label[label].x1 = (int16_t)0;
        rect_label[label].y0 = (int16_t)(img_h - 1);
        rect_label[label].y1 = (int16_t)0;

        while (start <= end)
        {
            curx = (int32_t)stackx[start];
            cury = (int32_t)stacky[start];

            for (i = 0; i < 8; i++)
            {
                xx = curx + offsetx[i];
                yy = cury + offsety[i];

                if ((xx < img_w) && (xx >= 0) && (yy < img_h) && (yy >= 0))
                {
                    if ((bina_img[img_w * yy + xx] == CHARACTER_PIXEL) && (label_img[img_w * yy + xx] == CHARACTER_PIXEL))
                    {
                        label_img[img_w * yy + xx] = (uint8_t)label;

                        end++;
                        stackx[end] = (uint8_t)xx;
                        stacky[end] = (uint8_t)yy;

                        area_lable++;

                        rect_label[label].x0 = (int16_t)MIN2(xx, rect_label[label].x0);
                        rect_label[label].x1 = (int16_t)MAX2(xx, rect_label[label].x1);
                        rect_label[label].y0 = (int16_t)MIN2(yy, rect_label[label].y0);
                        rect_label[label].y1 = (int16_t)MAX2(yy, rect_label[label].y1);
                    }
                }
            }

            start++;
        }

        // ������������Ѿ������ַ������±߽���ֱ���˳�����
        if ((rect_label[label].y0 == 0) && (rect_label[label].y1 == img_h - 1))
        {
            label++;
            break;
        }

        seed_point_flg = get_seed_point(bina_img, label_img, img_w, img_h, &seedx, &seedy);

        if (!seed_point_flg)
        {
            seedx = img_w >> 2;
            seed_point_flg = get_seed_point(bina_img, label_img, img_w, img_h, &seedx, &seedy);

            if (!seed_point_flg)
            {
                label++;
                break;
            }
        }
    }

    for (idx = 1; idx < label; idx++)
    {
        if ((rect_label[idx].x1 - rect_label[idx].x0 > 3) && (rect_label[idx].y1 - rect_label[idx].y0 > 3))
        {
            label_sucess_flg = 1;

            x0 = MIN2(x0, rect_label[idx].x0);
            x1 = MAX2(x1, rect_label[idx].x1);
            y0 = MIN2(y0, rect_label[idx].y0);
            y1 = MAX2(y1, rect_label[idx].y1);
        }
        else
        {
            for (i = 0; i < area; i++)
            {
                if (label_img[i] == (uint8_t)idx)
                {
                    label_img[i] = (uint8_t)0;
                }
            }
        }
    }

    if (label_sucess_flg)
    {
        // ����ַ��ıʻ�û�а�����׼�ַ��ı߽磬����Ҫ���ַ�������ȡ���Ŵ󵽱�׼�ַ��ı߽�
        if ((x0 > 0) || (x1 < (img_w - 1)) || (y0 > 0) || (y1 < img_h - 1))
        {
            resize_w = x1 - x0 + 1;
            resize_h = y1 - y0 + 1;

            CHECKED_MALLOC(resize_img, resize_w * resize_h * sizeof(uint8_t), CACHE_SIZE);

            memset(resize_img, 255, resize_w * resize_h);

            for (h = y0; h <= y1; h++)
            {
                for (w = x0; w <= x1; w++)
                {
                    if (label_img[img_w * h + w] > (uint8_t)0)
                    {
                        resize_img[resize_w * (h - y0) + w - x0] = CHARACTER_PIXEL;
                    }
                }
            }

            image_resize_using_interpolation(resize_img, bina_img, resize_w, resize_h, img_w, img_h);

            CHECKED_FREE(resize_img, resize_w * resize_h * sizeof(uint8_t));
        }
        else
        {
            for (i = 0; i < area; i++)
            {
                bina_img[i] = (label_img[i] > (uint8_t)0) ? (uint8_t)CHARACTER_PIXEL : (uint8_t)255;
            }
        }
    }

    CHECKED_FREE(stacky, sizeof(uint8_t) * area);
    CHECKED_FREE(stackx, sizeof(uint8_t) * area);
    CHECKED_FREE(label_img, sizeof(uint8_t) * area);

    return;
}
#undef LABEL_TIMES

static void chinese_frame_adjust(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t *restrict hist_w = NULL;
    int32_t *restrict hist_h = NULL;
    int32_t x0, x1, y0, y1;
    int32_t xx0, xx1, yy0, yy1;
    int32_t ch_w, ch_h;

    CHECKED_MALLOC(hist_w, sizeof(int32_t) * img_w, CACHE_SIZE);
    CHECKED_MALLOC(hist_h, sizeof(int32_t) * img_h, CACHE_SIZE);

    memset(hist_w, 0, sizeof(int32_t) * img_w);
    memset(hist_h, 0, sizeof(int32_t) * img_h);

    // ���㴹ֱͶӰ
    for (w = 0; w < img_w; w++)
    {
        for (h = 0; h < img_h; h++)
        {
            hist_w[w] += (bina_img[img_w * h + w] == (uint8_t)CHARACTER_PIXEL);
        }
    }

    x0 = 0;

    // ��߿���
    if (hist_w[0] != 0)
    {
        // ��ߴ��ڱ߿�
        if (hist_w[0] == img_h)
        {
            int32_t p;
            int32_t min_hist;

            for (w = 0; w < img_w / 2; w++)
            {
                if (hist_w[w] < img_h)
                {
                    // �󵽱߿�ı߽�
                    break;
                }
            }

            p = w;
            min_hist = hist_w[w];
            x0 = w;

            // ��������չ3�����أ�����3�����ط�Χ���ҵ���СͶӰ����Ϊ��߽�
            for (w = p + 1; w < p + 3; w++)
            {
                if (hist_w[w] < min_hist)
                {
                    min_hist = hist_w[w];
                    x0 = w;
                }
            }
        }

        // ��ߴ��ڽϳ�������
        else if (hist_w[0] > img_h * 3 / 4)
        {
            int32_t min_hist = hist_w[0];
            x0 = 0;

            // ������չ4�������ҵ����ȵ�
            for (w = 1; w < 4; w++)
            {
                if (hist_w[w] < min_hist)
                {
                    min_hist = hist_w[w];
                    x0 = w;
                }
            }

            // ������չ3���������ҵ��Ĳ���ͶӰֵ����2����Ϊ����ҵ�������Ϊ�ַ���Ϣ������ȥ��
            if (hist_w[x0] > 2)
            {
                x0 = 0;
            }
        }
        else
        {
            x0 = 0;
        }
    }

    // �ұ߿���ʱû����
    x1 = img_w - 1;

    ch_w = x1 - x0 + 1;

    // ����ˮƽͶӰ
    for (h = 0; h < img_h; h++)
    {
        for (w = x0; w <= x1; w++)
        {
            hist_h[h] += (bina_img[img_w * h + w] == (uint8_t)CHARACTER_PIXEL);
        }
    }

    y0 = 0;

    // �����ϱ߿�
    if (hist_h[0] == ch_w)
    {
        for (h = 0; h < img_h / 2; h++)
        {
            if (hist_h[h] != ch_w)
            {
                y0 = h;
                break;
            }
        }
    }


    y1 = img_h - 1;

    // �����±߿�
    if (hist_h[img_h - 1] == ch_w)
    {
        for (h = img_h - 1; h > img_h / 2; h--)
        {
            if (hist_h[h] != ch_w)
            {
                y1 = h;
                break;
            }
        }
    }

    ch_h = y1 - y0 + 1;

    // ͼ���ı߿��ಿ��ȥ��
    memset(hist_w, 0, sizeof(int32_t) * img_w);
    memset(hist_h, 0, sizeof(int32_t) * img_h);

    // ����ȥ���߿��Ĵ�ֱͶӰ
    for (w = x0; w <= x1; w++)
    {
        for (h = y0; h <= y1; h++)
        {
            hist_w[w] += (int32_t)(bina_img[img_w * h + w] == (uint8_t)CHARACTER_PIXEL);
        }
    }

    xx0 = x0;

    // ȥ����߿հײ���
    for (w = x0; w < x1; w++)
    {
        // �߽���������2�����ص����
        if ((hist_w[w] > 2) && (w - x0 < 2))
        {
            xx0 = w;
            break;
        }

        if ((hist_w[w] > 0) && (w - x0 >= 2))
        {
            xx0 = w;
            break;
        }
    }

    xx1 = x1;

    // ȥ���ұ߿հײ���
    for (w = x1; w > xx0; w--)
    {
        // �߽���������2�����ص����
        if ((hist_w[w] > 2) && (x1 - w < 2))
        {
            xx1 = w;
            break;
        }

        if ((hist_w[w] > 0) && (x1 - w >= 2))
        {
            xx1 = w;
            break;
        }
    }

    ch_w = xx1 - xx0 + 1;

    // ���¼���ȥ���߿����¶�λ���ұ߽���ˮƽͶӰ
    for (h = y0; h <= y1; h++)
    {
        for (w = xx0; w <= xx1; w++)
        {
            hist_h[h] += (int32_t)((bina_img[img_w * h + w] == (uint8_t)CHARACTER_PIXEL) ? (uint8_t)1 : (uint8_t)0);
        }
    }

    yy0 = y0;

    // ȥ���ϱ߿հײ���
    for (h = y0; h < y1; h++)
    {
        // �߽���������2�����ص����
        if ((hist_h[h] > 2) && (h - y0 < 2))
        {
            yy0 = h;
            break;
        }

        if ((hist_h[h] > 2) && (h - y0 >= 2))
        {
            yy0 = h;
            break;
        }
    }

    yy1 = y1;

    // ȥ���±߿հײ���
    for (h = y1; h > yy0; h--)
    {
        // �߽���������2�����ص����
        if ((hist_h[h] > 2) && (y1 - h < 2))
        {
            yy1 = h;
            break;
        }

        if ((hist_h[h] > 2) && (y1 - h >= 2))
        {
            yy1 = h;
            break;
        }
    }

    // �����������ұ߽緢���仯�������¶�λ�ĺ������쵽��������
    if ((xx0 > 0) || (xx1 < img_w - 1) || (yy0 > 0) || (yy1 < img_h - 1))
    {
        uint8_t *restrict new_bina_img = NULL;

        ch_w = xx1 - xx0 + 1;
        ch_h = yy1 - yy0 + 1;

        CHECKED_MALLOC(new_bina_img, sizeof(uint8_t) * ch_w * ch_h, CACHE_SIZE);

        for (h = yy0; h <= yy1; h++)
        {
            memcpy(new_bina_img + ch_w * (h - yy0), bina_img + img_w * h + xx0, ch_w);
        }

        image_resize_using_interpolation(new_bina_img, bina_img, ch_w, ch_h, img_w, img_h);

        CHECKED_FREE(new_bina_img, sizeof(uint8_t) * ch_w * ch_h);
    }

    CHECKED_FREE(hist_h, sizeof(int32_t) * img_h);
    CHECKED_FREE(hist_w, sizeof(int32_t) * img_w);

    return;
}

#define EXTEND_W (4)
#define EXTEND_H (4)

// ��һ�ַ�ʶ��
void single_character_recognition(uint8_t *restrict plate_img, int32_t plate_w, /*int32_t plate_h, */Rects pos,
                                  uint8_t *restrict chs, uint8_t *restrict trust,
                                  uint8_t *restrict ch_buf, SvmModel *model,
                                  uint8_t noise_suppress, uint8_t is_chinese, uint8_t *is_ch)
{
//  int32_t w;
    int32_t h;
    int32_t ch_w, ch_h;
    feature_type *restrict feat = NULL;
    uint8_t *restrict ch_img = NULL;
    uint8_t *restrict std_gray = NULL;
    uint8_t *restrict std_bina = NULL;
    int32_t count_total = 0;

    CLOCK_INITIALIZATION();

    ch_w = pos.x1 - pos.x0 + 1;
    ch_h = pos.y1 - pos.y0 + 1;

#ifdef _TMS320C6X_
    feat = (feature_type *)MEM_alloc(IRAM, sizeof(feature_type) * FEATURE_NUMS, 128);
    ch_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * (ch_w + EXTEND_W * 2) * (ch_h + EXTEND_H * 2), 128);
    std_gray = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * STD_AREA, 128);
    std_bina = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * STD_AREA, 128);
#else
    CHECKED_MALLOC(feat, sizeof(feature_type) * FEATURE_NUMS, CACHE_SIZE);
    CHECKED_MALLOC(ch_img, sizeof(uint8_t) * (ch_w + EXTEND_W * 2) * (ch_h + EXTEND_H * 2), CACHE_SIZE);
    CHECKED_MALLOC(std_gray, sizeof(uint8_t) * STD_AREA, CACHE_SIZE);
    CHECKED_MALLOC(std_bina, sizeof(uint8_t) * STD_AREA, CACHE_SIZE);
#endif

    memset(feat, 0, sizeof(feature_type) * FEATURE_NUMS);

    for (h = pos.y0; h <= pos.y1; h++)
    {
        memcpy(ch_img + ch_w * (h - pos.y0), plate_img + plate_w * h + pos.x0, ch_w);
    }

    gray_image_resize_linear(ch_img, std_gray, ch_w, ch_h, STD_W, STD_H);

//    if (is_chinese)
//    {
//         contrast_extend_for_gary_image(std_gray, std_gray, STD_W, STD_H);
//    }

    contrast_extend_for_gary_image(std_gray, std_gray, STD_W, STD_H);
    thresholding_avg_all(std_gray, std_bina, STD_W, STD_H);
//  thresholding_otsu(std_gray, std_bina, STD_W, STD_H, 0, STD_W-1, 0, STD_H-1);

    if (is_chinese)
    {
        chinese_frame_adjust(std_bina, STD_W, STD_H);
    }

    if (noise_suppress)
    {
        character_standardization_by_regional_growth(std_bina, STD_W, STD_H);
    }

    memcpy(ch_buf, std_gray, (uint32_t)STD_AREA);

    if (is_chinese)
    {
        // 48 + 112 + 68 = 228
        count_total += grid_feature_4x4(feat, std_bina, STD_W, STD_H);
        count_total += peripheral_feature(feat + count_total, std_bina, STD_W, STD_H, 1);
        count_total += cross_count_feature(feat + count_total, std_bina, STD_W, STD_H);

        *chs = svm_predict_multiclass_probability(model, feat, trust, is_ch);
    }
    else
    {
        // 48 + 56 + 160 = 264
        count_total += grid_feature_4x4(feat, std_bina, STD_W, STD_H);
//          count_total += peripheral_feature(feat + count_total, std_bina, STD_W, STD_H, 2);
//          count_total += peripheral_feature_x(feat + count_total, std_bina, STD_W, STD_H, 2);

        *chs = svm_predict_multiclass_probability(model, feat, trust, is_ch);
    }

//  if (*chs == '2')
//  {
//      pos.y0 = MAX2(pos.y0 - EXTEND_H, 0);
//      pos.y1 = MIN2(pos.y1 + EXTEND_H, plate_h - 1);
//
//      for (h = pos.y0; h <= pos.y1; h++)
//      {
//          memcpy(ch_img + ch_w * (h - pos.y0), plate_img + plate_w * h + pos.x0, ch_w);
//      }
//
//      gray_image_resize_linear(ch_img, std_gray, ch_w, ch_h, STD_W, STD_H);
//
//      memcpy(ch_buf, std_gray, STD_AREA);
//  }
//     if (*chs == '7') // ʶ���'7'ʱ�п������ַ�'T'���ұ߽�����һ��
//     {
//         int32_t extend_w = 0;
//
//         pos.x0 = MAX2(pos.x0 - 0, 0);
//         pos.x1 = MIN2(pos.x1 + EXTEND_W, plate_w - 1);
//
//         extend_w = pos.x1 - pos.x0;
//
//         for (h = pos.y0; h <= pos.y1; h++)
//         {
//             memcpy(ch_img + ch_w * (h - pos.y0), plate_img + plate_w * h + pos.x0, extend_w);
//         }
//
//         gray_image_resize_linear(ch_img, std_gray, extend_w, ch_h, STD_W, STD_H);
//
//         memcpy(ch_buf, std_gray, STD_AREA);
//  }
//  if (*trust < 90 && !is_chinese)
//  {
// //       max_filter_all(std_gray, std_bina, STD_W, STD_H);
//         thresholding_avg_all_(std_gray, std_bina, STD_W, STD_H);
//
//      character_standardization_by_regional_growth(std_bina, STD_W, STD_H);
//
//      count_total += grid_feature_4x4(feat, std_bina, STD_W, STD_H);
//
//         *chs = svm_predict_multiclass_probability(model, feat, trust, is_ch);
//
//      memcpy(ch_buf, std_gray, (uint32_t)STD_AREA);
//  }

#if 0

    // if (is_chinese /*&& (*chs == '1' || *chs == '1')*/)
    {
        FILE *jpg;
        char orig_name[1024];
        char gray_name[1024];
        char bina_name[1024];
        int32_t k;
        int32_t w, h;
        int32_t flag = 0;
        static int32_t frames = 1;
        IplImage *orig = cvCreateImage(cvSize(ch_w, ch_h), IPL_DEPTH_8U, 3);
        IplImage *gray = cvCreateImage(cvSize(STD_W, STD_H), IPL_DEPTH_8U, 3);
        IplImage *bina = cvCreateImage(cvSize(STD_W, STD_H), IPL_DEPTH_8U, 1);

        _IMAGE3_(orig, ch_img, ch_w, ch_h, 0);
        _IMAGE3_(gray, std_gray, STD_W, STD_H, 0);

        for (h = 0; h < STD_H; h++)
        {
            for (w = 0; w < STD_W; w++)
            {
                bina->imageData[bina->widthStep * h + w] = std_bina[STD_W * h + w] == 0 ? 0 : 255;
            }
        }

        cvShowImage("ch", bina);
        cvWaitKey(0);

        if (((*chs >= '0') && (*chs <= '9')) || ((*chs >= 'A') && (*chs <= 'Z')))
        {
            if (!(*chs == (uint8_t)'0' || *chs == (uint8_t)'2' || *chs == (uint8_t)'5'
                  || *chs == (uint8_t)'8' || *chs == (uint8_t)'7' || *chs == (uint8_t)'T'))
            {
                sprintf(orig_name, "ppp// %c// t%02dimg%04d_%c_o.jpg", *chs, *trust, frames, *chs);
                sprintf(gray_name, "ppp// %c// t%02dimg%04d_%c.jpg", *chs, *trust, frames, *chs);
                sprintf(bina_name, "ppp// %c// t%02dimg%04d_%c.bmp", *chs, *trust, frames, *chs);

//              cvSaveImage(orig_name, orig);
                //cvSaveImage(gray_name, gray);
                //cvSaveImage(bina_name, bina);
            }
        }
        else
        {
//            if (*trust < 90)
            {
                sprintf(orig_name, "ppp// t%02dimg%04d_%d.bmp", *trust, frames, *chs);
                sprintf(gray_name, "ppp// t%02dimg%04d_%d.jpg", *trust, frames, *chs);
                sprintf(bina_name, "ppp// t%02dimg%04d_%d.bmp", *trust, frames, *chs);

//                 cvSaveImage(orig_name, orig);
//                 cvSaveImage(gray_name, gray);
//                cvSaveImage(bina_name, bina);
            }
        }

        frames++;

        cvReleaseImage(&orig);
        cvReleaseImage(&gray);
        cvReleaseImage(&bina);
    }

#endif

#ifdef _TMS320C6X_
    MEM_free(IRAM, feat, sizeof(feature_type) * FEATURE_NUMS);
    MEM_free(IRAM, ch_img, sizeof(uint8_t) * (ch_w + EXTEND_W * 2) * (ch_h + EXTEND_H * 2));
    MEM_free(IRAM, std_gray, sizeof(uint8_t) * STD_AREA);
    MEM_free(IRAM, std_bina, sizeof(uint8_t) * STD_AREA);
#else
    CHECKED_FREE(std_bina, sizeof(uint8_t) * STD_AREA);
    CHECKED_FREE(std_gray, sizeof(uint8_t) * STD_AREA);
    CHECKED_FREE(ch_img, sizeof(uint8_t) * (ch_w + EXTEND_W * 2) * (ch_h + EXTEND_H * 2));
    CHECKED_FREE(feat,  sizeof(feature_type) * FEATURE_NUMS);
#endif

    return;
}

#undef FEATURE_NUMS

#define CAND_MAX  (3)   // ��ѡ���������Ŀ
#define CHS_NUM7  (8)   // �������ַ�����Ŀ
#define CHS_NUM5  (5)   // ˫�����ַ�����Ŀ
#define CHS_NUM6  (6)   // �������ַ�����Ŀ

typedef struct
{
    uint8_t ch_buf[CHS_NUM7][STD_W*STD_H]; // �ַ��ĻҶ�ͼ��
    int32_t ch_num;
    Rects ch[CHS_NUM7];
    uint8_t chs[CHS_NUM7];
    uint8_t trust[CHS_NUM7];
    uint8_t result;
    uint8_t not_ch_cnt;             // ��������ĸ�ĸ���
} CandInfo;

// ����ú�ѡ�����ǵ�������������ѡȡCAND_MAX����ѡ��������ڵ�λ��
static void interval_for_single_layer_plate(Rects *restrict ch, int32_t ch_num,
                                            uint8_t *restrict interval_cand_idx, int32_t idx_max)
{
    int32_t k, p;
    uint8_t *restrict interval_val = NULL;
    uint8_t *restrict interval_idx = NULL;
    uint8_t tmp_val;
    uint8_t tmp_idx;

    memset(interval_cand_idx, INVALID_IDX, idx_max);

    // �պ���6���ַ�ʱֱ��ָ��������ڵ�1�͵�2���ַ�֮�䣨����ͨ���������С���˷��õ���
    // �պ���7���ַ�ʱ�п����Ǻ��ֶ�ʧ�˶��������һ��α�ַ������Ի�����ȥ�������
    // ����6���ַ�ʱ����ַ����ȫ����Ϊ�Ƿ�
    if (ch_num == CHS_NUM6)
    {
        interval_cand_idx[0] = (uint8_t)0;
    }
    else if (ch_num > CHS_NUM6)
    {
        CHECKED_MALLOC(interval_val, sizeof(uint8_t) * (ch_num - 1), CACHE_SIZE);
        CHECKED_MALLOC(interval_idx, sizeof(uint8_t) * (ch_num - 1), CACHE_SIZE);

        // �����ַ����
        for (k = 0; k < ch_num - 1; k++)
        {
            interval_val[k] = (uint8_t)(ch[k + 1].x0 - ch[k].x1);
            interval_idx[k] = (uint8_t)k;
        }

        // �ַ�������ս�������
        for (k = 0; k < ch_num - 2; k++)
        {
            for (p = k + 1; p < ch_num - 1; p++)
            {
                if (interval_val[k] < interval_val[p])
                {
                    tmp_val = interval_val[k];
                    interval_val[k] = interval_val[p];
                    interval_val[p] = tmp_val;

                    tmp_idx = interval_idx[k];
                    interval_idx[k] = interval_idx[p];
                    interval_idx[p] = tmp_idx;
                }
            }
        }

        p = 0;

        for (k = 0; k < ch_num - 1; k++)
        {
            // �������������һ��������
            if (interval_val[k] > 2 && (interval_idx[k] < MAX2(ch_num - 5, 2)))
            {
                interval_cand_idx[p++] = interval_idx[k];

                // ����պ���7���ַ����������ǡ���ڵ�2�͵�3���ַ�֮��ʱֱ���˳�
                if ((interval_idx[k] == (uint8_t)1 && ch_num == 7) || (p == idx_max))
                {
                    break;
                }
            }
        }

        // ���û���ҵ����������ָ��һ�������
        if ((p == 0) && (interval_idx[0] < MAX2(ch_num - 5, 2)))
        {
            interval_cand_idx[0] = interval_idx[0];
        }

        CHECKED_FREE(interval_idx, sizeof(uint8_t) * (ch_num - 1));
        CHECKED_FREE(interval_val, sizeof(uint8_t) * (ch_num - 1));
    }

    return;
}

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

// ͨ��Ѱ������ַ�����ķ�ʽ��Ѱ�ҵ����Ƶ�����ַ����
static uint8_t best_combination_for_single_layer_plate(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                                       int32_t ch_h, int32_t plate_w, Rects *restrict ch, uint8_t *restrict chs,
                                                       uint8_t *trust, uint8_t *restrict ch_buf, int32_t *ch_num,
                                                       uint8_t *restrict interval_cand_idx,
                                                       SvmModel *restrict model_english,
                                                       SvmModel *restrict model_numbers,
                                                       SvmModel *restrict model_chinese,
                                                       uint8_t *not_ch_cnt, int nChNum)
{
    int32_t k, n;
    int32_t ch_w = 0;
    int32_t avg_w = 0;
    int32_t avg_h = 0;
    int32_t best = 0;
    uint8_t result = (uint8_t)0;
    uint8_t is_ch = (uint8_t)0;
    CandInfo cands[CAND_MAX]; // ���ÿһ����ѡ�����λ�ö�Ӧ����Ϣ
    CandInfo *cand = NULL;
    CandInfo ctmp;
    uint8_t interval_idx = (uint8_t)0;
    CLOCK_INITIALIZATION();
#ifdef _TMS320C6X_1
    printf("ch_num is: %d\n", *ch_num);
#endif

    memset(cands, 0, sizeof(cands));
    memset(trust, 0, sizeof(uint8_t) * nChNum);

    // ����ÿһ����ѡ�������
    for (k = 0; k < CAND_MAX; k++)
    {
        cand = &cands[k];
        interval_idx = interval_cand_idx[k];

        if (interval_cand_idx[k] == (uint8_t)255)
        {
            cand->result = (uint8_t)0;
            cand->ch_num = 0;
            cand->not_ch_cnt = (uint8_t)nChNum;
            break;
        }

        // ����=1��Ϊ������λ�ã����ﲻ��Ҫ���캺��
        cand->ch_num = 1;

        for (n = (int32_t)interval_idx; n < (int32_t)*ch_num; n++)
        {
            cand->ch[n - (int32_t)interval_idx + 1] = ch[n];

            cand->ch_num++;

            if (cand->ch_num == nChNum)
            {
                break;
            }
        }

        if (cand->ch_num != nChNum)
        {
            cand->result = (uint8_t)0;
            cand->ch_num = 0;
            cand->not_ch_cnt = (uint8_t)nChNum;
            continue;
        }

        // �����ַ����λ��
        character_position_adjustment(bina_img, plate_w, ch_h, &cand->ch[1], cand->ch_num - 1);

        avg_w = get_character_average_width(&cand->ch[1], cand->ch_num - 1, ch_h);
        avg_h = get_character_average_height(&cand->ch[1], cand->ch_num - 1, ch_h);

        // �Ժ��ֽ��ж�λ
        chinese_locating_by_least_square(cand->ch, cand->ch_num, avg_w, avg_h, /*plate_w, */ch_h, 0);

        // �����ַ��ָ��׼ȷ��
        if (cand->ch_num == 8)
        {
            cand->result = character_position_standardization(cand->ch, 7, plate_w);
        }
        else
        {
            cand->result = character_position_standardization(cand->ch, cand->ch_num, plate_w);
        }

        // �ַ��ָ��׼ȷ��̫��ֱ���ų�
        if ((cand->result < (uint8_t)85 && cand->ch_num != 8)
            || (cand->result < (uint8_t)75 && cand->ch_num == 8))
        {
            cand->result = (uint8_t)0;
            cand->ch_num = 0;
            cand->not_ch_cnt = (uint8_t)CHS_NUM7;
#if DEBUG_SEGMENT_PRINT
            printf("�ַ��ָ��׼ȷ��̫��ʱֱ���ų���ѡ����........\n");
#endif
            continue;
        }

#if DEBUG_SEGMENT_PRINT
        printf("max interval: %d----", interval_cand_idx[k]);
#endif
        // ��΢��չһ��
        character_border_extend(cand->ch, &cand->ch_num, 1, plate_w, ch_h);

        avg_w = get_character_average_width(&cand->ch[1], cand->ch_num - 1, ch_h);

        single_character_recognition(gray_img, plate_w, /*ch_h, */cand->ch[1], &cand->chs[1],
                                     &cand->trust[1], &cand->ch_buf[1][0], model_english, (1u), (0u), &is_ch);

        if (is_ch == (uint8_t)0)
        {
            cand->not_ch_cnt++;
        }

        for (n = 2; n < cand->ch_num; n++)
        {
            cand->chs[n] = (uint8_t)0;

            ch_w = cand->ch[n].x1 - cand->ch[n].x0 + 1;

            if (ch_w < avg_w * 3 / 4)
            {
                cand->chs[n] = (uint8_t)'1';
                cand->trust[n] = (uint8_t)90;
                is_ch = (uint8_t)1;
            }
            else
            {
                single_character_recognition(gray_img, plate_w, /*ch_h, */cand->ch[n], &cand->chs[n],
                                             &cand->trust[n], &cand->ch_buf[n][0], model_numbers, (1u), (0u), &is_ch);
            }

            if (is_ch == (uint8_t)0)
            {
                cand->not_ch_cnt++;
            }

            if (cand->not_ch_cnt >= (uint8_t)4)
            {
                cand->result = (uint8_t)0;
                cand->ch_num = 0;
                cand->not_ch_cnt = (uint8_t)CHS_NUM7;
                break;
            }
        }

#if DEBUG_SEGMENT_PRINT
        printf("not ch: %d\n", cand->not_ch_cnt);
#endif
    }

//  printf("k = %d \n", k);

    // ���շָ����ŶȽ�������
    for (k = 0; k < CAND_MAX - 1; k++)
    {
        for (n = k + 1; n < CAND_MAX; n++)
        {
            if (cands[k].result < cands[n].result)
            {
                ctmp = cands[n];
                cands[n] = cands[k];
                cands[k] = ctmp;
            }
        }
    }

    best = -1;

    // Ѱ����ѵ�λ��
    for (k = 0; k < CAND_MAX; k++)
    {
        if (cands[k].ch_num != 0)
        {
            best = k;

            // �������������ѡλ�÷�������ĸ����Ŀ���1����ʱȡ�ָ����ŶȽϴ�(����8)��
            // �ָ����Ŷȸ�����
            if (abs(cands[k].not_ch_cnt - cands[k + 1].not_ch_cnt) <= 1)
            {
                best = (cands[k].result > (cands[k + 1].result + 8u)) ? k : (k + 1);
            }

            break;
        }
    }

    if (best >= 0)
    {
        result = cands[best].result;
        *ch_num = cands[best].ch_num;
        *not_ch_cnt = cands[best].not_ch_cnt;

        memcpy(ch, cands[best].ch, cands[best].ch_num * sizeof(Rects));
        memcpy(chs, cands[best].chs, cands[best].ch_num * sizeof(uint8_t));
        memcpy(trust, cands[best].trust, cands[best].ch_num * sizeof(uint8_t));
        memcpy(ch_buf, &cands[best].ch_buf, cands[best].ch_num * STD_AREA * sizeof(uint8_t));

        // �Ժ��ֽ��о�ȷ��λ
        chinese_left_right_locating_by_projection(gray_img, plate_w, ch_h, ch, *ch_num, 0);

        // �Ժ��ֽ���ʶ��
        single_character_recognition(gray_img, plate_w, /*ch_h,*/
                                     ch[0], &chs[0], &trust[0], ch_buf, model_chinese, (0u), (1u), &is_ch);

        more_chs_1_delete(chs, ch, 7, not_ch_cnt);

        if (*not_ch_cnt >= 4)
        {
            result = 0u;
            *ch_num = 0;
            *not_ch_cnt = (uint8_t)CHS_NUM7;
        }
    }
    else
    {
        result = (uint8_t)0;
        *ch_num = 0;
        *not_ch_cnt = (uint8_t)CHS_NUM7;
    }

#if DEBUG_SEGMENT_PRINT
    printf("-------------------------------\n");
#endif

    return result;
}

uint8_t character_segmentation(uint8_t *restrict src_img, int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                               int32_t *ch_num1, uint8_t *restrict chs, uint8_t *trust, uint8_t *trust_avg,
                               uint8_t *restrict ch_buf,
                               SvmModel *restrict model_english,
                               SvmModel *restrict model_numbers,
                               SvmModel *restrict model_chinese,
                               uint8_t *not_ch_cnt, uint8_t bkgd_color, float tan_angle, int nChNum)
{
//  int32_t yy = 0;
    int32_t k;
    int32_t y0, y1;
    int32_t x0 = 0;
    int32_t x1 = plate_w - 1;
    int32_t ch_num;
    int32_t ch_h;
    uint8_t result = (uint8_t)0;
    uint8_t *restrict plate_img = NULL;
    uint8_t *restrict bina_img = NULL;
    uint8_t *restrict gray_img = NULL;
    Rects *restrict ch_tmp = NULL;
#if DEBUG_SEGMENT_SHOW
    int32_t w, h;
    uint8_t *hist = NULL;
    int32_t offset = 0;
    int32_t yt = 0;
    IplImage *plate;
    IplImage *resize;
#endif
    CLOCK_INITIALIZATION();

    if (tan_angle < -0.02 || tan_angle > 0.02)
    {
        shear_transform(src_img, tan_angle, plate_w, plate_h);
    }

#if DEBUG_SEGMENT_SHOW
    plate = cvCreateImage(cvSize(plate_w, plate_h * 10), IPL_DEPTH_8U, 3);
    resize = cvCreateImage(cvSize(300 * 3 / 2, 450 * 3 / 2), IPL_DEPTH_8U, 3);

    cvNamedWindow("resize", CV_WINDOW_AUTOSIZE);

    cvZero(plate);

    _IMAGE3_(plate, src_img, plate_w, plate_h, 0);
#endif

    *not_ch_cnt = nChNum;
    *ch_num1 = 0;

    if (plate_h < 10)
    {
        *ch_num1 = 0;
        return result;
    }

    CLOCK_START(ch_segment);

#ifdef _TMS320C6X_
    ch_tmp = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * CH_MAX, 128);
    plate_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    gray_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
#else
    CHECKED_MALLOC(ch_tmp, sizeof(Rects) * CH_MAX, CACHE_SIZE);
    CHECKED_MALLOC(plate_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(gray_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
#endif

    // ��ȥ������
    sub_filter(src_img, plate_w, plate_h);
//    result_tmp = sub_filter_percent(gray_img, bina_img, plate_w, plate_h);

    // ��ʾ��ȥ������Ч��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h;
    _IMAGE3_(plate, src_img, plate_w, plate_h, offset);
#endif

    contrast_extend_for_gary_image(src_img, src_img, plate_w, plate_h);

// ��ʾ�Ҷ�ͼ����ǿЧ��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 2;
    _IMAGE3_(plate, src_img, plate_w, plate_h, offset);
#endif

#if 0
    thresholding_percent(src_img, bina_img, plate_w, plate_h,
                         plate_w / 6, plate_w - plate_w / 6,
                         plate_h / 5, plate_h - plate_h / 5, 40);
#else

    thresholding_otsu(src_img, bina_img, plate_w, plate_h,
                      plate_w / 6, plate_w - plate_w / 6,
                      plate_h / 5, plate_h - plate_h / 5);
#endif

// ��ʾ��ֵ��Ч��
#if DEBUG_SEGMENT_SHOW
    offset  = plate_h * 3;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset);
#endif

    // ����ȥ��
    remove_small_region_by_labelling(bina_img, plate_w, plate_h);


    // ��ȷ��λ�������±߽�
    plate_upper_and_lower_locating_exact(bina_img, &y0, &y1, plate_w, plate_h);

    ch_h = y1 - y0 + 1;

    if (ch_h < 10)
    {
        *ch_num1 = 0;

        goto CH_SEG_EXIT;
    }

    // ��ȷ��λ�������ұ߽�
    if (bkgd_color == (uint8_t)1)
    {
        plate_left_and_right_locating_exact(bina_img, plate_w, plate_h, &x0, &x1, ch_h, bkgd_color);
    }
    else
    {
        x0 = 0;
        x1 = plate_w - 1;
    }

// ��ʾ�����������ұ߽綨λЧ��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 4;
    _IMAGE3_(plate, bina_img, plate_w, plate_h, offset);

    cvRectangle(plate, cvPoint(x0, y0 + offset), cvPoint(x1, y1 + offset),
                cvScalar(0, 255, 0, 0), 1, 1, 0);
#endif

    ch_num = 0;

    memcpy(gray_img, src_img + plate_w * y0, plate_w * ch_h);
    memcpy(bina_img, bina_img + plate_w * y0, plate_w * ch_h);

    // ȥ�����±߿�ĸ���
    remove_upper_and_lower_border(bina_img, plate_w, ch_h);

    // ȥ��������ǰ���У�����ǰ��ֻ��������1/3��Ϊ����������ͼ����������ʶ������0.2%����
    remove_isolated_lines(bina_img, 0, ch_h - 1, plate_w, ch_h);

    // ���õ�����ͶӰ����λ�ַ������ұ߽�
    character_left_and_right_locating_iteration(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // ����ͶӰ����λ�ַ������±߽�
    character_upper_and_lower_locating(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // ��ʾ�ַ��������ұ߽綨λЧ��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 5 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    // ɾ��̫С������
    remove_too_small_region(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    // ճ���ַ�����
    character_spliting(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    //deskewing_vertical_by_single_ch(gray_img, bina_img, plate_w, plate_h, ch_tmp, ch_num, ch_h);

    // ���ƵĴ�ֱ��бУ��
    // �򿪴�ֱ��бУ������ͣ�����շ�վ�ȳ��ƿ��ܴ��ڴ�ֱ��б�ĳ�������Ч����
    // ���Ƕ�һ�㿨�ڵȲ����ڴ�ֱ��б�ĳ�������΢����ʶ��Ч��
    if (deskewing_vertical(gray_img, bina_img, plate_w, plate_h, ch_tmp, ch_num, ch_h))
    {

        // ��ֱ��бУ�������·ָ�
        ch_num = 0;

        // ���õ�����ͶӰ����λ�ַ������ұ߽�
        character_left_and_right_locating_iteration(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

        // ����ͶӰ����λ�ַ������±߽�
        character_upper_and_lower_locating(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

        // ɾ��̫С������
        remove_too_small_region(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

        // ճ���ַ�����
        character_spliting(bina_img, ch_tmp, &ch_num, plate_w, ch_h);
    }

    // ճ���ַ��������Ҫ�ٴ����¶�λ�ַ����±߽�
    character_upper_and_lower_locating(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

// ��ʾճ���ַ�����Ч��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 6 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    // �����ַ��ϲ�
    character_merging(/*bina_img, */ch_tmp, &ch_num, /*plate_w, */ch_h);

    // ��ʾ�����ַ��ϲ�Ч��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 7 + y0;
    _IMAGE3_(plate, bina_img, plate_w, ch_h, offset);

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate,
                    cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    // �ַ�����
//    character_correct(bina_img, ch_tmp, &ch_num, plate_w, plate_h);

    // �ٴ�ȥ��̫С������
    remove_too_small_region(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

//  find_missing_character(ch_tmp, &ch_num, plate_w, ch_h);

    // ȥ�����ұ߽�ķ��ַ�����
    remove_non_character_region(bina_img, ch_tmp, &ch_num, plate_w, ch_h);

    CLOCK_END("ch_segment");

//     {
//         IplImage* plate0 = cvCreateImage(cvSize(plate_w, plate_h), IPL_DEPTH_8U, 3);
//         IplImage* bina0 = cvCreateImage(cvSize(plate_w, plate_h), IPL_DEPTH_8U, 3);
//         _IMAGE3_(plate0, gray_img, plate_w, plate_h, 0);
//         _IMAGE3_(bina0, bina_img, plate_w, plate_h, 0);
//         cvShowImage("tmp0", plate0);
//         cvShowImage("tmp1", bina0);
//         cvWaitKey(0);
//     }
// ��ʾ�����ַ��ָ�Ч��
#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 8;
    _IMAGE3_(plate, src_img, plate_w, plate_h, offset);

    offset = plate_h * 7 + y0;

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch_tmp[k].x0, ch_tmp[k].y0 + offset),
                    cvPoint(ch_tmp[k].x1, ch_tmp[k].y1 + offset), cvScalar(0, 255, 0, 0), 1, 1, 0);
    }

#endif

    if (plate_candidate_filtering(ch_tmp, ch_num, plate_w/*, plate_h*/))
    {
        // ���ڵ���6���ַ�ʱͨ�������������ȷʵ�Ƿ��ǵ����ƣ����ֵ�λ����ͨ������6���ַ�����С���˷�����λ��
        if (ch_num >= 6)
        {
            int32_t ch_num_tmp = ch_num;
            uint8_t interval_cand_idx[CAND_MAX];

            // ����ú�ѡ�����ǵ�������������ѡȡCAND_MAX����ѡ��������ڵ�λ��
            interval_for_single_layer_plate(ch_tmp, ch_num, interval_cand_idx, CAND_MAX);

            // ͨ��Ѱ������ַ�����ķ�ʽ��Ѱ�ҵ����Ƶ�����ַ����
            CLOCK_START(best_combination_for_single_layer_plate);
            result = best_combination_for_single_layer_plate(gray_img, bina_img, ch_h, plate_w, ch_tmp,
                                                             chs, trust, ch_buf, &ch_num, interval_cand_idx,
                                                             model_english, model_numbers, model_chinese, not_ch_cnt, nChNum);
            CLOCK_END("best_combination_for_single_layer_plate");

            // �п����ǳ��Ƶ�2���ַ��͵�3���ַ���Բ�㱻����һ���ַ���
            if (result == 0u && ch_num_tmp == 8)
            {
                int32_t i;
                int32_t w, h;
                ch_num = ch_num_tmp;
                //                 printf("\ny0 = %2d y1 = %2d �߶� = %2d\n", ch_tmp[2].y0, ch_tmp[2].y1, ch_tmp[2].y1 - ch_tmp[2].y0);

                for (h = ch_tmp[2].y0; h <= ch_tmp[2].y1; h++)
                {
                    for (w = ch_tmp[2].x0; w <= ch_tmp[2].x1; w++)
                    {
                        bina_img[plate_w * h + w] = 0u;
                    }
                }

                for (i = 2; i < 8; i++)
                {
                    ch_tmp[i] = ch_tmp[i + 1];
                }

                ch_num--;

                // ����ú�ѡ�����ǵ�������������ѡȡCAND_MAX����ѡ��������ڵ�λ��
                interval_for_single_layer_plate(ch_tmp, ch_num, interval_cand_idx, CAND_MAX);

                // ͨ��Ѱ������ַ�����ķ�ʽ��Ѱ�ҵ����Ƶ�����ַ����
                result = best_combination_for_single_layer_plate(gray_img, bina_img, ch_h, plate_w, ch_tmp,
                                                                 chs, trust, ch_buf, &ch_num, interval_cand_idx,
                                                                 model_english, model_numbers, model_chinese, not_ch_cnt, nChNum);
            }
        }
        else
        {
            ch_num = 0;
        }
    }
    else
    {
#if DEBUG_SEGMENT_PRINT
        printf("not plate candidate !\n");
        printf("----------------------------------\n");
#endif
        ch_num = 0;
    }

    *ch_num1 = ch_num;

//  printf("-------------result = %d ch_num = %d\n", result, *ch_num1);

    for (k = 0; k < ch_num; k++)
    {
        ch[k].x0 = (int16_t)(ch_tmp[k].x0);
        ch[k].x1 = (int16_t)(ch_tmp[k].x1);
        ch[k].y0 = (int16_t)(ch_tmp[k].y0 + y0);
        ch[k].y1 = (int16_t)(ch_tmp[k].y1 + y0);
    }

    if (ch_num == 7 || ch_num == 8)
    {
        int32_t trust_sum = 0;

        for (k = 0; k < ch_num; k++)
        {
            trust_sum += (int32_t)trust[k];
        }

        *trust_avg = (uint8_t)(trust_sum / ch_num);

        memcpy(src_img + plate_w * y0, gray_img, plate_w * ch_h);
    }
    else
    {
        *trust_avg = 0;
    }

#if DEBUG_SEGMENT_SHOW
    offset = plate_h * 8;

    for (k = 0; k < ch_num; k++)
    {
        cvRectangle(plate, cvPoint(ch[k].x0, ch[k].y0 + offset),
                    cvPoint(ch[k].x1, ch[k].y1 + offset), cvScalar(0, 0, 255, 0), 1, 1, 0);
    }

    /*��ʾʶ���ַ���ֵ��Ч�� �� �����ַ��Ĵ�ֱ��бУ��*/
    if (1)
    {

        int32_t h, w;
        int32_t high, with;
        memset(bina_img, 0, sizeof(uint8_t) * plate_w * plate_h);

        for (k = 0; k < ch_num; k++)
        {
            uint8_t *restrict plate_gray_char = NULL;
            uint8_t *restrict plate_bina_char = NULL;
            high = ch[k].y1 - ch[k].y0 + 1;
            with = ch[k].x1 - ch[k].x0 + 1;
            CHECKED_MALLOC(plate_gray_char, sizeof(uint8_t) * with * high, CACHE_SIZE);
            CHECKED_MALLOC(plate_bina_char, sizeof(uint8_t) * with * high, CACHE_SIZE);
            memset(plate_gray_char, 0, sizeof(uint8_t) * with * high);
            memset(plate_bina_char, 0, sizeof(uint8_t) * with * high);

            for (h = 0; h < high; h++)
            {
                for (w = 0; w < with; w++)
                {
                    plate_gray_char[h * with + w] = src_img[(h + ch[k].y0) * plate_w + (w + ch[k].x0)];
                }
            }

            thresholding_avg_all(plate_gray_char, plate_bina_char, with, high);

            for (w = 0; w < with * high; w++)
            {
                plate_bina_char[w] = plate_bina_char[w] == 0 ? 255 : 0;
            }

            // ��ʾ�����ַ��Ĵ�ֱ��бУ��
            if (1)
            {
                float tan;
                tan = get_tan(plate_bina_char, with, high);
                shear_transform(plate_bina_char, tan, with, high);
            }

            for (h = 0; h < high; h++)
            {
                for (w = 0; w < with; w++)
                {
                    bina_img[(h + ch[k].y0) * plate_w + (w + ch[k].x0)] = plate_bina_char[h * with + w];

                }
            }

            CHECKED_FREE(plate_gray_char, sizeof(uint8_t) * with * high);
            CHECKED_FREE(plate_bina_char, sizeof(uint8_t) * with * high);
        }

        // ��ʾͼ
        offset = plate_h * 9;
        _IMAGE3_(plate, bina_img, plate_w, plate_h, offset);

        for (k = 0; k < ch_num; k++)
        {
            cvRectangle(plate, cvPoint(ch[k].x0, ch[k].y0 + offset),
                        cvPoint(ch[k].x1, ch[k].y1 + offset), cvScalar(0, 0, 255, 0), 1, 1, 0);
        }
    }

    cvResize(plate, resize, CV_INTER_LINEAR);

    cvShowImage("resize", resize);
    cvWaitKey(0);

//    CHECKED_FREE(hist, );

    cvReleaseImage(&plate);
    cvReleaseImage(&resize);
    cvDestroyWindow("resize");
#endif

CH_SEG_EXIT:

#ifdef _TMS320C6X_
    MEM_free(IRAM, ch_tmp, sizeof(Rects) * CH_MAX);
    MEM_free(IRAM, plate_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, gray_img, sizeof(uint8_t) * plate_w * plate_h);
#else
    CHECKED_FREE(gray_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(bina_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(plate_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(ch_tmp, sizeof(Rects) * CH_MAX);
#endif

    return result;
}
