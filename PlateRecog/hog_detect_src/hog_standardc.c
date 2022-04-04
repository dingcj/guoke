#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "plate_64x32_standardc.h"

#include "../time_test.h"
#include "../malloc_aligned.h"

#include "hog_standardc.h"
#include "integral_hist_list.h"

#ifdef  WIN32
#include "..\ti64_c\cintrins.h"
#include "..\ti64_c\c64intrins.h"
#endif

#define THE_SAME_ORDER_AS_OPENCV 1   // 采用跟OPENCV相同的顺序排列HOG特征

#if 0
typedef union My32suf
{
    int i;
    unsigned u;
    float f;
}
My32suf;

int myFloor(double value)
{
    int temp = round(value);
    My32suf diff;

    diff.f = (float)(value - temp);

    return temp - (diff.i < 0);
}
#endif

/* brief:存储hog特征向量
*  param[in]: fp 存储文件路径
*  param[in]: descriptors 特征向量
*  param[in]: label 样本标签
*/
#ifdef WIN32
static void save_as_svm_format_c(FILE *fp, hogfeature_type* hog_feature, int hog_feature_len, int label)
{
    int cnt = 0;

    fprintf(fp, "%d ", label);

    for (cnt = 0; cnt < hog_feature_len; cnt++)
    {
        fprintf(fp, "%d:%f ", cnt + 1, (float)hog_feature[cnt]);
    }

    fprintf(fp, "\n");

    return;
}
#endif

#define MY_PI (3.1415926535897932384626433832795)
#define MY_DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */

/*
static const float atan2_p1 = 0.9997878412794807f * (float)(180 / MY_PI);
static const float atan2_p3 = -0.3258083974640975f * (float)(180 / MY_PI);
static const float atan2_p5 = 0.1555786518463281f * (float)(180 / MY_PI);
static const float atan2_p7 = -0.04432655554792128f * (float)(180 / MY_PI);

static float fastAtan2_1(float y, float x, int angleInDegrees)
{
    float scale = angleInDegrees ? 1 : (float)(MY_PI / 180);
    float ax = abs(x);
    float ay = abs(y);
    float a, c, c2;

    if (ax >= ay)
    {
        c = ay / (ax + (float)MY_DBL_EPSILON);
        c2 = c * c;
        a = (((atan2_p7 * c2 + atan2_p5) * c2 + atan2_p3) * c2 + atan2_p1) * c;
    }
    else
    {
        c = ax / (ay + (float)MY_DBL_EPSILON);
        c2 = c * c;
        a = 90.f - (((atan2_p7 * c2 + atan2_p5) * c2 + atan2_p3) * c2 + atan2_p1) * c;
    }

    if (x < 0)
    {
        a = 180.f - a;
    }

    if (y < 0)
    {
        a = 360.f - a;
    }

    return (float)(a * scale);
}
*/

#if USE_INTEGRAL_IMAGE
#if 0
static void cvIntegral_c(uint8_t *restrict src, integral_type *restrict sum, int img_w, int img_h)
{
    int x, y, k;
    int srcstep = img_w;
    int sumstep = img_w + 1;
    int cn = 1;

    memset(sum, 0, (img_w + cn) * sizeof(sum[0]));
    sum += sumstep + cn;

    for (y = 0; y < img_h; y++, src += srcstep - cn, sum += sumstep - cn)
    {
        for (k = 0; k < cn; k++, src++, sum++)
        {
            integral_type s = sum[-cn] = 0;

            for (x = 0; x < img_w; x += cn)
            {
                s += src[x];
                sum[x] = sum[x - sumstep] + s;
            }
        }
    }

    return;
}
#else
static void cvIntegral_c(uint8_t *restrict src, integral_type *restrict sum, int img_w, int img_h)
{
    int x, y, k;
    int srcstep = img_w;
    int sumstep = img_w + 1;
    int cn = 1;
    integral_type *restrict sum_up = sum + cn;

    memset(sum, 0, (img_w + cn) * sizeof(sum[0]));
    sum += sumstep + cn;

    for (y = 0; y < img_h; y++, src += srcstep, sum += sumstep, sum_up += sumstep)
    {
        integral_type s = 0;
        sum[-cn] = 0;

        for (x = 0; x < img_w; x += cn)
        {
            s += src[x];
            sum[x] = sum_up[x] + s;
        }
    }

    return;
}
#endif

