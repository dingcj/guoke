#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "../portab.h"
#include "../malloc_aligned.h"
#include "plate_tracking.h"
#include "../hog_detect_src/hog_detect_plate.h"
#include "../time_test.h"
//#include "plate_direction.h"

#ifdef USE_EVM_648111111
//=====================================================================
#include <csl_legacy.h>
#include <csl.h>
#include <csl_dat.h>
//=====================================================================
#endif // USE_EVM_648111111

#define CLASS_MAX   (20)

#define LINE_SIZE   (img_w)
#define COPY_TIMES  (img_h * 3 / 2)

#ifdef _TMS320C6X_
#pragma CODE_SECTION(lc_plate_tracking_create, ".text:lc_plate_tracking_create")
#pragma CODE_SECTION(lc_plate_tracking_recreate, ".text:lc_plate_tracking_recreate")
#pragma CODE_SECTION(lc_plate_tracking_destroy, ".text:lc_plate_tracking_destroy")
#pragma CODE_SECTION(lc_plate_tracking_set_rect_detect, ".text:lc_plate_tracking_set_rect_detect")
#pragma CODE_SECTION(lc_plate_tracking, ".text:lc_plate_tracking")
#pragma CODE_SECTION(object_corespondence, ".text:object_corespondence")
#pragma CODE_SECTION(plate_running_distance, ".text:plate_running_distance")
#pragma CODE_SECTION(same_character_numbers_with_history_plate, ".text:same_character_numbers_with_history_plate")
#pragma CODE_SECTION(image_quality, ".text:image_quality")
#pragma CODE_SECTION(predict_new_position, ".text:predict_new_position")
/*#pragma CODE_SECTION(object_delete, ".text:object_delete")*/
#pragma CODE_SECTION(object_info_statistic, ".text:object_info_statistic")
#pragma CODE_SECTION(voting_for_best_value, ".text:voting_for_best_value")
#pragma CODE_SECTION(voting_for_best_value_weighting, ".text:voting_for_best_value_weighting")
/*#pragma CODE_SECTION(object_info_stable_or_not, ".text:object_info_stable_or_not")*/
#pragma CODE_SECTION(object_stable_or_not, ".text:object_stable_or_not")
#endif

// 创建车牌跟踪句柄
void lc_plate_tracking_create(Trk_handle *trk_handle, int32_t img_w, int32_t img_h)
{
    PlateTracking *trk = (PlateTracking *)trk_handle;
    const int32_t area = img_w * img_h * 3 / 2;
    int32_t i;

    CHECKED_MALLOC(trk, sizeof(PlateTracking), CACHE_SIZE);
    memset(trk, 0, sizeof(PlateTracking));

    for (i = 0; i < TRACK_MAX; i++)
    {
        CHECKED_MALLOC(trk->obj[i].yuv, sizeof(uint8_t) * area, CACHE_SIZE);
    }

    trk->rect_detect.x0 = (int16_t)0;
    trk->rect_detect.x1 = (int16_t)(img_w - 1);
    trk->rect_detect.y0 = (int16_t)((img_h * 1) / 10);
    trk->rect_detect.y1 = (int16_t)((img_h * 9) / 10);

    *trk_handle = (Trk_handle)trk;

    return;
}

void lc_plate_tracking_recreate(Trk_handle trk_handle, int32_t img_w, int32_t img_h)
{
    PlateTracking *trk = (PlateTracking *)trk_handle;
    const int32_t area = img_w * img_h * 3 / 2;
    int32_t i;

    if (trk)
    {
        for (i = 0; i < TRACK_MAX; i++)
        {
            CHECKED_FREE(trk->obj[i].yuv, sizeof(uint8_t) * area);
        }

        for (i = 0; i < TRACK_MAX; i++)
        {
            trk->obj[i].available = 0;
            CHECKED_MALLOC(trk->obj[i].yuv, sizeof(uint8_t) * area, CACHE_SIZE);
        }

        trk->rect_detect.x0 = (int16_t)(0);
        trk->rect_detect.x1 = (int16_t)(img_w - 1);
        trk->rect_detect.y0 = (int16_t)((img_h * 1) / 10);
        trk->rect_detect.y1 = (int16_t)((img_h * 9) / 10);
    }

    return;
}

