#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../portab.h"
#include "../malloc_aligned.h"
#include "plate_recognition.h"
#include "plate_segmentation.h"
#include "plate_segmentation_double.h"
#include "plate_color.h"
#include "lsvm.h"
#include "feature.h"
//#include "similar_classify.h"
#include "../common/threshold.h"
#include "../common/image.h"
#include "lsvm_param_chinese.h"
#include "lsvm_param_jun_wjs.h"
#include "lsvm_param_english.h"
#include "lsvm_param_numbers.h"
#include "lsvm_param_xuejing.h"
#include "lsvm_param_ambiguous0.h"
#include "lsvm_param_ambiguous1.h"
#include "../time_test.h"
#include "../debug_show_image.h"

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#define FEATURE_NUMS (896)  // �����ַ�����������

static void similar_character_classify(Recognition *restrict reg, uint8_t *restrict ch_buf,
                                       uint8_t *restrict chs, uint8_t *restrict chs_trust, int32_t ch_num);
//static void set_character_as_unkown_while_no_convinced(uint8_t *restrict chs, uint8_t *restrict chs_trust);

#ifdef _TMS320C6X_
#pragma CODE_SECTION(lc_plate_recognition_create, ".text:lc_plate_recognition_create")
#pragma CODE_SECTION(lc_plate_recognition_destroy, ".text:lc_plate_recognition_destroy")
#pragma CODE_SECTION(lc_plate_recognition_get_chinese_mask, ".text:lc_plate_recognition_get_chinese_mask")
#pragma CODE_SECTION(lc_plate_recognition_set_chinese_mask, ".text:lc_plate_recognition_set_chinese_mask")
#pragma CODE_SECTION(lc_plate_recognition_get_chinese_default, ".text:lc_plate_recognition_get_chinese_default")
#pragma CODE_SECTION(lc_plate_recognition_set_chinese_default, ".text:lc_plate_recognition_set_chinese_default")
#pragma CODE_SECTION(lc_plate_recognition_get_chinese_trust_thresh, ".text:lc_plate_recognition_get_chinese_trust_thresh")
#pragma CODE_SECTION(lc_plate_recognition_set_chinese_trust_thresh, ".text:lc_plate_recognition_set_chinese_trust_thresh")
#pragma CODE_SECTION(lc_plate_recognition_single, ".text:lc_plate_recognition_single")
#pragma CODE_SECTION(lc_plate_recognition_double, ".text:lc_plate_recognition_double")
#pragma CODE_SECTION(gray_image_reverse_brightness, ".text:gray_image_reverse_brightness")
#pragma CODE_SECTION(yellow_plate_rectifying, ".text:yellow_plate_rectifying")
#pragma CODE_SECTION(police_plate_rectifying, ".text:police_plate_rectifying")
#pragma CODE_SECTION(determine_plate_background_color_yellow_or_white, ".text:determine_plate_background_color_yellow_or_white")
#pragma CODE_SECTION(jun_wjs_plate_rectifying, ".text:jun_wjs_plate_rectifying")
#pragma CODE_SECTION(get_plate_brightness, ".text:get_plate_brightness")
#pragma CODE_SECTION(similar_character_classify, ".text:similar_character_classify")
#endif

// ���������ַ�ʶ����
void lc_plate_recognition_create(Reg_handle *reg_handle)
{
    Recognition *reg = (Recognition *)reg_handle;

    // ���һ���ļ����ľ�̬��������һ���ļ������ã���Ӧ��ֻ�ڸú����ڲ������˱���

    // ����ʶ�����Ĥ(����ַ�ʶ��ľ���) 1: ���θú���ʶ�� 0: �����ú���ʶ��
#if 0
    uint8_t g_mask_chinese_default[31] =
    {
        //  �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� ԥ �� �� �� �� �� �� �� ³ ��
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1
    };
#else
    uint8_t g_mask_chinese_default[31] =
    {
        //  �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� ԥ �� �� �� �� �� �� �� ³ ��
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
#endif
    uint8_t g_mask_numbers_default[] =
    {
        //  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  G  H  ѧ J  K  L  M  N  �� P  Q  R  S  T  U  V  W  X  Y  Z 255
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t g_mask_jun_wjs_default[12] =
    {
        //  �� �� �� �� �� �� �� �� �� �� �� WJ
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t g_mask_xuejing_default[] =
    {
        //  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z  ѧ �� �� �� �� ̨ �� �� �� ʹ
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t g_mask_english_default[] =
    {
        //  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t g_mask_X_ver_Y_default[3] =
    {
        0, 0, 0
    };

    CHECKED_MALLOC(reg, sizeof(Recognition), CACHE_SIZE);
    memset(reg, 0, sizeof(Recognition));

    CHECKED_MALLOC(reg->model_english, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_numbers, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_chinese, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_jun_wjs, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_xuejing, sizeof(SvmModel), CACHE_SIZE);

    CHECKED_MALLOC(reg->model_0_v00_D, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_0_v00_Q, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_2_v00_Z, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_5_v00_S, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_8_v00_B, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_7_v00_T, sizeof(SvmModel), CACHE_SIZE);

    CHECKED_MALLOC(reg->model_0_v01_D, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_0_v01_Q, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_2_v01_Z, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_5_v01_S, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_8_v01_B, sizeof(SvmModel), CACHE_SIZE);
    CHECKED_MALLOC(reg->model_7_v01_T, sizeof(SvmModel), CACHE_SIZE);

    load_svm_model(reg->model_english, g_svm_english_label, g_mask_english_default);
    load_svm_model(reg->model_numbers, g_svm_numbers_label, g_mask_numbers_default);
    load_svm_model(reg->model_chinese, g_svm_chinese_label, g_mask_chinese_default);
    load_svm_model(reg->model_jun_wjs, g_svm_jun_wjs_label, g_mask_jun_wjs_default);
    load_svm_model(reg->model_xuejing, g_svm_xuejing_label, g_mask_xuejing_default);

    load_svm_model(reg->model_0_v00_D, g_svm_0_v00_D_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_0_v00_Q, g_svm_0_v00_Q_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_2_v00_Z, g_svm_2_v00_Z_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_5_v00_S, g_svm_5_v00_S_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_8_v00_B, g_svm_8_v00_B_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_7_v00_T, g_svm_7_v00_T_label, g_mask_X_ver_Y_default);

    load_svm_model(reg->model_0_v01_D, g_svm_0_v01_D_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_0_v01_Q, g_svm_0_v01_Q_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_2_v01_Z, g_svm_2_v01_Z_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_5_v01_S, g_svm_5_v01_S_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_8_v01_B, g_svm_8_v01_B_label, g_mask_X_ver_Y_default);
    load_svm_model(reg->model_7_v01_T, g_svm_7_v01_T_label, g_mask_X_ver_Y_default);

    reg->model_chinese->rho = (svm_type*)g_svm_chinese_rho;
    reg->model_numbers->rho = (svm_type*)g_svm_numbers_rho;
    reg->model_jun_wjs->rho = (svm_type*)g_svm_jun_wjs_rho;
    reg->model_xuejing->rho = (svm_type*)g_svm_xuejing_rho;
    reg->model_english->rho = (svm_type*)g_svm_english_rho;

    reg->model_0_v00_D->rho = (svm_type*)g_svm_0_v00_D_rho;
    reg->model_0_v00_Q->rho = (svm_type*)g_svm_0_v00_Q_rho;
    reg->model_2_v00_Z->rho = (svm_type*)g_svm_2_v00_Z_rho;
    reg->model_5_v00_S->rho = (svm_type*)g_svm_5_v00_S_rho;
    reg->model_8_v00_B->rho = (svm_type*)g_svm_8_v00_B_rho;
    reg->model_7_v00_T->rho = (svm_type*)g_svm_7_v00_T_rho;

    reg->model_0_v01_D->rho = (svm_type*)g_svm_0_v01_D_rho;
    reg->model_0_v01_Q->rho = (svm_type*)g_svm_0_v01_Q_rho;
    reg->model_2_v01_Z->rho = (svm_type*)g_svm_2_v01_Z_rho;
    reg->model_5_v01_S->rho = (svm_type*)g_svm_5_v01_S_rho;
    reg->model_8_v01_B->rho = (svm_type*)g_svm_8_v01_B_rho;
    reg->model_7_v01_T->rho = (svm_type*)g_svm_7_v01_T_rho;

    reg->model_chinese->svs = (svm_type*)g_svm_chinese_svs;
    reg->model_numbers->svs = (svm_type*)g_svm_numbers_svs;
    reg->model_jun_wjs->svs = (svm_type*)g_svm_jun_wjs_svs;
    reg->model_xuejing->svs = (svm_type*)g_svm_xuejing_svs;
    reg->model_english->svs = (svm_type*)g_svm_english_svs;

    reg->model_0_v00_D->svs = (svm_type*)g_svm_0_v00_D_svs;
    reg->model_0_v00_Q->svs = (svm_type*)g_svm_0_v00_Q_svs;
    reg->model_2_v00_Z->svs = (svm_type*)g_svm_2_v00_Z_svs;
    reg->model_5_v00_S->svs = (svm_type*)g_svm_5_v00_S_svs;
    reg->model_8_v00_B->svs = (svm_type*)g_svm_8_v00_B_svs;
    reg->model_7_v00_T->svs = (svm_type*)g_svm_7_v00_T_svs;

    reg->model_0_v01_D->svs = (svm_type*)g_svm_0_v01_D_svs;
    reg->model_0_v01_Q->svs = (svm_type*)g_svm_0_v01_Q_svs;
    reg->model_2_v01_Z->svs = (svm_type*)g_svm_2_v01_Z_svs;
    reg->model_5_v01_S->svs = (svm_type*)g_svm_5_v01_S_svs;
    reg->model_8_v01_B->svs = (svm_type*)g_svm_8_v01_B_svs;
    reg->model_7_v01_T->svs = (svm_type*)g_svm_7_v01_T_svs;

    reg->model_chinese->probAB = (svm_type*)g_svm_chinese_probAB;
    reg->model_numbers->probAB = (svm_type*)g_svm_numbers_probAB;
    reg->model_jun_wjs->probAB = (svm_type*)g_svm_jun_wjs_probAB;
    reg->model_xuejing->probAB = (svm_type*)g_svm_xuejing_probAB;
    reg->model_english->probAB = (svm_type*)g_svm_english_probAB;

    reg->model_0_v00_D->probAB = (svm_type*)g_svm_0_v00_D_probAB;
    reg->model_0_v00_Q->probAB = (svm_type*)g_svm_0_v00_Q_probAB;
    reg->model_2_v00_Z->probAB = (svm_type*)g_svm_2_v00_Z_probAB;
    reg->model_5_v00_S->probAB = (svm_type*)g_svm_5_v00_S_probAB;
    reg->model_8_v00_B->probAB = (svm_type*)g_svm_8_v00_B_probAB;
    reg->model_7_v00_T->probAB = (svm_type*)g_svm_7_v00_T_probAB;

    reg->model_0_v01_D->probAB = (svm_type*)g_svm_0_v01_D_probAB;
    reg->model_0_v01_Q->probAB = (svm_type*)g_svm_0_v01_Q_probAB;
    reg->model_2_v01_Z->probAB = (svm_type*)g_svm_2_v01_Z_probAB;
    reg->model_5_v01_S->probAB = (svm_type*)g_svm_5_v01_S_probAB;
    reg->model_8_v01_B->probAB = (svm_type*)g_svm_8_v01_B_probAB;
    reg->model_7_v01_T->probAB = (svm_type*)g_svm_7_v01_T_probAB;

#ifdef _TMS320C6X_
    reg->feat0 = (feature_type *)MEM_alloc(IRAM, sizeof(feature_type) * FEATURE_NUMS, 128);
    reg->feat1 = (feature_type *)MEM_alloc(IRAM, sizeof(feature_type) * FEATURE_NUMS, 128);
    reg->std_gray = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * STD_AREA, 128);
    reg->std_bina = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * STD_AREA, 128);
    reg->ch_buff0 = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * STD_AREA * 8, 128);
    reg->ch_buff1 = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * STD_AREA * 8, 128);
    reg->ch_rect0 = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * CH_MAX, 128);
    reg->ch_rect1 = (Rects *)MEM_alloc(IRAM, sizeof(Rects) * CH_MAX, 128);
#else
    CHECKED_MALLOC(reg->feat0, sizeof(feature_type) * FEATURE_NUMS, CACHE_SIZE);
    CHECKED_MALLOC(reg->feat1, sizeof(feature_type) * FEATURE_NUMS, CACHE_SIZE);
    CHECKED_MALLOC(reg->std_gray, sizeof(uint8_t) * STD_AREA, CACHE_SIZE);
    CHECKED_MALLOC(reg->std_bina, sizeof(uint8_t) * STD_AREA, CACHE_SIZE);
    CHECKED_MALLOC(reg->ch_buff0, sizeof(uint8_t) * STD_AREA * 8, CACHE_SIZE);
    CHECKED_MALLOC(reg->ch_buff1, sizeof(uint8_t) * STD_AREA * 8, CACHE_SIZE);
    CHECKED_MALLOC(reg->ch_rect0, sizeof(Rects) * CH_MAX, CACHE_SIZE);
    CHECKED_MALLOC(reg->ch_rect1, sizeof(Rects) * CH_MAX, CACHE_SIZE);
#endif

    *reg_handle = (Reg_handle)reg;

    return;
}