/* Function to calculate the integral histogram */
static int calculate_integral_histogram_c(uint8_t *restrict gray_img, integral_type** integrals,
                                          int bins_num, int img_w, int img_h)
{
    int i = 0;
    int x, y;
    int w, h;
    float magnitude = 0;
    float orientation = 0;
    float angleScale;
    float angle;
    int hidx0;
    int hidx1;
//    FILE * ftxt = fopen("my.txt", "w");
    /* Calculate the the x and y directions derivate of the gray scale image using a Sobel operator and
    obtain 2 gradient images for the x and y directions */
    int16_t *restrict sobelx16 = NULL;
    int16_t *restrict sobely16 = NULL;
    /* ptrb point to the beginning of the current rows color_img the bin images */
    bins_type** ptrb = (bins_type**)malloc(sizeof(bins_type*) * bins_num);
    /* Create an array of 9 images(9 because I assume bin size 20 degrees and unsigned gradient( 180/20 = 9),
    one for each bin which will have zeros for all pixels, except for the pixels color_img the original image for which
    the gradient values correspond to the particular bin. These will be referred to as bin images. These bin images
    will be then used to calculate the integral histogram, which will quicken the calculation of HOG descriptors */
    bins_type** bins_img = (bins_type**)malloc(sizeof(bins_type*) * bins_num);

    for (i = 0; i < bins_num; i++)
    {
        CHECKED_MALLOC(bins_img[i], img_w * img_h * sizeof(bins_type), 64);
        memset(bins_img[i], 0, img_w * img_h * sizeof(bins_type));
    }

    CHECKED_MALLOC(sobelx16, img_w * img_h * sizeof(int16_t), 64);
    CHECKED_MALLOC(sobely16, img_w * img_h * sizeof(int16_t), 64);

    memset(sobelx16, 0, img_w * img_h * sizeof(int16_t));
    memset(sobely16, 0, img_w * img_h * sizeof(int16_t));


//    cvEqualizeHist(gray_img, gray_img); // 这个操作对识别率没什么提升

    for (y = 1; y < img_h - 1; y++)
    {
        const uint8_t *restrict ptr_gray = (uint8_t*)(gray_img + img_w * y);
        const uint8_t *restrict ptr_prev = (uint8_t*)(gray_img + img_w * (y - 1));
        const uint8_t *restrict ptr_next = (uint8_t*)(gray_img + img_w * (y + 1));
        int16_t *restrict ptr_sobelx16 = (int16_t*)(sobelx16 + img_w * y);
        int16_t *restrict ptr_sobely16 = (int16_t*)(sobely16 + img_w * y);

        for (x = 1; x < img_w - 1; x++)
        {
            ptr_sobelx16[x] = (int16_t)((int16_t)ptr_gray[x + 1] - (int16_t)ptr_gray[x - 1]);
            ptr_sobely16[x] = (int16_t)((int16_t)ptr_next[x] - (int16_t)ptr_prev[x]);
        }
    }

    /* Calculate the bin images. The magnitude and orientation of the gradient at each pixel is calculated using
    the sobelx and sobely images.{Magnitude = sqrt(sq(sobelx) + sq(sobely)), gradient = itan(sobely/sobelx)}.
    Then according to the orientation of the gradient, the value of the corresponding pixel color_img the corresponding
    image is set */
    for (h = 0; h < img_h; h++)
    {
        /* ptrx and ptry point to beginning of the current row color_img the sobelx and sobely images respectively */
        int16_t *restrict ptrx = (int16_t*)(sobelx16 + img_w * h);
        int16_t *restrict ptry = (int16_t*)(sobely16 + img_w * h);

        for (i = 0; i < bins_num; i++)
        {
            ptrb[i] = (bins_type*)(bins_img[i] + img_w * h);
        }

        /* For every pixel's orientation and magnitude of gradient are calculated and corresponding
        values set for the bin images. */
        for (w = 0; w < img_w; w++)
        {
#if 1
            // 采用查表的方式以加快在DSP上的运行速度
            int32_t ptr_pix = (ptry[w] + 255) * 511 + ptrx[w] + 255;
            ptrb[integ_hist_list3[ptr_pix]][w] = integ_hist_list1[ptr_pix];
            ptrb[integ_hist_list4[ptr_pix]][w] = integ_hist_list2[ptr_pix];
#else
            magnitude = sqrt((int)(ptrx[w] * ptrx[w]) + (int)(ptry[w] * ptry[w]));
            /*magnitude = abs(ptrx[w]) + abs(ptry[w]);*/

            /* if the sobelx derivative is zero for a pixel, a small value is added to it to avoid division
            by zero. atan returns values of radians, which will being converted to degrees, correspond
            to values between -90 and 90 degrees. 90 is added to each orientation, to shift the orientation
            values range from {-90 ~ 90} to {0 ~ 180}.  */
#if 0

            if (ptrx[w] == 0)
            {
                if (ptry[w] == 0)
                {
                    orientation = 0;
                }
                else
                {
                    orientation = (atan(ptry[w] / (float)(ptrx[w] + 0.00001)) + PI / 2);
                }
            }
            else
            {
                orientation = (atan(ptry[w] / (float)ptrx[w]) + PI / 2);
            }

#else
            orientation = fastAtan2_1((float)ptry[w], (float)ptrx[w], 0);
#endif

//             if (orientation0 != orientation)
//             {
//                 printf("\n\n");
//             }
//
//             orientation = orientation0;

            angleScale = (float)(bins_num / MY_PI);
            angle = orientation * angleScale - 0.5f;
            hidx0 = floor(angle);
            hidx1 = 0;

            angle -= hidx0;

            if (hidx0 < 0)
            {
                hidx0 += bins_num;
            }
            else if (hidx0 >= bins_num)
            {
                hidx0 -= bins_num;
            }

            assert((unsigned)hidx0 < (unsigned)bins_num);

            hidx1 = hidx0 + 1;
            hidx1 &= (hidx1 < bins_num ? -1 : 0);

            /* The bin image is selected according to the orientation values. */
            ptrb[hidx0][w] = (bins_type)(magnitude * (1.f - angle));
            ptrb[hidx1][w] = (bins_type)(magnitude * angle);

//             if ((w + 0) % 64 == 0)
//             {
//                 fprintf(ftxt, "\n");
//             }
//
//             fprintf(ftxt, "%2d ", hidx0);
//             fprintf(ftxt, "%2d ", hidx1);
//
//             fprintf(ftxt, "%7.3f ", (float)ptrb[hidx0][w]);
//             fprintf(ftxt, "%7.3f ", (float)ptrb[hidx1][w]);


//             if (w < 16 && h < 16)
//             {
//                 if ((w + 0) % 16 == 0)
//                 {
//                     fprintf(ftxt, "\n");
//                 }
//
//                 fprintf(ftxt, "%2d ", hidx0);
//                 fprintf(ftxt, "%2d ", hidx1);
//
//                 fprintf(ftxt, "%7.3f ", (float)ptrb[hidx0][w]);
//                 fprintf(ftxt, "%7.3f ", (float)ptrb[hidx1][w]);
//             }
//
//          fprintf(ftxt, "\n ptrb=======================================\n");

            assert(hidx0 != hidx1);
#endif
//             if (w < 16 && h < 16)
//             {
//                 if ((w + 0) % 16 == 0)
//                 {
//                     fprintf(ftxt, "\n");
//                 }
//
//                 if (hidx0 == 0)
//                 {
//                     fprintf(ftxt, "%7.3f ", (float)ptrb[hidx0][w]);
//                 }
//                 else if (hidx1 == 0)
//                 {
//                     fprintf(ftxt, "%7.3f ", (float)ptrb[hidx1][w]);
//                 }
//                 else
//                 {
//                     fprintf(ftxt, "%7.3f ", 0.f);
//                 }
//             }
        }
    }

    free(ptrb);

    /* Integral images for each of the bin images are calculated */
    for (i = 0; i < bins_num; i++)
    {
        cvIntegral_c(bins_img[i], integrals[i], img_w, img_h);
    }

    for (i = 0; i < bins_num; i++)
    {
        CHECKED_FREE(bins_img[i], img_w * img_h * sizeof(bins_type));
    }

    free(bins_img);

    CHECKED_FREE(sobelx16, img_w * img_h * sizeof(int16_t));
    CHECKED_FREE(sobely16, img_w * img_h * sizeof(int16_t));

//    fclose(ftxt);

    /* The function returns an array of 9 images which constitute the integral histogram */
    return 0;
}
#endif