// 撤销车牌跟踪句柄
void lc_plate_tracking_destroy(Trk_handle trk_handle, int32_t img_w, int32_t img_h)
{
    PlateTracking *trk = (PlateTracking *)trk_handle;
    int32_t i;
    const int32_t area = img_w * img_h * 3 / 2;

    if (trk)
    {
        for (i = 0; i < TRACK_MAX; i++)
        {
            CHECKED_FREE(trk->obj[i].yuv, sizeof(uint8_t) * area);
        }

        CHECKED_FREE(trk, sizeof(PlateTracking));
    }

    return;
}

// 设置车牌识别的检测区域
void lc_plate_tracking_set_rect_detect(Trk_handle trk_handle, Rects rect_detect)
{
    PlateTracking *trk = (PlateTracking *)trk_handle;

    if (trk)
    {
        trk->rect_detect = rect_detect;
    }

    return;
}

// 对车牌进行hog检测
static void car_plate_detect(uint8_t *yuv420, int32_t img_w, int32_t img_h, ObjectInfo *oinfo)
{
    Rects head_rect;
    CLOCK_INITIALIZATION();

    head_rect.x0 = 0;
    head_rect.x1 = 0;
    head_rect.y0 = 0;
    head_rect.y1 = 0;

    CLOCK_START(hog_detect_proc);

    if (hog_detect_proc(yuv420, img_w, img_h, &oinfo->pos, &head_rect))
    {
        oinfo->plate_flag = 1u;
    }
    else
    {
        oinfo->plate_flag = 0u;
    }

    oinfo->plate_flag = 1u;

    CLOCK_END("hog_detect_proc");

    return;
}

inline static int32_t plate_running_distance(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    return ((int32_t)sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)));
}

// 通过加权投票的方式选择最准确的信息
static int32_t voting_for_best_value_weighting(int32_t *restrict vode_data,
                                               int32_t *restrict vode_weight, int32_t data_num)
{
    int32_t k;
    int32_t cls;
    int32_t *restrict vote_number = NULL;
    int32_t *restrict vote_value = NULL;
    int32_t vote_max = 0;
    int32_t best_value = 0;

    CHECKED_MALLOC(vote_number, sizeof(int32_t) * CLASS_MAX, CACHE_SIZE);
    CHECKED_MALLOC(vote_value, sizeof(int32_t) * CLASS_MAX, CACHE_SIZE);

    memset(vote_number, 0, sizeof(int32_t) * CLASS_MAX);

    // 将所有候选信息分配到不同的类中
    for (k = 0; k < data_num; k++)
    {
        for (cls = 0; cls < CLASS_MAX; cls++)
        {
            if (vote_number[cls] == 0)
            {
                vote_value[cls] = vode_data[k];
                vote_number[cls] += vode_weight[k];
                break;
            }
            else if (vote_value[cls] == vode_data[k])
            {
                vote_number[cls] += vode_weight[k];
                break;
            }
        }
    }

    // 通过投票统计得到票数最多的候选信息
    vote_max = 0;

    for (cls = 0; cls < CLASS_MAX; cls++)
    {
        if (vote_number[cls] > vote_max)
        {
            vote_max = vote_number[cls];
            best_value = vote_value[cls];
        }
    }

    CHECKED_FREE(vote_value, sizeof(int32_t) * CLASS_MAX);
    CHECKED_FREE(vote_number, sizeof(int32_t) * CLASS_MAX);

    return best_value;
}

// 通过投票的方式决定目标的信息
static int32_t voting_for_best_value(int32_t *restrict vode_data, int32_t data_num)
{
    int32_t k = 0;
    int32_t cls = 0;
    int32_t *restrict vote_number = NULL;
    int32_t *restrict vote_value = NULL;
    int32_t vote_max = 0;
    int32_t best_value = 0;

    CHECKED_MALLOC(vote_number, sizeof(int32_t) * CLASS_MAX, CACHE_SIZE);
    CHECKED_MALLOC(vote_value, sizeof(int32_t) * CLASS_MAX, CACHE_SIZE);

    memset(vote_number, 0, sizeof(int32_t) * CLASS_MAX);

    // 将所有候选信息分配到不同的类中
    for (k = 0; k < data_num; k++)
    {
        for (cls = 0; cls < CLASS_MAX; cls++)
        {
            if (vote_number[cls] == 0)
            {
                vote_value[cls] = vode_data[k];
                vote_number[cls]++;
                break;
            }
            else if (vote_value[cls] == vode_data[k])
            {
                vote_number[cls]++;
                break;
            }
        }
    }

    // 通过投票统计得到票数最多的候选信息
    vote_max = 0;

    for (cls = 0; cls < CLASS_MAX; cls++)
    {
        if (vote_number[cls] > vote_max)
        {
            vote_max = vote_number[cls];
            best_value = vote_value[cls];
        }
    }

    CHECKED_FREE(vote_value, sizeof(int32_t) * CLASS_MAX);
    CHECKED_FREE(vote_number, sizeof(int32_t) * CLASS_MAX);

    return best_value;
}

