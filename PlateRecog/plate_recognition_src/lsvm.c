#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "lsvm.h"
#include "sigmoid.h"
#include "../time_test.h"

#ifdef _TMS320C6X_
#include "IQmath_inline.h"
#endif

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#define NSHIFT    (20)
#define TRUST_MIN (80)

#ifdef WIN32
#define DEBUG_PROB_WRITE 0
#define DEBUG_SVM_WRITE 0
#define DEBUG_SVM_WRITES 0
#endif

typedef float trust_type;

#define CH_CLASS_MAX (40)

#ifdef  _TMS320C6X_
#pragma DATA_SECTION(prob_estimates, ".prob_table")
#pragma DATA_SECTION(pairwise_prob, ".prob_table")
#pragma DATA_SECTION(Q, ".prob_table")
#pragma DATA_SECTION(Qp, ".prob_table")
#pragma DATA_SECTION(p_tmp, ".prob_table")
#pragma DATA_SECTION(pairwise_prob_ptr, ".prob_table")

#pragma DATA_ALIGN(prob_estimates, 128)
#pragma DATA_ALIGN(pairwise_prob, 128)
#pragma DATA_ALIGN(Q, 128)
#pragma DATA_ALIGN(Qp, 128)
#pragma DATA_ALIGN(p_tmp, 128)
#pragma DATA_ALIGN(pairwise_prob_ptr, 128)

_iq9    Qp[CH_CLASS_MAX]; // iq9格式表示Qp, 精度0.001953125,最大范围4194303.998046875
_iq16   p_tmp[CH_CLASS_MAX]; // iq16格式表示p_tmp,精度0.000015259,最大范围32767.999984741
#endif

pro_type prob_estimates[CH_CLASS_MAX];
int16_t pairwise_prob[CH_CLASS_MAX][CH_CLASS_MAX];
int32_t Q[CH_CLASS_MAX][CH_CLASS_MAX];
int16_t *pairwise_prob_ptr[CH_CLASS_MAX * CH_CLASS_MAX];

#ifdef _TMS320C6X_
//#pragma CODE_SECTION(character_1_or_not, ".text:character_1_or_not")
#pragma CODE_SECTION(svm_predict_multiclass_probability, ".text:svm_predict_multiclass_probability")
#pragma CODE_SECTION(svm_predict_multiclass, ".text:svm_predict_multiclass")
#pragma CODE_SECTION(vector_dot_product_m, ".text:vector_dot_product_m")
#pragma CODE_SECTION(sigmoid_predict, ".text:sigmoid_predict")
#pragma CODE_SECTION(get_sigmoid_predict, ".text:get_sigmoid_predict")
#pragma CODE_SECTION(binary_search, ".text:binary_search")
#pragma CODE_SECTION(multiclass_probability, ".text:multiclass_probability")
#pragma CODE_SECTION(get_the_predict_result, ".text:get_the_predict_result")
void touch(const void *array, int32_t length);
#endif

static uint8_t get_the_predict_result(SvmModel *restrict model, int32_t vote_max_idx, uint8_t *restrict trust)
{
    // 注意车牌字符中没有'I'和'O'
    const char alphabet[36] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
        'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
    };

    if (model->label[vote_max_idx] < (uint8_t)36) // 返回字母和数字的ASCII码
    {
        return alphabet[model->label[vote_max_idx]];
    }
    else if (model->label[vote_max_idx] >= (uint8_t)100 && model->label[vote_max_idx] <= (uint8_t)130) // 返回省份汉字汉字的序号
    {
        // 当省份汉字的识别置信度小于一定阈值时设为默认汉字
        if (*trust <= model->trust_thresh)
        {
            return model->default_class;
        }
        else
        {
            return model->label[vote_max_idx];
        }
    }
    else if (model->label[vote_max_idx] >= (uint8_t)131 && model->label[vote_max_idx] <= (uint8_t)142) // 返回军牌汉字（北成广海济军空兰南沈京WJ）的序号
    {
        return model->label[vote_max_idx];
    }
    else // 最后一个字符是汉字（学警领港澳台挂试超使等）时也返回其序号
    {
        return model->label[vote_max_idx];
    }
}

#if 0
// 判断是否是数字'1'
static int32_t character_1_or_not(uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t result;
    int32_t avg_stroke_width;
    int32_t *restrict vertical_hist = NULL;

    CHECKED_MALLOC(vertical_hist, sizeof(int32_t) * img_h, CACHE_SIZE);

    result = 1;
    avg_stroke_width = 0;

    for (h = 0; h < img_h; h++)
    {
        vertical_hist[h] = 0;

        for (w = 0; w < img_w; w++)
        {
            vertical_hist[h] += (bina_img[img_w * h + w] == 0);
        }

        avg_stroke_width += vertical_hist[h];
    }

    avg_stroke_width /= img_h;

    for (h = 0; h < img_h; h++)
    {
        if (vertical_hist[h] - avg_stroke_width > 6)
        {
            result = 0;
            break;
        }
    }

    CHECKED_FREE(vertical_hist);

    return result;
}
#endif

