#ifndef _RECOGNITION_INTERFACE_H_
#define _RECOGNITION_INTERFACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "portab.h"

    typedef void* Reg_handle;   // �����ַ�ʶ����

#define LABEL_START_OF_CHINESE (100u)    // ʡ�ݺ��ֵı���Ǵ�100��ʼ��

#define CH_XUE   (143u)    // ѧ
#define CH_JING  (144u)    // ��
#define CH_LING  (145u)    // ��
#define CH_GANG  (146u)    // ��
#define CH_AO    (147u)    // ��
#define CH_TAI   (148u)    // ̨
#define CH_GUA   (149u)    // ��
#define CH_SHIY  (150u)    // ��
#define CH_CHAO  (151u)    // ��
#define CH_SHIG  (152u)    // ʹ

// ���������ַ�ʶ����
    void lc_plate_recognition_create(Reg_handle *reg_handle);

// �����ַ�ʶ��
    void lc_plate_recognition_single(Reg_handle reg_handle, uint8_t *restrict pgray_img,
                                     int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t *ch_num,
                                     uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *restrict chs_trust,
                                     uint8_t *restrict brightness,
                                     uint8_t *restrict plate_trust, uint8_t trust_thresh,
                                     uint8_t *bkgd_color, uint8_t *color_new,
                                     uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                     Rects *restrict rect_old, Rects *restrict rect_new);
    void lc_plate_recognition_double(Reg_handle reg_handle, uint8_t *restrict pgray_img, int32_t plate_h_up,
                                     int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t *ch_num,
                                     uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *restrict chs_trust,
                                     uint8_t *restrict brightness,
                                     uint8_t *restrict plate_trust, uint8_t trust_thresh,
                                     uint8_t *bkgd_color, uint8_t *color_new,
                                     uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                     Rects *restrict rect_old, Rects *restrict rect_new);
//���Ƶ�ɫ�б���
//����ֵ�� 0������ 1���Ƶ� 2���׵� 3���ڵ�
    uint8_t find_plate_color(uint8_t *yuv_img, int32_t img_w, int32_t img_h);

// ���������ַ�ʶ����
    void lc_plate_recognition_destroy(Reg_handle reg_handle);


// ���ú���(ʡ��)ʶ����Ĥ(����ַ�ʶ��ľ���) 0: �����ú���ʶ�� 1: ���θú���ʶ��
    void lc_plate_recognition_set_chinese_mask(Reg_handle reg_handle, uint8_t *chinese_mask);
    void lc_plate_recognition_get_chinese_mask(Reg_handle reg_handle, uint8_t *chinese_mask);

// ����Ĭ�ϵĺ���(ʡ��)�������ֵ����ŶȲ���ʱ(��̫�϶�ʱ)������Ĭ�Ϻ���
    void lc_plate_recognition_set_chinese_default(Reg_handle reg_handle, uint8_t chinese_default);
    void lc_plate_recognition_get_chinese_default(Reg_handle reg_handle, uint8_t *chinese_default);

// ���ú��ֵ����Ŷ���ֵ�������ڸ���ֵʱ����Ĭ�ϵ�ʡ�ݺ���
    void lc_plate_recognition_set_chinese_trust_thresh(Reg_handle reg_handle, uint8_t trust_thresh);
    void lc_plate_recognition_get_chinese_trust_thresh(Reg_handle reg_handle, uint8_t *trust_thresh);

#ifdef __cplusplus
};
#endif

#endif  // _RECOGNITION_INTERFACE_H_

