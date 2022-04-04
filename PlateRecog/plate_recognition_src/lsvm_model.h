#ifndef _LSVM_MODEL_H_
#define _LSVM_MODEL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    typedef struct
    {
        int32_t nr_class;       // ����������
        uint8_t *label;         // ������
        uint8_t *mask;          // �ַ�ʶ����Ĥ

        svm_type *svs;          // ���е�֧������
        svm_type *rho;          // ƫ��
        svm_type *probAB;       // pairwise probability information

        int32_t dimentions;     // ����ά��
        uint8_t trust_thresh;   // ���Ŷ���ֵ
        uint8_t default_class;  // Ĭ�ϵ����
    } SvmModel;

    void load_svm_model(SvmModel *model, int32_t *svm_coef, uint8_t *mask);

    void destroy_svm_model(SvmModel *model);

#ifdef __cplusplus
};
#endif

#endif // _LSVM_MODEL_H_