#ifdef WIN321
// svs和feature必须16字节对齐!!
static void vector_dot_product_m(svm_type *restrict svs, feature_type *restrict feature,
                                 svm_type *restrict decision_value, int32_t dimentions, int32_t combination)
{
    _asm
    {
        mov     ecx, combination   // put combination to ecx reg
        mov     eax, svs           // put src addr to eax reg
        mov     edx, decision_value        // put dst addr to edx reg
        outer_loop:
        mov     edi, dimentions    // put dimentions to edi reg
        shr     edi, 2             // divide dimentions with 4 by shifting 2 bits to right
        mov     ebx, feature       // put src2 addr to ebx reg
        pxor    xmm2, xmm2
        inner_loop:
        movaps  xmm0, [eax]
        movaps  xmm1, [ebx]

        mulps   xmm0, xmm1         // a4*b4, a3*b3, a2*b2, a1*b1
        addps   xmm2, xmm0

        add     eax, 16            // add src pointer by 16 bytes
        add     ebx, 16            // add dst pointer by 16 bytes
        dec     edi                // decrement count by 1
        jnz     inner_loop         // jump to inner_loop if not Zero

        // SSE3
//          haddps  xmm2, xmm2         // a4*b4+a3*b3, a2*b2+a1*b1, a4*b4+a3*b3, a2*b2+a1*b1
//          movaps  xmm1, xmm2         // a4*b4+a3*b3, a2*b2+a1*b1, a4*b4+a3*b3, a2*b2+a1*b1
//          psrlq   xmm2, 32           // 0, a4*b4+a3*b3, 0, a4*b4+a3*b3

        // SSE2
        movhlps xmm1, xmm2         // X, X, a4*b4, a3*b3, upper half not needed
        addps   xmm2, xmm1         // X, X, a2*b2+a4*b4, a1*b1+a3*b3,
        pshufd  xmm1, xmm2, 1      // X, X, X, a2*b2+a4*b4

        addss   xmm2, xmm1         // -, -, -, a1*b1+a3*b3+a2*b2+a4*b4
        movss   [edx], xmm2

        add     edx, 4
        dec     ecx
        jnz     outer_loop
    }
}
#elif defined _TMS320C6X_11
// static void vector_dot_product_m(svm_type *restrict svs, feature_type *restrict feature,
//                                  svm_type *restrict decision_value, int32_t dimentions, int32_t combination)
// {
//     int32_t i, j;
//     feature_type *restrict pfeat = feature;
//     svm_type *restrict psvs = svs;
//     svm_type *restrict pdecision_value = decision_value;
//     uint32_t u32SrcA0, u32SrcA1;
//     uint32_t u32SrcA2, u32SrcA3;
//     uint32_t u32SrcB0, u32SrcB1;
//     uint32_t u32Dst00, u32Dst01;
//     uint32_t u32Dst10, u32Dst11;
//     uint32_t u32Dst20, u32Dst21;
//     uint32_t u32Dst30, u32Dst31;
//     svm_type decision_val0;
//     svm_type decision_val1;
//     svm_type decision_val2;
//     svm_type decision_val3;
// #if DEBUG_SVM_WRITES
//     FILE *ftxt = fopen("t.txt", "aw");
//
//     if (ftxt == NULL)
//     {
//         printf("\n Can not open file!\n");
//     }
//
// #endif
//
//     DATA_ALIGNED_8B(psvs);
//     DATA_ALIGNED_8B(pfeat);
//     DATA_ALIGNED_8B(pdecision_value);
//
// #ifdef _TMS320C6X_
//     touch((const void *)pfeat, sizeof(feature_type) * dimentions);
// #endif
//
//     for (i = 0; i < combination; i++)
//     {
//         decision_value[i] = 0;
// #ifdef _TMS320C6X_
//         touch((const void *)psvs, sizeof(svm_type) * dimentions);
// #pragma MUST_ITERATE(48, , 4)
// #endif
//
//         // 注意下面的代码是基于特征的类型是16位有符号数写的
//         for (j = 0; j < dimentions; j += 4)
//         {
//             u32SrcA0 = _amem4(psvs + j + 0);
//             u32SrcA1 = _amem4(psvs + j + 1);
//             u32SrcA2 = _amem4(psvs + j + 2);
//             u32SrcA3 = _amem4(psvs + j + 3);
//
//             u32SrcB0 = _amem4(pfeat + j + 0);
//             u32SrcB1 = _amem4(pfeat + j + 2);
//
//             u32Dst00 = _mpyhl(u32SrcA0, u32SrcB0);
//             u32Dst01 = _mpyus(u32SrcA0, u32SrcB0);
//             u32Dst10 = _mpyh(u32SrcA1, u32SrcB0);
//             u32Dst11 = _mpyluhs(u32SrcA1, u32SrcB0);
//
//             u32Dst20 = _mpyhl(u32SrcA2, u32SrcB1);
//             u32Dst21 = _mpyus(u32SrcA2, u32SrcB1);
//             u32Dst30 = _mpyh(u32SrcA3, u32SrcB1);
//             u32Dst31 = _mpyluhs(u32SrcA3, u32SrcB1);
// #if 0
//             decision_val0 = psvs[j + 0] * pfeat[j + 0];
//             decision_val1 = psvs[j + 1] * pfeat[j + 1];
//             decision_val2 = psvs[j + 2] * pfeat[j + 2];
//             decision_val3 = psvs[j + 3] * pfeat[j + 3];
// #else
//             decision_val0 = (u32Dst00 << 16) + u32Dst01;
//             decision_val1 = (u32Dst10 << 16) + u32Dst11;
//             decision_val2 = (u32Dst20 << 16) + u32Dst21;
//             decision_val3 = (u32Dst30 << 16) + u32Dst31;
// #endif
//             pdecision_value[i] += (decision_val0 + decision_val1 + decision_val2 + decision_val3);
//
// #if DEBUG_SVM_WRITES
//             fprintf(ftxt, "%8d ", decision_val0);
// #endif
//         }
//
// #if DEBUG_SVM_WRITES
//         fprintf(ftxt, "\n");
// #endif
//         psvs += dimentions;
//     }
//
// #if DEBUG_SVM_WRITES
//     fclose(ftxt);
// #endif
//
//     return;
// }
static void vector_dot_product_m(svm_type *restrict svs, feature_type *restrict feature,
                                 svm_type *restrict decision_value, int32_t dimentions, int32_t combination)
{
    int32_t i, j;
    feature_type *restrict pfeat = feature;
    svm_type *restrict psvs0 = svs;
    svm_type *restrict psvs1 = svs + dimentions;
    svm_type *restrict pdecision_value = decision_value;

    DATA_ALIGNED_8B(psvs0);
    DATA_ALIGNED_8B(psvs1);
    DATA_ALIGNED_8B(pfeat);
    DATA_ALIGNED_8B(pdecision_value);

#ifdef _TMS320C6X_
//    touch((const void *)pfeat, sizeof(feature_type) * dimentions);
#endif

    for (i = 0; i < combination - 1; i += 2)
    {
        pdecision_value[i + 0] = 0;
        pdecision_value[i + 1] = 0;

#ifdef _TMS320C6X_
//        touch((const void *)psvs0, sizeof(svm_type) * (dimentions << 1));
#pragma MUST_ITERATE(48, , 4)
#endif

        for (j = 0; j < dimentions; j++)
        {
            pdecision_value[i + 0] += psvs0[j] * pfeat[j];
            pdecision_value[i + 1] += psvs1[j] * pfeat[j];
        }

        psvs0 += (dimentions << 1);
        psvs1 += (dimentions << 1);
    }

    if (combination & 0x1)
    {
        pdecision_value[i + 0] = 0;

#ifdef _TMS320C6X_
//        touch((const void *)psvs0, sizeof(svm_type) * dimentions);
#pragma MUST_ITERATE(48, , 4)
#endif

        for (j = 0; j < dimentions; j++)
        {
            pdecision_value[i + 0] += psvs0[j] * pfeat[j];
        }
    }

    return;
}
#else
static void vector_dot_product_m(svm_type *restrict svs, feature_type *restrict feature,
                                 svm_type *restrict decision_value, int32_t dimentions, int32_t combination)
{
    int32_t i, j;
    feature_type *restrict pfeat = feature;
    svm_type *restrict psvs = svs;
    svm_type *restrict pdecision_value = decision_value;

    DATA_ALIGNED_8B(psvs);
    DATA_ALIGNED_8B(pfeat);
    DATA_ALIGNED_8B(pdecision_value);

#ifdef _TMS320C6X_
//    touch((const void *)pfeat, sizeof(feature_type) * dimentions);
#pragma MUST_ITERATE(12)
//#pragma UNROLL(2)
#endif

    for (i = 0; i < combination; i++)
    {
        pdecision_value[i] = 0;

#ifdef _TMS320C6X_
        /*touch((const void *)psvs, sizeof(svm_type) * dimentions);*/
#pragma MUST_ITERATE(48, , 4)
#endif

        for (j = 0; j < dimentions; j++)
        {
            pdecision_value[i] += psvs[j] * pfeat[j];
        }

        psvs += dimentions;
    }

    return;
}
#endif

