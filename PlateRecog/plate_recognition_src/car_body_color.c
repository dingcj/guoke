#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../malloc_aligned.h"
#include "../common/threshold.h"
#include "car_body_color.h"

#ifdef  WIN32
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#include "../color_space_src/rgb2yuv.h"
#endif

#define DEBUG_COLOR 0

// 车身颜色模型1：          黑色  白色  银色  灰色  紫色  黄色  红色  蓝色  绿色  棕色  粉红
static uint8_t HSI_H[12] = {143,  72,   145,  136,  175,  33,   3,    152,  98,   19};
//static uint8_t HSI_S[12] = {52,   3,    19,   35,   129,  187,  147,  135,  89,   147};
//static uint8_t HSI_I[12] = {81,   214,  171,  117,  40,   134,  107,  113,  134,  100};
// 车身颜色模型2：          黑色  白色  银色  灰色  紫色  黄色  红色  蓝色  绿色  棕色
//static int16_t LAB_L[12] = {175,  250,  230,  200,  123,  227,  186,  190,  219,  190};
//static int16_t LAB_A[12] = {6,    16,   15,   12,   44,   -9,   92,   15,   -38,  30};
//static int16_t LAB_B[12] = {-3,   14,   4,    2,    -51,  146,  40,   -46,  29,   75};

//颜色空间转换，yuv转换为hsi和CIElab
static void yuv2hsvCIElab(uint8_t y, uint8_t u, uint8_t v,
                          int16_t *restrict H, uint8_t *restrict S, uint8_t *restrict V,
                          int16_t *restrict L, int16_t *restrict A, int16_t *restrict B)
{
    int32_t cb, cr;
    int32_t r, g, b;
    int32_t min, max, delta;
    int32_t hue;
    float Ix, Iy, Iz;
    float Xn, Yn, Zn;
    int32_t X, Y, Z;
    int32_t y_1;

    cb = (int32_t)u - (int32_t)128;
    cr = (int32_t)v - (int32_t)128;

#if 0
    r = (int32_t)(y + cr * 1.4075f);
    g = (int32_t)(y - cb * 0.39f - cr * 0.58f);
    b = (int32_t)(y + cb * 2.03f);
#else
    y_1 = y << 16;
    r = (y_1 + cr * 92241) / 65536;
    g = (y_1 - cb * 25559 - cr * 38010) / 65536;
    b = (y_1 + cb * 133038) / 65536;
#endif

    r = r > 255 ? 255 : r;
    g = g > 255 ? 255 : g;
    b = b > 255 ? 255 : b;

    r = r < 0 ? 0 : r;
    g = g < 0 ? 0 : g;
    b = b < 0 ? 0 : b;

    /*rgb到CIElab空间转换*/
    X = (181462 * r + 114799 * g + 74068 * b) / 65536;      // 2.7689*R + 1.7517*G + 1.1302*B
    Y = (65536 * r + 300856 * g + 3938 * b) / 65536;        // 1*R + 4.5907*G + 0.0601*B
    Z = (3512 * g + 366628 * b) / 65536;                    // 0*R + 0.0563*G + 5.5943*B

    Xn = 95.04f;
    Yn = 100.00f;
    Zn = 108.89f;

    Ix = (float)X / Xn;
    Iy = (float)Y / Yn;
    Iz = (float)Z / Zn;

    if (Iy > 0.008856)
    {
        *L = (int16_t)(116 * pow(Iy, 1.0 / 3) - 16);

        if (Ix > 0.008856)
        {
            *A = (int16_t)(500 * (pow(Ix, 1.0 / 3) - pow(Iy, 1.0 / 3)));
        }
        else
        {
            *A = (int16_t)(500 * (7.787 * Ix + 16 / 116 - pow(Iy, 1.0 / 3)));
        }

        if (Iz > 0.008856)
        {
            *B = (int16_t)(200 * (pow(Iy, 1.0 / 3) - pow(Iz, 1.0 / 3)));
        }
        else
        {
            *B = (int16_t)(200 * (pow(Iy, 1.0 / 3) - 7.787 * Iz - 16 / 116));
        }
    }
    else
    {
        *L = (int16_t)(116 * (7.787 * Iy + 16 / 116) - 16);

        if (Ix > 0.008856)
        {
            *A = (int16_t)(500 * (pow(Ix, 1.0 / 3) - 7.787 * Iy - 16 / 116));
        }
        else
        {
            *A = (int16_t)(500 * (7.787 * Ix + 16 / 116 - 7.787 * Iy - 16 / 116));
        }

        if (Iz > 0.008856)
        {
            *B = (int16_t)(200 * (7.787 * Iy + 16 / 116 - pow(Iz, 1.0 / 3)));
        }
        else
        {
            *B = (int16_t)(200 * (7.787 * Iy + 16 / 116 - 7.787 * Iz - 16 / 116));
        }
    }

    /*rgb到HSV空间转换*/
    min = MIN3(r, g, b);
    max = MAX3(r, g, b);
    *V = (uint8_t)((r + g + b) / 3);
    delta = max - min;

    if (delta == 0)
    {
        *S = 0;
        *H = 220;
        return;
    }

    if (max != 0)
    {
        *S = (uint8_t)(delta * 255 / max);
    }
    else
    {
        *S = 0;
    }

    if (*S == 0)
    {
        *H = 255 * 120 / 360;
    }
    else
    {
        if (r == max)
        {
            hue = 42 * (g - b) / delta;     // between yellow & magenta -60°－ 60°
        }                                   // 在r，g， b值相近时，即使有微小变化，H值的变化也很大。
        else if (g == max)
        {
            hue = 84 + 42 * (b - r) / delta;    // between cyan & yellow  60° － 180°
        }
        else
        {
            hue = 168 + 42 * (r - g) / delta;   // between magenta & cyan  180° － 300°
        }

        *H = (int16_t)hue;
    }

    return;
}