// ���������ַ�ʶ����
void lc_plate_recognition_destroy(Reg_handle reg_handle)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        destroy_svm_model(reg->model_numbers);
        destroy_svm_model(reg->model_chinese);
        destroy_svm_model(reg->model_jun_wjs);
        destroy_svm_model(reg->model_xuejing);
        destroy_svm_model(reg->model_english);

        destroy_svm_model(reg->model_0_v00_D);
        destroy_svm_model(reg->model_0_v00_Q);
        destroy_svm_model(reg->model_2_v00_Z);
        destroy_svm_model(reg->model_5_v00_S);
        destroy_svm_model(reg->model_8_v00_B);
        destroy_svm_model(reg->model_7_v00_T);

        destroy_svm_model(reg->model_0_v01_D);
        destroy_svm_model(reg->model_0_v01_Q);
        destroy_svm_model(reg->model_2_v01_Z);
        destroy_svm_model(reg->model_5_v01_S);
        destroy_svm_model(reg->model_8_v01_B);
        destroy_svm_model(reg->model_7_v01_T);

        CHECKED_FREE(reg->model_numbers, sizeof(SvmModel));
        CHECKED_FREE(reg->model_chinese, sizeof(SvmModel));
        CHECKED_FREE(reg->model_jun_wjs, sizeof(SvmModel));
        CHECKED_FREE(reg->model_xuejing, sizeof(SvmModel));
        CHECKED_FREE(reg->model_english, sizeof(SvmModel));

        CHECKED_FREE(reg->model_0_v00_D, sizeof(SvmModel));
        CHECKED_FREE(reg->model_0_v00_Q, sizeof(SvmModel));
        CHECKED_FREE(reg->model_2_v00_Z, sizeof(SvmModel));
        CHECKED_FREE(reg->model_5_v00_S, sizeof(SvmModel));
        CHECKED_FREE(reg->model_8_v00_B, sizeof(SvmModel));
        CHECKED_FREE(reg->model_7_v00_T, sizeof(SvmModel));

        CHECKED_FREE(reg->model_0_v01_D, sizeof(SvmModel));
        CHECKED_FREE(reg->model_0_v01_Q, sizeof(SvmModel));
        CHECKED_FREE(reg->model_2_v01_Z, sizeof(SvmModel));
        CHECKED_FREE(reg->model_5_v01_S, sizeof(SvmModel));
        CHECKED_FREE(reg->model_8_v01_B, sizeof(SvmModel));
        CHECKED_FREE(reg->model_7_v01_T, sizeof(SvmModel));

#ifdef _TMS320C6X_
        MEM_free(IRAM, reg->feat0, sizeof(feature_type) * FEATURE_NUMS);
        MEM_free(IRAM, reg->feat1, sizeof(feature_type) * FEATURE_NUMS);
        MEM_free(IRAM, reg->std_gray, sizeof(uint8_t) * STD_AREA);
        MEM_free(IRAM, reg->std_bina, sizeof(uint8_t) * STD_AREA);
        MEM_free(IRAM, reg->ch_buff0, sizeof(uint8_t) * STD_AREA * 8);
        MEM_free(IRAM, reg->ch_buff1, sizeof(uint8_t) * STD_AREA * 8);
        MEM_free(IRAM, reg->ch_rect0, sizeof(Rects) * CH_MAX);
        MEM_free(IRAM, reg->ch_rect1, sizeof(Rects) * CH_MAX);
#else
        CHECKED_FREE(reg->feat0, sizeof(feature_type) * FEATURE_NUMS);
        CHECKED_FREE(reg->feat1, sizeof(feature_type) * FEATURE_NUMS);
        CHECKED_FREE(reg->std_gray, sizeof(uint8_t) * STD_AREA);
        CHECKED_FREE(reg->std_bina, sizeof(uint8_t) * STD_AREA);
        CHECKED_FREE(reg->ch_buff0, sizeof(uint8_t) * STD_AREA * 8);
        CHECKED_FREE(reg->ch_buff1, sizeof(uint8_t) * STD_AREA * 8);
        CHECKED_FREE(reg->ch_rect0, sizeof(Rects) * CH_MAX);
        CHECKED_FREE(reg->ch_rect1, sizeof(Rects) * CH_MAX);
#endif
        CHECKED_FREE(reg, sizeof(Recognition));
    }

    return;
}

// ��ȡ����(ʡ��)ʶ����Ĥ(����ַ�ʶ��ľ���) 0: �����ú���ʶ�� 1: ���θú���ʶ��
void lc_plate_recognition_get_chinese_mask(Reg_handle reg_handle, uint8_t *chinese_mask)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        memcpy(chinese_mask, reg->model_chinese->mask, reg->model_chinese->nr_class);
    }

    return;
}

// ���ú���(ʡ��)ʶ����Ĥ(����ַ�ʶ��ľ���) 0: �����ú���ʶ�� 1: ���θú���ʶ��
void lc_plate_recognition_set_chinese_mask(Reg_handle reg_handle, uint8_t *chinese_mask)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        memcpy(reg->model_chinese->mask, chinese_mask, reg->model_chinese->nr_class);
    }

    return;
}

// ��ȡĬ�ϵĺ���(ʡ��)�������ֵ����ŶȲ���ʱ(��̫�϶�ʱ)������Ĭ�Ϻ���
void lc_plate_recognition_get_chinese_default(Reg_handle reg_handle, uint8_t *chinese_default)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        *chinese_default = (reg->model_chinese)->default_class;
    }

    return;
}

// ����Ĭ�ϵĺ���(ʡ��)�������ֵ����ŶȲ���ʱ(��̫�϶�ʱ)������Ĭ�Ϻ���
void lc_plate_recognition_set_chinese_default(Reg_handle reg_handle, uint8_t chinese_default)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        reg->model_chinese->default_class = chinese_default;
    }

    return;
}