#define TABLE_NUMS (999)

#if 0
// 在有序表src[0..n-1]中进行二分查找value值（无法找到value值时返回其左右2节点中的一个，返回的结点值不一定是离value最近的）
static int32_t binary_search(int32_t* src, int32_t value_finding, int32_t n)
{
    // 置当前查找区间上、下界的初值
    int32_t low = 0;
    int32_t high = n - 1;
    int32_t mid = low + (high - low) / 2;

    while (low <= high)
    {
        mid = low + (high - low) / 2;

        // 使用(low + high) / 2会有整数溢出的问题（当low + high的结果大于表达式结果类型所能表示的最大值时），
        // 产生溢出后再/2是不会产生正确结果的，而low + (high - low) / 2不存在这个问题
        if (src[mid] == value_finding)
        {
            return mid; //查找成功返回
        }

        if (src[mid] > value_finding)
        {
            high = mid - 1; // 继续在src[low..mid-1]中查找
        }
        else
        {
            low = mid + 1;  // 继续在src[mid+1..high]中查找
        }
    }

    return mid;
}
#else
static int32_t binary_search(int32_t* src, int32_t value_finding, int32_t n)
{
    // 置当前查找区间上、下界的初值
    int32_t low = 0;
    int32_t high = n - 1;
    int32_t mid = low + (high - low) / 2;
    int32_t i;

    for (i = 0; i < 10; i++)
    {
        if (src[mid] > value_finding)
        {
            high = mid - 1; // 继续在src[low..mid-1]中查找
        }
        else
        {
            low = mid + 1;  // 继续在src[mid+1..high]中查找
        }

        mid = (low + high) / 2;
    }

    return mid;
}
#endif

