#ifndef _HOG_STANDARDC_H_
#define _HOG_STANDARDC_H_

#include "../portab.h"
#include "../malloc_aligned.h"

#define NSHIFT  (20)

/* wqm 20131107
注意：生成特征用于训练的时候必须将USE_INT_HOG_FEATURE置为0，即采用浮动数据类型来保存特征，
由于用整型数据保存时将特征放大了(1 << NSHIFT)倍，因而训练的时候会产生越界导致训练不准确 */
#define USE_INT_HOG_FEATURE 1 // 将HOG特征转换成整型数据进行运算以加快系统在DSP中的运行速度
#define USE_INTEGRAL_IMAGE  0 // 是否采用积分直方图来计算HOG特征

#define NORMALIZE_L2_Hys 0
#define NORMALIZE_L2_NORM 1
#define NORMALIZE_L1_sqrt 0

typedef uint8_t bins_type;
typedef int32_t integral_type;
typedef int16_t cellfeature_type;

#if USE_INT_HOG_FEATURE
typedef int32_t hogfeature_type;
#else
typedef float hogfeature_type;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MyRect
{
    int x;
    int y;
    int width;
    int height;
}
MyRect;

static MyRect myRect(int x, int y, int width, int height)
{
    MyRect r;

    r.x = x;
    r.y = y;
    r.width = width;
    r.height = height;

    return r;
}

typedef struct MySize
{
    int width;
    int height;
}
MySize;

static MySize mySize(int width, int height)
{
    MySize s;

    s.width = width;
    s.height = height;

    return s;
}

typedef struct MyMat
{
    int type;
    int step;

    /* for internal use only */
    int* refcount;
    int hdr_refcount;

    union
    {
        uint8_t* ptr;
        int16_t* s;
        int* i;
        float* fl;
        double* db;
    } data;

    union
    {
        int rows;
        int height;
    };

    union
    {
        int cols;
        int width;
    };
}
MyMat;


//void save_as_svm_format_c(FILE *fp, hogfeature_type* hog_feature, int hog_feature_len, int label);

int calculate_hog_features_c(uint8_t* img, hogfeature_type* hog_feature, int* hog_feature_len,
							 MySize window, MySize block, MySize blockstride,
							 MySize cell, int bins_num, int normalization,
							 int img_w, int img_h);

int calculate_hog_features_and_detect_c(uint8_t *restrict gray_img, 
										hogfeature_type *restrict hog_feature, 
										int *restrict hog_feature_len,
                                        MySize window, MySize block, MySize blockstride,
                                        MySize cell, int bins_num, int normalization,
                                        int img_w, int img_h,
                                        MyRect* foundLocations, int* found_num);

#ifdef __cplusplus
};
#endif

#endif //_HOG_STANDARDC_H_