/* Function to calculate the integral histogram */
static int calculate_cell_featrue_image_c(uint8_t *restrict gray_img, cellfeature_type *restrict cell_feature,
                                          int cell_w, int cell_h,
                                          int bins_num, int img_w, int img_h)
{
    int i = 0;
    int x, y;
    int w, h;
    int integ_list_tmp[9] = {1, 2, 3, 4, 5, 6, 7, 8, 0};
//    FILE * ftxt = fopen("my.txt", "w");
    /* Calculate the the x and y directions derivate of the gray scale image using a Sobel operator and
    obtain 2 gradient images for the x and y directions */
    int16_t *restrict sobelx16 = NULL;
    int16_t *restrict sobely16 = NULL;

    CHECKED_MALLOC(sobelx16, img_w * img_h * sizeof(int16_t), 64);
    CHECKED_MALLOC(sobely16, img_w * img_h * sizeof(int16_t), 64);

    memset(cell_feature, 0, (img_w / cell_w) * (img_h / cell_h) * bins_num * sizeof(cellfeature_type));

//    cvEqualizeHist(gray_img, gray_img); // 这个操作对识别率没什么提升

#if 1
    memset(sobelx16, 0, img_w * img_h * sizeof(int16_t));
    memset(sobely16, 0, img_w * img_h * sizeof(int16_t));

    for (y = 1; y < img_h - 1; y++)
    {
        const uint8_t *restrict ptr_gray = (uint8_t*)(gray_img + img_w * y);
        const uint8_t *restrict ptr_prev = (uint8_t*)(gray_img + img_w * (y - 1));
        const uint8_t *restrict ptr_next = (uint8_t*)(gray_img + img_w * (y + 1));
        int16_t *restrict ptr_sobelx16 = (int16_t*)(sobelx16 + img_w * y);
        int16_t *restrict ptr_sobely16 = (int16_t*)(sobely16 + img_w * y);

        for (x = 1; x < img_w - 1; x++)
        {
            ptr_sobelx16[x] = (int16_t)((int16_t)ptr_gray[x + 1] - (int16_t)ptr_gray[x - 1]);
            ptr_sobely16[x] = (int16_t)((int16_t)ptr_next[x] - (int16_t)ptr_prev[x]);
        }
    }

#else
    // wqm 20131112
    {
        const int32_t area = img_w * img_h;
        const int32_t count = (area >> 4) - 1;
        uint8_t *restrict A_src = gray_img + 2;
        uint8_t *restrict B_src = gray_img + 0;
        int16_t *restrict A_dst = sobelx16 + 1;
        uint32_t u32SrcA0, u32SrcA1, u32SrcA2, u32SrcA3;
        uint32_t u32SrcB0, u32SrcB1, u32SrcB2, u32SrcB3;
        double u64TmpA0, u64TmpA1, u64TmpA2, u64TmpA3;
        double u64TmpB0, u64TmpB1, u64TmpB2, u64TmpB3;
        uint32_t u32Dst00, u32Dst01, u32Dst02, u32Dst03, u32Dst04, u32Dst05, u32Dst06, u32Dst07;

        for (i = 0; i < count; i++)
        {
            // .reg u32SrcAH:u32SrcAL;   |x|x|x|x|x|x|x|x| | |
            // .reg u32SrcBH:u32SrcBL;   | | |x|x|x|x|x|x|x|x|
            u32SrcA0 = _mem4(A_src + (i << 4) +  0);
            u32SrcA1 = _mem4(A_src + (i << 4) +  4);
            u32SrcA2 = _mem4(A_src + (i << 4) +  8);
            u32SrcA3 = _mem4(A_src + (i << 4) + 12);

            u32SrcB0 =  _mem4(B_src + (i << 4) +  0);
            u32SrcB1 =  _mem4(B_src + (i << 4) +  4);
            u32SrcB2 =  _mem4(B_src + (i << 4) +  8);
            u32SrcB3 =  _mem4(B_src + (i << 4) + 12);

            u64TmpA0 = _mpyu4(u32SrcA0, 0x01010101);
            u64TmpA1 = _mpyu4(u32SrcA1, 0x01010101);
            u64TmpA2 = _mpyu4(u32SrcA2, 0x01010101);
            u64TmpA3 = _mpyu4(u32SrcA3, 0x01010101);

            u64TmpB0 = _mpyu4(u32SrcB0, 0x01010101);
            u64TmpB1 = _mpyu4(u32SrcB1, 0x01010101);
            u64TmpB2 = _mpyu4(u32SrcB2, 0x01010101);
            u64TmpB3 = _mpyu4(u32SrcB3, 0x01010101);

            u32Dst00 = _sub2(_lo(u64TmpA0), _lo(u64TmpB0));
            u32Dst01 = _sub2(_hi(u64TmpA0), _hi(u64TmpB0));
            u32Dst02 = _sub2(_lo(u64TmpA1), _lo(u64TmpB1));
            u32Dst03 = _sub2(_hi(u64TmpA1), _hi(u64TmpB1));
            u32Dst04 = _sub2(_lo(u64TmpA2), _lo(u64TmpB2));
            u32Dst05 = _sub2(_hi(u64TmpA2), _hi(u64TmpB2));
            u32Dst06 = _sub2(_lo(u64TmpA3), _lo(u64TmpB3));
            u32Dst07 = _sub2(_hi(u64TmpA3), _hi(u64TmpB3));

            _mem4(A_dst + (i << 4) +  0) = u32Dst00;
            _mem4(A_dst + (i << 4) +  2) = u32Dst01;
            _mem4(A_dst + (i << 4) +  4) = u32Dst02;
            _mem4(A_dst + (i << 4) +  6) = u32Dst03;
            _mem4(A_dst + (i << 4) +  8) = u32Dst04;
            _mem4(A_dst + (i << 4) + 10) = u32Dst05;
            _mem4(A_dst + (i << 4) + 12) = u32Dst06;
            _mem4(A_dst + (i << 4) + 14) = u32Dst07;
        }

        for (y = 1; y < img_h - 1; y++)
        {
            const uint8_t *restrict ptr_prev = (uint8_t*)(gray_img + img_w * (y - 1));
            const uint8_t *restrict ptr_next = (uint8_t*)(gray_img + img_w * (y + 1));
            int16_t *restrict ptr_sobely16 = (int16_t*)(sobely16 + img_w * y);

            for (x = 0; x < img_w / 16; x++)
            {
                u32SrcA0 = _mem4((void *)(ptr_next + (x << 4) +  0));
                u32SrcA1 = _mem4((void *)(ptr_next + (x << 4) +  4));
                u32SrcA2 = _mem4((void *)(ptr_next + (x << 4) +  8));
                u32SrcA3 = _mem4((void *)(ptr_next + (x << 4) + 12));

                u32SrcB0 =  _mem4((void *)(ptr_prev + (x << 4) +  0));
                u32SrcB1 =  _mem4((void *)(ptr_prev + (x << 4) +  4));
                u32SrcB2 =  _mem4((void *)(ptr_prev + (x << 4) +  8));
                u32SrcB3 =  _mem4((void *)(ptr_prev + (x << 4) + 12));

                u64TmpA0 = _mpyu4(u32SrcA0, 0x01010101);
                u64TmpA1 = _mpyu4(u32SrcA1, 0x01010101);
                u64TmpA2 = _mpyu4(u32SrcA2, 0x01010101);
                u64TmpA3 = _mpyu4(u32SrcA3, 0x01010101);

                u64TmpB0 = _mpyu4(u32SrcB0, 0x01010101);
                u64TmpB1 = _mpyu4(u32SrcB1, 0x01010101);
                u64TmpB2 = _mpyu4(u32SrcB2, 0x01010101);
                u64TmpB3 = _mpyu4(u32SrcB3, 0x01010101);

                u32Dst00 = _sub2(_lo(u64TmpA0), _lo(u64TmpB0));
                u32Dst01 = _sub2(_hi(u64TmpA0), _hi(u64TmpB0));
                u32Dst02 = _sub2(_lo(u64TmpA1), _lo(u64TmpB1));
                u32Dst03 = _sub2(_hi(u64TmpA1), _hi(u64TmpB1));
                u32Dst04 = _sub2(_lo(u64TmpA2), _lo(u64TmpB2));
                u32Dst05 = _sub2(_hi(u64TmpA2), _hi(u64TmpB2));
                u32Dst06 = _sub2(_lo(u64TmpA3), _lo(u64TmpB3));
                u32Dst07 = _sub2(_hi(u64TmpA3), _hi(u64TmpB3));

                _mem4(ptr_sobely16 + (x << 4) +  0) = u32Dst00;
                _mem4(ptr_sobely16 + (x << 4) +  2) = u32Dst01;
                _mem4(ptr_sobely16 + (x << 4) +  4) = u32Dst02;
                _mem4(ptr_sobely16 + (x << 4) +  6) = u32Dst03;
                _mem4(ptr_sobely16 + (x << 4) +  8) = u32Dst04;
                _mem4(ptr_sobely16 + (x << 4) + 10) = u32Dst05;
                _mem4(ptr_sobely16 + (x << 4) + 12) = u32Dst06;
                _mem4(ptr_sobely16 + (x << 4) + 14) = u32Dst07;
            }

            for (x = (x << 4); x < img_w; x++)
            {
                ptr_sobely16[x] = (int16_t)((int16_t)ptr_next[x] - (int16_t)ptr_prev[x]);
            }
        }

        // 如果后面不访问边界上这些元素的话，下面的操作可以省略 wqm 20131112
        // 将上下2行置为0
//         memset(sobelx16 + img_w * 0,          0, img_w * sizeof(sobelx16[0]));
//         memset(sobely16 + img_w * 0,          0, img_w * sizeof(sobely16[0]));
//         memset(sobelx16 + img_w * (img_h - 1), 0, img_w * sizeof(sobelx16[0]));
//         memset(sobely16 + img_w * (img_h - 1), 0, img_w * sizeof(sobely16[0]));
//
//         // 将左右2行置为0
//         for (y = 0; y < img_h; y++)
//         {
//             int16_t *restrict ptr_sobelx16 = (int16_t*)(sobelx16 + img_w * y);
//             int16_t *restrict ptr_sobely16 = (int16_t*)(sobely16 + img_w * y);
//
//             ptr_sobelx16[0] = ptr_sobelx16[img_w - 1] = 0;
//             ptr_sobely16[0] = ptr_sobely16[img_w - 1] = 0;
//         }
    }
#endif

    /* Calculate the bin images. The magnitude and orientation of the gradient at each pixel is calculated using
    the sobelx and sobely images.{Magnitude = sqrt(sq(sobelx) + sq(sobely)), gradient = itan(sobely/sobelx)}.
    Then according to the orientation of the gradient, the value of the corresponding pixel color_img the corresponding
    image is set */

    for (h = 1; h < img_h / cell_h * cell_h - 1; h++)
    {
        /* ptrx and ptry point to beginning of the current row color_img the sobelx and sobely images respectively */
        int16_t *restrict ptrx = (int16_t*)(sobelx16 + img_w * h);
        int16_t *restrict ptry = (int16_t*)(sobely16 + img_w * h);
        int32_t cell_width = (img_w / cell_w) * bins_num;

        /* For every pixel's orientation and magnitude of gradient are calculated and corresponding
        values set for the bin images. */
        for (w = 1; w < img_w / cell_w * cell_w - 1; w++)
        {
            // 采用查表的方式以加快在DSP上的运行速度
            int32_t ptr_pix = (ptry[w] + 255) * 511 + ptrx[w] + 255;
            int32_t cell_tmp = cell_width * (h / cell_h) + bins_num * (w / cell_w);
            int32_t integ_list3 = integ_hist_list3[ptr_pix];
            int32_t integ_list4 = integ_list_tmp[integ_list3];

            cell_feature[cell_tmp + integ_list3] += integ_hist_list1[ptr_pix];
            cell_feature[cell_tmp + integ_list4] += integ_hist_list2[ptr_pix];
        }
    }

    CHECKED_FREE(sobelx16, img_w * img_h * sizeof(int16_t));
    CHECKED_FREE(sobely16, img_w * img_h * sizeof(int16_t));

//    fclose(ftxt);

    /* The function returns an array of 9 images which constitute the integral histogram */
    return 0;
}