// 统计视频流中检测到的车牌信息
static void object_info_statistic(ObjectInfo *restrict info, int32_t info_num, ObjectInfo *restrict stat_info)
{
    int32_t k, p;
    int32_t sum;
    int32_t *restrict vote_data = NULL;
    int32_t *restrict vote_weight = NULL;

    if (info_num <= 0)
    {
        return;
    }

    CHECKED_MALLOC(vote_data, sizeof(int32_t) * info_num, CACHE_SIZE);
    CHECKED_MALLOC(vote_weight, sizeof(int32_t) * info_num, CACHE_SIZE);

    // 采用投票的方法确定目标的信息

    // 统计得到最准确的车牌置信度
    sum = 0;

    for (k = 0; k < info_num; k++)
    {
        sum += (int32_t)info[k].trust;
    }

    stat_info->trust = (uint8_t)(sum / info_num);

    // 统计得到车牌单个字符的置信度
    for (p = 0; p < CH_NUMS - 1; p++)
    {
        int32_t info_num_tmp = info_num;

        sum = 0;

        for (k = 0; k < info_num; k++)
        {
            // 置信度为0,表示为非字符，不作统计
            if (info[k].chs_trust[p] == (uint8_t)0)
            {
                info_num_tmp--;
            }
            else
            {
                sum += (int32_t)info[k].chs_trust[p];
            }
        }

        if (info_num_tmp != 0)
        {
            stat_info->chs_trust[p] = (uint8_t)(sum / info_num_tmp);
        }
        else
        {
            stat_info->chs_trust[p] = (uint8_t)0;
        }
    }

    // 统计得到最准确的车牌颜色
    for (k = 0; k < info_num; k++)
    {
        vote_data[k] = (int32_t)info[k].color;
    }

    stat_info->color = (uint8_t)voting_for_best_value(vote_data, info_num);

    // 统计得到最准确的车身颜色
    for (k = 0; k < info_num; k++)
    {
        vote_data[k] = (int32_t)info[k].car_color;
    }

    stat_info->car_color = (uint8_t)voting_for_best_value(vote_data, info_num);

    // 统计结果为未知时，以最后一次识别结果为准
    if (stat_info->car_color == 99u)
    {
        stat_info->car_color = info[info_num - 1].car_color;
    }

    // 统计得到最准确的车身颜色置信度
    for (k = 0; k < info_num; k++)
    {
        vote_data[k] = (int32_t)info[k].car_color_trust;
    }

    stat_info->car_color_trust = (uint8_t)voting_for_best_value(vote_data, info_num);

    // 统计得到最准确的车牌字符数
    for (k = 0; k < info_num; k++)
    {
        vote_data[k] = (int32_t)info[k].ch_num;
    }

    stat_info->ch_num = (uint8_t)voting_for_best_value(vote_data, info_num);

    // 统计得到最准确的车牌平均亮度
    for (k = 0; k < info_num; k++)
    {
        vote_data[k] = (int32_t)info[k].brightness;
    }

    stat_info->brightness = (uint8_t)voting_for_best_value(vote_data, info_num);

    // 统计得到最准确的车牌字符
    for (p = 0; p < (int32_t)(stat_info->ch_num); p++)
    {
        int32_t stat_num = 0;

        for (k = 0; k < info_num; k++)
        {
            // 置信度为0，表示为非字符，不作统计
            if (info[k].chs_trust[p] > (uint8_t)0)
            {
                vote_data[stat_num] = (int32_t)info[k].chs[p];
                vote_weight[stat_num] = (int32_t)info[k].chs_trust[p];
                stat_num++;
            }
        }

        if (stat_num > 0)
        {
            stat_info->chs[p] = (uint8_t)voting_for_best_value_weighting(vote_data, vote_weight, stat_num);
        }
    }

    stat_info->chs[stat_info->ch_num] = (uint8_t)'\0';

//    for (k = 0; k < info_num; k++)
//    {
//        printf("%3d---\n", info[k].chs[0]);
//    }

    CHECKED_FREE(vote_weight, sizeof(int32_t) * info_num);
    CHECKED_FREE(vote_data, sizeof(int32_t) * info_num);

    return;
}

#define SAME_NUMBER_MIN (6)

