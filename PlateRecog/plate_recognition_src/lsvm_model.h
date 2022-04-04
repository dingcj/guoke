#ifndef _LSVM_MODEL_H_
#define _LSVM_MODEL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

    typedef struct
    {
        int32_t nr_class;       // 分类类别个数
        uint8_t *label;         // 分类标号
        uint8_t *mask;          // 字符识别掩膜

        svm_type *svs;          // 所有的支撑向量
        svm_type *rho;          // 偏置
        svm_type *probAB;       // pairwise probability information

        int32_t dimentions;     // 特征维数
        uint8_t trust_thresh;   // 置信度阈值
        uint8_t default_class;  // 默认的类别
    } SvmModel;

    void load_svm_model(SvmModel *model, int32_t *svm_coef, uint8_t *mask);

    void destroy_svm_model(SvmModel *model);

#ifdef __cplusplus
};
#endif

#endif // _LSVM_MODEL_H_