#if USE_INT_HOG_FEATURE

static float Q_rsqrt(float number)
{
    long i;
    float x2, y;
    const float threehalfs = 1.5f;

    x2 = number * 0.5f;
    y = number;
    i = *(long *)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (threehalfs - (x2 * y * y));
    return y;
}

static void normalizeBlockHistogram_c(hogfeature_type *restrict hog_block, cellfeature_type *restrict ptr_cell, int sz)
{
    int i;
    hogfeature_type sum = 1; // 加1防止除数为0
    hogfeature_type scale;
    hogfeature_type thresh = (1 << NSHIFT) / 5; // (float)descriptor->L2HysThreshold;

#if NORMALIZE_L2_NORM
#if 1

    for (i = 0; i < sz; i++)
    {
        sum += ptr_cell[i] * ptr_cell[i];
    }

#else
    hogfeature_type sum0 = 0;
    hogfeature_type sum1 = 0;
    hogfeature_type sum2 = 0;
    hogfeature_type sum3 = 0;
    uint32_t u32SrcA0, u32SrcA1, u32SrcA2, u32SrcA3;

    for (i = 0; i < sz / 8; i++)
    {
        u32SrcA0 = _mem4(&ptr_cell[(i << 3) + 0]);
        u32SrcA1 = _mem4(&ptr_cell[(i << 3) + 2]);
        u32SrcA2 = _mem4(&ptr_cell[(i << 3) + 4]);
        u32SrcA3 = _mem4(&ptr_cell[(i << 3) + 6]);
        sum0 += _dotp2(u32SrcA0, u32SrcA0);
        sum1 += _dotp2(u32SrcA1, u32SrcA1);
        sum2 += _dotp2(u32SrcA2, u32SrcA2);
        sum3 += _dotp2(u32SrcA3, u32SrcA3);
    }

    for (i = (i << 3); i < sz; i++)
    {
        sum += ptr_cell[i] * ptr_cell[i];
    }

    sum += sum0 + sum1 + sum2 + sum3;
#endif

    scale = (hogfeature_type)(Q_rsqrt((float)sum) * (1 << NSHIFT));

    for (i = 0; i < sz; i++)
    {
        /*hog_block[i] = MIN2(ptr_cell[i] * scale, thresh);*/
        hog_block[i] = ptr_cell[i] * scale;
    }

#elif NORMALIZE_L1_sqrt

    for (i = 0; i < sz; i++)
    {
        sum += ptr_cell[i];
    }

    for (i = 0; i < sz; i++)
    {
        hog_block[i] = sqrt((float)ptr_cell[i] / (float)sum) * (1 << NSHIFT);
    }

#endif
}
#else
void normalizeBlockHistogram_c(float *restrict hog_block, cellfeature_type *restrict ptr_cell, int sz)
{
    int i;
    float sum = 1;
    float scale;
    float thresh = 0.2f; // (float)descriptor->L2HysThreshold;

#if NORMALIZE_L2_Hys

    for (i = 0; i < sz; i++)
    {
        sum += hog_block[i] * hog_block[i];
    }

    scale = 1.f / (sqrt(sum) + sz * 0.1f);

    for (i = 0, sum = 0; i < sz; i++)
    {
        hog_block[i] = MIN2(hog_block[i] * scale, thresh);
        sum += hog_block[i] * hog_block[i];
    }

    scale = 1.f / (sqrt(sum) + 1e-3f);

    for (i = 0; i < sz; i++)
    {
        hog_block[i] *= scale;
    }

#elif NORMALIZE_L2_NORM

    for (i = 0; i < sz; i++)
    {
        sum += ptr_cell[i] * ptr_cell[i];
    }

    scale = 1.f / sqrt(sum);

//  for (i = 0, sum = 0; i < sz; i++)
//  {
//      hog_block[i] = MIN2(ptr_cell[i] * scale, thresh);
//  }

    for (i = 0, sum = 0; i < sz; i++)
    {
        hog_block[i] = ptr_cell[i] * scale;
    }

#elif NORMALIZE_L1_sqrt

    for (i = 0; i < sz; i++)
    {
        sum += ptr_cell[i];
    }

    scale = 1.f / (sum);

    for (i = 0, sum = 0; i < sz; i++)
    {
        hog_block[i] = sqrt(ptr_cell[i] * scale);
    }

#endif
}
#endif