// ��ȡ���ֵ����Ŷ���ֵ�������ڸ���ֵʱ����Ĭ�ϵ�ʡ�ݺ���
void lc_plate_recognition_get_chinese_trust_thresh(Reg_handle reg_handle, uint8_t *trust_thresh)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        *trust_thresh = reg->model_chinese->trust_thresh;
    }

    return;
}

// ���ú��ֵ����Ŷ���ֵ�������ڸ���ֵʱ����Ĭ�ϵ�ʡ�ݺ���
void lc_plate_recognition_set_chinese_trust_thresh(Reg_handle reg_handle, uint8_t trust_thresh)
{
    Recognition *reg = (Recognition *)reg_handle;

    if (reg)
    {
        reg->model_chinese->trust_thresh = trust_thresh;
    }

    return;
}

// �Ҷ�ͼ��ɫ
#ifdef WIN321
static void gray_image_reverse_brightness(uint8_t *restrict plate_dst, uint8_t *restrict plate_src,
                                          int32_t plate_w, int32_t plate_h)
{
    int32_t pix;
    const int32_t area = plate_w * plate_h;
    uint8_t *restrict src_img = plate_src;
    uint8_t *restrict dst_img = plate_dst;

    _asm
    {
        mov     eax, src_img       // put src addr to eax reg
        mov     ebx, dst_img       // put dst addr to edx reg
        mov     ecx, area          // put combination to ecx reg
        shr     ecx, 5             // divide dimentions with 32 by shifting 5 bits to right

        reverse_loop:

        pcmpeqb xmm0, xmm0         // xmm0 = FFFF FFFF FFFF FFFF H
        pcmpeqb xmm2, xmm2         // xmm2 = FFFF FFFF FFFF FFFF H

        movaps  xmm1, [eax   ]
        movaps  xmm3, [eax+16]

        psubusb  xmm0, xmm1        // 0xFF-a3, 0xFF-a2, 0xFF-a0, 0xFF-a0
        psubusb  xmm2, xmm3        // 0xFF-a3, 0xFF-a2, 0xFF-a0, 0xFF-a0

        movaps  [ebx   ], xmm0
        movaps  [ebx+16], xmm2

        add     eax, 32            // add src pointer by 32 bytes
        add     ebx, 32            // add dst pointer by 32 bytes
        dec     ecx                // decrement count by 1
        jnz     reverse_loop       // jump to reverse_loop if not Zero
    }

    for (pix = ((area >> 5) << 5); pix < area; pix++)
    {
        dst_img[pix] = 255 - src_img[pix];
    }

    return;
}
#elif defined _TMS320C6X_
static void gray_image_reverse_brightness(uint8_t *restrict plate_dst, uint8_t *restrict plate_src,
                                          int32_t plate_w, int32_t plate_h)
{
    int32_t i;
    int32_t pix;
    const int32_t area = plate_w * plate_h;
    uint8_t *restrict src_img = plate_src;
    uint8_t *restrict dst_img = plate_dst;
    uint32_t u32Src0, u32Src1, u32Src2, u32Src3;
    uint32_t u32Dst0, u32Dst1, u32Dst2, u32Dst3;

    // �����������Ľ�ǰ��9522 | 10147 | 7407 | 16426 (cycles)
    // �����������Ľ���2860 |  3728 | 2884 |  8932 (cycles)

    for (i = 0; i < area / 16; i++)
    {
        u32Src0 = _amem4(src_img + (i << 4) +  0);
        u32Src1 = _amem4(src_img + (i << 4) +  4);
        u32Src2 = _amem4(src_img + (i << 4) +  8);
        u32Src3 = _amem4(src_img + (i << 4) + 12);

        u32Dst0 = _sub4(0xFFFFFFFF, u32Src0);
        u32Dst1 = _sub4(0xFFFFFFFF, u32Src1);
        u32Dst2 = _sub4(0xFFFFFFFF, u32Src2);
        u32Dst3 = _sub4(0xFFFFFFFF, u32Src3);

        _amem4(dst_img + (i << 4) +  0) = u32Dst0;
        _amem4(dst_img + (i << 4) +  4) = u32Dst1;
        _amem4(dst_img + (i << 4) +  8) = u32Dst2;
        _amem4(dst_img + (i << 4) + 12) = u32Dst3;
    }

    for (pix = (i << 4); pix < area; pix++)
    {
        dst_img[pix] = 255 - src_img[pix];
    }

    return;
}
#else
static void gray_image_reverse_brightness(uint8_t *restrict plate_dst, uint8_t *restrict plate_src,
                                          int32_t plate_w, int32_t plate_h)
{
    int32_t pix;
    const int32_t area = plate_w * plate_h;
    uint8_t *restrict src_img = plate_src;
    uint8_t *restrict dst_img = plate_dst;

    for (pix = 0; pix < area; pix++)
    {
        dst_img[pix] = (uint8_t)((255u) - src_img[pix]);
    }

    return;
}
#endif

// �Ծ��Ƶ�У��
static int32_t police_plate_rectifying(Recognition *restrict reg, uint8_t *restrict plate_img,
                                       int32_t plate_w, int32_t plate_h, Rects *restrict ch,
                                       uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *trust)
{
    int32_t k;
    int32_t x, y, z;
    int32_t xx, yy;
    // �����ʹ��渡����
    int32_t a, b;
    int32_t denominator;
//  const int32_t std_len = 409;        // ��׼���Ƴ���409mm
    const int32_t std_ch_w = 45;        // ��׼�����ַ����45mm
//  const int32_t std_ch_i = 12;        // ��׼�����ַ����12mm
//  const int32_t std_max_i = 34;       // ��׼�����ַ������34mm
//  const int32_t std_ch_x[9] = {0, 57, 136, 193, 250, 307, 364};// ÿһ����׼�ַ����ĵ��X����
//  const int32_t std_ch_x[7] = {0, 79, 136, 193, 250, 307, 364};
    const int32_t std_ch_x[8] = {0, 79, 79 + 57 * 1, 79 + 57 * 2, 79 + 57 * 3, 79 + 57 * 4, 79 + 57 * 5, 79 + 57 * 6}; // ÿһ����׼�ַ����ĵ��X����
    SvmModel *restrict model_chinese = reg->model_chinese;
//  SvmModel *restrict model_numbers = reg->model_numbers;
    SvmModel *restrict model_xuejing = reg->model_xuejing;
    Rects ch_tmp;
    int32_t ch_w = 0;
    int32_t ch_h = 0;
    int32_t sum_x = 0;
    int32_t sum_y = 0;
    int32_t sum_z = 0;
    int32_t sum_x_x = 0;
    int32_t sum_y_y = 0;
    int32_t sum_z_z = 0;
    int32_t sum_x_y = 0;
    int32_t sum_x_z = 0;
    int32_t n = 0;
    uint8_t *restrict std_buf_jing = NULL;
//  uint8_t *restrict std_buf_temp = NULL;
    uint8_t chs_jing;
    uint8_t trust_jing;
    uint8_t is_ch;

    // ����ͨ����С���˷���λ'��'��

    // ��Ϊ���Ƶ��������ch[0]��ch[1]֮�䣬�������Ǽ���ch[0]�Ƿ��ַ���ch[1]���������ĺ����ַ�
    // ����������Ҫͨ��ch[1]~ch[6]��6���ַ���λ��ȥ������һ�������ַ���������λ��ch_tmp

    // ������һ���ַ����������ĵ��x����
    // ��ϵĲ������̣�x = a * z + b ����x��ʾ�ַ������ĵ�x����
    // z��ʾ��ǰ�ַ����ĵ㵽���һ���ַ����������ĵ�ľ���(x����)
    // =========================================
    // |��|-|A|0|1|2|3|��|
    //  |--z-|
    //  |--z---|
    //  |---z----|
    //  |-----z----|
    //  |------z-----|
    //  |--------z------|
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
    sum_x_x = 0;
    sum_z_z = 0;
    sum_x_z = 0;
    n = 0;

    for (k = 2; k < 7; k++)
    {
        x = (ch[k].x0 + ch[k].x1) / 2;

        sum_x   += x;
        sum_x_x += x * x;
        sum_z   += std_ch_x[k];
        sum_z_z += std_ch_x[k] * std_ch_x[k];
        sum_x_z += x * std_ch_x[k];

        n++;
    }

    denominator = (sum_z * sum_z - n * sum_z_z);

    a = (sum_x * sum_z - n * sum_x_z);
    b = (sum_x_z * sum_z - sum_x * sum_z_z);

    z = std_ch_x[7];

    xx = (a * z + b) / denominator;

//  ch_w = ((ch[6].x1 - ch[6].x0 + 1) + (ch[5].x1 - ch[5].x0 + 1)) / 2;
    ch_w = (a * std_ch_w + b) / denominator;

    ch_tmp.x0 = (int16_t)(xx - ch_w / 2);
    ch_tmp.x1 = (int16_t)(xx + ch_w / 2);

    if (ch_tmp.x1 > plate_w + 4)
    {
        return 0;
    }
    else
    {
        ch_tmp.x1 = (int16_t)MIN2(ch_tmp.x1, plate_w - 1);
    }

    ch_tmp.x0 = (int16_t)MAX2(ch_tmp.x0, ch[6].x1 + 2);

    if (ch_tmp.x1 - ch_tmp.x0 < ch_w * 2 / 3)
    {
        return 0;
    }

    // ������һ���ַ����������ĵ��y����
    // ��ϵĲ������̣�y = a * x + b ����y��ʾ�ַ������ĵ�y����
    // ����֪�ַ�����x����ʱ�����Լ���õ���Ӧ��y����
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
    sum_y_y = 0;
    sum_x_y = 0;
    n = 0;

    for (k = 2; k < 7; k++)
    {
        x = (ch[k].x0 + ch[k].x1) / 2;
        y = (ch[k].y0 + ch[k].y1) / 2;

        sum_x   += x;
        sum_x_x += x * x;
        sum_y   += y;
        sum_y_y += y * y;
        sum_x_y += x * y;

        n++;
    }

    denominator = sum_x * sum_x - n * sum_x_x;

    if (denominator != 0)
    {
        a = (sum_x * sum_y - n * sum_x_y);
        b = (sum_x_y * sum_x - sum_y * sum_x_x);

        yy = (a * xx + b) / denominator;

        ch_h = ((ch[6].y1 - ch[6].y0 + 1) + (ch[5].y1 - ch[5].y0 + 1)) / 2;

        ch_tmp.y0 = (int16_t)(yy - ch_h / 2);
        ch_tmp.y1 = (int16_t)(yy + ch_h / 2);

        ch_tmp.y0 = (int16_t)MAX2(ch_tmp.y0, 0);
        ch_tmp.y1 = (int16_t)MIN2(ch_tmp.y1, plate_h - 1);

        if (ch_tmp.y1 - ch_tmp.y0 < ch_h * 2 / 3)
        {
            ch_tmp.y0 = ch[6].y0;
            ch_tmp.y1 = ch[6].y1;
        }
    }
    else
    {
        ch_tmp.y0 = ch[6].y0;
        ch_tmp.y1 = ch[6].y1;
    }

    CHECKED_MALLOC(std_buf_jing, sizeof(uint8_t) * STD_AREA, CACHE_SIZE);

    single_character_recognition(plate_img, plate_w, /*plate_h, */ch_tmp,
                                 &chs_jing, &trust_jing, std_buf_jing, model_xuejing, (0u), (0u), &is_ch);

    if ((chs_jing == CH_JING)/* && (trust_jing > 80)*/) // ����
    {
        for (k = 1; k < 7; k++)
        {
            ch[k - 1] = ch[k];
            chs[k - 1] = chs[k];
            trust[k - 1] = trust[k];
            memcpy(&ch_buf[STD_AREA * (k - 1)], &ch_buf[STD_AREA * k], (uint32_t)STD_AREA);
        }

        chs[6] = CH_JING;
        trust[6] = trust_jing;
        ch[6] = ch_tmp;
        memcpy(&ch_buf[STD_AREA * 6], std_buf_jing, (uint32_t)STD_AREA);

        // �ֶ�λ����
        chinese_locating_by_least_square(ch, 7, ch_w, ch_h, /*plate_w, */plate_h, 1);

        // ��ȷ��λ����
        chinese_left_right_locating_by_projection(plate_img, plate_w, plate_h, ch, 7, 1);

        single_character_recognition(plate_img, plate_w, /*plate_h, */ch[0],
                                     &chs[0], &trust[0], ch_buf, model_chinese, (0u), (1u), &is_ch);

        CHECKED_FREE(std_buf_jing, sizeof(uint8_t) * STD_AREA);

        return 1;
    }
    else
    {
        CHECKED_FREE(std_buf_jing, sizeof(uint8_t) * STD_AREA);

        return 0;
    }
}