#if 0
static void get_sigmoid_predict(int16_t *restrict pairwise_prob1[],
                                svm_type *restrict dec_values,
                                svm_type *restrict probAB, const int32_t combination)
{
    int32_t p;

#ifdef _TMS320C6X_
#pragma MUST_ITERATE(12)
#endif

    for (p = 0; p < combination; p++)
    {
        svm_type decision_value = dec_values[p];
        svm_type A = probAB[p * 2 + 0];
        svm_type B = probAB[p * 2 + 1];
        int64_t value_tmp = (int64_t)((int64_t)decision_value * A) / (1 << NSHIFT);
        int32_t value_now = (int32_t)(value_tmp + B);
        int32_t index = 0;

        index = binary_search(g_sigmoid_table, value_now, TABLE_NUMS);

        *(pairwise_prob_ptr[p * 2 + 0]) = g_sigmoid_value[index];
        *(pairwise_prob_ptr[p * 2 + 1]) = g_sigmoid_value[998 - index];
    }

    return;
}
#else
static void get_sigmoid_predict(int16_t *restrict pairwise_prob1[],
                                svm_type *restrict dec_values,
                                svm_type *restrict probAB, const int32_t combination)
{
    int32_t p = 0;
//    int32_t i = 0;
    int64_t value_tmp;
    int32_t value_now;
    int16_t *restrict pairwise_prob_pij = NULL;
    int16_t *restrict pairwise_prob_pji = NULL;
    int32_t low = 0;
    int32_t high = 0;
    int32_t mid = 0;

#ifdef _TMS320C6X_
#pragma MUST_ITERATE(12)
#pragma UNROLL(2)
#endif

    for (p = 0; p < combination; p++)
    {
        value_tmp = (int64_t)((int64_t)dec_values[p] * probAB[p * 2 + 0]) >> NSHIFT;
        value_now = (int32_t)(value_tmp + probAB[p * 2 + 1]);

        low = 0;
        high = TABLE_NUMS - 1;
        mid = low + high / 2;

#if 0

        for (i = 0; i < 10; i++)
        {
            if (g_sigmoid_table[mid] > value_now)
            {
                high = mid - 1; // 继续在src[low..mid-1]中查找
            }
            else
            {
                low = mid + 1;  // 继续在src[mid+1..high]中查找
            }

            mid = (low + high) / 2;
        }

#else
        // modified by Quanming Wang 2012.09.06
        // 在TI DSP中只能对最内层的循环进行软件流水，所以将最内层循环展开后可以更好地在外层循环中实现软件流水
        // 同时，在这个例子中将最内层循环展开后还可以更好地利用A B两侧的资源，具体查看反汇编剖析文件
        // 循环展开前后的性能对比（单位cycles）
        // get_sigmoid_predict() cost ----- 51822 : 27968
        // get_sigmoid_predict() cost ----- 69210 : 30584
        // get_sigmoid_predict() cost ----- 66544 : 27334
        // get_sigmoid_predict() cost ----- 66552 : 27190
        // get_sigmoid_predict() cost ----- 66218 : 27198
        // get_sigmoid_predict() cost ----- 66282 : 27174
        // get_sigmoid_predict() cost ----- 81659 : 37934

        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;
        (g_sigmoid_table[mid] > value_now) ? (high = mid - 1) : (low = mid + 1);
        mid = (low + high) / 2;

#endif

        pairwise_prob_pij = pairwise_prob_ptr[p * 2 + 0];
        pairwise_prob_pji = pairwise_prob_ptr[p * 2 + 1];

        *pairwise_prob_pij = TABLE_NUMS - mid;
        *pairwise_prob_pji = mid + 1;
    }

    return;
}
#endif

