#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "plate_deskewing.h"
#include "plate_segmentation.h"
//#include "../common/threshold.h"
#include "../common/image.h"
//#include "opencv2/opencv.hpp"
//using namespace cv;

#ifdef _TMS320C6X_
#include <tistdtypes.h>
#include <std.h>
#include <mem.h>
#endif

#ifdef WIN321
#include "cv.h"
#include "cxcore.h"
#include "highgui.h"
#define DEBUG_SHOW_DESKEW 0
#endif

#ifdef _TMS320C6X_
#pragma CODE_SECTION(plate_deskewing_single, ".text:plate_deskewing_single")
#pragma CODE_SECTION(plate_deskewing_double, ".text:plate_deskewing_double")
#pragma CODE_SECTION(thresholding_avg_to_0or1, ".text:thresholding_avg_to_0or1")
#pragma CODE_SECTION(image_rotate_coarse_v1, ".text:image_rotate_coarse_v1")
#pragma CODE_SECTION(get_max_grad_of_current_angle, ".text:get_max_grad_of_current_angle")
#pragma CODE_SECTION(image_rotate_precise_v1, ".text:image_rotate_precise_v1")
//#pragma CODE_SECTION(image_rotate_coarse_h1, ".text:image_rotate_coarse_h1")
//#pragma CODE_SECTION(image_rotate_precise_h1, ".text:image_rotate_precise_h1")
#endif

#define NSHIFT 15

//利用梯度信息二值化
int32_t thresholding_e_x_y_0or1(uint8_t *gray_img, uint8_t *bina_img,

                                int32_t img_w, int32_t img_h)
{
    int32_t pix;
    int32_t w, h;
    int32_t sum_exy;
    int32_t sum_gray;
    int32_t area = img_w * img_h;
    int32_t ex, ey, exy;
    int32_t thr = 1;

    sum_exy = 0;
    sum_gray = 0;

    for (h = 1; h < img_h - 1; h++) //
    {
        for (w = 1; w < img_w - 1; w++)
        {
            pix = h * img_w + w;
            ex = (int32_t)(gray_img[pix + 1] - gray_img[pix - 1]);
            ey = (int32_t)(gray_img[pix + img_w] - gray_img[pix - img_w]);
            exy = abs(ex) + abs(ey);
            exy = (exy * (int32_t)gray_img[pix]) >> 8;

            sum_exy += exy;
            sum_gray += exy * (int32_t)gray_img[pix];
        }
    }

    if (sum_exy)
    {
        thr = sum_gray / sum_exy;
    }

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = ((int32_t)gray_img[pix] < thr) ? 0u : 1u;
    }

    return thr;
}

// 根据均值进行图像二值化，取图像的中心部分去除干扰
#ifdef _TMS320C6X_
static int32_t thresholding_avg_to_0or1(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                        int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    int32_t w, h;
    int32_t pix;
    const int32_t area = img_w * img_h;
    int32_t cnt = (w1 - w0 + 1) * (h1 - h0 + 1);
    int32_t avg;
    uint32_t sum = 0;
    int32_t n, k;
    uint32_t t0, t1, t2, t3;
    uint32_t t4 = 0;
    uint32_t edge0, edge1, edge2, edge3;
    uint32_t comp0, comp1, comp2, comp3;
    uint32_t ret0, ret1, ret2, ret3;

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            sum += gray_img[img_w * h + w];
        }
    }

    avg = sum / cnt;

    t0 = t1 = avg;
    t2 = _pack2(t0, t1);
    t3 = t2 << 8;
    t4 = t2 | t3;

    n = area >> 4;

    for (k = 0; k < n; k++)
    {
        edge0 = _amem4(gray_img + (k << 4) +  0);
        edge1 = _amem4(gray_img + (k << 4) +  4);
        edge2 = _amem4(gray_img + (k << 4) +  8);
        edge3 = _amem4(gray_img + (k << 4) + 12);

        comp0 = _cmpgtu4(edge0, t4);
        comp1 = _cmpgtu4(edge1, t4);
        comp2 = _cmpgtu4(edge2, t4);
        comp3 = _cmpgtu4(edge3, t4);

        ret0  = _xpnd4(comp0);
        ret1  = _xpnd4(comp1);
        ret2  = _xpnd4(comp2);
        ret3  = _xpnd4(comp3);

        _amem4(bina_img + (k << 4) +  0) = ret0 & 0x01010101;
        _amem4(bina_img + (k << 4) +  4) = ret1 & 0x01010101;
        _amem4(bina_img + (k << 4) +  8) = ret2 & 0x01010101;
        _amem4(bina_img + (k << 4) + 12) = ret3 & 0x01010101;
    }

    for (pix = n << 4; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] > avg) ? 1 : 0x00;
    }

    return avg;
}
#elif WIN321
static int32_t thresholding_avg_to_0or1(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                        int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    int32_t w, h;
    int32_t pix;
    const int32_t area = img_w * img_h;
    const int32_t cnt = ((w1 - w0 + 1) * (h1 - h0 + 1));
    uint32_t avg;
    uint32_t sum = (0u);
    int32_t n;
    uint8_t avg128[16];
    uint8_t mask01[16];

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            sum += gray_img[img_w * h + w];
        }
    }

    avg = (sum / (uint32_t)cnt);

    memset(avg128, avg, (16u));
    memset(mask01, 0x01, (16u));

    n = area >> 4;

    _asm
    {
        mov     ecx, n          // put count to ecx reg
        mov     esi, gray_img   // put src address to esi reg
        mov     edi, bina_img   // put dst address to edi reg

        movdqu  xmm1, avg128

        pxor    xmm3, xmm3      // xmm3 = 0000 0000 0000 0000 H
//      pcmpeqb xmm2, xmm2      // xmm2 = FFFF FFFF FFFF FFFF H
        movdqu  xmm2, mask01    // xmm2 = 0001 0001 0001 0001 H

        code_loop:
        movdqu  xmm0, [esi]

        // psubusb 将压缩无符号字节整数相减，如果单个字节结果小于0（负值），则将饱和值00H写入目标操作数
        psubusb xmm0, xmm1

        // pcmpeqb 将目的寄存器与源存储器按字节比较，如果对应字节相等，就置目的寄存器对应字节为FFH，否则为00H
        pcmpeqb xmm0, xmm3      // xmm3 = 0000 0000 0000 0000 H

        // pandn 目的寄存器128个二进制位先取'非'，再'与'源存储器128个二进制位，内存变量必须16字节对齐
        pandn   xmm0, xmm2

        movdqu  [edi], xmm0
        add     esi, 16         // add src pointer by 16 bytes
        add     edi, 16         // add dst pointer by 16 bytes
        dec     ecx             // decrement count by 1
        jnz     code_loop       // jump to code_loop if not Zero
    }

    for (pix = n << 4; pix < (int32_t)area; pix++)
    {
        bina_img[pix] = (uint8_t)((gray_img[pix] > (uint8_t)avg) ? (uint8_t)1 : (uint8_t)0);
    }

    return avg;
}
#else
static int32_t thresholding_avg_to_0or1(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                        int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    int32_t w, h;
    int32_t pix;
    const int32_t area = img_w * img_h;
    const int32_t cnt = (w1 - w0 + 1) * (h1 - h0 + 1);
    int32_t avg;
    uint32_t sum = 0;

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            sum += (uint32_t)gray_img[img_w * h + w];
        }
    }

    avg = (uint32_t)(sum / cnt);

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] > (uint8_t)avg) ? (uint8_t)1 : (uint8_t)0;
    }

    return avg;
}
#endif