// ��'ѧ'���ƵĽ���
static int32_t yellow_plate_rectifying(uint8_t *restrict plate_img, int32_t plate_w, /*int32_t plate_h,*/
                                       SvmModel *restrict model_xuejing, Rects *restrict ch,
                                       uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *trust)
{
    uint8_t is_ch;
    uint8_t chs_tmp;
    uint8_t trust_tmp = (uint8_t)0;

    single_character_recognition(plate_img, plate_w, /*plate_h, */ch[6], &chs_tmp,
                                 &trust_tmp, ch_buf + STD_AREA * 6, model_xuejing, (0u), (0u), &is_ch);

    if (chs_tmp == CH_XUE /*&& trust_tmp > trust[6]*/)  // ������
    {
        chs[6] = CH_XUE;
        trust[6] = trust_tmp;

        return 1;
    }
    else
    {
        return 0;
    }
}

// �Ծ��ƺ��ֵ�ʶ��
static int32_t jun_wjs_plate_rectifying(uint8_t *restrict plate_img, int32_t plate_w, /*int32_t plate_h,*/
                                        SvmModel *restrict model_jun_wjs, Rects *restrict ch,
                                        uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *chs_trust)
{
    uint8_t is_ch;

    single_character_recognition(plate_img, plate_w, /*plate_h, */ch[0], &chs[0],
                                 &chs_trust[0], ch_buf, model_jun_wjs, (0u), (1u), &is_ch);

    return 1;
}

#define DEBUG_OUT 0

// ȷ�����Ƶ�ɫ�ǻ�ɫ���ǰ�ɫ
static uint8_t determine_plate_background_color_yellow_or_white(uint8_t *restrict yuv_img,
                                                                int32_t img_w, int32_t img_h, Rects rect)
{
    int32_t h;
    int32_t plate_w;
    int32_t plate_h;
    uint8_t plate_color;
    uint8_t *restrict image_y = yuv_img;
    uint8_t *restrict image_u = yuv_img + img_w * img_h;
    uint8_t *restrict image_v = yuv_img + (img_w * img_h * 5 >> 2);
    uint8_t *restrict pcolor_y = NULL;
    uint8_t *restrict pcolor_u = NULL;
    uint8_t *restrict pcolor_v = NULL;
    uint8_t *restrict pcolor = NULL;
#if DEBUG_OUT
    FILE *fp = NULL;
    char bufname[512];
#endif

    plate_w = ((rect.x1 - rect.x0 + 1 + 7) >> 3) << 3;
    plate_h = ((rect.y1 - rect.y0 + 1 + 1) >> 1) << 1;

#if DEBUG_OUT
    sprintf(bufname, "y%dx%d.yuv", plate_w, plate_h);
    fp = fopen(bufname, "w");
#endif

    CHECKED_MALLOC(pcolor, sizeof(uint8_t) * plate_w * plate_h * 3 >> 1, CACHE_SIZE);

    pcolor_y = pcolor;
    pcolor_u = pcolor + plate_w * plate_h;
    pcolor_v = pcolor + ((plate_w * plate_h * 5) >> 2);

    for (h = 0; h < plate_h; h++)
    {
        memcpy(pcolor_y + plate_w * h, image_y + img_w * (rect.y0 + h) + rect.x0, plate_w);
    }

    for (h = 0; h < (plate_h >> 1); h++)
    {
        memcpy(pcolor_u + (plate_w >> 1) * h, image_u + (img_w >> 1) * ((rect.y0 >> 1) + h) + (rect.x0 >> 1), plate_w >> 1);
        memcpy(pcolor_v + (plate_w >> 1) * h, image_v + (img_w >> 1) * ((rect.y0 >> 1) + h) + (rect.x0 >> 1), plate_w >> 1);
    }

    plate_color = plate_yellow_or_white(pcolor, plate_w, plate_h);

#if DEBUG_OUT
    fwrite(pcolor_y, plate_w * plate_h * 3 >> 1, 1, fp);
    fclose(fp);
#endif

    CHECKED_FREE(pcolor, sizeof(uint8_t) * plate_w * plate_h * 3 >> 1);

    return plate_color;
}
#undef DEBUG_OUT

// ���س��Ƶ�ƽ������ֵ
static uint8_t get_plate_brightness(uint8_t *restrict yuv_img, int32_t img_w, /*int32_t img_h, */Rects rect)
{
    int32_t w, h;
    int32_t gray_sum = 0;
    uint8_t brightness = (uint8_t)0;
    const int32_t rect_w = rect.x1 - rect.x0 + 1;
    const int32_t rect_h = rect.y1 - rect.y0 + 1;
    const int32_t rect_area = rect_w * rect_h;

    for (h = rect.y0; h <= rect.y1; h++)
    {
        for (w = rect.x0; w <= rect.x1; w++)
        {
            gray_sum += (int32_t)(yuv_img[img_w * h + w]);
        }
    }

    brightness = (uint8_t)(gray_sum / rect_area);

    return brightness;
}

#define NSHIFT 15