static inline void sigmoid_predict(int16_t *pairwise_prob0, int16_t *pairwise_prob1,
                                   svm_type decision_value, svm_type A, svm_type B)
{
#if 1
    int64_t value_tmp = (int64_t)((int64_t)decision_value * A) / (1 << NSHIFT);
    int32_t value_now = (int32_t)(value_tmp + B);
    int32_t index = 0;
#if 0
    // 测试用例
    value_now = -7242272; // index = 0;
    value_now = -7242271; // index = 0;
    value_now = 0;        // index = 500;
    value_now = 7242257;  // index = 998;
    value_now = 7242258;  // index = 998;
    value_now = 6514390;  // index = 997或998;
#endif

    index = binary_search(g_sigmoid_table, value_now, TABLE_NUMS);

    *pairwise_prob0 = g_sigmoid_value[index];
    *pairwise_prob1 = g_sigmoid_value[998 - index];
#else
    pro_type predict_value;
    pro_type min_prob = 1e-7;
    pro_type fApB = ((pro_type)((int64_t)decision_value * A) / (1 << NSHIFT) + B) / (1 << NSHIFT);

    if (fApB >= 0)
    {
        predict_value = exp(-fApB) / (1.0 + exp(-fApB));
    }
    else
    {
        predict_value = 1.0 / (1 + exp(fApB));
    }

    *pairwise_prob0 = MIN2(MAX2(predict_value, min_prob), 1 - min_prob);
    *pairwise_prob1 = 1 - *pairwise_prob0;
#endif

#if 0
    {
        FILE *ftxt = NULL;
        int32_t k = 0;

        ftxt = fopen("ftxt.txt", "w");

        if (ftxt == NULL)
        {
            printf("\ncan not open file !\n");
            return 0;
        }

        printf("\n\n");

        //    fprintf(ftxt, "\n");
//         for (k = TABLE_NUMS; k > 0; k--)
//         {
//             pro_type s = (pro_type)(k) / 1000;
//             pro_type x = log((pro_type)1 / s - 1);
//
//             printf("%3d ", k);
//             //      fprintf(ftxt, "%10f ", x);
//             //fprintf(ftxt, "%8d, ", (int32_t)(x * (1 << 20)));
//             //      fprintf(ftxt, "%5d, ", ((int32_t)(x * (1 << 20)) >> 12));
//             fprintf(ftxt, "%3d, ", k);
//
//             if (k % 10 == 0)
//             {
//                 printf("\n");
//                 fprintf(ftxt, "\n");
//             }
//         }

        for (k = TABLE_NUMS; k > 0; k--)
        {
            pro_type s = (pro_type)(k) / 1000;
            pro_type x = log((pro_type)1 / s - 1);

            printf("%3d ", k);
            //      fprintf(ftxt, "%10f ", x);
            //fprintf(ftxt, "%8d, ", (int32_t)(x * (1 << 20)));
            //      fprintf(ftxt, "%5d, ", ((int32_t)(x * (1 << 20)) >> 12));
            fprintf(ftxt, "%3d, ", k);

            if (k % 10 == 0)
            {
                printf("\n");
                fprintf(ftxt, "\n");
            }
        }

        fclose(ftxt);
    }
#endif

    return;
}

