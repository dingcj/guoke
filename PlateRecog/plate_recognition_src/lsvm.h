#ifndef _LSVM_H_
#define _LSVM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"
#include "lsvm_model.h"

    uint8_t svm_predict_multiclass_probability(SvmModel *restrict model, feature_type *restrict feat,
                                               uint8_t *restrict trust, uint8_t *restrict is_ch);

    uint8_t svm_predict_multiclass(SvmModel *restrict model, feature_type *restrict feat,
                                   uint8_t *restrict trust, uint8_t *restrict is_ch);

#ifdef __cplusplus
};
#endif

#endif