// 判断车牌信息是否稳定，如果稳定则可以提前返回车牌信息
static int32_t object_stable_or_not(ObjectForTracking *restrict obj, int32_t trk)
{
    int32_t k;
    int32_t stable_flg = 0;
    int32_t info_num = obj[trk].info_idx;
    ObjectInfo *restrict info = obj[trk].info;

    if (obj[trk].time >= AFFIRMANCE)
    {
        // stable_flg为1时表示最近连续10帧检测、识别到该车牌的号牌完全一样
        if (info_num > 10)
        {
            stable_flg = 1;

            // 采用投票的方法确定目标的信息
            for (k = info_num - 2; k > info_num - 11; k--)
            {
                if (strcmp((const char *)info[info_num - 1].chs, (const char *)info[k].chs))
                {
                    stable_flg = 0;
                    break;
                }
            }
        }
    }

    return stable_flg;
}

// 车牌清晰度质量计算
static int32_t image_quality(uint8_t *restrict yuv_img, int32_t img_w, /*int32_t img_h, */Rects *plate_rect)
{
    int32_t w, h;
    int32_t plate_w, plate_h;
    uint8_t *restrict in00 = NULL;
    uint8_t *restrict in01 = NULL;
    uint8_t *restrict in10 = NULL;
    uint8_t *restrict pimg = NULL;
    int32_t total_value = 0;

    plate_w = plate_rect->x1 - plate_rect->x0 + 1;
    plate_h = plate_rect->y1 - plate_rect->y0 + 1;

    for (h = 0; h < plate_h - 1; h++)
    {
        pimg = yuv_img + img_w * (plate_rect->y0 + h) + plate_rect->x0;

        for (w = 0; w < plate_w - 1; w++)
        {
            in00 = pimg + w;
            in01 = in00 + 1;
            in10 = in00 + img_w;
            total_value += abs(*in00 - *in01) + abs(*in00 - *in10);
        }
    }

    return total_value;
}

// 判断当前车牌字符与上一个车牌字符是否完全相同
static int32_t all_characters_same_with_history_plate(ObjectForTracking *restrict obj, ObjectInfo *restrict info)
{
    int32_t i;
    int32_t idx = obj->info_idx - 1;

    if (obj->info[idx].ch_num == info->ch_num)
    {
        for (i = 0; i < (int32_t)(info->ch_num); i++)
        {
            if ((obj->info[idx].chs[i] != info->chs[i]) || ((char)info->chs[i] == '?'))
            {
                return 0;
            }
        }

        return 1;
    }
    else
    {
        return 0;
    }
}

// 判断车牌识别字符吻合度，用于排除车牌附近的伪车牌被加入跟踪序列
static int32_t same_character_numbers_with_history_plate(ObjectForTracking *restrict obj, ObjectInfo *restrict info)
{
    int32_t i, j;
    int32_t same_num; // 当前车牌与历史跟踪车牌相同字符的数量

    for (i = 0; i < obj->info_idx; i++)
    {
        same_num = 0;

        // 待关联车牌字符数与历史跟踪车牌字符数相等，一一对应比较
        if (obj->info[i].ch_num == info->ch_num)
        {
            for (j = 0; j < (int32_t)(info->ch_num); j++)
            {
                if (obj->info[i].chs[j] == info->chs[j])
                {
                    same_num++;
                }
            }

            if (same_num >= 4)
            {
                return 1;
            }
        }
    }

    return 0;
}