//#ifdef WIN32
#if defined(__GNUC__) || defined(WIN32)
// Method 2 from the multiclass_prob paper by Wu, Lin, and Weng
static void multiclass_probability(int32_t k, int16_t (*r)[CH_CLASS_MAX], pro_type *p)
{
    int32_t t, j;
    int32_t iter = 0;
    int32_t max_iter = MIN2(5, k);
//  int32_t Q[CH_CLASS_MAX][CH_CLASS_MAX];
    pro_type Qp[CH_CLASS_MAX];
    pro_type pQp;
    int32_t eps = (int32_t)(0.005 * 1000 * 1000) / k;

    CLOCK_INITIALIZATION();

#if DEBUG_PROB_WRITE
    static int32_t out_idx = 0;
    FILE *ftxt = fopen("ftxtd.txt", "aw");

    if (ftxt == NULL)
    {
        printf("\ncan not open file !\n");
        return;
    }

#endif

    for (t = 0; t < k; t++)
    {
        p[t] = (pro_type)((1.0 * 1) / k);

        Q[t][t] = 0;

        for (j = 0; j < t; j++)
        {
            Q[t][t] += r[j][t] * r[j][t];
            Q[t][j] = Q[j][t];
        }

        for (j = t + 1; j < k; j++)
        {
            Q[t][t] += r[j][t] * r[j][t];
            Q[t][j] = -r[j][t] * r[t][j];
        }
    }

    for (iter = 0; iter < max_iter; iter++)
    {
        pro_type max_error = 0;

        // stopping condition, recalculate QP, pQP for numerical accuracy
        pQp = 0;

        for (t = 0; t < k; t++)
        {
            Qp[t] = 0;

            for (j = 0; j < k; j++)
            {
                Qp[t] += Q[t][j] * p[j];
            }

            pQp += p[t] * Qp[t];
        }

        for (t = 0; t < k; t++)
        {
            pro_type error = (pro_type)(fabs(Qp[t] - pQp));

            if (error > max_error)
            {
                max_error = error;
            }
        }

        if (max_error < eps)
        {
            break;
        }

        for (t = 0; t < k; t++)
        {
            pro_type diff = (-Qp[t] + pQp) / Q[t][t];

            p[t] += diff;

            pQp = (pQp + diff * (diff * Q[t][t] + 2 * Qp[t])) / (1 + diff) / (1 + diff);

            for (j = 0; j < k; j++)
            {
                Qp[j] = (Qp[j] + diff * Q[t][j]) / (1 + diff);
                p[j] /= (1 + diff);
            }
        }
    }

#if DEBUG_PROB_WRITE
    printf("\niter numbers in multiclass_prob %d", iter);
    fprintf(ftxt, "\n%4d iter numbers in multiclass_prob %d", out_idx++, iter);
#endif

    if (iter >= max_iter)
    {
//        printf("Exceeds max_iter in multiclass_prob\n");
    }

#if DEBUG_PROB_WRITE
    fclose(ftxt);
#endif

    return;
}
#elif defined _TMS320C6X_
static void multiclass_probability(int32_t k, int16_t (*r)[CH_CLASS_MAX], pro_type *p)
{
    int32_t t, j;
    int32_t iter = 0;
    int32_t max_iter = MIN2(2, k);    // 利用IQmath库进行改进，迭代2次效果最接近改进前效果
    _iq9    pQp;                      // iq9格式表示pQp,精度0.001953125,最大范围4194303.998046875
//    int32_t eps = (int32_t)(0.005 * 1000 * 1000) / k;

    CLOCK_INITIALIZATION();

#if DEBUG_PROB_WRITE
    FILE *ftxt = fopen("ftxtd.txt", "aw");

    if (ftxt == NULL)
    {
        printf("\ncan not open file !\n");
        return;
    }

#endif

    CLOCK_START(chinese_frame_adjust);

    for (t = 0; t < k; t++)
    {
        // p_tmp用于保存p的iq16格式数据
        p_tmp[t] = _FtoIQ16(1.0 / k);

        Q[t][t] = 0;

        for (j = 0; j < t; j++)
        {
            Q[t][t] += r[j][t] * r[j][t];
            Q[t][j] = Q[j][t];
        }

        for (j = t + 1; j < k; j++)
        {
            Q[t][t] += r[j][t] * r[j][t];
            Q[t][j] = -r[j][t] * r[t][j];
        }
    }

    for (iter = 0; iter < max_iter; iter++)
    {
        //      pro_type max_error = 0;

        // stopping condition, recalculate QP, pQP for numerical accuracy
        pQp = 0;

        for (t = 0; t < k; t++)
        {
            Qp[t] = 0;

            for (j = 0; j < k; j++)
            {
                Qp[t] += _IQ9mpyIQX(p_tmp[j], 16, _IQ5(Q[t][j]), 5); // Qp[t] += Q[t][j] * p[j];
            }

            pQp += _IQ9mpyIQX(p_tmp[t], 16, Qp[t], 9); // pQp += p[t] * Qp[t];
        }

#if 0   // 只做2次，所以这里不用判断什么时候退出

        for (t = 0; t < k; t++)
        {
//          pro_type error = fabs(Qp[t] - pQp);
            int32_t error = (int32_t)_IQ9toF(_IQ9abs(Qp[t] - pQp));

            if (error >= eps)
            {
                break;
            }
        }

        if (t == k)
        {
            break;
        }

#endif

#ifdef _TMS320C6X_
#pragma MUST_ITERATE(12)
#endif

        for (t = 0; t < k; t++)
        {
            pro_type diff = _IQ9toF(-Qp[t] + pQp) / Q[t][t];
            _iq16 diff_iq = _IQ16(diff);
            _iq30 diff_tmp = _FtoIQ30(1.0 / (1 + diff));

            p_tmp[t] += diff_iq;

//          pQp = (pQp + diff * (pQp + Qp[t])) / (1 + diff) / (1 + diff);
            pQp = _IQ9mpyIQX(_IQ9mpyIQX(pQp, 9, diff_tmp, 30), 9, diff_tmp, 30)
                  + _IQ9mpy(_IQ9mpyIQX(diff_iq, 16, diff_tmp, 30), _IQ9mpyIQX((pQp + Qp[t]), 9, diff_tmp, 30));

#ifdef _TMS320C6X_
#pragma MUST_ITERATE(12)
#endif

            for (j = 0; j < k; j++)
            {
//              Qp[j] = (Qp[j] + diff * Q[t][j]) / (1 + diff);
                Qp[j] = _IQ9mpyIQX(Qp[j], 9, diff_tmp, 30) + _IQ9mpyI32(_IQ9mpyIQX(diff_iq, 16, diff_tmp, 30), Q[t][j]);

//              p[j] /= (1 + diff);
                p_tmp[j] = _IQ16mpyIQX(p_tmp[j], 16, diff_tmp, 30);
            }
        }
    }

    // 将iq16格式的p_tmp转换回浮点格式的p
    for (t = 0; t < k; t++)
    {
        p[t] = _IQ16toF(p_tmp[t]);
    }

#if DEBUG_PROB_WRITE
//    printf("\niter numbers in multiclass_prob %d", iter);
    fprintf(ftxt, "\niter numbers in multiclass_prob %d", iter);
#endif

    if (iter >= max_iter)
    {
//        printf("Exceeds max_iter in multiclass_prob\n");
    }

#if DEBUG_PROB_WRITE
    fclose(ftxt);
#endif

    return;
}
#endif