/********************************************************************
Function:       image_rotate_precise
Description:    双线性插值方式实现图像垂直旋转(针对灰度图像和RGB彩色图像)
                旋转后的图像和原图像相同大小，因为旋转产生的多余部分被忽略

  1.  Param:
    src_img:    源图像
    dst_img:    目标图像
    angle:      角度(角度制)
    plate_w：   图像宽度
    plate_h：   图像高度

    2.  Return:
    void
********************************************************************/
static void image_rotate_precise_v1(uint8_t *restrict rott_img, uint8_t *restrict plate_img,
                                    int32_t angle, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    float xx, yy;
    int32_t x, y;
    int32_t fx, fy, fxy;
    int32_t ix, iy;
    int32_t pixel;
    int32_t m0, m1, m2, m3;
    float angle_arc = angle * 3.14f / 180;  // 弧度
    const int32_t border_x0 = 1 << NSHIFT;
    const int32_t border_x1 = (plate_w - 2) << NSHIFT;
    const int32_t border_y0 = 1 << NSHIFT;
    const int32_t border_y1 = (plate_h - 2) << NSHIFT;
    int32_t cosx = (int32_t)(cos(-angle_arc) * (1 << NSHIFT));
    int32_t sinx = (int32_t)(sin(-angle_arc) * (1 << NSHIFT));

    memset(rott_img, 0, sizeof(uint8_t) * plate_w * plate_h);

    xx = plate_w / 2.0f;
    yy = plate_h / 2.0f;

    m0 = (int32_t)(-xx * cosx + yy * sinx + xx * (1 << NSHIFT));
    m1 = (int32_t)(-xx * sinx - yy * cosx + yy * (1 << NSHIFT));

    // 对图像进行旋转
    for (h = 0; h < plate_h; h++)
    {
        m2 = m0 - h * sinx;
        m3 = m1 + h * cosx;

        for (w = 0; w < plate_w; w++)
        {
            x = w * cosx + m2;
            y = w * sinx + m3;
            //  x = (w - xx) * cosx - (h - yy) * sinx + xx;
            //  y = (w - xx) * sinx + (h - yy) * cosx + yy;

            if ((x >= border_x0) && (x <= border_x1) && (y >= border_y0) && (y <= border_y1))
            {
                fx = x - ((x >> NSHIFT) << NSHIFT);
                fy = y - ((y >> NSHIFT) << NSHIFT);
                fxy = (fx * fy) >> NSHIFT;
                ix = x >> NSHIFT;
                iy = y >> NSHIFT;

                pixel = (int32_t)(((1 << NSHIFT) - fy - fx + fxy) * (int32_t)plate_img[(plate_w * (iy + 0) + ix)]
                                  + (fx - fxy) * (int32_t)plate_img[(plate_w * (iy + 0) + (ix + 1))]
                                  + (fy - fxy) * (int32_t)plate_img[(plate_w * (iy + 1) + (ix + 0))]
                                  + (fxy) * (int32_t)plate_img[(plate_w * (iy + 1) + (ix + 1))]);

                rott_img[(plate_w * h + w)] = (uint8_t)(pixel >> NSHIFT);
            }
        }
    }

    // 填充图像左右边界的空白象素
    for (h = 0; h < plate_h; h++)
    {
        for (w = 8; w >= 0; w--)
        {
            if (rott_img[plate_w * h + w] == (uint8_t)0)
            {
                rott_img[plate_w * h + w] = rott_img[plate_w * h + (w + 1)];
            }
        }

        for (w = plate_w - 8; w < plate_w; w++)
        {
            if (rott_img[plate_w * h + w] == (uint8_t)0)
            {
                rott_img[plate_w * h + w] = rott_img[plate_w * h + (w - 1)];
            }
        }
    }

    return;
}

#define DEBUG_ROTATE_COARSE_FILE_OUT

#ifdef DEBUG_ROTATE_COARSE_FILE_OUT
#define DEBUG_ROTATE_COARSE_FILE_WRITE(file_name)    \
    {                                                 \
        FILE *txt = fopen(file_name, "w");            \
        int32_t pix;                                  \
        if(txt == NULL)                               \
        {                                             \
            printf("can't open %s!\n", file_name);    \
            while(1);                                 \
        }                                             \
        for(pix = 0; pix < plate_w * plate_h; pix++)      \
        {                                             \
            if(pix % plate_w == 0)                         \
            {                                         \
                fprintf(txt, "\n");                   \
            }                                         \
            fprintf(txt, "%3d ", rott_img[pix]);      \
        }                                             \
        fclose(txt);                                  \
    }
#else
#define DEBUG_ROTATE_COARSE_FILE_WRITE(file_name)    NULL
#endif

