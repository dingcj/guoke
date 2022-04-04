#ifndef _LP_TRACKING_H_
#define _LP_TRACKING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"


#define TRACK_MAX     (20)      // Ŀ��켣�������ֵ
#define INVALID_ID    (255)

#if 1
//==============================================================
// ���ڳ�����ͣ�����ȳ���ͣ���Ƚϳ��ĳ���
#define APPEAR_MAX    (150)     // Ŀ�����֡�����ֵ
#define DISAPPEAR_MAX (50)      // ȷ��Ŀ�궪ʧ������֡��
#define AFFIRMANCE    (3)       // ȷ��Ϊ����ȤĿ���֡��
//==============================================================
#else
//==============================================================
// ���ڵ�·��صȳ���ͣ���Ƚ϶̵ĳ���
#define APPEAR_MAX    (15)      // Ŀ�����֡�����ֵ
#define DISAPPEAR_MAX (8)       // ȷ��Ŀ�궪ʧ������֡��
#define AFFIRMANCE    (3)       // ȷ��Ϊ����ȤĿ���֡��
//==============================================================
#endif

#define FIFO_LEN      (100)     // Ŀ����ϢFIFO
#define IMG_BORDER    (16)      // ͼ��߽�

#define SAME_PLATE_NUMS  (5)    // ���漸���Ѿ�����ĳ�����Ϣ�Ա�����ͬ�����ظ����
#define HISTORY_PLATE_FRAME (1500)  //��ʷ���ƴ�����֡��,1500֡ԼΪ1����

    typedef void* Trk_handle;   // ���Ƹ��پ��

    typedef struct
    {
        Rects pos;           // ��Ŀ��ʵ�ʼ���λ��
        uint8_t trk_idx;     // ��Ŀ���ڸ����б��е����(0, 1, 2, .....TRACK_MAX-1)
        uint8_t ch_num;      // ��Ŀ��ĳ����ַ���Ŀ
        uint8_t chs[CH_NUMS];
        uint8_t chs_trust[CH_NUMS];
        uint8_t trust;
        uint8_t brightness;
        uint8_t car_color;
        uint8_t car_color_trust;
        uint8_t color;
        uint8_t directions;
        uint8_t plate_flag;      // ���Ʊ�־: 0:���Ϊ�ǳ��� 1:���Ϊ����
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
        int32_t already_output_flg; // ��Ŀ���Ƿ��Ѿ�����ı�־��Ϊ1ʱ��ʾ������ǰ��ĸ��ٹ�����ʶ����Ϣ���ȶ�������ͼƬ�Ѿ������
        Rects pred_pos;             // ��Ŀ��Ԥ��õ���λ��
        Rects real_pos;             // ��Ŀ�걻����ͼ���е�λ�ã������س�����Ϣʱ�����ֵ������ǳ����ڷ���ͼƬ�е�λ�ã�
        Rects start_pos;            // ��Ŀ�����ʼλ��
        Rects end_pos;              // ��Ŀ���ڼ��ͼ���е�����λ��
        uint8_t *yuv;               // ��Ŀ����ʧ֮ǰ��Ӧ��yuv����
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
        HistoryPlate    history_plate[SAME_PLATE_NUMS];  // ��ʷ���ƣ��ܹ�����5�����ֱ𱣴�3000֡
    } PlateTracking;

// �������Ƹ��پ��
    void lc_plate_tracking_create(Trk_handle *trk_handle, int32_t img_w, int32_t img_h);

// ͼ���С�ı��������복�Ƹ���ʹ��ͼ���ڴ�
    void lc_plate_tracking_recreate(Trk_handle trk_handle, int32_t img_w, int32_t img_h);

// �������Ƹ��پ��
void lc_plate_tracking_destroy(Trk_handle trk_handle, int32_t img_w, int32_t img_h);

// ���Ƹ���
    void lc_plate_tracking(Trk_handle trk_handle, uint8_t *restrict saveyuv, uint8_t *restrict yuv420,
                           ObjectInfo *restrict oinfo, int32_t onum,
                           ObjectInfo *restrict sinfo, int32_t *snum,
                           uint8_t *restrict save_flag, int32_t img_w, int32_t img_h);

// ���ó���ʶ��ļ������
    void lc_plate_tracking_set_rect_detect(Trk_handle trk_handle, Rects rect_detect);

#ifdef __cplusplus
};
#endif

#endif