#define IMAGE_ELEM( image, elemtype, row, col )       \
    (((elemtype*)((image) + (img_w+1)*(row)))[(col)])

/* The following demonstrates how the integral histogram calculated using the above function can be used to
calculate the histogram of oriented gradients for any rectangular region of the image */

/* The following function takes in the rectangular cell as input for which the histogram of oriented gradients
*  has to be calculated, a matrix hog_cell of dimensions 1x9 to store the bin values of the integral histogram
*/
static void calculate_hog_feature_in_cell_c(MyRect cell, cellfeature_type *restrict ptr_hog_tmp, integral_type** integrals,
                                            cellfeature_type *restrict cell_feature,
                                            int bins_num, int normalization, int img_w, int img_h)
{
#if USE_INTEGRAL_IMAGE
    int i = 0;

    /* Calculate the bin values of the histogram one by one */
    for (i = 0; i < bins_num; i++)
    {
        integral_type a = IMAGE_ELEM(integrals[i], integral_type, cell.y, cell.x);
        integral_type b = IMAGE_ELEM(integrals[i], integral_type, (cell.y + cell.height), (cell.x + cell.width));
        integral_type c = IMAGE_ELEM(integrals[i], integral_type, cell.y, (cell.x + cell.width));
        integral_type d = IMAGE_ELEM(integrals[i], integral_type, (cell.y + cell.height), cell.x);

        ptr_hog_tmp[i] = (cellfeature_type)((a + b) - (c + d));
    }

#else
    int32_t cell_width = (img_w / cell.width) * bins_num;
    cellfeature_type *restrict ptr_cell = cell_feature + (cell.y / cell.height) * cell_width + (cell.x / cell.width) * bins_num;
    memcpy(ptr_hog_tmp, ptr_cell, sizeof(ptr_cell[0]) * bins_num);
#endif

    //normalizeBlockHistogram_c(ptr_hog_tmp, 9);

    return;
}