/********************************************************************
Function:       image_rotate_coarse
Description:    对一通道图像进行快速垂直旋转(部分图像信息丢失)

  1.  Param:
    src_img:    源图像
    dst_img:    目标图像
    angle:      角度(角度制)
    plate_w：   图像宽度
    plate_h：   图像高度

    2.  Return:
    void
********************************************************************/
#define SHIFT_NUMBER (10)
//#ifdef WIN32
#if defined(__GNUC__) || defined(WIN32)
static void image_rotate_coarse_v1(uint8_t *restrict rott_img, uint8_t *restrict bina_img, uint16_t *restrict tanx_img,
                                   int32_t angle, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t address0;
//     int32_t address0, address1, address2, address3;
//     int32_t address4, address5, address6, address7;
    // 0度到8度的正弦值并乘以1024 (1 << SHIFT_NUMBER)以将浮点数转换成整数运算
    const int32_t tan0to8[] = {0, 18, 36, 54, 72, 90, 108, 126, 144, 160, 178, 195, 213, 230, 248, 265, 282, 299, 316, 333, 350};
    int32_t tan_x;  // 正弦值
    uint8_t *restrict bina_ptr = NULL;

    memset(rott_img, 0, sizeof(uint8_t) * plate_w * plate_h);

    if (angle > 0)
    {
        tan_x = tan0to8[angle];

        for (h = 0; h < plate_h - 1; h++)
        {
            bina_ptr = bina_img + plate_w * h;

            for (w = 0; w < plate_w; w++)
            {
                address0 = MIN2(((w * tan_x) >> SHIFT_NUMBER) + h, plate_h - 1) * plate_w + w;
                rott_img[address0] = bina_ptr[w];
            }
        }
    }
    else
    {
        tan_x = tan0to8[-angle];

        for (h = 0; h < plate_h - 1; h++)
        {
            bina_ptr = bina_img + plate_w * h;

            for (w = 0; w < plate_w; w++)
            {
                address0 = MIN2((((plate_w - w) * tan_x) >> SHIFT_NUMBER) + h, plate_h - 1) * plate_w + w;
                rott_img[address0] = bina_ptr[w];
            }
        }
    }

    /*DEBUG_ROTATE_COARSE_FILE_WRITE("rotate.txt");*/

    return;
}
#else
static void image_rotate_coarse_v1(uint8_t *restrict rott_img, uint8_t *restrict bina_img, uint16_t *restrict tanx_img,
                                   int32_t angle, int32_t plate_w_tmp, int32_t plate_h_tmp)
{
    int32_t w, h;
    uint16_t plate_w = plate_w_tmp;
    uint16_t plate_h = plate_h_tmp;
    uint16_t plate_h_max = plate_h_tmp - 1;
    const int32_t area = plate_w * plate_h;
    // 0度到8度的正弦值并乘以1024 (1 << SHIFT_NUMBER)以将浮点数转换成整数运算
    const int32_t tan0to8[] = {0, 18, 36, 54, 72, 90, 108, 126, 144};
    int32_t tan_x;  // 正弦值
    uint8_t *restrict bina_ptr = bina_img;
    uint16_t *restrict tanx_ptr = tanx_img; // tanx_img用来保存一些中间结果以减少运算

    memset(rott_img, 0, sizeof(uint8_t) * plate_w * plate_h);

    if (angle > 0)
    {
        tan_x = tan0to8[angle];

        for (w = 0; w < plate_w; w++)
        {
            tanx_ptr[w] = (uint16_t)((w * tan_x) >> SHIFT_NUMBER);
        }
    }
    else
    {
        tan_x = tan0to8[-angle];

        for (w = 0; w < plate_w; w++)
        {
            tanx_ptr[w] = (uint16_t)(((plate_w - w) * tan_x) >> SHIFT_NUMBER);
        }
    }

    for (h = 0; h < plate_h - 1; h++)
    {
        int32_t i;
        uint32_t u32SrcHH = (h << 16) | h;
        uint32_t u32MaxHH = (plate_h_max << 16) | plate_h_max;
        uint32_t u32SrcPW = (plate_w << 16) | plate_w;
        uint32_t u32Tanx0, u32Tanx1, u32Tanx2, u32Tanx3;
        uint32_t u32Addr0, u32Addr1, u32Addr2, u32Addr3;
        uint32_t u32Addr4, u32Addr5, u32Addr6, u32Addr7;
        uint32_t u32Dst00, u32Dst01, u32Dst02, u32Dst03;
        int32_t address0, address1, address2, address3;
        int32_t address4, address5, address6, address7;

        bina_ptr = bina_img + plate_w * h;

        for (i = 0; i < plate_w / 8; i++)
        {
            u32Tanx0 = _amem4(tanx_ptr + (i << 3) + 0);
            u32Tanx1 = _amem4(tanx_ptr + (i << 3) + 2);
            u32Tanx2 = _amem4(tanx_ptr + (i << 3) + 4);
            u32Tanx3 = _amem4(tanx_ptr + (i << 3) + 6);

            u32Dst00 = _add2(u32Tanx0, u32SrcHH);
            u32Dst01 = _add2(u32Tanx1, u32SrcHH);
            u32Dst02 = _add2(u32Tanx2, u32SrcHH);
            u32Dst03 = _add2(u32Tanx3, u32SrcHH);

            u32Dst00 = _min2(u32Dst00, u32MaxHH);
            u32Dst01 = _min2(u32Dst01, u32MaxHH);
            u32Dst02 = _min2(u32Dst02, u32MaxHH);
            u32Dst03 = _min2(u32Dst03, u32MaxHH);

            u32Addr0 = _mpyu(u32Dst00, u32SrcPW);
            u32Addr1 = _mpyhu(u32Dst00, u32SrcPW);
            u32Addr2 = _mpyu(u32Dst01, u32SrcPW);
            u32Addr3 = _mpyhu(u32Dst01, u32SrcPW);
            u32Addr4 = _mpyu(u32Dst02, u32SrcPW);
            u32Addr5 = _mpyhu(u32Dst02, u32SrcPW);
            u32Addr6 = _mpyu(u32Dst03, u32SrcPW);
            u32Addr7 = _mpyhu(u32Dst03, u32SrcPW);

            address0 = u32Addr0 + (i << 3) + 0;
            address1 = u32Addr1 + (i << 3) + 1;
            address2 = u32Addr2 + (i << 3) + 2;
            address3 = u32Addr3 + (i << 3) + 3;
            address4 = u32Addr4 + (i << 3) + 4;
            address5 = u32Addr5 + (i << 3) + 5;
            address6 = u32Addr6 + (i << 3) + 6;
            address7 = u32Addr7 + (i << 3) + 7;

            rott_img[address0] = bina_ptr[(i << 3) + 0];
            rott_img[address1] = bina_ptr[(i << 3) + 1];
            rott_img[address2] = bina_ptr[(i << 3) + 2];
            rott_img[address3] = bina_ptr[(i << 3) + 3];
            rott_img[address4] = bina_ptr[(i << 3) + 4];
            rott_img[address5] = bina_ptr[(i << 3) + 5];
            rott_img[address6] = bina_ptr[(i << 3) + 6];
            rott_img[address7] = bina_ptr[(i << 3) + 7];
        }

        for (w = ((plate_w >> 3) << 3); w < plate_w; w++)
        {
            address0 = MIN2(tanx_ptr[w] + h, plate_h_max) * plate_w + w;
            rott_img[address0] = bina_ptr[w];
        }
    }

    /*    DEBUG_ROTATE_COARSE_FILE_WRITE("rotate.txt");*/

    return;
}
#endif
#undef SHIFT_NUMBER

