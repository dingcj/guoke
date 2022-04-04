#ifndef _LP_TRACKING_H_
#define _LP_TRACKING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"


#define TRACK_MAX     (20)      // 目标轨迹数量最大值
#define INVALID_ID    (255)

#if 1
//==============================================================
// 用于车管所停车场等车辆停留比较长的场合
#define APPEAR_MAX    (150)     // 目标跟踪帧数最大值
#define DISAPPEAR_MAX (50)      // 确认目标丢失的连续帧数
#define AFFIRMANCE    (3)       // 确认为感兴趣目标的帧数
//==============================================================
#else
//==============================================================
// 用于道路监控等车辆停留比较短的场合
#define APPEAR_MAX    (15)      // 目标跟踪帧数最大值
#define DISAPPEAR_MAX (8)       // 确认目标丢失的连续帧数
#define AFFIRMANCE    (3)       // 确认为感兴趣目标的帧数
//==============================================================
#endif

#define FIFO_LEN      (100)     // 目标信息FIFO
#define IMG_BORDER    (16)      // 图像边界

#define SAME_PLATE_NUMS  (5)    // 保存几个已经输出的车牌信息以避免相同车牌重复输出
#define HISTORY_PLATE_FRAME (1500)  //历史车牌存留的帧数,1500帧约为1分钟

    typedef void* Trk_handle;   // 车牌跟踪句柄

    typedef struct
    {
        Rects pos;           // 该目标实际检测的位置
        uint8_t trk_idx;     // 该目标在跟踪列表中的序号(0, 1, 2, .....TRACK_MAX-1)
        uint8_t ch_num;      // 该目标的车牌字符数目
        uint8_t chs[CH_NUMS];
        uint8_t chs_trust[CH_NUMS];
        uint8_t trust;
        uint8_t brightness;
        uint8_t car_color;
        uint8_t car_color_trust;
        uint8_t color;
        uint8_t directions;
        uint8_t plate_flag;      // 车牌标志: 0:检测为非车牌 1:检测为车牌
    } ObjectInfo;

    typedef struct
    {
        int32_t available;
        int32_t obj_w;
        int32_t obj_h;
        int32_t trace[AFFIRMANCE][2];
        int32_t trace_num;
        ObjectInfo info[FIFO_LEN];
        int32_t info_idx;
        int32_t time;
        int32_t disappear_cnt;
        int32_t already_output_flg; // 该目标是否已经输出的标志，为1时表示车牌在前面的跟踪过程中识别信息很稳定，车牌图片已经输出过
        Rects pred_pos;             // 该目标预测得到的位置
        Rects real_pos;             // 该目标被保存图像中的位置（当返回车牌信息时，这个值保存的是车牌在返回图片中的位置）
        Rects start_pos;            // 该目标的起始位置
        Rects end_pos;              // 该目标在检测图像中的最新位置
        uint8_t *yuv;               // 该目标消失之前对应的yuv数据
        int32_t quality_value;
    } ObjectForTracking;

    typedef struct
    {
        uint8_t chs_last_time[CH_NUMS];
        int32_t chs_last_time_frame;
    } HistoryPlate;

    typedef struct
    {
        ObjectForTracking obj[TRACK_MAX];
        Rects rect_detect;
        HistoryPlate    history_plate[SAME_PLATE_NUMS];  // 历史车牌，总共保存5个，分别保存3000帧
    } PlateTracking;

// 创建车牌跟踪句柄
    void lc_plate_tracking_create(Trk_handle *trk_handle, int32_t img_w, int32_t img_h);

// 图像大小改变重新申请车牌跟踪使用图像内存
    void lc_plate_tracking_recreate(Trk_handle trk_handle, int32_t img_w, int32_t img_h);

// 撤销车牌跟踪句柄
void lc_plate_tracking_destroy(Trk_handle trk_handle, int32_t img_w, int32_t img_h);

// 车牌跟踪
    void lc_plate_tracking(Trk_handle trk_handle, uint8_t *restrict saveyuv, uint8_t *restrict yuv420,
                           ObjectInfo *restrict oinfo, int32_t onum,
                           ObjectInfo *restrict sinfo, int32_t *snum,
                           uint8_t *restrict save_flag, int32_t img_w, int32_t img_h);

// 设置车牌识别的检测区域
    void lc_plate_tracking_set_rect_detect(Trk_handle trk_handle, Rects rect_detect);

#ifdef __cplusplus
};
#endif

#endif