// 输出车牌的信息，同时，当目标已经到边界或者消失的时间过长时删除该目标
static void object_out_put(ObjectForTracking *restrict obj, int32_t trk,
                           ObjectInfo *restrict sinfo, int32_t *snum, int32_t delete_flg,
                           uint8_t *restrict saveyuv, int32_t img_w, int32_t img_h,
                           HistoryPlate *history_plate)
{
    int32_t ch_same_num = 0; // 与上一个车牌相同的字符个数
    int32_t m, n;
    int32_t same_plate_flag = 0; // 重复车牌标志
    int32_t err_plate_cnt = 0;

    if ((obj[trk].time >= AFFIRMANCE) && (obj[trk].already_output_flg == 0))
    {
        object_info_statistic(obj[trk].info, obj[trk].info_idx, &sinfo[*snum]);

        if (obj[trk].start_pos.y0 < obj[trk].end_pos.y0)
        {
            sinfo[*snum].directions = (uint8_t)1; // 车牌从上到下运动
        }
        else if (obj[trk].start_pos.y0 > obj[trk].end_pos.y0)
        {
            sinfo[*snum].directions = (uint8_t)2; // 车牌从下到上运动
        }
        else
        {
            sinfo[*snum].directions = (uint8_t)0; // 未知运动方向
        }

        // 统计跟踪车牌信息中的hog检测,如果对跟踪的车牌进行hog检测，60%以上都为非车牌，则认为是非车牌，不输出此车牌
//========================================================================
        for (m = 0; m < obj[trk].info_idx; m++)
        {
            err_plate_cnt += (obj[trk].info[m].plate_flag == 0u) ? 1u : 0u;
        }

        if (err_plate_cnt * 100 / obj[trk].info_idx > 60)
        {
            *snum = 0;
            obj[trk].available = 0; // 删除该跟踪节点
            return;
        }

//=========================================================================

        // 跟历史车牌进行对比，避免重复车牌
//====================================================================
        for (m = 0; m < SAME_PLATE_NUMS; m++)
        {
            ch_same_num = 0;

            for (n = 0; n < (int32_t)(sinfo[*snum].ch_num); n++)
            {
                if (sinfo[*snum].chs[n] == history_plate[m].chs_last_time[n])
                {
                    ch_same_num++;
                }
            }

            if (ch_same_num == (int32_t)(sinfo[*snum].ch_num))
            {
                same_plate_flag = 1;
                break;
            }
        }

        if (same_plate_flag == 1)
        {
            *snum = 0;
            obj[trk].already_output_flg = 1; // 设置输出标志表示该目标已输出
        }
        else
        {
            sinfo[*snum].pos = obj[trk].real_pos;
            sinfo[*snum].trk_idx = (uint8_t)trk;
            memcpy(saveyuv, obj[trk].yuv, img_w * img_h * 3 / 2);
            (*snum)++;

            // 将新出现的车牌添加到历史车牌队列中
            for (m = 0; m < SAME_PLATE_NUMS - 1; m++)
            {
                memcpy(history_plate[m].chs_last_time, history_plate[m + 1].chs_last_time, (uint32_t)CH_NUMS);
                history_plate[m].chs_last_time_frame = history_plate[m + 1].chs_last_time_frame;
            }

            memcpy(history_plate[SAME_PLATE_NUMS - 1].chs_last_time, sinfo[0].chs, (uint32_t)CH_NUMS);
            history_plate[SAME_PLATE_NUMS - 1].chs_last_time_frame = HISTORY_PLATE_FRAME;

            obj[trk].already_output_flg = 1; // 设置输出标志表示该目标已输出
        }
    }

    // 当目标已经到边界或者消失的时间过长时删除该目标
    if (delete_flg)
    {
        obj[trk].available = 0; // 删除该跟踪节点
    }

    return;
}

// 预测目标的下一个位置
static void predict_new_position(ObjectForTracking *restrict obj, Rects rect_detect,
                                 HistoryPlate *history_plate,
                                 ObjectInfo *restrict sinfo, int32_t *snum,
                                 uint8_t *restrict saveyuv, int32_t img_w, int32_t img_h)
{
    int32_t n, k;
    int32_t sum_diff_x;
    int32_t sum_diff_y;
    int32_t avg_x, avg_y;
    const int32_t border_x0 = rect_detect.x0 + IMG_BORDER;
    const int32_t border_x1 = rect_detect.x1 - IMG_BORDER;
    const int32_t border_y0 = rect_detect.y0 + IMG_BORDER;
    const int32_t border_y1 = rect_detect.y1 - IMG_BORDER;

    for (n = 0; n < TRACK_MAX; n++)
    {
        if (obj[n].available && obj[n].trace_num == AFFIRMANCE)
        {
            sum_diff_x = 0;
            sum_diff_y = 0;

            for (k = 1; k < AFFIRMANCE; k++)
            {
                sum_diff_x += obj[n].trace[k][0] - obj[n].trace[k - 1][0];
                sum_diff_y += obj[n].trace[k][1] - obj[n].trace[k - 1][1];
            }

            avg_x = (int32_t)(sum_diff_x / (float)(AFFIRMANCE - 1) + 0.5f);
            avg_y = (int32_t)(sum_diff_y / (float)(AFFIRMANCE - 1) + 0.5f);

            // 得到预测的位置
            obj[n].pred_pos.x0 = (int16_t)(obj[n].trace[AFFIRMANCE - 1][0] + avg_x);
            obj[n].pred_pos.y0 = (int16_t)(obj[n].trace[AFFIRMANCE - 1][1] + avg_y);
            obj[n].pred_pos.x1 = (int16_t)(obj[n].pred_pos.x0 + obj[n].obj_w - 1);
            obj[n].pred_pos.y1 = (int16_t)(obj[n].pred_pos.y0 + obj[n].obj_h - 1);

            // 越界处理
            if (obj[n].pred_pos.x0 < border_x0 || obj[n].pred_pos.x1 > border_x1
                || obj[n].pred_pos.y0 < border_y0 || obj[n].pred_pos.y1 > border_y1)
            {
                if (*snum == 0) // 确保每帧只返回一个车牌信息
                {
                    object_out_put(obj, n, sinfo, snum, 1, saveyuv, img_w, img_h, history_plate);
                }
            }
        }
    }

    return;
}