/***************************************************
 置信度确定函数
 通过计算识别颜色与模板颜色中心值的距离，得到置信度
****************************************************/
static void get_car_color_trust(int16_t max_H, uint8_t max_S, uint8_t max_I,
                                /*int16_t max_l, int16_t max_a, int16_t max_b,*/
                                uint8_t *color_trust, uint8_t HSI_num)
{
    int32_t dis = 0;
    uint8_t HSI_S[12] = {52,   3,    19,   35,   129,  187,  147,  135,  89,   147};
    uint8_t HSI_I[12] = {81,   214,  171,  117,  40,   134,  107,  113,  134,  100};
//  dis = (max_l - LAB_L[HSI_num]) * (max_l - LAB_L[HSI_num])
//      + (max_a - LAB_A[HSI_num]) * (max_a - LAB_A[HSI_num])
//      + (max_b - LAB_B[HSI_num]) * (max_b - LAB_B[HSI_num]);
//  dis = sqrt(dis);

    if ((int32_t)HSI_num < 2)
    {
        dis = ((int32_t)max_S - (int32_t)HSI_S[HSI_num]) * ((int32_t)max_S - (int32_t)HSI_S[HSI_num])
              + ((int32_t)max_I - (int32_t)HSI_I[HSI_num]) * ((int32_t)max_I - (int32_t)HSI_I[HSI_num]);
        dis = (int32_t)sqrt(dis) / 3;
    }
    else if ((int32_t)HSI_num < 4)
    {
        dis = ((int32_t)max_S - ((int32_t)HSI_S[2] + (int32_t)HSI_S[3]) / 2) * ((int32_t)max_S
                                                                                - ((int32_t)HSI_S[2] + (int32_t)HSI_S[3]) / 2)
              + ((int32_t)max_I - ((int32_t)HSI_I[2] + (int32_t)HSI_I[3]) / 2) * ((int32_t)max_I
                                                                                  - ((int32_t)HSI_I[2] + (int32_t)HSI_I[3]) / 2);
        dis = (int32_t)sqrt(dis) / 6;
    }
    else
    {
        dis = ((int32_t)max_H - (int32_t)HSI_H[HSI_num]) * ((int32_t)max_H - (int32_t)HSI_H[HSI_num])
              + ((int32_t)max_S - (int32_t)HSI_S[HSI_num]) * ((int32_t)max_S - (int32_t)HSI_S[HSI_num]) / 4
              + ((int32_t)max_I - (int32_t)HSI_I[HSI_num]) * ((int32_t)max_I - (int32_t)HSI_I[HSI_num]) / 4;
        dis = (int32_t)sqrt(dis);

        if ((int32_t)HSI_num < 6)
        {
            dis /= 4;
        }
        else if ((int32_t)HSI_num == 6)
        {
            dis /= 3;
        }
        else if ((int32_t)HSI_num == 7)
        {
            dis /= 2;
        }
        else if ((int32_t)HSI_num == 8)
        {
            dis /= 3;
        }
        else
        {
            dis /= 4;
        }
    }

//  if ((HSI_num == 1) || (HSI_num == 2))
//  {
//      dis /= 2;
//  }
//  else if (HSI_num == 0)
//  {
//      dis /= 4;
//  }
//  else if (HSI_num != 7)
//  {
//      dis /= 8;
//  }
//  else
//  {
//      dis /= 4;
//  }
    dis = MIN2(dis, 40);

    *color_trust = (uint8_t)(*color_trust - dis);

    return;
}

