#ifndef _LOCATION_INTERFACE_H_
#define _LOCATION_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"

typedef void* Loc_handle;   // ���ƶ�λ���

#define USE_DOUBLE_PLATE_RECOG  1   // ˫����ʶ�𿪹أ�1: ����˫����ʶ�� 0: �ر�˫����ʶ��
                                    // ����ͣ������������ʱ�ر�˫����ʶ��

#define RECT_MAX (80)       // ���Ƶ������Ŀ, ������Ƶ�ﳵ�ƺ�ѡ����϶�, ��˽����޵���Ϊ80

    typedef struct
    {
        int32_t cand_num;               // ���ƺ�ѡ�������Ŀ
        int32_t real_num;               // ʵ�ʼ�⳵�Ƶ���Ŀ
        Rects rect_cand[RECT_MAX];      // ���ƺ�ѡ����
        Rects rect_real[RECT_MAX];      // ʵ�ʼ��ĳ�������
        uint8_t plate_double_flag[RECT_MAX]; // ˫���Ʊ�־��0:������ 1:˫����
        Rects rect_double_up[RECT_MAX];      // ˫�����ϲ����򣬵�������Ч
        uint8_t ch_buf[RECT_MAX][24 * 32 * 10]; // �����ַ�ͼ��
        uint8_t ch_num[RECT_MAX];       //ʶ������ĳ����ַ���Ŀ
        uint8_t chs[RECT_MAX][7 + 1];   // ʶ������ĳ����ַ�
        uint8_t chs_trust[RECT_MAX][8]; // ���������ַ������Ŷȣ���7������Ϊ8��������������
        uint8_t trust[RECT_MAX];        // ���������ַ������Ŷ�
        uint8_t color[RECT_MAX];        // ������ɫ
        uint8_t brightness[RECT_MAX];   // ��������ֵ1 ~ 255
        uint8_t car_color[RECT_MAX];    // ������ɫ 0:��ɫ 1:���� 2:��ɫ 3:��ɫ 4:��ɫ 5:��ɫ 6:��ɫ 7:��ɫ 8:��ɫ 9:��ɫ 99:δ֪ 255:δʶ��
        uint8_t car_color_trust[RECT_MAX];  //������ɫ���Ŷ�
        uint8_t save_flag[RECT_MAX];    // ��ǰ֡���õ��ĳ����Ƿ���Ҫ�û�����(����Ƶ������ȡ����ʱ��ͬһ������ֻ��ȡһ��)
        // 0:���豣�� 1:��Ҫ����
    } LocationInfo;

// �������ƶ�λ���
    void lc_plate_location_create(Loc_handle *loc_handle, int32_t img_w, int32_t img_h);

// ͼ���С�ı�����������ڴ�
    void lc_plate_location_recreate(Loc_handle loc_handle, int32_t img_w, int32_t img_h);

// ���ƶ�λ
    void lc_plate_location(Loc_handle loc_handle, uint8_t *restrict yuv_img, LocationInfo *plate);

// �������ƶ�λ���
void lc_plate_location_destroy(Loc_handle loc_handle, int32_t img_w, int32_t img_h);


// ���ƶ�λĬ�ϲ�������
    void lc_plate_location_param_default(Loc_handle loc_handle);

// ���ƶ�λ��������
    void lc_plate_location_set_plate_width(Loc_handle loc_handle, int32_t plate_w_min, int32_t plate_w_max);
    void lc_plate_location_get_plate_width(Loc_handle loc_handle, int32_t *plate_w_min, int32_t *plate_w_max);

    void lc_plate_location_set_rect_detect(Loc_handle loc_handle, Rects rect_detect);
    void lc_plate_location_get_rect_detect(Loc_handle loc_handle, Rects *rect_detect);

    void lc_plate_location_get_video_flag(Loc_handle loc_handle, uint8_t *video_flag);
    void lc_plate_location_set_video_flag(Loc_handle loc_handle, uint8_t video_flag);

#ifdef __cplusplus
};
#endif

#endif  // _LOCATION_INTERFACE_H_