// 目标关联匹配
static void object_corespondence(ObjectForTracking *restrict obj, Rects rect_detect,
                                 HistoryPlate *history_plate,
                                 ObjectInfo *restrict sinfo, int32_t *snum,
                                 ObjectInfo *restrict oinfo, int32_t onum,
                                 uint8_t *restrict saveyuv, uint8_t *restrict yuv420,
                                 int32_t img_w, int32_t img_h)
{
    int32_t trk;
    int32_t k;
    int32_t o;
    int32_t thresh;
    int32_t rect_w, rect_h;
    int32_t smooth_w, smooth_h;
    int32_t center_x, center_y;
    const int32_t border_x0 = rect_detect.x0 + IMG_BORDER;
    const int32_t border_x1 = rect_detect.x1 - IMG_BORDER;
    const int32_t border_y0 = rect_detect.y0 + IMG_BORDER;
    const int32_t border_y1 = rect_detect.y1 - IMG_BORDER;
//  int32_t result = 0;
    uint8_t *restrict used_table = NULL;
    ObjectForTracking *restrict objc = NULL;
    ObjectInfo *restrict oinfoc = NULL;
    int32_t running_distance = 0;

    CHECKED_MALLOC(used_table, sizeof(uint8_t) * onum, CACHE_SIZE);
    memset(used_table, 0, onum);

    // 遍历已经跟踪的目标
    for (trk = 0; trk < TRACK_MAX; trk++)
    {
        objc = &obj[trk];

        // 为0时表示该标号trk中没有存放实际的目标
        if (objc->available == 0)
        {
            continue;
        }

        objc->disappear_cnt--;
        thresh = objc->obj_w;

        // 遍历当前帧检测得到的目标
        for (o = 0; o < onum; o++)
        {
            oinfoc = &oinfo[o];

            // 不为0时表示目标已经关联成功，遍历下一个目标
            if (used_table[o] != (uint8_t)0)
            {
                continue;
            }

            running_distance = plate_running_distance(objc->pred_pos.x0, objc->pred_pos.y0, oinfoc->pos.x0, oinfoc->pos.y0);

            // 关联成功
            if (all_characters_same_with_history_plate(objc, oinfoc) ||
                ((running_distance < thresh * 2) && same_character_numbers_with_history_plate(objc, oinfoc)))
            {
                // 用于统计车牌跟踪成功数量
                objc->available++;

                used_table[o] = (uint8_t)1;

                // 更新目标的外接矩形框
                oinfoc->trk_idx = (uint8_t)trk;

                // 当车牌在边界上时不更新车牌图片
                if (oinfoc->pos.x0 > border_x0 && oinfoc->pos.x1 < border_x1
                    && oinfoc->pos.y0 > border_y0 && oinfoc->pos.y1 < border_y1)
                {
                    int32_t quality_value = 0;

                    objc->end_pos = oinfoc->pos;    // 保存目标的实际位置

                    quality_value = image_quality(yuv420, img_w, /*img_h, */&oinfoc->pos);

                    if (quality_value > objc->quality_value)
                    {
#ifdef USE_EVM_648111111
                        uint32_t xfrID_out;
                        xfrID_out = DAT_Copy((void *)yuv420, objc->yuv, img_w * img_h * 3 / 2);
                        DAT_Wait(xfrID_out);
#else
                        memcpy(objc->yuv, yuv420, img_w * img_h * 3 / 2);
#endif
                        objc->quality_value = quality_value;
                        objc->real_pos = oinfoc->pos;   // 保存目标在返回图像中位置
                    }
                }

#if 1
                // 平滑更新
                rect_w = oinfoc->pos.x1 - oinfoc->pos.x0 + 1;
                rect_h = oinfoc->pos.y1 - oinfoc->pos.y0 + 1;
                center_x = oinfoc->pos.x0 + rect_w / 2;
                center_y = oinfoc->pos.y0 + rect_h / 2;
                smooth_w = (int32_t)(objc->obj_w * 0.8f + rect_w * 0.2f);
                smooth_h = (int32_t)(objc->obj_h * 0.8f + rect_h * 0.2f);
                objc->pred_pos.x0 = (int16_t)(center_x - smooth_w / 2);
                objc->pred_pos.x1 = (int16_t)(center_x + smooth_w / 2);
                objc->pred_pos.y0 = (int16_t)(center_y - smooth_h / 2);
                objc->pred_pos.y1 = (int16_t)(center_y + smooth_h / 2);
#else
                // 一次性更新
                objc->pred_pos = oinfoc->pos;
#endif

                objc->obj_w = objc->pred_pos.x1 - objc->pred_pos.x0 + 1;
                objc->obj_h = objc->pred_pos.y1 - objc->pred_pos.y0 + 1;

                // 更新轨迹
                if (objc->trace_num < AFFIRMANCE)
                {
                    objc->trace[objc->trace_num][0] = objc->pred_pos.x0;
                    objc->trace[objc->trace_num][1] = objc->pred_pos.y0;
                    objc->trace_num++;
                }
                else
                {
                    for (k = 1; k < AFFIRMANCE; k++)
                    {
                        objc->trace[k - 1][0] = objc->trace[k][0];
                        objc->trace[k - 1][1] = objc->trace[k][1];
                    }

                    objc->trace[AFFIRMANCE - 1][0] = objc->pred_pos.x0;
                    objc->trace[AFFIRMANCE - 1][1] = objc->pred_pos.y0;
                }

                // 更新目标存在的时间
                if (objc->time < AFFIRMANCE)
                {
                    objc->time++;
                }

                // 重置目标消失计数器
                objc->disappear_cnt = DISAPPEAR_MAX;

                // 注意：只要跟踪上之后每帧的目标都保存下来
//===========================================================================
                if (objc->info_idx < FIFO_LEN)
                {
                    car_plate_detect(yuv420, img_w, img_h, oinfoc);
                    objc->info[objc->info_idx] = oinfo[o];
                    objc->info_idx++;
                }
                else
                {
                    for (k = 1; k < FIFO_LEN; k++)
                    {
                        objc->info[k - 1] = objc->info[k];
                    }

                    objc->info[FIFO_LEN - 1] = oinfo[o];
                }

//===========================================================================
            }
        }
    }

    // 新目标 --- 如果该车牌在旧的跟踪列表中没有对应的记录，则作为新的记录插入到跟踪列表中
    for (o = 0; o < onum; o++)
    {
        oinfoc = &oinfo[o];

        // used_table[o] = 1就会提前终止循环，应该加在循环里面
        if (used_table[o])
        {
            continue;
        }

        // 缩小车牌跟踪开始的区域范围，避免车牌在边界上停留而导致出现新目标
        if (oinfoc->pos.x0 < (border_x0 + IMG_BORDER) || oinfoc->pos.x1 > (border_x1 - IMG_BORDER)
            || oinfoc->pos.y0 < (border_y0 + IMG_BORDER) || oinfoc->pos.y1 > (border_y1 - IMG_BORDER))
        {
            continue;
        }

        for (trk = 0; trk < TRACK_MAX; trk++)
        {
            objc = &obj[trk];

            if (objc->available == 0)  // 跟踪列表为空，可用，插入新目标
            {
                oinfoc->trk_idx = (uint8_t)trk;

                car_plate_detect(yuv420, img_w, img_h, oinfoc);
                objc->info[0] = oinfo[o];
                objc->info_idx = 1;
                objc->available = 1;
                objc->start_pos = oinfoc->pos;  // 初始化目标的起始位置
                objc->end_pos = oinfoc->pos;
                objc->real_pos = oinfoc->pos;
                objc->pred_pos = oinfoc->pos;
                objc->trace[0][0] = objc->pred_pos.x0;
                objc->trace[0][1] = objc->pred_pos.y0;
                objc->trace_num = 1;
                objc->obj_w = oinfoc->pos.x1 - oinfoc->pos.x0 + 1;
                objc->obj_h = oinfoc->pos.y1 - oinfoc->pos.y0 + 1;
                objc->time = 1;
                objc->disappear_cnt = DISAPPEAR_MAX;
                objc->already_output_flg = 0;
                objc->quality_value = image_quality(yuv420, img_w, /*img_h, */&oinfoc->pos);
#ifdef USE_EVM_648111111
                uint32_t xfrID_out;
                xfrID_out = DAT_Copy((void *)yuv420, objc->yuv, img_w * img_h * 3 / 2);
                DAT_Wait(xfrID_out);
#else
                memcpy(objc->yuv, yuv420, img_w * img_h * 3 / 2);
#endif
                break;
            }
        }
    }

    // 目标丢失的情况
    for (trk = 0; trk < TRACK_MAX; trk++)
    {
        objc = &obj[trk];

        if (objc->available)
        {
            int32_t stable_flg = 0;

            stable_flg = object_stable_or_not(obj, trk);

            // 目标丢失的计数器递减到0时认为目标彻底丢失，返回该车牌信息并在跟踪列表中删除该车牌的信息
            if (objc->disappear_cnt <= 0)
            {
                if (*snum == 0) // 确保每帧只返回一个车牌信息
                {
                    object_out_put(obj, trk, sinfo, snum, 1, saveyuv, img_w, img_h, history_plate);
                }
            }
            // 当车牌跟踪次数达到最大值或者识别信息稳定之后输出该车牌信息，但在跟踪列表中继续保留该车牌的信息
            else if ((objc->available >= APPEAR_MAX) || stable_flg)
            {
                if (*snum == 0) // 确保每帧只返回一个车牌信息
                {
                    object_out_put(obj, trk, sinfo, snum, 0, saveyuv, img_w, img_h, history_plate);
                }
            }
            // 目标丢失计数器没有递减到0，只更新目标轨迹信息
            else if (objc->disappear_cnt < DISAPPEAR_MAX)
            {
                if (objc->trace_num < AFFIRMANCE)
                {
                    objc->trace[objc->trace_num][0] = objc->pred_pos.x0;
                    objc->trace[objc->trace_num][1] = objc->pred_pos.y0;
                    objc->trace_num++;
                }
                else
                {
                    for (k = 1; k < AFFIRMANCE; k++)
                    {
                        objc->trace[k - 1][0] = objc->trace[k][0];
                        objc->trace[k - 1][1] = objc->trace[k][1];
                    }

                    objc->trace[AFFIRMANCE - 1][0] = objc->pred_pos.x0;
                    objc->trace[AFFIRMANCE - 1][1] = objc->pred_pos.y0;
                }
            }
        }
    }

    CHECKED_FREE(used_table, sizeof(uint8_t) * onum);

    return;
}