// ������ʶ��
void lc_plate_recognition_single(Reg_handle reg_handle, uint8_t *restrict pgray_img,
                                 int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t *ch_num,
                                 uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *restrict chs_trust,
                                 uint8_t *restrict brightness,
                                 uint8_t *restrict plate_trust, uint8_t trust_thresh,
                                 uint8_t *bkgd_color, uint8_t *color_new,
                                 uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                 Rects *restrict rect_old, Rects *restrict rect_new)
{
    Recognition *reg = (Recognition *)reg_handle;
    int32_t area;
    int32_t k;
    int32_t sum;
    int32_t resize_w;
    int32_t resize_h;
    // ��׺0��δ��ɫ��Ӧ�Ĳ���
    // ��׺1����ɫ���Ӧ�Ĳ���
    uint8_t *restrict gray_img = NULL;
    uint8_t *restrict gray_img0 = NULL;
    uint8_t *restrict gray_img1 = NULL;
    uint8_t chs0[CH_NUMS];
    uint8_t chs1[CH_NUMS];
    uint8_t trust0[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t trust1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t seg_trust0 = (uint8_t)0; // �ָ����Ŷ�
    uint8_t seg_trust1 = (uint8_t)0; // �ָ����Ŷ�
    uint8_t reg_trust0 = (uint8_t)0; // ʶ�����Ŷ�
    uint8_t reg_trust1 = (uint8_t)0; // ʶ�����Ŷ�
    uint8_t tot_trust0 = (uint8_t)0; // �ָ��ʶ�����Ŷ�֮��
    uint8_t tot_trust1 = (uint8_t)0; // �ָ��ʶ�����Ŷ�֮��
    int32_t ch_num0 = 0;
    int32_t ch_num1 = 0;
    uint8_t not_ch0 = (uint8_t)255;
    uint8_t not_ch1 = (uint8_t)255;
    Rects *restrict ch_rect0 = reg->ch_rect0;
    Rects *restrict ch_rect1 = reg->ch_rect1;
    uint8_t *restrict ch_buff0 = reg->ch_buff0;
    uint8_t *restrict ch_buff1 = reg->ch_buff1;
    SvmModel *restrict model_chinese = reg->model_chinese;
    SvmModel *restrict model_numbers = reg->model_numbers;
    SvmModel *restrict model_jun_wjs = reg->model_jun_wjs;
    SvmModel *restrict model_xuejing = reg->model_xuejing;
    SvmModel *restrict model_english = reg->model_english;
    CLOCK_INITIALIZATION();

    if (plate_h <= 0)
    {
        return;
    }

    // ���Ƹ߶����ŵ�35�����أ���ȵȱ�������
    resize_h = 35;
    resize_w = plate_w * resize_h / plate_h;
    area = resize_w * resize_h;

    // Ĭ�ϵĺ���ѡ����뿪��
    model_chinese->mask[model_chinese->default_class - LABEL_START_OF_CHINESE] = (0u);

    CHECKED_MALLOC(gray_img, sizeof(uint8_t) * area, CACHE_SIZE);
    CHECKED_MALLOC(gray_img0, sizeof(uint8_t) * area, CACHE_SIZE);
    CHECKED_MALLOC(gray_img1, sizeof(uint8_t) * area, CACHE_SIZE);

    gray_image_resize_linear_for_center(pgray_img, gray_img0, plate_w, plate_h, resize_w, resize_h);

    CLOCK_START(character_segmentation);

    memcpy(gray_img, gray_img0, sizeof(uint8_t) * area);

    // ��ɫǰ
    if ((*bkgd_color == 255) || (*bkgd_color == 0))
    {
        uint8_t* pbGray = malloc(resize_w * resize_h);
        memcpy(pbGray, gray_img0, resize_w * resize_h);
        seg_trust0 = character_segmentation(pbGray, resize_w, resize_h, ch_rect0, &ch_num0,
                                            chs0, trust0, &reg_trust0, ch_buff0,
                                            model_english, model_numbers, model_chinese, &not_ch0, (0u), 0.0, 7);
        if (seg_trust0 == 0u || reg_trust0 < 65)
        {
            memcpy(pbGray, gray_img0, resize_w * resize_h);
            seg_trust0 = character_segmentation(pbGray, resize_w, resize_h, ch_rect0, &ch_num0,
                chs0, trust0, &reg_trust0, ch_buff0,
                model_english, model_numbers, model_chinese, &not_ch0, (0u), 0.5, 7);
            if (seg_trust0 == 0u || reg_trust0 < 65)
            {
                memcpy(pbGray, gray_img0, resize_w * resize_h);
                seg_trust0 = character_segmentation(pbGray, resize_w, resize_h, ch_rect0, &ch_num0,
                    chs0, trust0, &reg_trust0, ch_buff0,
                    model_english, model_numbers, model_chinese, &not_ch0, (0u), -0.5, 7);
            }
        }

        free(pbGray);
    }

    // ��ɫ��
    if ((*bkgd_color == 255) || (*bkgd_color == 1))
    {
        gray_image_reverse_brightness(gray_img1, gray_img, resize_w, resize_h);

        uint8_t* pbGray = malloc(resize_w * resize_h);
        memcpy(pbGray, gray_img1, resize_w * resize_h);
        seg_trust1 = character_segmentation(pbGray, resize_w, resize_h, ch_rect1, &ch_num1,
                                            chs1, trust1, &reg_trust1, ch_buff1,
                                            model_english, model_numbers, model_chinese, &not_ch1, (1u), 0.0, 8);
        if (seg_trust1 == 0u)
        {
            memcpy(pbGray, gray_img1, resize_w * resize_h);
            seg_trust1 = character_segmentation(pbGray, resize_w, resize_h, ch_rect1, &ch_num1,
                chs1, trust1, &reg_trust1, ch_buff1,
                model_english, model_numbers, model_chinese, &not_ch1, (1u), 0.5, 8);
            if (seg_trust1 == 0u)
            {
                memcpy(pbGray, gray_img1, resize_w * resize_h);
                seg_trust1 = character_segmentation(pbGray, resize_w, resize_h, ch_rect1, &ch_num1,
                    chs1, trust1, &reg_trust1, ch_buff1,
                    model_english, model_numbers, model_chinese, &not_ch1, (1u), -0.5, 8);
            }
        }
        free(pbGray);
    }

    CLOCK_END("character_segmentation");

    tot_trust0 = (uint8_t)((seg_trust0 + reg_trust0) / (2u));
    tot_trust1 = (uint8_t)((seg_trust1 + reg_trust1) / (2u));

#if 0
    printf("-------------seg_trust0 = %d reg_trust0 = %d\n", seg_trust0, reg_trust0);
    printf("-------------seg_trust1 = %d reg_trust1 = %d\n", seg_trust1, reg_trust1);
    printf("-------------tot_trust0 = %d ch_num0 = %d\n", tot_trust0, ch_num0);
    printf("-------------tot_trust1 = %d ch_num1 = %d\n", tot_trust1, ch_num1);
#endif

    if (tot_trust0 >= tot_trust1) // �ڵװ���
    {
        *bkgd_color = 0;

        if ((not_ch0 >= (4u)) || (tot_trust0 == (0u)))
        {
            *ch_num = 0;
            *plate_trust = 0;
        }
        else
        {
            *ch_num = ch_num0;
            memcpy(chs, chs0, sizeof(chs0));
            memcpy(chs_trust, trust0, sizeof(trust0));
            memcpy(ch_pos, ch_rect0, sizeof(Rects) * MIN2(ch_num0, 7));
            memcpy(ch_buf, ch_buff0, sizeof(uint8_t) * STD_AREA * MIN2(ch_num0, 7));
            *plate_trust = reg_trust0;
        }
    }
    else // �׵׺���
    {
        *bkgd_color = 1;

        if (not_ch1 >= (4u))
        {
            *ch_num = 0;
            *plate_trust = 0;
        }
        else
        {
            *ch_num = ch_num1;
            memcpy(chs, chs1, sizeof(chs1));
            memcpy(chs_trust, trust1, sizeof(trust1));
            memcpy(ch_pos, ch_rect1, sizeof(Rects) * MIN2(ch_num1, 7));
            memcpy(ch_buf, ch_buff1, sizeof(uint8_t) * STD_AREA * MIN2(ch_num1, 7));

            *plate_trust = reg_trust1;

            if (*ch_num == 7)
            {
                int32_t trainer_flag = 0;
                int32_t police_flag = 0;

                trainer_flag = yellow_plate_rectifying(gray_img1, resize_w, /*resize_h, */
                                                       model_xuejing, ch_pos, ch_buf, chs, chs_trust);

                // ������ǽ�����
                if (trainer_flag == 0)
                {
                    // �Ƿ��Ǿ���
                    police_flag = police_plate_rectifying(reg, gray_img1, resize_w, resize_h,
                                                          ch_pos, ch_buf, chs, chs_trust);

                    if (police_flag)
                    {
                        *bkgd_color = 3; // �׵׾���
                    }
                }
                else
                {
//                    *bkgd_color = 1; // ��������
                }

                // ���Ϊ�������ƻ��߾��ƣ�Ӧ�����¼��㳵�����Ŷ�
                if (trainer_flag == 1 || police_flag == 1)
                {
                    int32_t trust_tmp = 0;

                    for (k = 0; k < *ch_num; k++)
                    {
                        trust_tmp += (int32_t)(chs_trust[k]);
                    }

                    *plate_trust = (uint8_t)(trust_tmp / *ch_num);
                }
            }
        }
    }

    // ע�⣺��������Ŷ��ж���ʱ��Ѻܶ������ĳ��ƶ��ų�����
    if (((*ch_num == 7) || (*ch_num == 8)) && MAX2(tot_trust0, tot_trust1) > trust_thresh)
    {
        chs[*ch_num] = (uint8_t)'\0';

        // ���ַ���λ�óߴ�������Ż�ԭ
        for (k = 0; k < *ch_num; k++)
        {
            ch_pos[k].x0 = (int16_t)MAX2(ch_pos[k].x0 * plate_h / resize_h, 0);
            ch_pos[k].x1 = (int16_t)MIN2(ch_pos[k].x1 * plate_h / resize_h, plate_w - 1);
            ch_pos[k].y0 = (int16_t)MAX2(ch_pos[k].y0 * plate_h / resize_h, 0);
            ch_pos[k].y1 = (int16_t)MIN2(ch_pos[k].y1 * plate_h / resize_h, plate_h - 1);
        }

        (*rect_old).x1 = (int16_t)((*rect_old).x0 + ch_pos[*ch_num - 1].x1);
        (*rect_old).x0 = (int16_t)((*rect_old).x0 + ch_pos[0].x0);
        (*rect_old).y1 = (int16_t)((*rect_old).y0 + ch_pos[0].y1);
        (*rect_old).y0 = (int16_t)((*rect_old).y0 + ch_pos[0].y0);

        *rect_new = *rect_old;

#if DEBUG_DSP_PC_COMPARE
        // ��ӡʶ��ģ�������Ϣ
        {
            int i;
            FILE* fmodule_info_out = fopen("module_info_out.txt", "a+");

            fprintf(fmodule_info_out, "======================================\n");

            for (i = 0; i < *ch_num; i++)
            {
                fprintf(fmodule_info_out, "pos: (%3d, %3d, %3d, %3d)\n",
                        ch_pos[i].x0, ch_pos[i].x1, ch_pos[i].y0, ch_pos[i].y1);
            }

            fprintf(fmodule_info_out, "Rect: (%3d, %3d, %3d, %3d)\n",
                    (*rect_old).x0, (*rect_old).x1, (*rect_old).y0, (*rect_old).y1);
            fprintf(fmodule_info_out, "======================================\n");
            fclose(fmodule_info_out);
        }
#endif

//      if (chs[6] == CH_XUE)
//      {
//          *bkgd_color = 1; // ��������
//      }
//      else if (chs[6] == CH_JING)
//      {
//          *bkgd_color = 3; // �׵׾���
//      }

        if (*bkgd_color == 0u) // 0������
        {
            *color_new = 0u;
        }
        else if (*bkgd_color == 3u) // 3�ǰ׵׾���
        {
            *color_new = 3u;
        }
        else if (chs[6] == CH_XUE)
        {
            *bkgd_color = 1u; // ��������
            *color_new = *bkgd_color;
        }
//      else if (*bkgd_color == 1)
//      {
//          *color_new = 1;
//      }
        else // ȷ���ǰ��ƻ��ǻ���
        {
            //*color_new = determine_plate_background_color_yellow_or_white(yuv_img, img_w, img_h, *rect_new);
        }

        if ((*color_new == 2u) && (*ch_num == 7)) // ����ǰ׵ף����ƻ����侯�ƣ�
        {
            jun_wjs_plate_rectifying(gray_img1, resize_w, /*resize_h, */model_jun_wjs,
                                     ch_rect1, ch_buf, chs, chs_trust);
        }

        //*brightness = get_plate_brightness(yuv_img, img_w, /*img_h, */*rect_new);

        if (*ch_num == 7 || *ch_num == 8)
        {
            similar_character_classify(reg, ch_buf, chs, chs_trust, *ch_num);

            if (*ch_num == 8)
            {
                if (chs[2] == '0')
                {
                    chs[2] = 'D';
                }
                for (int t = 3; t < 7; t++)
                {
                    if (chs[t] == 'B' || chs[t] == 'Q')
                    {
                        chs[t] = '8';
                    }
                    if (chs[t] == 'D')
                    {
                        chs[t] = '0';
                    }
                    if (chs[t] == 'S')
                    {
                        chs[t] = '5';
                    }
                    if (chs[t] == 'T')
                    {
                        chs[t] = '7';
                    }
                }

                *color_new = 10;
            }
        }

//        set_character_as_unkown_while_no_convinced(chs, chs_trust);

        sum = 0;

        for (k = 0; k < *ch_num; k++)
        {
            sum += (int32_t)(chs_trust[k]);
        }

        *plate_trust = (uint8_t)(sum / *ch_num);

//         if (*plate_trust < trust_thresh)
//         {
//             *ch_num = 0;
//         }
    }
    else
    {
        *ch_num = 0;
    }

    CHECKED_FREE(gray_img1, sizeof(uint8_t) * area);
    CHECKED_FREE(gray_img0, sizeof(uint8_t) * area);
    CHECKED_FREE(gray_img, sizeof(uint8_t) * area);

    return;
}

// ˫����ʶ��
void lc_plate_recognition_double(Reg_handle reg_handle, uint8_t *restrict pgray_img, int32_t plate_h_up,
                                 int32_t plate_w, int32_t plate_h, Rects *restrict ch_pos, int32_t *ch_num,
                                 uint8_t *restrict ch_buf, uint8_t *restrict chs, uint8_t *restrict chs_trust,
                                 uint8_t *restrict brightness,
                                 uint8_t *restrict plate_trust, uint8_t trust_thresh,
                                 uint8_t *bkgd_color, uint8_t *color_new,
                                 uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                                 Rects *restrict rect_old, Rects *restrict rect_new)
{
    Recognition *reg = (Recognition *)reg_handle;
    int32_t area;
    int32_t area_up;
    int32_t k;
    int32_t sum;
    int32_t resize_up_w;
    int32_t resize_up_h;
    int32_t resize_down_w;
    int32_t resize_down_h;
    int32_t plate_h_down;
    // ��׺0��δ��ɫ��Ӧ�Ĳ���
    // ��׺1����ɫ���Ӧ�Ĳ���
    uint8_t *restrict gray_down_img0 = NULL;
    uint8_t *restrict gray_down_img1 = NULL;
    uint8_t *restrict gray_up_img0 = NULL;
    uint8_t *restrict gray_up_img1 = NULL;
    int32_t center_rate0 = 0;
    int32_t center_rate1 = 0;
    uint8_t chs0[CH_NUMS];
    uint8_t chs1[CH_NUMS];
    uint8_t trust0[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t trust1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t seg_up_trust0 = (uint8_t)0;   // �ϲ�ָ����Ŷ�
    uint8_t seg_up_trust1 = (uint8_t)0;   // �ϲ�ָ����Ŷ�
    uint8_t seg_down_trust0 = (uint8_t)0; // �²�ָ����Ŷ�
    uint8_t seg_down_trust1 = (uint8_t)0; // �²�ָ����Ŷ�
    uint8_t reg_trust0 = (uint8_t)0; // ʶ�����Ŷ�
    uint8_t reg_trust1 = (uint8_t)0; // ʶ�����Ŷ�
    uint8_t tot_trust0 = (uint8_t)0; // �ָ��ʶ�����Ŷ�֮��
    uint8_t tot_trust1 = (uint8_t)0; // �ָ��ʶ�����Ŷ�֮��
    int32_t ch_num0 = 0;
    int32_t ch_num1 = 0;
    uint8_t not_ch0 = (uint8_t)255;
    uint8_t not_ch1 = (uint8_t)255;
    Rects *restrict ch_rect0 = reg->ch_rect0;
    Rects *restrict ch_rect1 = reg->ch_rect1;
    uint8_t *restrict ch_buff0 = reg->ch_buff0;
    uint8_t *restrict ch_buff1 = reg->ch_buff1;
    SvmModel *restrict model_chinese = reg->model_chinese;
    SvmModel *restrict model_numbers = reg->model_numbers;
    SvmModel *restrict model_jun_wjs = reg->model_jun_wjs;
//    SvmModel *restrict model_xuejing = reg->model_xuejing;
    SvmModel *restrict model_english = reg->model_english;
    CLOCK_INITIALIZATION();

    if (plate_h <= 10)
    {
        return;
    }

    // Ĭ�ϵĺ���ѡ����뿪��
    model_chinese->mask[model_chinese->default_class - LABEL_START_OF_CHINESE] = (0u);

    // �ϲ�����
    resize_up_h = 35;
    resize_up_w = plate_w * resize_up_h / plate_h_up;

    // ��Ȳ��ܳ����߶ȵ�10��
    if (resize_up_w > resize_up_h * 10)
    {
        return;
    }

    area_up = resize_up_w * resize_up_h;

    CHECKED_MALLOC(gray_up_img0, sizeof(uint8_t) * area_up, CACHE_SIZE);
    CHECKED_MALLOC(gray_up_img1, sizeof(uint8_t) * area_up, CACHE_SIZE);

    gray_image_resize_linear_for_center(pgray_img, gray_up_img0, plate_w, plate_h_up, resize_up_w, resize_up_h);

    if ((*bkgd_color == 255) || (*bkgd_color == 0))
    {
        seg_up_trust0 = character_segmentation_double_up(gray_up_img0, resize_up_w, resize_up_h, ch_rect0, &ch_num0,
                                                         chs0, trust0, &reg_trust0, ch_buff0,
                                                         model_english, model_numbers, model_chinese, &not_ch0, (0u));
        // ���߳������رյ�ũ�ó�ʶ��
        ch_num0 = 0;

        if (ch_num0 != 0)
        {
            center_rate0 = ((ch_rect0[1].x0 + ch_rect0[0].x1) / 2) * 100 / resize_up_w;
        }
    }

    if ((*bkgd_color == 255) || (*bkgd_color == 1))
    {
        gray_image_reverse_brightness(gray_up_img1, gray_up_img0, resize_up_w, resize_up_h);

        seg_up_trust1 = character_segmentation_double_up(gray_up_img1, resize_up_w, resize_up_h, ch_rect1, &ch_num1,
                                                         chs1, trust1, &reg_trust1, ch_buff1,
                                                         model_english, model_numbers, model_chinese, &not_ch1, (1u));

        if (ch_num1 != 0)
        {
            center_rate1 = ((ch_rect1[1].x0 + ch_rect1[0].x1) / 2) * 100 / resize_up_w;
        }
    }


    CHECKED_FREE(gray_up_img1, sizeof(uint8_t) * area_up);
    CHECKED_FREE(gray_up_img0, sizeof(uint8_t) * area_up);

    // �²�����
    // ���Ƹ߶����ŵ�35�����أ���ȵȱ�������
    plate_h_down = plate_h - plate_h_up - 1;
    resize_down_h = 35;
    resize_down_w = plate_w * resize_down_h / plate_h_down;

    // ��Ȳ��ܳ����߶ȵ�10��
    if (resize_down_w > resize_down_h * 10)
    {
        return;
    }

    area = resize_down_w * resize_down_h;

    CHECKED_MALLOC(gray_down_img0, sizeof(uint8_t) * area, CACHE_SIZE);
    CHECKED_MALLOC(gray_down_img1, sizeof(uint8_t) * area, CACHE_SIZE);

    // �ϲ��⵽�ַ����Ž����²�����ķ���
    CLOCK_START(character_segmentation_double_down);

    if ((ch_num0 != 0) || (ch_num1 != 0))
    {
        gray_image_resize_linear_for_center(pgray_img + plate_w * (plate_h_up + 1),
                                            gray_down_img0, plate_w, plate_h_down, resize_down_w, resize_down_h);

        // ��ɫǰ
        if (((*bkgd_color == 255) || (*bkgd_color == 0)) && (ch_num0 != 0))
        {
            seg_down_trust0 = character_segmentation_double_down(gray_down_img0, resize_down_w, resize_down_h,
                                                                 ch_rect0, &ch_num0, chs0,
                                                                 trust0, &reg_trust0, ch_buff0,
                                                                 model_english, model_numbers, model_chinese,
                                                                 &not_ch0, (0u), center_rate0);
        }

        // ��ɫ��
        if (((*bkgd_color == 255) || (*bkgd_color == 1)) && (ch_num1 != 0))
        {
            gray_image_reverse_brightness(gray_down_img1, gray_down_img0, resize_down_w, resize_down_h);

            seg_down_trust1 = character_segmentation_double_down(gray_down_img1, resize_down_w, resize_down_h,
                                                                 ch_rect1, &ch_num1, chs1,
                                                                 trust1, &reg_trust1, ch_buff1,
                                                                 model_english, model_numbers, model_chinese,
                                                                 &not_ch1, (1u), center_rate1);
        }
    }

    CLOCK_END("character_segmentation_double_down");

    if (seg_down_trust0 != (0u))
    {
        tot_trust0 = (uint8_t)((seg_up_trust0 * (3u) + seg_down_trust0 * (5u)) / (8u));
    }
    else
    {
        tot_trust0 = (0u);
    }

    if (seg_down_trust1 != (0u))
    {
        tot_trust1 = (uint8_t)((seg_up_trust1 * (2u) + seg_down_trust1 * (5u)) / (7u));
    }
    else
    {
        tot_trust1 = (0u);
    }

    if (tot_trust0 >= tot_trust1) // �ڵװ���
    {
        // ˫���ƺڵװ��֣�ȷ��Ϊ����
        *bkgd_color = 6;

        if ((not_ch0 >= (4u)) || (tot_trust0 == (0u)))
        {
            *ch_num = 0;
            *plate_trust = 0;
        }
        else
        {
            *ch_num = ch_num0;
            memcpy(chs, chs0, sizeof(chs0));
            memcpy(chs_trust, trust0, sizeof(trust0));
            memcpy(ch_pos, ch_rect0, sizeof(Rects) * MIN2(ch_num0, 8));
            memcpy(ch_buf, ch_buff0, sizeof(uint8_t) * STD_AREA * MIN2(ch_num0, 8));
            *plate_trust = reg_trust0;
        }
    }
    else // �׵׺���
    {
        *bkgd_color = 1;

        if (not_ch1 >= (4u) || (tot_trust1 == (0u)))
        {
            *ch_num = 0;
            *plate_trust = 0;
        }
        else
        {
            *ch_num = ch_num1;
            memcpy(chs, chs1, sizeof(chs1));
            memcpy(chs_trust, trust1, sizeof(trust1));
            memcpy(ch_pos, ch_rect1, sizeof(Rects) * MIN2(ch_num1, 7));
            memcpy(ch_buf, ch_buff1, sizeof(uint8_t) * STD_AREA * MIN2(ch_num1, 7));

            // ��ʱ������˫���ƾ�����Ŀǰ��û��˫���ƾ������������Ұ���GA36-2007�涨������ֻ�е�����
            // ������ֻ�е�����
//             if (*ch_num == 7)
//             {
//                 int32_t trainer_flag = 0;
//                 int32_t police_flag = 0;
//
//                 trainer_flag = yellow_plate_rectifying(gray_img1, resize_w, /*resize_h, */
//                                                        model_xuejing, ch_pos, ch_buf, chs, chs_trust);
//
//                 // ������ǽ�����
//                 if (trainer_flag == 0)
//                 {
//                     // �Ƿ��Ǿ���
//                     police_flag = police_plate_rectifying(reg, gray_img1, resize_w, resize_h,
//                                                           ch_pos, ch_buf, chs, chs_trust);
//
//                     if (police_flag)
//                     {
//                         *bkgd_color = 3; // �׵׾���
//                     }
//                 }
//
// //              else
// //              {
// //                  *bkgd_color = 1; // ��������
// //              }
//             }

            *plate_trust = reg_trust1;
        }
    }

    // ע�⣺��������Ŷ��ж���ʱ��Ѻܶ������ĳ��ƶ��ų�����
    if ((*ch_num == 7 || *ch_num == 8) && (*plate_trust > trust_thresh))
    {
        chs[*ch_num] = (uint8_t)'\0';

        // ���ַ���λ�óߴ�������Ż�ԭ
        for (k = 0; k < *ch_num - 5; k++)             // �ϲ������ַ�
        {
            ch_pos[k].x0 = (int16_t)MAX2(ch_pos[k].x0 * plate_h_up / resize_up_h, 0);
            ch_pos[k].x1 = (int16_t)MIN2(ch_pos[k].x1 * plate_h_up / resize_up_h, plate_w - 1);
            ch_pos[k].y0 = (int16_t)MAX2(ch_pos[k].y0 * plate_h_up / resize_up_h, 0);
            ch_pos[k].y1 = (int16_t)MIN2(ch_pos[k].y1 * plate_h_up / resize_up_h, plate_h - 1);
        }

        for (k = *ch_num - 5; k < *ch_num; k++)       // �²������ַ�
        {
            ch_pos[k].x0 = (int16_t)MAX2(ch_pos[k].x0 * plate_h_down / resize_down_h, 0);
            ch_pos[k].x1 = (int16_t)MIN2(ch_pos[k].x1 * plate_h_down / resize_down_h, plate_w - 1);
            ch_pos[k].y0 = (int16_t)MAX2(ch_pos[k].y0 * plate_h_down / resize_down_h, 0) + plate_h_up;
            ch_pos[k].y1 = (int16_t)MIN2(ch_pos[k].y1 * plate_h_down / resize_down_h, plate_h - 1) + plate_h_up;
        }

        rect_old->x1 = rect_old->x0 + ch_pos[*ch_num - 1].x1;    // �����ұ߽����²������ұ��ַ��ұ߽�Ϊ׼
        rect_old->x0 = rect_old->x0 + ch_pos[*ch_num - 5].x0;    // ������߽����²���������ַ���߽�Ϊ׼
        rect_old->y1 = rect_old->y0 + ch_pos[*ch_num - 5].y1;    // �����±߽����²���������ַ��±߽�Ϊ׼
        rect_old->y0 = rect_old->y0 + ch_pos[0].y0;              // �����ϱ߽����ϲ���������ַ��ϱ߽�Ϊ׼

        *rect_new = *rect_old;

#if DEBUG_DSP_PC_COMPARE
        // ��ӡʶ��ģ�������Ϣ
        {
            int i;
            FILE* fmodule_info_out = fopen("module_info_out.txt", "a+");

            fprintf(fmodule_info_out, "======================================\n");

            for (i = 0; i < *ch_num; i++)
            {
                fprintf(fmodule_info_out, "pos: (%3d, %3d, %3d, %3d)\n",
                        ch_pos[i].x0, ch_pos[i].x1, ch_pos[i].y0, ch_pos[i].y1);
            }

            fprintf(fmodule_info_out, "Rect: (%3d, %3d, %3d, %3d)\n",
                    (*rect_old).x0, (*rect_old).x1, (*rect_old).y0, (*rect_old).y1);
            fprintf(fmodule_info_out, "======================================\n");
            fclose(fmodule_info_out);
        }
#endif

//      if (chs[6] == CH_XUE)
//      {
//          *bkgd_color = 1; // ��������
//      }
//      else if (chs[6] == CH_JING)
//      {
//          *bkgd_color = 3; // �׵׾���
//      }

        if (*bkgd_color == 0) // 0������
        {
            *color_new = 0;
        }
        else if (*bkgd_color == 3) // 3�ǰ׵׾���
        {
            *color_new = 3;
        }
        else if (*bkgd_color == 6) // 6������
        {
            *color_new = 6;
        }
//      else if (*bkgd_color == 1)
//      {
//          *color_new = 1;
//      }
        else // ȷ���ǰ��ƻ��ǻ���
        {
            *color_new = determine_plate_background_color_yellow_or_white(yuv_img, img_w, img_h, *rect_new);
        }

        if (*color_new == 2) // ����ǰ׵ף����ƻ����侯�ƣ�
        {
            jun_wjs_plate_rectifying(gray_down_img1, resize_down_w, /*resize_h, */model_jun_wjs,
                                     ch_rect1, ch_buf, chs, chs_trust);
        }

        *brightness = get_plate_brightness(yuv_img, img_w, /*img_h, */*rect_new);

        if (*ch_num > 0)
        {
            similar_character_classify(reg, ch_buf, chs, chs_trust, *ch_num);
        }

//        set_character_as_unkown_while_no_convinced(chs, chs_trust);

        sum = 0;

        for (k = 0; k < *ch_num; k++)
        {
            sum += (int32_t)(chs_trust[k]);
        }

        *plate_trust = (uint8_t)(sum / *ch_num);

        if (*plate_trust < trust_thresh)
        {
            *ch_num = 0;
        }
    }
    else
    {
        *ch_num = 0;
    }

    CHECKED_FREE(gray_down_img1, sizeof(uint8_t) * area);
    CHECKED_FREE(gray_down_img0, sizeof(uint8_t) * area);

    return;
}

static void similar_character_classify(Recognition *restrict reg, uint8_t *restrict ch_buf,
                                       uint8_t *restrict chs, uint8_t *restrict chs_trust, int32_t ch_num)
{
    int32_t k;
//  int32_t w, h;
    SvmModel *restrict model_0_v00_D = reg->model_0_v00_D;  // �����ַ�0��D���б�ģ��
    SvmModel *restrict model_0_v00_Q = reg->model_0_v00_Q;  // �����ַ�0��Q���б�ģ��
    SvmModel *restrict model_2_v00_Z = reg->model_2_v00_Z;  // �����ַ�2��Z���б�ģ��
    SvmModel *restrict model_5_v00_S = reg->model_5_v00_S;  // �����ַ�5��S���б�ģ��
    SvmModel *restrict model_8_v00_B = reg->model_8_v00_B;  // �����ַ�8��B���б�ģ��
    SvmModel *restrict model_7_v00_T = reg->model_7_v00_T;  // �����ַ�7��T���б�ģ��
    SvmModel *restrict model_0_v01_D = reg->model_0_v01_D;  // �����ַ�0��D���б�ģ��
    SvmModel *restrict model_0_v01_Q = reg->model_0_v01_Q;  // �����ַ�0��Q���б�ģ��
    SvmModel *restrict model_2_v01_Z = reg->model_2_v01_Z;  // �����ַ�2��Z���б�ģ��
    SvmModel *restrict model_5_v01_S = reg->model_5_v01_S;  // �����ַ�5��S���б�ģ��
    SvmModel *restrict model_8_v01_B = reg->model_8_v01_B;  // �����ַ�8��B���б�ģ��
    SvmModel *restrict model_7_v01_T = reg->model_7_v01_T;  // �����ַ�7��T���б�ģ��
    feature_type *restrict feat0 = reg->feat0;
    feature_type *restrict feat1 = reg->feat1;
    uint8_t *restrict std_bina = reg->std_bina;
    uint8_t char0 = (uint8_t)0;
    uint8_t char1 = (uint8_t)0;
    uint8_t is_ch = (uint8_t)0;
    uint8_t trust0 = (uint8_t)0;
    uint8_t trust1 = (uint8_t)0;

    if ((chs[1] == (uint8_t)'D') && (ch_num == 7))
    {
        int32_t count_total0 = 0;
        int32_t count_total1 = 0;

        thresholding_avg_all(ch_buf + STD_AREA * 1, std_bina, STD_W, STD_H);
        character_standardization_by_regional_growth(std_bina, STD_W, STD_H);

        count_total0 += grid_feature_pixels(feat0 + count_total0, std_bina, STD_W, STD_H);
        count_total1 += peripheral_direction_feature(feat1 + count_total1, std_bina, STD_W, STD_H, 1, 1);

        char0 = svm_predict_multiclass(model_0_v00_D, feat0, &trust0, &is_ch);
        char1 = svm_predict_multiclass(model_0_v01_D, feat1, &trust1, &is_ch);

        if (char0 == char1)
        {
            chs[1] = char0;
            chs_trust[1] = (uint8_t)95;
        }
        else
        {
            chs_trust[1] = (uint8_t)50;
        }
    }

    for (k = ch_num - 5; k < ch_num; k++)
    {
        if (chs[k] == (uint8_t)'0' || chs[k] == (uint8_t)'2' || chs[k] == (uint8_t)'5'
            || chs[k] == (uint8_t)'8' || chs[k] == (uint8_t)'7' || chs[k] == (uint8_t)'T')
        {
            int32_t count_total0 = 0;
            int32_t count_total1 = 0;
//            uint8_t trust_tmp = chs_trust[k];

            thresholding_avg_all(ch_buf + STD_AREA * k, std_bina, STD_W, STD_H);
//          thresholding_otsu(ch_buf + STD_AREA * k, std_bina, STD_W, STD_H, 0, STD_W-1, 0, STD_H-1);

            character_standardization_by_regional_growth(std_bina, STD_W, STD_H);

            count_total0 += grid_feature_pixels(feat0 + count_total0, std_bina, STD_W, STD_H);
            count_total1 += peripheral_direction_feature(feat1 + count_total1, std_bina, STD_W, STD_H, 1, 1);

            if (chs[k] == (uint8_t)'0')
            {
                char0 = svm_predict_multiclass(model_0_v00_D, feat0, &trust0, &is_ch);
                char1 = svm_predict_multiclass(model_0_v01_D, feat1, &trust1, &is_ch);

                if (char0 == (uint8_t)'D' && char1 == (uint8_t)'D' /*&& (trust0 + trust1) > 90*2*/)
                {
                    chs[k] = (uint8_t)'D';
                    chs_trust[k] = (uint8_t)95;
                }
                else
                {
                    char0 = svm_predict_multiclass(model_0_v00_Q, feat0, &trust0, &is_ch);
                    char1 = svm_predict_multiclass(model_0_v01_Q, feat1, &trust1, &is_ch);

                    if (char0 == char1 /*&& (trust0 + trust1) > 85*2*/)
                    {
                        chs[k] = char0;
                        chs_trust[k] = (uint8_t)95;
                    }
                    else
                    {
                        chs_trust[k] = (uint8_t)50;
                    }
                }
            }
            else
            {
                if (chs[k] == (uint8_t)'2' || chs[k] == (uint8_t)'Z')
                {
                    char0 = svm_predict_multiclass(model_2_v00_Z, feat0, &trust0, &is_ch);
                    char1 = svm_predict_multiclass(model_2_v01_Z, feat1, &trust1, &is_ch);
                }
                else if (chs[k] == (uint8_t)'5' || chs[k] == (uint8_t)'S')
                {
                    char0 = svm_predict_multiclass(model_5_v00_S, feat0, &trust0, &is_ch);
                    char1 = svm_predict_multiclass(model_5_v01_S, feat1, &trust1, &is_ch);
                }
                else if (chs[k] == (uint8_t)'8' || chs[k] == (uint8_t)'B')
                {
                    char0 = svm_predict_multiclass(model_8_v00_B, feat0, &trust0, &is_ch);
                    char1 = svm_predict_multiclass(model_8_v01_B, feat1, &trust1, &is_ch);
                }
                else if (chs[k] == (uint8_t)'7' || chs[k] == (uint8_t)'T')
                {
                    char0 = svm_predict_multiclass(model_7_v00_T, feat0, &trust0, &is_ch);
                    char1 = svm_predict_multiclass(model_7_v01_T, feat1, &trust1, &is_ch);
                }

                if (char0 == char1)
                {
                    chs[k] = char0;
                    chs_trust[k] = (uint8_t)95;
                }
                else
                {
                    chs_trust[k] = (uint8_t)50;
                }
            }

#if 0
//            if (chs[k] == '0' || chs[k] == '0')
            {
                char gray_name[1024];
                char bina_name[1024];
                int32_t w, h;
                int32_t flag = 0;
                static int32_t frames = 1;
                uint8_t *restrict grayimg = ch_buf + STD_AREA * k;
                IplImage *gray = cvCreateImage(cvSize(STD_W, STD_H), IPL_DEPTH_8U, 1);
                IplImage *bina = cvCreateImage(cvSize(STD_W, STD_H), IPL_DEPTH_8U, 1);

                for (h = 0; h < STD_H; h++)
                {
                    for (w = 0; w < STD_W; w++)
                    {
                        bina->imageData[bina->widthStep * h + w] = std_bina[STD_W * h + w];// == 0 ? 0 : 255;
                    }
                }

                for (h = 0; h < STD_H; h++)
                {
                    for (w = 0; w < STD_W; w++)
                    {
                        gray->imageData[gray->widthStep * h + w] = grayimg[STD_W * h + w];
                    }
                }

//                 if ((char0 != char1) || ((char0 == char1) && (trust0 < 97 || trust1 < 97)))
//                 if (((chs[k] >= '0') && (chs[k] <= '9')) || ((chs[k] >= 'A') && (chs[k] <= 'Z')))
//                 {
//                     sprintf(gray_name, "ppp//%c//t%02d_%02d_%02dimg%04d_%c.bmp", chs[k], trust_tmp, trust0, trust1, frames, chs[k]);
//                     sprintf(bina_name, "ppp//%c//t%02d_%02d_%02dimg%04d_%c.jpg", chs[k], trust_tmp, trust0, trust1, frames, chs[k]);
//                     cvSaveImage(gray_name, gray);
//                     cvSaveImage(bina_name, bina);
//                 }

                frames++;

                cvShowImage("gray", gray);
                cvShowImage("bina", bina);
                cvWaitKey(0);
            }
#endif
        }
    }

    return;
}
#undef FEATURE_NUMS

#if 0
static void set_character_as_unkown_while_no_convinced(uint8_t *restrict chs, uint8_t *restrict chs_trust)
{
    int32_t k = 0;

    for (k = 1; k < 7; k++)
    {
        if (chs_trust[k] < 90)
        {
            chs[k] = '?';
        }
    }

    return;
}
#endif
