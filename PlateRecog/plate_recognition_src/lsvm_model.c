#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../malloc_aligned.h"
#include "lsvm_model.h"

#ifdef _TMS320C6X_
#pragma CODE_SECTION(load_svm_model, ".text:load_svm_model")
#pragma CODE_SECTION(destroy_svm_model, ".text:destroy_svm_model")
#endif

void load_svm_model(SvmModel *model, int32_t *svm_coef, uint8_t *mask)
{
    int32_t i = 0;
    int32_t index = 0;
    int32_t nr_class = 0;

    // �����Ŀ
    model->nr_class = svm_coef[index++];
    // ����ά��
    model->dimentions = svm_coef[index++];

    nr_class = model->nr_class;

    // ÿһ��ı��
    CHECKED_MALLOC(model->label, nr_class, CACHE_SIZE);

    for (i = 0; i < nr_class; i++)
    {
        model->label[i] = (uint8_t)svm_coef[index++];
    }

    CHECKED_MALLOC(model->mask, sizeof(uint8_t) * nr_class, CACHE_SIZE);
    memcpy(model->mask, mask, nr_class);

    model->trust_thresh  = (uint8_t)75;     // �ַ�ʶ��Ľ��󣬽����ֵ�ʶ�����Ŷȷ�ֵ��50����Ϊ75
    model->default_class = (uint8_t)100;

    return;
}

void destroy_svm_model(SvmModel *model)
{
    int32_t nr_class = model->nr_class;

    CHECKED_FREE(model->label, sizeof(uint8_t) * nr_class);
    CHECKED_FREE(model->mask, sizeof(uint8_t) * nr_class);

    return;
}