void lc_plate_tracking(Trk_handle trk_handle, uint8_t *restrict saveyuv, uint8_t *restrict yuv420,
                       ObjectInfo *restrict oinfo, int32_t onum,
                       ObjectInfo *restrict sinfo, int32_t *snum,
                       uint8_t *restrict save_flag, int32_t img_w, int32_t img_h)
{
    PlateTracking *restrict ptrk = (PlateTracking *)trk_handle;
    ObjectForTracking *restrict obj = ptrk->obj;
    Rects rect_detect = ptrk->rect_detect;
//    uint8_t *restrict chs_last_time = (uint8_t *)ptrk->history_plate;
    int32_t trk, o;

    *snum = 0;

    // 目标位置预测
    predict_new_position(obj, rect_detect, ptrk->history_plate, sinfo, snum, saveyuv, img_w, img_h);

    // 目标关联
    object_corespondence(obj, rect_detect, ptrk->history_plate, sinfo, snum, oinfo, onum, saveyuv, yuv420, img_w, img_h);

    // 每个历史车牌只存留1500帧，当帧数递减到0时，就清除该历史车牌
    for (o = 0; o < SAME_PLATE_NUMS; o++)
    {
        ptrk->history_plate[o].chs_last_time_frame--;

        if (ptrk->history_plate[o].chs_last_time_frame < 0)
        {
            memset(ptrk->history_plate[o].chs_last_time, 0, (uint32_t)CH_NUMS);
            ptrk->history_plate[o].chs_last_time_frame = 0;
        }
    }

    memset(save_flag, 0, sizeof(int8_t) * onum);

    // 确定真正的目标(显示)
    for (o = 0; o < onum; o++)
    {
        trk = (int32_t)oinfo[o].trk_idx;

        if (trk != INVALID_ID)
        {
            if (obj[trk].available
                && obj[trk].time >= AFFIRMANCE                // 去除干扰
                && obj[trk].disappear_cnt == DISAPPEAR_MAX)   // 不显示暂时消失的目标
            {
                save_flag[o] = (uint8_t)1;
            }
        }
    }

    return;
}