/******************************************************************
 颜色识别函数
 识别出10种颜色
 将银色和灰色合并为银灰
 为了与外部接口对应，调整识别颜色代码
 颜色代码： 0    1    2    3    4    5    6    7    8    9    99
            白色 银灰 黄色 粉色 红色 紫色 绿色 蓝色 棕色 黑色 未知
 ******************************************************************/
static uint8_t body_color(uint8_t *restrict yuv_img, int32_t img_w, int32_t img_h,
                          int32_t change_number, uint8_t *car_color_trust)
{
    int32_t i, tmp;
    int32_t w, h;
    uint8_t y, u, v;
    uint8_t HSI_num;
    uint8_t color_trust;
    int32_t Hist_Y[256] = {0};
    int32_t Hist_U[256] = {0};
    int32_t Hist_V[256] = {0};
    int16_t LAB_L[12] = {175,  250,  230,  200,  123,  227,  186,  190,  219,  190};
    int32_t tmp_Y, tmp_U, tmp_V;
    int32_t max_Y = 0;
    int32_t max_U = 0;
    int32_t max_V = 0;
    int16_t max_H = 0;
    uint8_t max_S = (uint8_t)0;
    uint8_t max_I = (uint8_t)0;
    int16_t max_l = 0;
    int16_t max_a = 0;
    int16_t max_b = 0;
    uint8_t *restrict y_buf = yuv_img;
    uint8_t *restrict u_buf = yuv_img + img_w * img_h;
    uint8_t *restrict v_buf = yuv_img + img_w * img_h + img_w * img_h / 4;

    for (h = 0; h < img_h; h++)
    {
        tmp = (img_w >> 1) * (h >> 1);

        for (w = 0; w < img_w; w++)
        {
            y = y_buf[img_w * h + w];
            u = u_buf[tmp + (w >> 1)];
            v = v_buf[tmp + (w >> 1)];
            Hist_Y[y]++;
            Hist_U[u]++;
            Hist_V[v]++;
        }
    }

    tmp_Y = 0;
    tmp_U = 0;
    tmp_V = 0;

    for (i = 0; i < 256; i++)
    {
        if (Hist_Y[i] > tmp_Y)
        {
            tmp_Y = Hist_Y[i];
            max_Y = i;
        }

        if (Hist_U[i] > tmp_U)
        {
            tmp_U = Hist_U[i];
            max_U = i;
        }

        if (Hist_V[i] > tmp_V)
        {
            tmp_V = Hist_V[i];
            max_V = i;
        }
    }

    yuv2hsvCIElab((uint8_t)max_Y, (uint8_t)max_U, (uint8_t)max_V, &max_H, &max_S, &max_I, &max_l, &max_a, &max_b);

    HSI_num = (uint8_t)99;
    color_trust = (uint8_t)100;

    if (change_number > 100)
    {
        color_trust -= (uint8_t)5;
    }

    if (tmp_Y < (img_w * img_h / 10))
    {
        color_trust -= (uint8_t)5;
    }

    if ((abs(max_a) < 30) && (abs(max_b) < 30) && (max_I > (uint8_t)100))
    {
        int32_t ColorDis;
        int32_t min_dis = 255;
        int32_t min_num = (1 << 20);

        for (i = 1; i < 4; i++)
        {
            ColorDis = abs(max_l - LAB_L[i]);

            if (ColorDis < min_dis)
            {
                min_dis = ColorDis;
                min_num = i;
            }
        }

        if ((min_num == 1) && (max_S < (uint8_t)30))
        {
            HSI_num = (uint8_t)1;

            //白色中，阈值较偏向银色的，修正为银色
            if ((max_l < 245) && (max_I < (uint8_t)210))
            {
                HSI_num = (uint8_t)2;
//              color_trust -= 5;
            }
        }
        else if ((min_num == 3) && (max_S < (uint8_t)70))
        {
            HSI_num = (uint8_t)3;

            //亮度较高的灰色，纠正为银白色
            if (max_I > (uint8_t)150)
            {
                HSI_num = (uint8_t)2;
//              color_trust -= 5;
            }
        }
        else if ((min_num == 2) && (max_S < (uint8_t)42))
        {
            HSI_num = (uint8_t)2;

            //亮度较低的银色，纠正为灰色
            if (max_I <= (uint8_t)140)
            {
                HSI_num = (uint8_t)3;
//              color_trust -= 5;
            }
        }
    }
    else if ((abs(max_a) <= 30) && (abs(max_b) <= 30) && (max_I <= (uint8_t)100) && (max_S <= (uint8_t)125))
    {
        HSI_num = (uint8_t)0;

        //纯度很低的黑色，纠正为灰色
        if (max_S < (uint8_t)20)
        {
            HSI_num = (uint8_t)3;
//          color_trust -= 5;
        }
        //绿色分量较高的黑色，纠正为绿色
        else if (max_a <= -20)
        {
            HSI_num = (uint8_t)8;
//          color_trust -= 5;
        }
        //蓝色分量较高的黑色，纠正为蓝色
        else if (max_b <= -30)
        {
            HSI_num = (uint8_t)7;
//          color_trust -= 5;
        }

        //红色分量较高的黑色，纠正为红色
        if ((HSI_num == (uint8_t)0) && (max_H < 20))
        {
            HSI_num = (uint8_t)6;
//          color_trust -= 5;
        }
    }

    if (HSI_num == (uint8_t)99)
    {
        int32_t min_Dis = 255;
        int32_t Color_Dis;
        int32_t min_num = (1 << 20);

        for (i = 4; i <= 9; i++)
        {
            Color_Dis = abs(max_H - (int16_t)HSI_H[i]);

            if (Color_Dis < min_Dis)
            {
                min_Dis = Color_Dis;
                min_num = i;
            }
        }

        HSI_num = (uint8_t)min_num;

        //对蓝绿色再次划分
        if ((min_num == 7) || (min_num == 8))
        {
            //绿色A分量为负
            if ((max_a < -15) && (max_a < max_b))
            {
                HSI_num = (uint8_t)8;
            }
            //蓝色B分量为负
            else if ((max_b < -20) && (max_b < max_a))
            {
                HSI_num = (uint8_t)7;
            }

            if (HSI_num == (uint8_t)7)
            {
                //纠正可能被识别为蓝色的黑色
                if ((max_S < (uint8_t)125) && (max_I < (uint8_t)50))
                {
                    HSI_num = (uint8_t)0;
//                  color_trust -= 5;
                }

                //纠正被识别成蓝色的银色
                if ((max_I > (uint8_t)150) && (abs(max_a) < 30) && (abs(max_b) < 30))
                {
                    HSI_num = (uint8_t)2;
//                  color_trust -= 5;
                }
            }

            if (HSI_num == (uint8_t)8)
            {
                //纠正识别成绿色的银色
                if ((max_I > (uint8_t)180) && (abs(max_a) < 30) && (abs(max_b) < 30))
                {
                    HSI_num = (uint8_t)2;
//                  color_trust -= 5;
                }
            }
        }
    }

    get_car_color_trust(max_H, max_S, max_I, /*max_l, max_a, max_b, */&color_trust, HSI_num);

    //同步外部接口颜色代码定义，合并银色和灰色为银灰
//  if ((HSI_num > 2) && (HSI_num < 10))
//  {
//      HSI_num -= 1;
//  }

    switch ((int32_t)HSI_num)
    {
        case 0:
            HSI_num = (uint8_t)9;
            break;

        case 1:
            HSI_num = (uint8_t)0;
            break;

        case 2:
            HSI_num = (uint8_t)1;
            break;

        case 3:
            HSI_num = (uint8_t)1;
            break;

        case 4:
            HSI_num = (uint8_t)5;
            break;

        case 5:
            HSI_num = (uint8_t)2;
            break;

        case 6:
            HSI_num = (uint8_t)4;
            break;

        case 7:
            HSI_num = (uint8_t)7;
            break;

        case 8:
            HSI_num = (uint8_t)6;
            break;

        case 9:
            HSI_num = (uint8_t)8;
            break;

        case 10:
            HSI_num = (uint8_t)3;
            break;

        default:
            HSI_num = (uint8_t)99;
            break;
    }

    *car_color_trust = color_trust;

    return HSI_num;
}

