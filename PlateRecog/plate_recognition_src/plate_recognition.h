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

#define CH_MAX   (20)             // 字符的最大数目
#define STD_W    (24)             // 标准训练字符的宽度
#define STD_H    (32)             // 标准训练字符的高度
#define STD_AREA (STD_W * STD_H)  // 标准训练字符的面积

    typedef struct
    {
        SvmModel *model_numbers;  // 数字字母模型
        SvmModel *model_chinese;  // 汉字模型
        SvmModel *model_jun_wjs;  // 军牌汉字模型（包括WJ）
        SvmModel *model_xuejing;  // 学、警、领、港、澳、台、挂、试、超、使
        SvmModel *model_english;

        SvmModel *model_0_v00_D;  // 相似字符0和D的判别模型
        SvmModel *model_0_v00_Q;  // 相似字符0和Q的判别模型
        SvmModel *model_2_v00_Z;  // 相似字符2和Z的判别模型
        SvmModel *model_5_v00_S;  // 相似字符5和S的判别模型
        SvmModel *model_8_v00_B;  // 相似字符8和B的判别模型
        SvmModel *model_7_v00_T;  // 相似字符7和T的判别模型

        SvmModel *model_0_v01_D;  // 相似字符0和D的判别模型
        SvmModel *model_0_v01_Q;  // 相似字符0和Q的判别模型
        SvmModel *model_2_v01_Z;  // 相似字符2和Z的判别模型
        SvmModel *model_5_v01_S;  // 相似字符5和S的判别模型
        SvmModel *model_8_v01_B;  // 相似字符8和B的判别模型
        SvmModel *model_7_v01_T;  // 相似字符7和T的判别模型

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
