#ifndef _LC_PLATE_PARAMS_H_
#define _LC_PLATE_PARAMS_H_

#ifdef PALTE_THREADS
#include <windows.h>
#include <process.h>
#endif

#include "portab.h"
#include "location_interface.h"
#include "recognition_interface.h"
#include "plate_tracking_src/plate_tracking.h"

#define RECTS_NUM   (5)           // ÿ֡ͼ���м�⳵�Ƶ������Ŀ
#define NUM_FRAMES  (25)

#define PLATE_MAX_WIDTH  400      // ��λ�ĳ���СͼƬ�Ŀ��
#define PLATE_MAX_HEIGHT 160      // ��λ�ĳ���СͼƬ�ĸ߶�

/********************************************************************
 �����ַ�chs�ı������:
 ���chs>=100˵���Ǻ��֣�����chs=136��Ӧ����"��"��chs=150��Ӧ����"��"
 ���chs<100˵�������ֻ���ĸ��chs��ŵ������ֺʹ�д��ĸ��Ӧ��ASCII��
********************************************************************/
typedef struct
{
    int32_t plate_num;              // ��ǰ֡���õ��ĳ�����Ŀ
    Rects   plate_list[RECTS_NUM];  // ��ǰ֡���õ��ĳ�����ԭʼͼ���е�λ��
    Rects   plate_rect[RECTS_NUM];  // ������ץ�ĵĳ���ͼƬ�е�λ��
    Rects   car_window_rect[RECTS_NUM]; //������ץ��ͼƬ�е�λ��
    uint8_t *buf_flage;             // ץ�ĵĳ���ͼƬ0: ���һ��ͼƬ 1: ��������һ��ͼƬ

    uint8_t chs[RECTS_NUM][CH_NUMS];    // ʶ������ĳ����ַ����루Ϊ֧�����ƣ����ַ����޸�Ϊ8�����һ�����ַ�����������
    int8_t  character[RECTS_NUM][2 + 5 + 2 + 1]; // ʶ������ĳ����ַ����ַ�����ע�⣺���һ�������ַ�Ҳ�п����Ǻ��֣�����ռ2���ַ������һ�����ַ�����������
    uint8_t chs_trust[RECTS_NUM][CH_NUMS];  // ���������ַ������Ŷ�
    uint8_t trust[RECTS_NUM];       // �����ַ������Ŷ�
    uint8_t color[RECTS_NUM];       // ������ɫ 0:���� 1:�Ƶ� 2:�׵ף����� 3:�׵ף����� 4:���� 10:����Դ����99:δ֪
    uint8_t type[RECTS_NUM];        // �������� 1:������������ 2:С���������� 3:����Դ���� 16:������������ 23:������������ 99:δ֪
    uint8_t ch_num[RECTS_NUM];      // ȷ�������ַ���Ŀ
    uint8_t brightness[RECTS_NUM];  // ��������ֵ1 ~ 255

    int32_t count[RECTS_NUM];       // ���Ƽ���ֵ��ͬһ��������Ƶ�г��ֵĴ���
    uint8_t directions[RECTS_NUM];  // �����˶����� 0:δ֪���� 1:���ƴ��������˶� 2:���ƴ��������˶�
    uint8_t speeds[RECTS_NUM][2];   // �����˶��ٶ�
    uint8_t car_color[RECTS_NUM];   // ������ɫ 0:��ɫ 1:���� 2:��ɫ 3:��ɫ 4:��ɫ 5:��ɫ 6:��ɫ 7:��ɫ 8:��ɫ 9:��ɫ 99:δ֪ 255:δʶ��
    uint8_t car_color_trust[RECT_MAX];  //������ɫ���Ŷ�
    int32_t save_num;               // �û���Ҫ����ĳ��ƺ���Ŀ
    uint8_t *pic_buff;              // ץ�ĵĳ���ͼƬ(Ŀǰ��JPG��ʽ)����ʼ��ַ
    uint8_t *pic_small[RECTS_NUM];  // ץ�ĵĳ���ͼƬ�ж�λ�ĳ���СͼƬ(Ŀǰ��JPG��ʽ)����ʼ��ַ
    uint8_t *pic_window[RECTS_NUM]; // ץ�ĵĳ���ͼƬ�ж�λ�ĳ�������СͼƬ(Ŀǰ��JPG��ʽ)����ʼ��ַ
    int32_t pbuff_lens;             // ץ�ĵĳ���ͼƬ�Ĵ�С(��λ���ֽ�)
    int32_t psmall_lens[RECTS_NUM]; // ץ�ĵĳ���ͼƬ�ж�λ�ĳ���СͼƬ�Ĵ�С(��λ���ֽ�)
    int32_t pwindow_lens[RECTS_NUM];// ץ�ĵĳ���ͼƬ�ж�λ�ĳ�������СͼƬ�Ĵ�С(��λ���ֽ�)
} PlateInfo;