/*车身颜色判断区域选取函数，利用水平方向和垂直方向的跳变数进行阈值判断，
  未采用otsu方式，实测出otsu方式效果不如此方式好*/
static int32_t decide_body_area(uint8_t *restrict gray_img, /*uint8_t *restrict bina_img, */int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t change_num = 0;

//  int32_t thresh_flag;
//  thresh_flag = thresholding_otsu(gray_img, bina_img, img_w, img_h, 0, img_w - 1, 0, img_h - 1);
    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w - 1; w++)
        {
//          if (bina_img[h * img_w + w] != bina_img[h * img_w + w + 1])
            if (abs(gray_img[h * img_w + w] - gray_img[h * img_w + w + 1]) > 10)
            {
                change_num++;
            }
        }
    }

    for (w = 0; w < img_w; w++)
    {
        for (h = 0; h < img_h - 1; h++)
        {
            //          if (bina_img[h * img_w + w] != bina_img[h * img_w + w + 1])
            if (abs(gray_img[h * img_w + w] - gray_img[(h + 1) * img_w + w]) > 15)
            {
                change_num++;
            }
        }
    }

//  printf("跳变数: %4d\n", change_num);

    return change_num;
}

static void get_body_area(uint8_t *restrict yuv_data, uint8_t *restrict bodyimg, int32_t img_w, int32_t img_h,
                          int32_t body_w, int32_t body_h, Rects *restrict rect_body)
{
    int32_t w, h;
    uint8_t *restrict yuv_y = yuv_data;
    uint8_t *restrict yuv_u = yuv_data + img_w * img_h;
    uint8_t *restrict yuv_v = yuv_data + img_w * img_h * 5 / 4;
    uint8_t *restrict bodyimg_y = bodyimg;
    uint8_t *restrict bodyimg_u = bodyimg + body_w * body_h;
    uint8_t *restrict bodyimg_v = bodyimg + body_w * body_h * 5 / 4;

    for (h = 0; h < body_h; h++)
    {
        for (w = 0; w < body_w; w++)
        {
            bodyimg_y[body_w * h + w] = yuv_y[img_w * (h + rect_body->y0) + rect_body->x0 + w];
        }
    }

    for (h = 0; h < body_h / 2; h++)
    {
        for (w = 0; w < body_w / 2; w++)
        {
            bodyimg_u[body_w / 2 * h + w] = yuv_u[img_w / 2 * (h + rect_body->y0 / 2) + rect_body->x0 / 2 + w];
            bodyimg_v[body_w / 2 * h + w] = yuv_v[img_w / 2 * (h + rect_body->y0 / 2) + rect_body->x0 / 2 + w];
        }
    }

    return;
}

