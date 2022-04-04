#ifndef _PLATE_RUNS_H_
#define _PLATE_RUNS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"
#include "../location_interface.h"
#include "plate_location.h"
    
typedef struct      // ��������
{
    int16_t start;     // �е���ʼֵ
    int16_t end;       // �е���ֵֹ
    int16_t row;       // ����
}PlateRun;

typedef struct nodelink // ��������
{
    int16_t start;     // �е���ʼֵ
    int16_t end;       // �е���ֵֹ
    struct nodelink *next;
}PlateRunLink;

typedef struct      // ��������
{
    int16_t row;       // �����������ڵ���
    PlateRunLink *head;
    PlateRun *p_head;
    int16_t p_num;      // ���е�������
}PlateRunVecs;

void find_plate_candidate_by_plate_runs(PlateLocation *restrict loc,
                                        uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                        int32_t img_w, int32_t img_h, int32_t rect_w, int32_t rect_h, Rects rect_detect,
                                        Rects *restrict rect_cand, int32_t *rect_num);

#ifdef __cplusplus
};
#endif

#endif