/* This function takes in a block as a rectangle and calculates the hog features of the block by dividing
*  it into cells of size cell(the supplied parameter), calculating the hog features for each cell using the
*  function calculate_hog_feature_in_cell(...), concatenating the so obtained vectors for each cell and then normalizing
*  over the concatenated vector to obtain the hog features for a block
*/
static void calculate_hog_feature_in_block_c(MyRect block, hogfeature_type *restrict hog_block, integral_type** integrals,
                                             cellfeature_type *restrict cell_feature, MySize cell,
                                             int block_feature_length, int bins_num, int normalization,
                                             int img_w, int img_h)
{
    int cell_x;
    int cell_y;
    cellfeature_type hog_tmp[36];
    cellfeature_type *restrict ptr_hog_tmp = hog_tmp;

#if THE_SAME_ORDER_AS_OPENCV

    for (cell_x = block.x; cell_x <= block.x + block.width - cell.width; cell_x += cell.width)
    {
        for (cell_y = block.y; cell_y <= block.y + block.height - cell.height; cell_y += cell.height)
        {
            //Vps_printf("calculate_hog_feature_in_cell_c\n");
            calculate_hog_feature_in_cell_c(myRect(cell_x, cell_y, cell.width, cell.height),
                                            ptr_hog_tmp, integrals, cell_feature, bins_num, normalization, img_w, img_h);
            ptr_hog_tmp += bins_num;
        }
    }

#else

    for (cell_y = block.y; cell_y <= block.y + block.height - cell.height; cell_y += cell.height)
    {
        for (cell_x = block.x; cell_x <= block.x + block.width - cell.width; cell_x += cell.width)
        {
            calculate_hog_feature_in_cell_c(myRect(cell_x, cell_y, cell.width, cell.height),
                                            ptr_hog_tmp, integrals, bins_num, normalization, img_w, img_h);
            ptr_hog_tmp += bins_num;
        }
    }

#endif

    if (normalization != -1)
    {
        normalizeBlockHistogram_c(hog_block, hog_tmp, block_feature_length);
    }

    return;
}