uint8_t svm_predict_multiclass_probability(SvmModel *restrict model, feature_type *restrict feat,
                                           uint8_t *restrict trust, uint8_t *restrict is_ch)
{
    int32_t i, j;
    int32_t p = 0;
    int32_t nr_class = model->nr_class;
    int32_t dimentions = model->dimentions;
    const int32_t combination = nr_class * (nr_class - 1) / 2;
    uint8_t *restrict mask = model->mask;
    svm_type *restrict probAB = model->probAB;
    svm_type *restrict rho = model->rho;
    svm_type *restrict dec_values = NULL;
    int16_t *restrict vote = NULL;
    int32_t vote_max_idx = 0;
    uint8_t predict_result = (uint8_t)0;

    CLOCK_INITIALIZATION();

#if DEBUG_SVM_WRITE
    FILE *ftxt = fopen("t.txt", "aw");

    if (ftxt == NULL)
    {
        printf("\n Can not open file!\n");
    }

#endif

#ifdef _TMS320C6X_
    vote = (int16_t *)MEM_alloc(IRAM, sizeof(int16_t) * nr_class, CACHE_SIZE);
    dec_values = (svm_type *)MEM_alloc(IRAM, sizeof(svm_type) * combination, CACHE_SIZE);
#else
    CHECKED_MALLOC(vote, sizeof(int16_t) * nr_class, CACHE_SIZE);
    CHECKED_MALLOC(dec_values, sizeof(svm_type) * combination, CACHE_SIZE);
#endif

    memset(vote, 0, sizeof(int16_t) * nr_class);
    memset(pairwise_prob, 0, sizeof(pairwise_prob));

    *is_ch = 1;

    vector_dot_product_m(model->svs, feat, dec_values, dimentions, combination);

#ifdef _TMS320C6X_1
    printf("nr_class is                   : %d\n", nr_class);
    printf("dimentions is                 : %d\n", dimentions);
    printf("combination is                : %d\n", combination);
    printf("vector_dot_product_m() cost   : %d\n", time);
#endif

#if DEBUG_SVM_WRITE
//    printf("\n");
    fprintf(ftxt, "\n");
#endif

#if 0

    for (i = 0; i < nr_class - 1; i++)
    {
        for (j = i + 1; j < nr_class; j++)
        {
            dec_values[p] -= rho[p];

            sigmoid_predict(&pairwise_prob[i][j], &pairwise_prob[j][i],
                            dec_values[p], probAB[p * 2 + 0], probAB[p * 2 + 1]);

            if (dec_values[p] > 0)
            {
                vote[i]++;
            }
            else
            {
                vote[j]++;
            }

            p++;
        }
    }

#else

    p = 0;

    for (i = 0; i < nr_class - 1; i++)
    {
        for (j = i + 1; j < nr_class; j++)
        {
            dec_values[p] -= rho[p];

            if (dec_values[p] > 0)
            {
                vote[i]++;
            }
            else
            {
                vote[j]++;
            }

            pairwise_prob_ptr[p * 2 + 0] = &pairwise_prob[i][j];
            pairwise_prob_ptr[p * 2 + 1] = &pairwise_prob[j][i];

            p++;
        }
    }

    get_sigmoid_predict(pairwise_prob_ptr, dec_values, probAB, combination);

#endif

#if DEBUG_SVM_WRITE
    fprintf(ftxt, "\n");

    for (i = 0; i < nr_class; i++)
    {
        fprintf(ftxt, "%5d ", i);
    }

    fprintf(ftxt, "\n");

    for (i = 0; i < nr_class; i++)
    {
        float prob_sum = 0;
        fprintf(ftxt, "%2d ", i);

        for (j = 0; j < nr_class; j++)
        {
            fprintf(ftxt, "%3d ", pairwise_prob[i][j]);
            prob_sum += pairwise_prob[i][j];
        }

        fprintf(ftxt, "----%.3f\n", prob_sum / (nr_class - 1));
    }

    fclose(ftxt);
#endif

    multiclass_probability(nr_class, pairwise_prob, prob_estimates);

    vote_max_idx = 0;

    for (i = 0; i < nr_class; i++)
    {
        if (mask[i] == (uint8_t)0)
        {
            vote_max_idx = i++;
            break;
        }
    }

#if 1

    // 根据投票数目决定划分为哪一类
    for (; i < nr_class; i++)
    {
        if ((vote[i] > vote[vote_max_idx]) && (mask[i] == (uint8_t)0))
        {
            vote_max_idx = i;
        }
    }

#else

    // 根据识别置信度的高低决定划分为哪一类
    for (; i < nr_class; i++)
    {
        if ((prob_estimates[i] > prob_estimates[vote_max_idx]) && (mask[i] == (uint8_t)0))
        {
            vote_max_idx = i;
        }
    }

#endif

    *trust = (uint8_t)(prob_estimates[vote_max_idx] * 100);

    // 对置信度作[0 100]的限定
    *trust = MIN2(MAX2(*trust, (0u)), (100u));

    // 识别置信度小于一定值时认为是非字符
    if (*trust < TRUST_MIN)
    {
        *is_ch = 0;
    }

#ifdef _TMS320C6X_
    MEM_free(IRAM, vote, sizeof(int16_t) * nr_class);
    MEM_free(IRAM, dec_values, sizeof(svm_type) * combination);
#else
    CHECKED_FREE(dec_values, sizeof(svm_type) * combination);
    CHECKED_FREE(vote, sizeof(int16_t) * nr_class);
#endif

    predict_result = get_the_predict_result(model, vote_max_idx, trust);

    return predict_result;
}