#if 0
// 对一通道图像进行快速水平旋转(部分图像信息丢失)
static void image_rotate_coarse_h1(uint8_t *restrict plate_img, int32_t angle, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t y1;
    uint8_t *restrict temp_img = NULL;
    uint16_t *restrict tanx_img = NULL;
    float angle_arc = angle * 3.14f / 180;  // 弧度
    float t = (float)fabs(tan(angle_arc));

#ifdef _TMS320C6X_
    tanx_img = (uint16_t *)MEM_alloc(IRAM, sizeof(uint16_t) * plate_h, 128);
    temp_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
#else
    CHECKED_MALLOC(tanx_img, sizeof(uint16_t) * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(temp_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
#endif

    memset(temp_img, 0, sizeof(uint8_t) * plate_w * plate_h);

    if (angle > 0)
    {
        for (h = 0; h < plate_h; h++)
        {
            tanx_img[h] = (int32_t)(h * t);
        }
    }
    else
    {
        for (h = 0; h < plate_h; h++)
        {
            tanx_img[h] = (int32_t)((plate_h - h) * t);
        }
    }

    for (w = plate_w - 1; w >= 0; w--)
    {
        for (h = 0; h < plate_h; h++)
        {
            y1 = MIN2(w + tanx_img[h], plate_w - 1);

            temp_img[plate_w * h + y1] = plate_img[plate_w * h + w];
        }
    }

    memcpy(plate_img, temp_img, sizeof(uint8_t) * plate_w * plate_h);

#ifdef _TMS320C6X_
    MEM_free(IRAM, tanx_img, sizeof(uint16_t) * plate_h);
    MEM_free(IRAM, temp_img, sizeof(uint8_t) * plate_w * plate_h);
#else
    CHECKED_FREE(tanx_img);
    CHECKED_FREE(temp_img);
#endif

    return;
}

static void plate_deskewing_h(uint8_t *restrict plate_img, int32_t cmp_y0, int32_t cmp_y1,
                              int32_t plate_w, int32_t plate_h, int32_t x0, int32_t x1)
{
//  int32_t w;
    int32_t h;
//  int32_t k;
//    int32_t rangle; // 车牌的旋转角度
    int32_t dangle; // 车牌的实际倾斜角度
//    int32_t grad;
//  int32_t curr_max;
//  int32_t last_max;
    int32_t *restrict hist_img = NULL;
    uint8_t *restrict rott_img = NULL;
    uint8_t *restrict bina_img = NULL;
//  uint8_t *restrict cany_img;

#if DEBUG_SHOW_DESKEW
    IplImage* gray = cvCreateImage(cvSize(plate_w, plate_h * 4), 8, 1);
    cvNamedWindow("gray", 1);
    printf("\n========================================================\n");
#endif

#ifdef _TMS320C6X_
    hist_img = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * plate_w, 128);
    rott_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
#else
    CHECKED_MALLOC(hist_img, sizeof(int32_t) * plate_w, CACHE_SIZE);
    CHECKED_MALLOC(rott_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
//  CHECKED_MALLOC(cany_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
#endif

//  last_max = 0;
    dangle = 0;

//  thresholding_avg_to_0or1(plate_img, bina_img, plate_w, plate_h, 0, plate_w-1, 0, plate_h-1);
    thresholding_avg_to_0or1(plate_img, bina_img, plate_w, plate_h, x0, x1, 0, plate_h - 1);

//  vertical_sobel_simple(plate_img, bina_img, plate_w, plate_h);
//
//  thresholding_percent_opt0(bina_img, cany_img, plate_w, plate_h, 0, plate_w - 1, 0, plate_h - 1, 5);
    /*
        for(rangle = -16; rangle <= 16; rangle+=2)
        {
            memcpy(rott_img, bina_img, plate_w * plate_h);

            image_rotate_coarse_h1(rott_img, rangle, plate_w, plate_h);

            memset(hist_img, 0, sizeof(int32_t) * plate_w);

            curr_max = 0;
            {
                int32_t w, h;
                int32_t y1;
                uint16_t *restrict tanx_img;
                float angle_arc = rangle * 3.14f / 180; // 弧度
                float t = (float)fabs(tan(angle_arc));

                CHECKED_MALLOC(tanx_img, sizeof(uint16_t) * plate_h, CACHE_SIZE);

                if(rangle > 0)
                {
                    for(h = 0; h < plate_h; h++)
                    {
                        tanx_img[h] = (int32_t)(h * t);
                    }
                }
                else
                {
                    for(h = 0; h < plate_h; h++)
                    {
                        tanx_img[h] = (int32_t)((plate_h - h) * t);
                    }
                }

                for(w = plate_w - 16; w >= 16; w--)
                {
                    k = 0;
                    for(h = 24; h < plate_h-24; h++)
                    {
                        y1 = MIN2(w + tanx_img[h], plate_w - 1);

                        k += bina_img[plate_w * h + y1] > 0;
                    }
                    hist_img[w] = k;
                    curr_max += k;
                }
                CHECKED_FREE(tanx_img);
            }

    //      curr_max = 0;
    //      for(w = 16; w < plate_w-16; w++)
    //      {
    //          k = 0;
    ////            for(h = 0; h < plate_h; h++)
    //          for(h = 16; h < plate_h-16; h++)
    //          {
    //              k += rott_img[plate_w * h + w] > 0;
    //          }
    //          hist_img[w] = k;
    ////            curr_max += k;
    //      }

            // 计算当前角度下的最大梯度值
    //      curr_max = 0;
    //      for(w = 1; w < plate_w - 1; w++)
    //      {
    //          grad = abs(hist_img[w] - hist_img[w - 1]) + abs(hist_img[w] - hist_img[w + 1]);
    //
    //          if(grad > curr_max)
    //          {
    //              curr_max = grad;
    //          }
    //      }

            curr_max = 0;
            for(w = 1; w < plate_w - 1; w++)
            {
                curr_max += hist_img[w] < 5;
            }
            // 记录具有最大梯度值的角度
            if(curr_max > last_max)
            {
                last_max = curr_max;
                dangle = rangle;
            }

    #if DEBUG_SHOW_DESKEW
            memset(gray->imageData, 0, gray->widthStep * plate_h * 3);

            for(h = 0; h < plate_h; h++)
            {
                for(w = 0; w < plate_w; w++)
                {
                    gray->imageData[gray->widthStep * h + w] = plate_img[plate_w * h + w];
                }
            }
            for(h = 0; h < plate_h; h++)
            {
                for(w = 0; w < plate_w; w++)
                {
                    gray->imageData[gray->widthStep * (h + plate_h * 1) + w] = rott_img[plate_w * h + w];
                }
            }
            for(w = 0; w < plate_w; w++)
            {
                for(h = 0; h < hist_img[w]; h++)
                {
                    gray->imageData[gray->widthStep * (plate_h - 1 - h + plate_h * 2) + w] = (uint8_t)255;
                }
            }
            if(last_max == curr_max)
            {
                uint8_t *temp_img;

                CHECKED_MALLOC(temp_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
                memcpy(temp_img, plate_img, sizeof(uint8_t) * plate_w * plate_h);

                image_rotate_coarse_h1(temp_img, dangle, plate_w, plate_h);

                for(h = 0; h < plate_h; h++)
                {
                    for(w = 0; w < plate_w; w++)
                    {
                        gray->imageData[gray->widthStep * (h + plate_h * 3) + w] = temp_img[plate_w * h + w];
                    }
                }
                CHECKED_FREE(temp_img);
            }
            printf("---angle: %2d, curr_max: %3d\n", rangle, curr_max);

            cvShowImage("gray", gray);
            cvWaitKey(0);
    #endif
        }
    */
    dangle = -3;

    if (dangle != 0)
    {
//      image_rotate_precise_h1(plate_img, dangle, plate_w, plate_h);
        image_rotate_coarse_h1(plate_img, dangle, plate_w, plate_h);
    }

    h = plate_h - cmp_y0 - cmp_y1;
    memcpy(plate_img, plate_img + plate_w * cmp_y0, plate_w * h);

#if DEBUG_SHOW_DESKEW
    printf("========================================================\n");
    cvReleaseImage(&gray);
    cvDestroyWindow("gray");
#endif

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist_img, sizeof(int32_t) * plate_h);
    MEM_free(IRAM, rott_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
#else
    CHECKED_FREE(bina_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(rott_img, sizeof(uint8_t) * plate_w * plate_h);
    CHECKED_FREE(hist_img, sizeof(int32_t) * plate_h);
//  CHECKED_FREE(cany_img);
#endif

    return;
}
#endif

#if DEBUG_SHOW_DESKEW
void plate_deskewing_debug_show(IplImage* gray, uint8_t *plate_img, uint8_t *rott_img, int32_t *hist_img,
                                int32_t plate_w, int32_t plate_h,
                                int32_t rangle, int32_t last_max, int32_t curr_max)
{
    int32_t w, h;

    cvZero(gray);

    for (h = 0; h < plate_h; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            gray->imageData[gray->widthStep * h + w] = plate_img[plate_w * h + w];
        }
    }

    for (h = 0; h < plate_h; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            gray->imageData[gray->widthStep * h + w + (plate_w * 1)] = (rott_img[plate_w * h + w] > 0) ? 255 : 0;
        }
    }

    for (h = 0; h < plate_h; h++)
    {
        for (w = 0; w < hist_img[h]; w++)
        {
            gray->imageData[gray->widthStep * h + w + (plate_w * 2)] = (uint8_t)255;
        }
    }

    if (last_max == curr_max)
    {
        image_rotate_precise_v1(rott_img, plate_img, rangle, plate_w, plate_h);

        for (h = 0; h < plate_h; h++)
        {
            for (w = 0; w < plate_w; w++)
            {
                gray->imageData[gray->widthStep * h + w + (plate_w * 3)] = rott_img[plate_w * h + w];
            }
        }
    }

    printf("---angle: %2d, curr_max: %3d\n", rangle, curr_max);

    cvShowImage("gray -- bina -- hist -- right", gray);
    cvWaitKey(100000);

    return;
}

void plate_deskewing_debug_show1(IplImage* gray, uint8_t *plate_img, uint8_t *rott_img, int32_t *hist_img,
                                 int32_t deskew_w, int32_t deskew_h, int32_t plate_w, int32_t plate_h,
                                 int32_t rangle, int32_t last_max, int32_t curr_max)
{
    int32_t w, h;

    cvZero(gray);

    for (h = 0; h < plate_h; h++)
    {
        for (w = 0; w < plate_w; w++)
        {
            gray->imageData[gray->widthStep * h + w] = plate_img[plate_w * h + w];
        }
    }

    for (h = 0; h < deskew_h; h++)
    {
        for (w = 0; w < deskew_w; w++)
        {
            gray->imageData[gray->widthStep * h + w + (plate_w * 1)] = (rott_img[deskew_w * h + w] > 0) ? 255 : 0;
        }
    }

    for (h = 0; h < deskew_h; h++)
    {
        for (w = 0; w < hist_img[h]; w++)
        {
            gray->imageData[gray->widthStep * h + w + (plate_w * 2)] = (uint8_t)255;
        }
    }

    //     if (last_max == curr_max)
    //     {
    //         image_rotate_precise_v1(rott_img, plate_img, rangle, plate_w, plate_h);
    //
    //         for (h = 0; h < plate_h; h++)
    //         {
    //             for (w = 0; w < plate_w; w++)
    //             {
    //                 gray->imageData[gray->widthStep * h + w + (plate_w * 3)] = rott_img[plate_w * h + w];
    //             }
    //         }
    //     }

    printf("---angle: %2d, curr_max: %3d\n", rangle, curr_max);

    cvShowImage("gray -- bina -- hist -- right", gray);
    cvWaitKey(0);

    return;
}
#endif

// 计算最大梯度方差
int32_t get_max_grad_var_of_current_angle(uint8_t *restrict rott_img, int32_t *restrict hist_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t bina_sum = 0;
    int32_t grad;
    int32_t sum, var;
    uint8_t *restrict rott_ptr = rott_img;

    memset(hist_img, 0, sizeof(int32_t) * plate_h);

    for (h = 0; h < plate_h; h++)
    {
        bina_sum = 0;

        for (w = 0; w < plate_w; w++)
        {
            bina_sum += *(rott_ptr + w);
        }

        hist_img[h] = bina_sum;
        rott_ptr += plate_w;
    }

    // 计算当前角度下的最大梯度方差
    sum = 0;
    var = 0;

    //     for (h = 1; h < plate_h - 1; h++)
    //      for (h = 16; h < plate_h - 16; h++)
    // 去掉上下5%区域进行统计，避免上下边框干扰
    for (h = plate_h * 5 / 100; h < plate_h - plate_h * 5 / 100; h++)
    {
        grad = abs(hist_img[h] - hist_img[h - 1]) + abs(hist_img[h] - hist_img[h + 1]);
        sum += grad;
        var += grad * grad;
    }

    var -= sum;

    return var;
}

// 计算最大梯度
int32_t get_max_grad_of_current_angle(uint8_t *restrict rott_img, int32_t *restrict hist_img, int32_t plate_w, int32_t plate_h)
{
    int32_t w, h;
    int32_t bina_sum = 0;
    int32_t curr_max;
    int32_t grad;
    uint8_t *restrict rott_ptr = rott_img;

    memset(hist_img, 0, sizeof(int32_t) * plate_h);

    for (h = 0; h < plate_h; h++)
    {
        bina_sum = 0;

        for (w = 0; w < plate_w; w++)
        {
            bina_sum += *(rott_ptr + w);
        }

        hist_img[h] = bina_sum;
        rott_ptr += plate_w;
    }

    // 计算当前角度下的最大梯度值
    curr_max = 0;

    //    for (h = 1; h < plate_h - 1; h++)
    //     for (h = 16; h < plate_h - 16; h++)
    // 去掉上下5%区域进行统计，避免上下边框干扰, 测试了82413个车牌，识别率提高了0.8%
    for (h = plate_h * 5 / 100; h < plate_h - plate_h * 5 / 100; h++)
    {
        grad = abs(hist_img[h] - hist_img[h - 1]) + abs(hist_img[h] - hist_img[h + 1]);

        if (grad > curr_max)
        {
            curr_max = grad;
        }
    }

    return curr_max;
}




#if 0
// 单层牌倾斜校正
void plate_deskewing_single(uint8_t *restrict plate_img, int32_t cmp_y0, int32_t cmp_y1,
                            int32_t plate_w, int32_t plate_h, int32_t x0, int32_t x1)
{
    int32_t w, h;
    int32_t bina_sum = 0;
    int32_t rangle; // 车牌的旋转角度
    int32_t dangle; // 车牌的实际倾斜角度
    int32_t grad;
    int32_t curr_max;
    int32_t last_max;
    int32_t *restrict hist_img = NULL;
    uint8_t *restrict rott_img = NULL;
    uint8_t *restrict rott_ptr = NULL;
    uint8_t *restrict bina_img = NULL;
    uint16_t *restrict tanx_img = NULL;

    CLOCK_INITIALIZATION();

#if DEBUG_SHOW_DESKEW
    IplImage* gray = cvCreateImage(cvSize(plate_w * 4, plate_h), 8, 1);
    cvNamedWindow("gray -- bina -- hist -- right", 1);
    printf("\n========================================================\n");
#endif

#ifdef _TMS320C6X_1
    hist_img = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * plate_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    rott_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    tanx_img = (uint16_t *)MEM_alloc(IRAM, sizeof(uint16_t) * plate_w, 128);
#else
    CHECKED_MALLOC(hist_img, sizeof(int32_t) * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(bina_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(rott_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
    CHECKED_MALLOC(tanx_img, sizeof(uint16_t) * plate_w, CACHE_SIZE);
#endif

    last_max = 0;
    dangle = 0;

    thresholding_avg_to_0or1(plate_img, bina_img, plate_w, plate_h, x0, x1, 0, plate_h - 1);


    for (rangle = -8; rangle <= 8; rangle++)
    {
        if (rangle != 0)
        {
            image_rotate_coarse_v1(rott_img, bina_img, tanx_img, rangle, plate_w, plate_h);
        }
        else
        {
            memcpy(rott_img, bina_img, plate_w * plate_h);
        }

        curr_max = get_max_grad_of_current_angle(rott_img, hist_img, plate_w, plate_h);

        // 记录具有最大梯度值的角度
        if (curr_max > last_max)
        {
            last_max = curr_max;
            dangle = rangle;
        }

#if DEBUG_SHOW_DESKEW
        plate_deskewing_debug_show(gray, plate_img, rott_img, hist_img,
                                   plate_w, plate_h, rangle, last_max, curr_max);
#endif
    }

    h = plate_h - cmp_y0 - cmp_y1;

    if (dangle != 0)
    {
        image_rotate_precise_v1(rott_img, plate_img, dangle, plate_w, plate_h);

        memcpy(plate_img, rott_img + plate_w * cmp_y0, plate_w * h);
    }
    else
    {
        memcpy(plate_img, plate_img + plate_w * cmp_y0, plate_w * h);
    }

//    memcpy(plate_img, rott_img, plate_w * plate_h);
//    plate_deskewing_h(plate_img, cmp_y0, cmp_y1, plate_w, plate_h, x0, x1);

#if DEBUG_SHOW_DESKEW
    printf("========================================================\n");
    cvReleaseImage(&gray);
    cvDestroyWindow("gray -- bina -- hist -- right");
#endif

#ifdef _TMS320C6X_1
    MEM_free(IRAM, hist_img, sizeof(int32_t) * plate_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, rott_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, tanx_img, sizeof(uint16_t) * plate_w);
#else
    CHECKED_FREE(hist_img);
    CHECKED_FREE(bina_img);
    CHECKED_FREE(rott_img);
    CHECKED_FREE(tanx_img);
#endif

    return;
}
#else
// 单层牌倾斜校正
void plate_deskewing_single(uint8_t *restrict plate_img, int32_t cmp_y0, int32_t cmp_y1,
                            int32_t plate_w, int32_t plate_h, int32_t x0, int32_t x1)
{
    int32_t h;
    int32_t rangle; // 车牌的旋转角度
    int32_t dangle; // 车牌的实际倾斜角度
    int32_t curr_max;
    int32_t last_max;
    int32_t *restrict hist_img = NULL;
    uint8_t *restrict rott_img = NULL;
    uint8_t *restrict bina_img = NULL;
    uint16_t *restrict tanx_img = NULL; // tanx_img的最后一行用来保存一些中间结果以减少运算
    int32_t peak_cnt;

#if DEBUG_SHOW_DESKEW
    IplImage* gray = cvCreateImage(cvSize(plate_w * 4, plate_h), 8, 1);
    cvNamedWindow("gray -- bina -- hist -- right", 1);
    printf("\n========================================================\n");
#endif

#ifdef _TMS320C6X_1
    hist_img = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * plate_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    rott_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    tanx_img = (uint16_t *)MEM_alloc(IRAM, sizeof(uint16_t) * plate_w, 128);
#else
//     CHECKED_MALLOC(hist_img, sizeof(int32_t) * plate_h, CACHE_SIZE);
//     CHECKED_MALLOC(bina_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
//     CHECKED_MALLOC(rott_img, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
//     CHECKED_MALLOC(tanx_img, sizeof(uint16_t) * plate_w, CACHE_SIZE);
    //hist_img = new int32_t [plate_h];
    //bina_img = new uint8_t [plate_w * plate_h];
    //rott_img = new uint8_t [plate_w * plate_h];
    //tanx_img = new uint16_t [plate_w];

    hist_img = malloc(plate_h * sizeof(int32_t));
    bina_img = malloc(plate_w * plate_h * sizeof(uint8_t));
    rott_img = malloc(plate_w * plate_h * sizeof(uint8_t));
    tanx_img = malloc(plate_w * sizeof(uint16_t));
#endif

    last_max = 0;
    dangle = 0;

#if 1
    thresholding_e_x_y_0or1(plate_img, bina_img, plate_w, plate_h);
#else
    thresholding_avg_to_0or1(plate_img, bina_img, plate_w, plate_h, x0, x1, 0, plate_h - 1);
#endif

//     {
//         Mat img(plate_h - cmp_y0 - cmp_y1, plate_w, CV_8UC1);
//         memcpy(img.data, plate_img + plate_w * cmp_y0, img.rows * plate_w);
//         cvtColor(img, img, CV_GRAY2BGR);
// 
//         Point2f center(img.cols / 2, img.rows / 2);
//         for (int angle = -20; angle < 20; angle++)
//         {
//             Mat m = cv::getRotationMatrix2D(center, angle, 1.0);
// 
//             Mat rotImg;
//             cv::warpAffine(img, rotImg, m, img.size(), INTER_LINEAR);
// 
//             imshow("rot", rotImg);
//             waitKey(0);
//         }
//     }

    peak_cnt = 0;

    for (rangle = 0; rangle >= -20; rangle--)
    {
        if (rangle != 0)
        {
            image_rotate_coarse_v1(rott_img, bina_img, tanx_img, rangle, plate_w, plate_h);
        }
        else
        {
            memcpy(rott_img, bina_img, plate_w * plate_h);
        }

#if 1
        curr_max = get_max_grad_var_of_current_angle(rott_img, hist_img, plate_w, plate_h);
#else
        curr_max = get_max_grad_of_current_angle(rott_img, hist_img, plate_w, plate_h);
#endif

        // 记录具有最优值的角度
        if (curr_max >= last_max)
        {
            last_max = curr_max;
            dangle = rangle;
            peak_cnt = 0;
        }
        else
        {
            peak_cnt++;
        }

//         if (peak_cnt > 3)
//         {
//             break;
//         }

//        thresholding_e_x_y_0or1(plate_img, bina_img, plate_w, plate_h);
//        printf("---angle: %2d\n", rangle);

#if DEBUG_SHOW_DESKEW
        plate_deskewing_debug_show(gray, plate_img, rott_img, hist_img,
                                   plate_w, plate_h, rangle, last_max, curr_max);
#endif
    }

    peak_cnt = 0;

    for (rangle = 1; rangle <= 20; rangle++)
    {
        image_rotate_coarse_v1(rott_img, bina_img, tanx_img, rangle, plate_w, plate_h);

#if 1
        curr_max = get_max_grad_var_of_current_angle(rott_img, hist_img, plate_w, plate_h);
#else
        curr_max = get_max_grad_of_current_angle(rott_img, hist_img, plate_w, plate_h);
#endif

        // 记录具有最优值的角度
        if (curr_max > last_max)
        {
            last_max = curr_max;
            dangle = rangle;
            peak_cnt = 0;
        }
        else
        {
            peak_cnt++;
        }

//         if (peak_cnt > 3)
//         {
//             break;
//         }

#if DEBUG_SHOW_DESKEW
        plate_deskewing_debug_show(gray, plate_img, rott_img, hist_img,
                                   plate_w, plate_h, rangle, last_max, curr_max);
#endif
    }

    h = MAX2(0, plate_h - cmp_y0 - cmp_y1);

    if (dangle != 0)
    {
        image_rotate_precise_v1(rott_img, plate_img, dangle, plate_w, plate_h);
        memcpy(plate_img, rott_img + plate_w * cmp_y0, plate_w * h);
    }
    else
    {
        memcpy(plate_img, plate_img + plate_w * cmp_y0, plate_w * h);
    }

//    memcpy(plate_img, rott_img, plate_w * plate_h);
//    plate_deskewing_h(plate_img, cmp_y0, cmp_y1, plate_w, plate_h, x0, x1);


#if DEBUG_SHOW_DESKEW
    printf("========================================================\n");
    cvReleaseImage(&gray);
    cvDestroyWindow("gray -- bina -- hist -- right");
#endif

#ifdef _TMS320C6X_1
    MEM_free(IRAM, hist_img, sizeof(int32_t) * plate_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, rott_img, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, tanx_img, sizeof(uint16_t) * plate_w);
#else
//     CHECKED_FREE(tanx_img, sizeof(uint16_t) * plate_w);
//     CHECKED_FREE(rott_img, sizeof(uint8_t) * plate_w * plate_h);
//     CHECKED_FREE(bina_img, sizeof(uint8_t) * plate_w * plate_h);
//     CHECKED_FREE(hist_img, sizeof(int32_t) * plate_h);
    //delete[] tanx_img;
    //delete[] rott_img;
    //delete[] bina_img;
    //delete[] hist_img;
    free(tanx_img);
    free(rott_img);
    free(bina_img);
    free(hist_img);
#endif

    return;
}
#endif

// 双层牌倾斜校正
void plate_deskewing_double(uint8_t *restrict plate_img, int32_t cmp_y0, int32_t cmp_y1,
                            int32_t plate_w, int32_t plate_h, int32_t plate_h_up, int32_t x0, int32_t x1)
{
    int32_t h;
    int32_t rangle; // 车牌的旋转角度
    int32_t dangle; // 车牌的实际倾斜角度
    int32_t curr_max;
    int32_t last_max;
    int32_t *restrict hist_img = NULL;
    uint8_t *restrict rott_img = NULL;
    uint8_t *restrict rott_img_all = NULL;
    uint8_t *restrict bina_img = NULL;
    uint16_t *restrict tanx_img = NULL; // tanx_img的最后一行用来保存一些中间结果以减少运算
    int32_t peak_cnt;
    int32_t deskew_w, deskew_h;
    Rects rect_deskew;

#if DEBUG_SHOW_DESKEW
    IplImage* gray = cvCreateImage(cvSize(plate_w * 4, plate_h), 8, 1);
    cvNamedWindow("gray -- bina -- hist -- right", 1);
    printf("\n========================================================\n");
#endif

    // 只计算下层区域的倾斜角度
    rect_deskew.x0 = 0;
    rect_deskew.x1 = plate_w - 1;
    rect_deskew.y0 = MAX2(cmp_y0 + plate_h_up - 16, 0);
    rect_deskew.y1 = plate_h - 1;
    deskew_w = plate_w;
    deskew_h = rect_deskew.y1 - rect_deskew.y0 + 1;

    if (deskew_h <= 0)
    {
        h = plate_h - cmp_y0 - cmp_y1;
        memcpy(plate_img, plate_img + plate_w * cmp_y0, plate_w * h);
        return;
    }

#ifdef _TMS320C6X_1
    hist_img = (int32_t *)MEM_alloc(IRAM, sizeof(int32_t) * deskew_h, 128);
    bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * deskew_w * deskew_h, 128);
    rott_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * deskew_w * deskew_h, 128);
    rott_img_all = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * plate_w * plate_h, 128);
    tanx_img = (uint16_t *)MEM_alloc(IRAM, sizeof(uint16_t) * deskew_w, 128);
#else
//     CHECKED_MALLOC(hist_img, sizeof(int32_t) * deskew_h, CACHE_SIZE);
//     CHECKED_MALLOC(bina_img, sizeof(uint8_t) * deskew_w * deskew_h, CACHE_SIZE);
//     CHECKED_MALLOC(rott_img, sizeof(uint8_t) * deskew_w * deskew_h, CACHE_SIZE);
//     CHECKED_MALLOC(rott_img_all, sizeof(uint8_t) * plate_w * plate_h, CACHE_SIZE);
//     CHECKED_MALLOC(tanx_img, sizeof(uint16_t) * deskew_w, CACHE_SIZE);
    //hist_img = new int32_t[deskew_h];
    //bina_img = new uint8_t[deskew_w * deskew_h];
    //rott_img = new uint8_t[deskew_w * deskew_h];
    //rott_img_all = new uint8_t[plate_w * plate_h];
    //tanx_img = new uint16_t[deskew_w];

    hist_img = malloc(deskew_h * sizeof(int32_t));
    bina_img = malloc(deskew_w * deskew_h * sizeof(uint8_t));
    rott_img = malloc(deskew_w * deskew_h * sizeof(uint8_t));
    rott_img_all = malloc(plate_w * plate_h * sizeof(uint8_t));
    tanx_img = malloc(deskew_w * sizeof(uint16_t));
#endif

    last_max = 0;
    dangle = 0;

    thresholding_avg_to_0or1(plate_img + rect_deskew.y0 * deskew_w, bina_img,
                             deskew_w, deskew_h, x0, x1, 0, deskew_h - 1);

    peak_cnt = 0;

    for (rangle = 0; rangle >= -8; rangle--)
    {
        if (rangle != 0)
        {
            image_rotate_coarse_v1(rott_img, bina_img, tanx_img, rangle, deskew_w, deskew_h);
        }
        else
        {
            memcpy(rott_img, bina_img, deskew_w * deskew_h);
        }

        curr_max = get_max_grad_of_current_angle(rott_img, hist_img, deskew_w, deskew_h);

        // 记录具有最大梯度值的角度
        if (curr_max >= last_max)
        {
            last_max = curr_max;
            dangle = rangle;
            peak_cnt = 0;
        }
        else
        {
            peak_cnt++;
        }

        if (peak_cnt > 3)
        {
            break;
        }

#if DEBUG_SHOW_DESKEW
        plate_deskewing_debug_show1(gray, plate_img, rott_img, hist_img, deskew_w, deskew_h,
                                    plate_w, plate_h, rangle, last_max, curr_max);
#endif
    }

    peak_cnt = 0;

    for (rangle = 1; rangle <= 8; rangle++)
    {
        image_rotate_coarse_v1(rott_img, bina_img, tanx_img, rangle, deskew_w, deskew_h);

        curr_max = get_max_grad_of_current_angle(rott_img, hist_img, deskew_w, deskew_h);

        // 记录具有最大梯度值的角度
        if (curr_max > last_max)
        {
            last_max = curr_max;
            dangle = rangle;
            peak_cnt = 0;
        }
        else
        {
            peak_cnt++;
        }

        if (peak_cnt > 3)
        {
            break;
        }

#if DEBUG_SHOW_DESKEW
        plate_deskewing_debug_show1(gray, plate_img, rott_img, hist_img, deskew_w, deskew_h,
                                    plate_w, plate_h, rangle, last_max, curr_max);
#endif
    }

    h = MAX2(0, plate_h - cmp_y0 - cmp_y1);

    // 对整个双层牌区域进行倾斜校正
    if (dangle != 0)
    {
        image_rotate_precise_v1(rott_img_all, plate_img, dangle, plate_w, plate_h);
        memcpy(plate_img, rott_img_all + plate_w * cmp_y0, plate_w * h);
    }
    else
    {
        memcpy(plate_img, plate_img + plate_w * cmp_y0, plate_w * h);
    }

//    memcpy(plate_img, rott_img, plate_w * plate_h);
//    plate_deskewing_h(plate_img, cmp_y0, cmp_y1, plate_w, plate_h, x0, x1);

#if DEBUG_SHOW_DESKEW
    printf("========================================================\n");
    cvReleaseImage(&gray);
    cvDestroyWindow("gray -- bina -- hist -- right");
#endif

#ifdef _TMS320C6X_1
    MEM_free(IRAM, hist_img, sizeof(int32_t) * deskew_h);
    MEM_free(IRAM, bina_img, sizeof(uint8_t) * deskew_w * deskew_h);
    MEM_free(IRAM, rott_img, sizeof(uint8_t) * deskew_w * deskew_h);
    MEM_free(IRAM, rott_img_all, sizeof(uint8_t) * plate_w * plate_h);
    MEM_free(IRAM, tanx_img, sizeof(uint16_t) * deskew_w);
#else
//     CHECKED_FREE(tanx_img, sizeof(uint16_t) * deskew_w);
//     CHECKED_FREE(rott_img_all, sizeof(uint8_t) * plate_w * plate_h);
//     CHECKED_FREE(rott_img, sizeof(uint8_t) * deskew_w * deskew_h);
//     CHECKED_FREE(bina_img, sizeof(uint8_t) * deskew_w * deskew_h);
//     CHECKED_FREE(hist_img, sizeof(int32_t) * deskew_h);
    //delete[] tanx_img;
    //delete[] rott_img_all;
    //delete[] rott_img;
    //delete[] bina_img;
    //delete[] hist_img;

    
    free(tanx_img);
    free(rott_img_all);
    free(rott_img);
    free(bina_img);
    free(hist_img);
#endif

    return;
}