typedef struct
{
    int32_t       img_w;                     // ͼ����
    int32_t       img_h;                     // ͼ��߶�

    int32_t       video_format;              // ��Ƶ��ʽ
    uint8_t       position_flag;             // �Ƿ����ó��ƺ�ɫ���ο� 0: ������(Ĭ��) 1: ����
    uint8_t       car_color_enable;          // �Ƿ����ó�����ɫʶ�� 0:��ʶ��(Ĭ��) 1:ʶ��

    int32_t       location_enable;
    int32_t       recognition_enable;
    int32_t       tracking_enable;
    uint8_t       video_flag;                 // ��Ƶ(��ͼƬ)��ʶ
    uint8_t       trust_thresh;               // ����ʶ�����Ŷȵ���ֵ

    Loc_handle    loc;                        // ���ƶ�λ���
    Reg_handle    reg;                        // �����ַ�ʶ����
    Trk_handle    trk;                        // ���Ƹ��پ��
//  Rects         *plate_pos;                 // �û���Ҫ�����ͼƬ�г��Ƶ�λ��
    uint8_t       *srcyuv420;                 // �û��������ͼƬ(yuv420��ʽ������ͼ��Ҫת����yuv420��ʽ)
//  uint8_t       *pic_buff;                  // �û���Ҫ�����ͼƬ(ץ�ĵĳ���ͼƬ��Ŀǰ��JPG��ʽ)
    int32_t       img_quality;                // ץ�ĳ���ͼƬ(JPG)��ѹ��������1 ~ 100��ֵԽ������Խ�á�ͼƬ���ļ�ҲԽ��

#ifdef PALTE_THREADS
    unsigned long hThread;                    // ��Ƶ�����߳̾��
    uint8_t       bThread;                    // ��Ƶ�����߳����б�־
    int32_t       index_save;                 // ��������Ƶ֡�ı�������
    int32_t       index_proce;                // ��������Ƶ֡�Ĵ�������
    HANDLE        handle_wait;                // ���ڵȴ���дframe_flag��־
    HANDLE        handle_frame;               // ����֪ͨ��Ƶ�����߳�����Ƶ��Ҫ����
    uint8_t       frame_flag[NUM_FRAMES];     // ���ڱ�־�����е���Ƶ֡�Ƿ���
    uint8_t       *frame420[NUM_FRAMES];      // ���ڻ�����Ƶ֡
    PLATECALLBACK plateCallBack;              // �û��ص�����
    void          *context;                   // �û�������ָ��
    HANDLE        handle_param;               // ���ڻ��淽ʽ�еȴ����ò���
#endif

    int32_t       proce_type;                 // 0.�޻���ͼƬ����1.�޻�����Ƶ����2.�л���ͼƬ����3.�л�����Ƶ����
    PlateInfo     *pinfo;                     // ʶ������Ϣ
    LocationInfo  *linfo;                     // ���ƶ�λ��Ϣ

    uint8_t       OSD_flag;                   // �Ƿ���ͼƬ�ϵ��ӳ����ַ�
    int32_t       OSD_pos_x;                  // �����ַ�����λ��
    int32_t       OSD_pos_y;                  // �����ַ�����λ��
    double        OSD_scale;                  // ���ӳ������ű���
    uint8_t       deskew_flag;                // �Ƿ�Գ�������бУ��
    uint8_t       car_window_enable;          // �Ƿ�����������ʶ�� 0: ��ʶ�� 1: ʶ��
} PlateAnalysis;


#endif  // _LC_PLATE_PARAMS_H_