// SVM预测主函数---采用one-to-one方式进行多分类
uint8_t svm_predict_multiclass(SvmModel *restrict model, feature_type *restrict feat,
                               uint8_t *restrict trust, uint8_t *restrict is_ch)
{
    int32_t i, j;
    int32_t p = 0;
    int32_t vote_max_idx;
    int32_t nr_class = model->nr_class;
    int32_t dimentions = model->dimentions;
    const int32_t combination = nr_class * (nr_class - 1) / 2;
    uint8_t *restrict mask = model->mask;
    int16_t *restrict vote = NULL;
    svm_type *restrict dec_values = NULL;
    trust_type *restrict trust_tab = NULL;
    trust_type r;
    uint8_t predict_result = (uint8_t)'0';

    CLOCK_INITIALIZATION();

#ifdef _TMS320C6X_
    vote = (int16_t *)MEM_alloc(IRAM, sizeof(int16_t) * nr_class, CACHE_SIZE);
    trust_tab = (trust_type *)MEM_alloc(IRAM, sizeof(trust_type) * nr_class, CACHE_SIZE);
    dec_values = (svm_type *)MEM_alloc(IRAM, sizeof(svm_type) * combination, CACHE_SIZE);
#else
    CHECKED_MALLOC(vote, sizeof(int16_t) * nr_class, CACHE_SIZE);
    CHECKED_MALLOC(trust_tab, sizeof(trust_type) * nr_class, CACHE_SIZE);
    CHECKED_MALLOC(dec_values, sizeof(svm_type) * combination, CACHE_SIZE);
#endif

    memset(vote, 0, sizeof(int16_t) * nr_class);
    memset(trust_tab, 0, sizeof(trust_type) * nr_class);

    *is_ch = 1;

    vector_dot_product_m(model->svs, feat, dec_values, dimentions, combination);

#ifdef _TMS320C6X_1
    printf("\n nr_class is                : %d", nr_class);
    printf("\n dimentions is              : %d", dimentions);
    printf("\n combination is             : %d", combination);
    printf("\n vector_dot_product_m() cost: %d\n", time);
#endif

    p = 0;

    for (i = 0; i < nr_class - 1; i++)
    {
        for (j = i + 1; j < nr_class; j++)
        {
            dec_values[p] -= model->rho[p];

            if (dec_values[p] > 0)
            {
                vote[i]++;
                trust_tab[i] += dec_values[p];
            }
            else
            {
                vote[j]++;
                trust_tab[j] += (-dec_values[p]);
            }

            p++;
        }
    }

    // 根据投票数目决定划分为哪一类
    vote_max_idx = 0;

    for (i = 0; i < nr_class; i++)
    {
        if (mask[i] == (uint8_t)0)
        {
            vote_max_idx = i++;
            break;
        }
    }

    for (; i < nr_class; i++)
    {
        if ((vote[i] > vote[vote_max_idx]) && (mask[i] == (uint8_t)0))
        {
            vote_max_idx = i;
        }
    }

    if (vote[vote_max_idx] > 0)
    {
        r = trust_tab[vote_max_idx] / (trust_type)(vote[vote_max_idx] * (1 << NSHIFT));
        *trust = (uint8_t)(100 * (1 - (trust_type)exp(-r)) / (1 + (trust_type)exp(-r)));
    }
    else
    {
        *trust = 0;
    }

    // 识别置信度小于一定值时认为是非字符
    if (*trust < 50)
    {
        *is_ch = 0;
    }

#ifdef _TMS320C6X_
    MEM_free(IRAM, vote, sizeof(int16_t) * nr_class);
    MEM_free(IRAM, trust_tab, sizeof(trust_type) * nr_class);
    MEM_free(IRAM, dec_values, sizeof(svm_type) * combination);
#else
    CHECKED_FREE(dec_values, sizeof(svm_type) * combination);
    CHECKED_FREE(trust_tab, sizeof(trust_type) * nr_class);
    CHECKED_FREE(vote, sizeof(int16_t) * nr_class);
#endif

    predict_result = get_the_predict_result(model, vote_max_idx, trust);

    return predict_result;
}

#undef DEBUG_SVM_WRITE
#undef DEBUG_SVM_WRITES
#undef CH_CLASS_MAX
#undef TABLE_NUMS