/* This function takes color_img a window(64x128 pixels, but can be easily modified for other window sizes)
*  and calculates the hog features for the window. It can be used to calculate the feature vector for a
*  64x128 pixel image as well. This window/image is the training/detection window which is used for training
*  or on which the final detection is done. The hog features are computed by dividing the window into
*  overlapping blocks, calculating the hog vectors for each block using calculate_hog_feature_in_block(...)and
*  concatenating the so obtained vectors to obtain the hog feature vector for the window
*/
static void calculate_hog_feature_in_window_c(integral_type** integrals, cellfeature_type *restrict cell_feature,
                                              MyRect window, MySize block, MySize cell,
                                              int block_feature_length, int hog_feature_length,
                                              int bins_num, int normalization, hogfeature_type* hog_feature_window,
                                              int img_w, int img_h)
{
    /* A cell size of 8x8 pixels is considered and each block is divided into 2x2 such cells(i.e. the
    block is 16x16 pixels). So a 64x128 pixels window would be divided into 7x15 overlapping blocks */
    int block_x;
    int block_y;
    hogfeature_type *restrict vector_block = hog_feature_window;

#if THE_SAME_ORDER_AS_OPENCV

    for (block_x = window.x; block_x <= window.x + window.width - block.width; block_x += cell.width)
    {
        for (block_y = window.y; block_y <= window.y + window.height - block.height; block_y += cell.height)
        {
            //Vps_printf("calculate_hog_feature_in_block_c\n");
            calculate_hog_feature_in_block_c(myRect(block_x, block_y, block.width, block.height),
                                             vector_block, integrals, cell_feature, cell,
                                             block_feature_length, bins_num, normalization,
                                             img_w, img_h);
            vector_block += block_feature_length;
        }
    }

#else

    for (block_y = window.y; block_y <= window.y + window.height - block.height; block_y += cell.height)
    {
        for (block_x = window.x; block_x <= window.x + window.width - block.width; block_x += cell.width)
        {
            calculate_hog_feature_in_block_c(myRect(block_x, block_y, block.width, block.height),
                                             vector_block, integrals, cell,
                                             block_feature_length, bins_num, normalization,
                                             img_w, img_h);
            vector_block += block_feature_length;
        }
    }

#endif

    return;
}

/* Function to calculate the hog features of a sample */
int calculate_hog_features_c(uint8_t* gray_img, hogfeature_type* hog_feature, int* hog_feature_len,
                             MySize window, MySize block, MySize blockstride,
                             MySize cell, int bins_num, int normalization,
                             int img_w, int img_h)
{
    integral_type** integrals = NULL;
    cellfeature_type *restrict cell_feature = NULL;
    //int i = 0;
    /* A default block size of 2x2 cells is considered */
    int block_w = block.width / cell.width;
    int block_h = block.height / cell.height;
    int block_feature_length = block_w * block_h * bins_num;
    /* The length of the feature vector for a cell is 9(since no. of bins is 9),
    for block it would be  9 * (no. of cells of the block) = 9*4 = 36.
    And the length of the feature vector for a window would be 36 * (no. of blocks of the window) */
    int hog_feature_length = (((window.width - block.width) / cell.width) + 1) *
                             (((window.height - block.height) / cell.height) + 1) * block_feature_length;
#if USE_INTEGRAL_IMAGE
    /* Create an array of 9 images(note the dimensions of the image, the cvIntegral() requires the size to be that),
    to store the integral images calculated from the above bin images.
    These 9 integral images together constitute the integral histogram */
    integrals = (integral_type**)malloc(sizeof(integral_type*) * bins_num);

    for (i = 0; i < bins_num; i++)
    {
        CHECKED_MALLOC(integrals[i], (img_w + 1) * (img_h + 1) * sizeof(integral_type), 64);
        memset(integrals[i], 0, (img_w + 1) * (img_h + 1) * sizeof(integral_type));
    }

    /* Calculation of the integral histogram for fast calculation of hog features */
    calculate_integral_histogram_c(gray_img, integrals, bins_num, img_w, img_h);
#else
    CHECKED_MALLOC(cell_feature, (img_w / cell.width) * (img_h / cell.height) * bins_num * sizeof(cellfeature_type), 64);
    calculate_cell_featrue_image_c(gray_img, cell_feature, cell.width, cell.height, bins_num, img_w, img_h);
#endif

    calculate_hog_feature_in_window_c(integrals, cell_feature,
                                      myRect(0, 0, window.width, window.height),
                                      block, cell,
                                      block_feature_length, hog_feature_length,
                                      bins_num, normalization,
                                      hog_feature, img_w, img_h);

#if USE_INTEGRAL_IMAGE

    for (i = 0; i < bins_num; i++)
    {
        CHECKED_FREE(integrals[i], (img_w + 1) * (img_h + 1) * sizeof(integral_type));
    }

    free(integrals);
#else
    CHECKED_FREE(cell_feature, (img_w / cell.width) * (img_h / cell.height) * bins_num * sizeof(cellfeature_type));
#endif

    *hog_feature_len = hog_feature_length;

    return 0;
}

