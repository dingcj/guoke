#ifndef _LOCATION_H_
#define _LOCATION_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _TMS320C6X_
#define PLATE_RUN_BIT   // ��DSPƽ̨����λ������ʽ���д���
#endif

#include "../location_interface.h"

#define RECT_MAX    (80)    // ���Ƶ������Ŀ, ������Ƶ�ﳵ�ƺ�ѡ������Ŀ�϶࣬��˽����޵���Ϊ80

//#define PLATE_SAVE 0
//#define DBUG_WRITE 0
//#define SHOW_TRUST 1

    typedef struct
    {
        uint8_t *gray_img;          // ������ĻҶ�ͼ��
        uint8_t *edge_img;          // ��������ݶ�ͼ��
        uint8_t *bina_img;          // ������Ķ�ֵͼ��

//  int32_t video_flag;         // �Ƿ�����Ƶ��
        // �������
        Rects rect_detect;          // ���������
        int32_t img_w;              // ͼ����
        int32_t img_h;              // ͼ��߶�
        int32_t plate_w_min;        // Ŀ�����С���
        int32_t plate_w_max;        // Ŀ��������
        int32_t plate_h_min;        // Ŀ�����С�߶�
        int32_t plate_h_max;        // Ŀ������߶�

#ifdef USE_HAAR_DETECTION
        Cascade* cascade;           // ����������
        int32_t min_neighbors;      // ��С�����Ա��Ŀ(���ڹ��˸���)
        float scale;                // ͼ�����ŵĳ߶�����
#endif

        uint8_t haar_enable;        // ʹ��HAAR��λ
        uint8_t video_flag;         // ��Ƶ����ͼƬ��ʶ

#if DEBUG_RUNS
        uint8_t runs_image[2000 * 1300];
#endif
    } PlateLocation;


#ifdef __cplusplus
};
#endif

#endif  // _LOCATION_H_