void car_body_color(uint8_t *restrict yuv_data, int32_t img_w, int32_t img_h,
                    Rects *rect_real, uint8_t *car_color, uint8_t *car_color_trust)
{
    uint8_t body_flag = (uint8_t)99;
    uint8_t trust_flag = (uint8_t)0;
    int32_t body_w, body_h;
    Rects   rect_body;
    int32_t change_num1, change_num2;
    uint8_t *restrict bodyimg = NULL;
    uint8_t *restrict bodyimg1 = NULL;
    uint8_t *restrict bodyimg2 = NULL;
    uint8_t *restrict bina_img = NULL;

#if DEBUG_COLOR
    int32_t w, h;
    uint8_t *bmpimg1 = NULL;
    uint8_t *bmpimg2 = NULL;
    IplImage *cvimg = NULL;
    IplImage *cvimg2 = NULL;
    uint8_t *img_test = NULL;
#endif
    body_w = ((rect_real->x1 - rect_real->x0 + 1 + 7) >> 3) << 3;
    body_h = ((rect_real->y1 - rect_real->y0 + 1 + 1) >> 1) << 1;

    CHECKED_MALLOC(bina_img, body_w * body_h * 3 / 2, CACHE_SIZE);
    CHECKED_MALLOC(bodyimg1, body_w * body_h * 3 / 2, CACHE_SIZE);
    CHECKED_MALLOC(bodyimg2, body_w * body_h * 3 / 2, CACHE_SIZE);

    rect_body.x0 = (int16_t)(rect_real->x0);
    rect_body.x1 = (int16_t)(rect_real->x0 + body_w);
    rect_body.y0 = (int16_t)(MAX2(rect_real->y0 - 5 * body_h, 0));
    rect_body.y1 = (int16_t)(MAX2(rect_real->y0 + body_h - 5 * body_h, 0));

    if (rect_body.y0 == 0)
    {
        goto EXT;
    }

    change_num1 = 100;
    change_num2 = 100;

    get_body_area(yuv_data, bodyimg1, img_w, img_h, body_w, body_h, &rect_body);
    change_num1 = decide_body_area(bodyimg1, /*bina_img, */body_w, body_h);

    if (change_num1 > 100)
    {
        rect_body.y0 = (int16_t)MAX2(rect_real->y0 - 6 * body_h, 0);
        rect_body.y1 = (int16_t)MAX2(rect_real->y0 + body_h - 6 * body_h, 0);

        if (rect_body.y0 == 0)
        {
            bodyimg = bodyimg1;
        }
        else
        {
            get_body_area(yuv_data, bodyimg2, img_w, img_h, body_w, body_h, &rect_body);
            change_num2 = decide_body_area(bodyimg2, /*bina_img, */body_w, body_h);

            if (change_num2 > 100)
            {
                if (change_num2 > change_num1)
                {
                    bodyimg = bodyimg1;
                }
                else
                {
                    bodyimg = bodyimg2;
                }
            }
            else
            {
                bodyimg = bodyimg2;
            }
        }

    }
    else
    {
        bodyimg = bodyimg1;
    }

    body_flag = body_color(bodyimg, body_w, body_h, MIN2(change_num1, change_num2), &trust_flag);

#if DEBUG_COLOR
    /*DEBUG功能用于显示车牌上方5个高度和6个高度的区域*/
    cvimg2 = cvCreateImage(cvSize(img_w, img_h), IPL_DEPTH_8U, 3);
    CHECKED_MALLOC(img_test, img_w * img_h * 3, CACHE_SIZE);
    yuv2bgr(yuv_data, img_test, img_w, img_h);

    rect_body.y0 = MAX2(rect_real->y0 - 6 * body_h, 0);
    rect_body.y1 = MAX2(rect_real->y0 + body_h - 6 * body_h, 0);

    for (h = rect_body.y0; h < rect_body.y1; h++)
    {
        img_test[img_w * h * 3 + rect_body.x0 * 3 + 0] = 255;
        img_test[img_w * h * 3 + rect_body.x0 * 3 + 1] = 0;
        img_test[img_w * h * 3 + rect_body.x0 * 3 + 2] = 0;
        img_test[img_w * h * 3 + rect_body.x1 * 3 + 0] = 255;
        img_test[img_w * h * 3 + rect_body.x1 * 3 + 1] = 0;
        img_test[img_w * h * 3 + rect_body.x1 * 3 + 2] = 0;
    }

    for (w = rect_body.x0; w < rect_body.x1; w++)
    {
        img_test[img_w * rect_body.y0 * 3 + w * 3 + 0] = 255;
        img_test[img_w * rect_body.y0 * 3 + w * 3 + 1] = 0;
        img_test[img_w * rect_body.y0 * 3 + w * 3 + 2] = 0;
        img_test[img_w * rect_body.y1 * 3 + w * 3 + 0] = 255;
        img_test[img_w * rect_body.y1 * 3 + w * 3 + 1] = 0;
        img_test[img_w * rect_body.y1 * 3 + w * 3 + 2] = 0;
    }

    for (h = rect_body.y0 + body_h + 1; h < rect_body.y1 + body_h + 1; h++)
    {
        img_test[img_w * h * 3 + rect_body.x0 * 3 + 0] = 0;
        img_test[img_w * h * 3 + rect_body.x0 * 3 + 1] = 255;
        img_test[img_w * h * 3 + rect_body.x0 * 3 + 2] = 0;
        img_test[img_w * h * 3 + rect_body.x1 * 3 + 0] = 0;
        img_test[img_w * h * 3 + rect_body.x1 * 3 + 1] = 255;
        img_test[img_w * h * 3 + rect_body.x1 * 3 + 2] = 0;
    }

    for (w = rect_body.x0; w < rect_body.x1; w++)
    {
        img_test[img_w * (rect_body.y0 + body_h + 1) * 3 + w * 3 + 0] = 0;
        img_test[img_w * (rect_body.y0 + body_h + 1) * 3 + w * 3 + 1] = 255;
        img_test[img_w * (rect_body.y0 + body_h + 1) * 3 + w * 3 + 2] = 0;
        img_test[img_w * (rect_body.y1 + body_h + 1) * 3 + w * 3 + 0] = 0;
        img_test[img_w * (rect_body.y1 + body_h + 1) * 3 + w * 3 + 1] = 255;
        img_test[img_w * (rect_body.y1 + body_h + 1) * 3 + w * 3 + 2] = 0;
    }

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            cvimg2->imageData[cvimg2->widthStep * h + w * 3 + 0] = img_test[img_w * h * 3 + w * 3 + 0];
            cvimg2->imageData[cvimg2->widthStep * h + w * 3 + 1] = img_test[img_w * h * 3 + w * 3 + 1];
            cvimg2->imageData[cvimg2->widthStep * h + w * 3 + 2] = img_test[img_w * h * 3 + w * 3 + 2];
        }
    }

//  cvSaveImage("wyf.bmp", cvimg2);
    cvNamedWindow("picture", CV_WINDOW_AUTOSIZE);
    cvShowImage("picture", cvimg2);
    cvWaitKey(0);
    cvDestroyWindow("picture");
    CHECKED_FREE(img_test, img_w * img_h * 3);
    cvReleaseImage(&cvimg2);
#endif

EXT:
    CHECKED_FREE(bodyimg2, body_w * body_h * 3 / 2);
    CHECKED_FREE(bodyimg1, body_w * body_h * 3 / 2);
    CHECKED_FREE(bina_img, body_w * body_h * 3 / 2);

    *car_color_trust = trust_flag;
    *car_color = body_flag;

    return;
}