/* Function to calculate the hog features and detect */
int calculate_hog_features_and_detect_c(uint8_t *restrict gray_img,
                                        hogfeature_type *restrict hog_feature,
                                        int *restrict hog_feature_len,
                                        MySize window, MySize block, MySize blockstride,
                                        MySize cell, int bins_num, int normalization,
                                        int img_w, int img_h,
                                        MyRect* foundLocations, int* found_num)
{
    int k;
    int w, h;
    int idx = 0;
    //int i = 0;
    integral_type** integrals = NULL;
    cellfeature_type *restrict cell_feature = NULL;
    hogfeature_type *restrict ptr_hog_feature = NULL;
#if USE_INT_HOG_FEATURE
    int *restrict svm_ptr = NULL;
    int s = 0;
#else
    float *restrict svm_ptr = NULL;
    float s = 0;
#endif
    int tmp_w = 0;
    int tmp_h = 0;
    /* A default block size of 2x2 cells is considered */
    int block_w = block.width / cell.width;
    int block_h = block.height / cell.height;
    int block_feature_length = block_w * block_h * bins_num;
    /* The length of the feature vector for a cell is 9(since no. of bins is 9),
    for block it would be  9 * (no. of cells of the block) = 9*4 = 36.
    And the length of the feature vector for a window would be 36 * (no. of blocks of the window) */
    int hog_feature_length = (((window.width - block.width) / cell.width) + 1) *
                             (((window.height - block.height) / cell.height) + 1) * block_feature_length;
    CLOCK_INITIALIZATION();
#if USE_INTEGRAL_IMAGE
    /* Create an array of 9 images(note the dimensions of the image, the cvIntegral() requires the size to be that),
    to store the integral images calculated from the above bin images.
    These 9 integral images together constitute the integral histogram */
    integrals = (integral_type**)malloc(sizeof(integral_type*) * bins_num);

    for (i = 0; i < bins_num; i++)
    {
        CHECKED_MALLOC(integrals[i], (img_w + 1) * (img_h + 1) * sizeof(integral_type), 64);
        memset(integrals[i], 0, (img_w + 1) * (img_h + 1) * sizeof(integral_type));
    }

    /* Calculation of the integral histogram for fast calculation of hog features */
    calculate_integral_histogram_c(gray_img, integrals, bins_num, img_w, img_h);
#else
    CHECKED_MALLOC(cell_feature, (img_w / cell.width) * (img_h / cell.height) * bins_num * sizeof(cellfeature_type), 64);
    /* Calculation of the integral histogram for fast calculation of hog features*/
    CLOCK_START(calculate_integral_histogram_c);
    calculate_cell_featrue_image_c(gray_img, cell_feature, cell.width, cell.height, bins_num, img_w, img_h);
    CLOCK_END("calculate_integral_histogram_c");
#endif
    CLOCK_START(calculate_hog_feature_in_window_c);

    tmp_h = (img_h - window.height) - ((img_h - window.height) % blockstride.height);
    tmp_w = (img_w - window.width) - ((img_w - window.width) % blockstride.width);

    for (h = 0; h <= tmp_h; h += blockstride.height)
    {
        for (w = 0; w <= tmp_w; w += blockstride.width)
        {
            calculate_hog_feature_in_window_c(integrals, cell_feature,
                                              myRect(w, h, window.width, window.height),
                                              block, cell,
                                              block_feature_length, hog_feature_length,
                                              bins_num, normalization,
                                              hog_feature, img_w, img_h);

#if USE_INT_HOG_FEATURE
            ptr_hog_feature = hog_feature;
            svm_ptr = carface64x32_LSVM_int;
            s = 0;

            for (k = 0; k < hog_feature_length; k++)
            {
                s += ((long long)ptr_hog_feature[k] * (long long)svm_ptr[k]) >> NSHIFT;
            }

#else
            ptr_hog_feature = hog_feature;
            svm_ptr = carface64x32_LSVM_float;
            s = 0;

            for (k = 0; k < hog_feature_length; k++)
            {
                s += ptr_hog_feature[k] * svm_ptr[k];
            }

#endif
            s += svm_ptr[k]; // s - rho

            if (s > 0)
            {
                foundLocations[idx].x = w;
                foundLocations[idx].y = h;
                foundLocations[idx].width = window.width;
                foundLocations[idx].height = window.height;
                idx++;

//              save_as_svm_format_c(trainfile, hog_feature, hog_feature_len, +1);
            }
        }
    }

    CLOCK_END("calculate_hog_feature_in_window_c");

//  fclose(trainfile);

#if USE_INTEGRAL_IMAGE

    for (i = 0; i < bins_num; i++)
    {
        CHECKED_FREE(integrals[i], (img_w + 1) * (img_h + 1) * sizeof(integral_type));
    }

    free(integrals);
#else
    CHECKED_FREE(cell_feature, (img_w / cell.width) * (img_h / cell.height) * bins_num * sizeof(cellfeature_type));
#endif

    *hog_feature_len = hog_feature_length;
    *found_num = idx;

    return 0;
}

#undef THE_SAME_ORDER_AS_OPENCV
