#ifndef _PLATE_H_
#define _PLATE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"
#include "../plate_recognition_src/lsvm_model.h"
#include "../recognition_interface.h"

    typedef struct
    {
        int x0;
        int x1;
        int len;
    } Sect;

#define CH_MAX   (20)             // �ַ��������Ŀ
#define STD_W    (24)             // ��׼ѵ���ַ��Ŀ��
#define STD_H    (32)             // ��׼ѵ���ַ��ĸ߶�
#define STD_AREA (STD_W * STD_H)  // ��׼ѵ���ַ������

    typedef struct
    {
        SvmModel *model_numbers;  // ������ĸģ��
        SvmModel *model_chinese;  // ����ģ��
        SvmModel *model_jun_wjs;  // ���ƺ���ģ�ͣ�����WJ��
        SvmModel *model_xuejing;  // ѧ�������졢�ۡ��ġ�̨���ҡ��ԡ�����ʹ
        SvmModel *model_english;

        SvmModel *model_0_v00_D;  // �����ַ�0��D���б�ģ��
        SvmModel *model_0_v00_Q;  // �����ַ�0��Q���б�ģ��
        SvmModel *model_2_v00_Z;  // �����ַ�2��Z���б�ģ��
        SvmModel *model_5_v00_S;  // �����ַ�5��S���б�ģ��
        SvmModel *model_8_v00_B;  // �����ַ�8��B���б�ģ��
        SvmModel *model_7_v00_T;  // �����ַ�7��T���б�ģ��

        SvmModel *model_0_v01_D;  // �����ַ�0��D���б�ģ��
        SvmModel *model_0_v01_Q;  // �����ַ�0��Q���б�ģ��
        SvmModel *model_2_v01_Z;  // �����ַ�2��Z���б�ģ��
        SvmModel *model_5_v01_S;  // �����ַ�5��S���б�ģ��
        SvmModel *model_8_v01_B;  // �����ַ�8��B���б�ģ��
        SvmModel *model_7_v01_T;  // �����ַ�7��T���б�ģ��

        feature_type *feat0;
        feature_type *feat1;

        uint8_t *std_gray;
        uint8_t *std_bina;
        uint8_t *ch_buff0;
        uint8_t *ch_buff1;

        Rects *ch_rect0;
        Rects *ch_rect1;
    } Recognition;


#ifdef WIN32_DEBUG
    char savename[1000 + 1];
#endif

#ifdef __cplusplus
};
#endif

#endif
