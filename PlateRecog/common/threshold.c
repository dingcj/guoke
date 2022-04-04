#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../malloc_aligned.h"
#include "threshold.h"
//#include "../dma_interface.h"
#include "../debug_show_image.h"

#ifdef  WIN32
#include "..\ti64_c\cintrins.h"
#include "..\ti64_c\c64intrins.h"
#endif

#ifdef  WIN32_DEBUG
#include <string.h>
#endif

#define SIZE_INT32_0256 (sizeof(int32_t) * 256)
#define SIZE_UINT8_0256 (sizeof(uint8_t) * 256)
#define SIZE_INT32_1024 (sizeof(int32_t) * 1024)

#define DEBUG_VERTICAL_SOBEL0

#ifdef DEBUG_VERTICAL_SOBEL
#define DEBUG_VERTICAL_SOBEL_FILE_WRITE(file_name)    \
    {                                                 \
        FILE *txt = fopen(file_name, "w");            \
        int32_t pix;                                  \
        if(txt == NULL)                               \
        {                                             \
            printf("can't open %s!\n", file_name);    \
            while(1);                                 \
        }                                             \
        for(pix = 0; pix < img_w * img_h; pix++)      \
        {                                             \
            if(pix % 32 == 0)                         \
            {                                         \
                fprintf(txt, "\n");                   \
            }                                         \
            fprintf(txt, "%3d ", edge_img[pix]);      \
        }                                             \
        fclose(txt);                                  \
    }
#else
#define DEBUG_VERTICAL_SOBEL_FILE_WRITE(file_name)    NULL
#endif

// 对图像其中一块区域求水平梯度
#ifdef _TMS320C6X_
void vertical_sobel_simple_rect(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h,
                                Rects rect, int32_t rect_w, int32_t rect_h, int32_t rect_h2)
{
    int32_t w, h;
    int32_t edge;
    uint32_t u32SrcA0, u32SrcA1, u32SrcA2, u32SrcA3;
    uint32_t u32SrcB0, u32SrcB1, u32SrcB2, u32SrcB3;
    uint32_t u32Dst00, u32Dst01, u32Dst02, u32Dst03;

    if (rect_w != ((rect.x1 - rect.x0) / 2 * 2)
        || rect_h != ((rect.y1 - rect.y0) / 2 * 2)
        || rect_h2 != rect_h / 2)
    {
        return;
    }

    for (h = 0; h < rect_h2; h++)
    {
        int32_t cnt = rect_w >> 4;
        uint8_t *restrict A_src = gray_img + img_w * (rect.y0 + h * 2) + rect.x0;
        uint8_t *restrict A_dst = edge_img + rect_w * h + 1;

        for (w = 0; w < cnt; w++)
        {
            // .reg u32SrcAH:u32SrcAL;   |x|x|x|x|x|x|x|x| | |
            // .reg u32SrcBH:u32SrcBL;   | | |x|x|x|x|x|x|x|x|
            u32SrcA0 = _mem4(A_src + (w << 4) + 0);
            u32SrcA1 = _mem4(A_src + (w << 4) + 4);
            u32SrcA2 = _mem4(A_src + (w << 4) + 8);
            u32SrcA3 = _mem4(A_src + (w << 4) + 12);

            u32SrcB0 =  _mem4(A_src + (w << 4) + 2);
            u32SrcB1 =  _mem4(A_src + (w << 4) + 6);
            u32SrcB2 =  _mem4(A_src + (w << 4) + 10);
            u32SrcB3 =  _mem4(A_src + (w << 4) + 14);

            u32Dst00 = _subabs4(u32SrcA0, u32SrcB0);
            u32Dst01 = _subabs4(u32SrcA1, u32SrcB1);
            u32Dst02 = _subabs4(u32SrcA2, u32SrcB2);
            u32Dst03 = _subabs4(u32SrcA3, u32SrcB3);

            _mem4(A_dst + (w << 4) + 0) = u32Dst00;
            _mem4(A_dst + (w << 4) + 4) = u32Dst01;
            _mem4(A_dst + (w << 4) + 8) = u32Dst02;
            _mem4(A_dst + (w << 4) + 12) = u32Dst03;
        }

        for (w = rect_w >> 4 << 4; w < rect_w - 1; w++)
        {
            edge = (int32_t)gray_img[img_w * (rect.y0 + h * 2) + rect.x0 + w + 1]
                   - gray_img[img_w * (rect.y0 + h * 2) + rect.x0 + w - 1];
            edge_img[rect_w * h + w] = (uint8_t)abs(edge);
        }
    }

    // 将左右边界上的点置为0
    edge_img[0] = 0;
    edge_img[rect_w * rect_h2 - 1] = 0;

    for (h = 1; h < rect_h2; h++)
    {
        memset(edge_img + rect_w * h - 1, 0, 2);
    }

    return;
}
#else
void vertical_sobel_simple_rect(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h,
                                Rects rect, int32_t rect_w, int32_t rect_h, int32_t rect_h2)
{
    int32_t w, h;
    int32_t edge = 0;

    if (rect_w != ((rect.x1 - rect.x0) / 2 * 2)
        || rect_h != ((rect.y1 - rect.y0) / 2 * 2)
        || rect_h2 != rect_h / 2)
    {
        return;
    }

    for (h = 0; h < rect_h2; h++)
    {
        for (w = 1; w < rect_w - 1; w++)
        {
            edge = (int32_t)gray_img[img_w * (rect.y0 + h * 2) + rect.x0 + w + 1]
                   - (int32_t)gray_img[img_w * (rect.y0 + h * 2) + rect.x0 + w - 1];
            edge_img[rect_w * h + w] = (uint8_t)abs(edge);
        }
    }

    // 将左右边界上的点置为0
    edge_img[0] = 0u;
    edge_img[rect_w * rect_h2 - 1] = 0u;

    for (h = 1; h < rect_h2; h++)
    {
        memset(edge_img + rect_w * h - 1, 0, 2u);
    }

    return;
}
#endif

#ifdef WIN321
void vertical_sobel_simple(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h)
{
    int32_t h;
    int32_t pix;
    int32_t edge;
    const int32_t area = img_w * img_h;
    const int32_t count = (area >> 5) - 1;
    uint8_t *restrict edge_ptr = edge_img + img_w - 1;

#define USE_NEW_WAY 0

#if USE_NEW_WAY
    uint8_t *restrict edge_dst = edge_img + 0;
#else
    uint8_t *restrict edge_dst = edge_img + 1;
#endif

    _asm
    {
        mov  ecx, count             // put count to ecx reg
        mov  esi, gray_img          // put src address to esi reg
        mov  edi, edge_dst          // put dst address to edi reg

        code_loop:
        movdqa   xmm0, [esi + 0x00] // move aligned double quadword from xmm2/m126 to xmm0
        movdqu   xmm1, [esi + 0x02] // move unaligned double quadword from xmm2/m126 to xmm1
        movdqa   xmm3, [esi + 0x10] // move aligned double quadword from xmm2/m126 to xmm1
        movdqu   xmm4, [esi + 0x12] // move unaligned double quadword from xmm2/m126 to xmm1

        movdqa   xmm2, xmm0
        movdqa   xmm5, xmm3

        // psubusb 将压缩无符号字节整数相减，如果单个字节结果小于0（负值），则将饱和值00H写入目标操作数。
        // psubusw 将压缩无符号字整数相减，如果单个字结果小于 0（负值），则将饱和值0000H写入目标操作数。
        psubusb  xmm0, xmm1
        psubusb  xmm1, xmm2

        psubusb  xmm3, xmm4
        psubusb  xmm4, xmm5

        por      xmm0, xmm1          // 绝对值和0做或运算还是绝对值
        por      xmm3, xmm4          // 绝对值和0做或运算还是绝对值

#if USE_NEW_WAY
        movntdq  [edi + 0x00], xmm0  // move aligned double quadword Non-Temporal
        movntdq  [edi + 0x10], xmm3  // move aligned double quadword Non-Temporal
#else
        movdqu   [edi + 0x00], xmm0  // move unaligned double quadword from xmm2/m126 to xmm1
        movdqu   [edi + 0x10], xmm3  // move unaligned double quadword from xmm2/m126 to xmm1
#endif
        add      esi, 0x20           // add src pointer by 32 bytes
        add      edi, 0x20           // add dst pointer by 32 bytes
        dec      ecx                 // decrement count by 1
        jnz      code_loop           // jump to code_loop if not Zero

#if USE_NEW_WAY
        sfence                       // flush write buffer
        emms
#endif
    }
#undef USE_NEW_WAY

    // 将左右边界上的点置为0
    edge_img[0] = (uint8_t)0;
    edge_img[img_w * img_h - 1] = (uint8_t)0;

    for (h = 0; h < img_h - 1; h++)
    {
        _mem2(edge_ptr) = 0x0000;
        edge_ptr += img_w;
    }

    // 计算末尾几个垂直边界值
    for (pix = (count << 5) + 1; pix < area - 1; pix++)
    {
        edge = (int32_t)(gray_img[pix - 1] - gray_img[pix + 1]);
        edge_img[pix] = (uint8_t)abs(edge);
    }

    DEBUG_VERTICAL_SOBEL_FILE_WRITE("vertical_sobel1.txt");

    return;
}
#elif defined _TMS320C6X_
void vertical_sobel_simple(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h)
{
    int32_t i;
    int32_t h;
    int32_t pix;
    int32_t edge;
    const int32_t area = img_w * img_h;
    const int32_t count = (area >> 4) - 1;
    uint8_t *restrict A_src = gray_img + 0;
    uint8_t *restrict B_src = gray_img + 2;
    uint8_t *restrict A_dst = edge_img + 1;
    uint8_t *restrict edge_ptr = edge_img + img_w - 1;
    uint32_t u32SrcA0, u32SrcA1, u32SrcA2, u32SrcA3;
    uint32_t u32SrcB0, u32SrcB1, u32SrcB2, u32SrcB3;
    uint32_t u32Dst00, u32Dst01, u32Dst02, u32Dst03;

    // 1363073 cycles
    for (i = 0; i < count; i++)
    {
        // .reg u32SrcAH:u32SrcAL;   |x|x|x|x|x|x|x|x| | |
        // .reg u32SrcBH:u32SrcBL;   | | |x|x|x|x|x|x|x|x|
        u32SrcA0 = _amem4(A_src + (i << 4) + 0);
        u32SrcA1 = _amem4(A_src + (i << 4) + 4);
        u32SrcA2 = _amem4(A_src + (i << 4) + 8);
        u32SrcA3 = _amem4(A_src + (i << 4) + 12);

        u32SrcB0 =  _mem4(B_src + (i << 4) + 0);
        u32SrcB1 =  _mem4(B_src + (i << 4) + 4);
        u32SrcB2 =  _mem4(B_src + (i << 4) + 8);
        u32SrcB3 =  _mem4(B_src + (i << 4) + 12);

        u32Dst00 = _subabs4(u32SrcA0, u32SrcB0);
        u32Dst01 = _subabs4(u32SrcA1, u32SrcB1);
        u32Dst02 = _subabs4(u32SrcA2, u32SrcB2);
        u32Dst03 = _subabs4(u32SrcA3, u32SrcB3);

        _mem4(A_dst + (i << 4) + 0) = u32Dst00;
        _mem4(A_dst + (i << 4) + 4) = u32Dst01;
        _mem4(A_dst + (i << 4) + 8) = u32Dst02;
        _mem4(A_dst + (i << 4) + 12) = u32Dst03;
    }

    // 将左右边界上的点置为0
    edge_img[0] = 0;
    edge_img[img_w * img_h - 1] = 0;

    for (h = 0; h < img_h - 1; h++)
    {
        _mem2(edge_ptr) = 0x0000;
        edge_ptr += img_w;
    }

    // 计算末尾几个垂直边界值
    for (pix = (count << 4) + 1; pix < area - 1; pix++)
    {
        edge = (int32_t)(gray_img[pix - 1] - gray_img[pix + 1]);
        edge_img[pix] = (uint8_t)abs(edge);
    }

//    DEBUG_VERTICAL_SOBEL_FILE_WRITE("vertical_sobel.txt");

    return;
}
#else
void vertical_sobel_simple(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t edge;
    const int32_t area = img_w * img_h;
    uint8_t *restrict edge_ptr = edge_img + img_w - 1;

    for (h = 0; h < img_h; h++)
    {
        for (w = 1; w < img_w - 1; w++)
        {
            edge = (int32_t)(gray_img[img_w * h + w - 1] - gray_img[img_w * h + w + 1]);
            edge_img[img_w * h + w] = (uint8_t)abs(edge);
        }
    }

    // 将左右边界上的点置为0
    for (h = 0; h < img_h; h++)
    {
        edge_img[h * img_w + 0] = 0;
        edge_img[h * img_w + img_w - 1] = 0;
    }

    DEBUG_VERTICAL_SOBEL_FILE_WRITE("vertical_sobel1.txt");

    return;
}
#endif

#if 0
void vertical_sobel(const uint8_t *restrict in, uint8_t *restrict out, int32_t img_w, int32_t img_h)
{
    int32_t H, O, V, i;
    int32_t i00, i01, i02;
    int32_t i10,      i12;
    int32_t i20, i21, i22;
    int32_t w = img_w;

    /* -------------------------------------------------------------------- */
    /*  Iterate over entire image as a single, continuous raster line.      */
    /* -------------------------------------------------------------------- */
    for (i = 0; i < img_w * (img_h - 2) - 2; i++)
    {
        /* ---------------------------------------------------------------- */
        /*  Read in the required 3x3 region from the input.                 */
        /* ---------------------------------------------------------------- */
        i00 = in[i    ]; i01 = in[i    + 1]; i02 = in[i    + 2];
        i10 = in[i +  w];                  i12 = in[i +  w + 2];
        i20 = in[i + 2 * w]; i21 = in[i + 2 * w + 1]; i22 = in[i + 2 * w + 2];

        /* ---------------------------------------------------------------- */
        /*  Apply horizontal and vertical filter masks.  The final filter   */
        /*  output is the sum of the absolute values of these filters.      */
        /* ---------------------------------------------------------------- */
        H = 0;

        V = -   i00         +   i02
            - 2 * i10       + 2 * i12
            -   i20         +   i22;

        O = abs(H) + abs(V);

        /* ---------------------------------------------------------------- */
        /*  Clamp to 8-bit range.  The output is always positive due to     */
        /*  the absolute value, so we only need to check for overflow.      */
        /* ---------------------------------------------------------------- */
        if (O > 255)
        {
            O = 255;
        }

        /* ---------------------------------------------------------------- */
        /*  Store it.                                                       */
        /* ---------------------------------------------------------------- */
        out[i + 1] = O;
    }

    return;
}
#endif

#define HIST_LOOKUPS_SIZE (sizeof(uint8_t) * 256 * 4)

// 对比度拉伸
void contrast_extend_for_gary_image(uint8_t *restrict dst_img, uint8_t *restrict src_img, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t n;
    int32_t area = img_w * img_h;
    int32_t thresh = area / 20;
    int32_t left, right;
    int32_t sum;
    int32_t *restrict hist = NULL;
    uint8_t *restrict hist_lookups = NULL;
//  float ratio;

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_lookups = (uint8_t *)MEM_alloc(IRAM, HIST_LOOKUPS_SIZE, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_lookups, HIST_LOOKUPS_SIZE, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);

    // 生成直方图
#if 1
    for (h = 0; h < img_h; h++)
    {
        for (w = img_w / 3; w < img_w - img_w / 3; w++)
        {
            hist[src_img[img_w * h + w]]++;
        }
    }
#else
    for (n = 0; n < area; n++)
    {
        hist[src_img[n]]++;
    }
#endif

    // 寻找左边界
    sum = 0;
    n = 0;

    while (sum < thresh)
    {
        sum += hist[n];
        n++;
    }

    left = n - 1;
    left = MAX2(0, left);

    // 寻找右边界
    sum = 0;
    n = 255;

    while (sum < thresh)
    {
        sum += hist[n];
        n--;
    }

    right = n + 1;
    right = MIN2(255, right);

//  ratio = (float)(right - left) / 255.0f;
//  printf("\n----left = %d----right = %d -----%f\n", left, right, ratio);

#if 1
    // 计算查找表
    memset(hist_lookups, 0, left);

    for (n = left; n < right; n++)
    {
        hist_lookups[n] = (uint8_t)((n - left) * 255 / (right - left));
    }

    memset(hist_lookups + right, 255, 256 - right);

    for (n = 0; n < area; n++)
    {
        dst_img[n] = hist_lookups[src_img[n]];
    }

#else
    // 计算查找表
    memset(hist_lookups, 0, left * 4);

    for (n = left; n < right; n++)
    {
        hist_lookups[n * 4 + 0] = (uint8_t)((n - left) * 255 / (right - left));
        hist_lookups[n * 4 + 1] = (uint8_t)((n - left) * 255 / (right - left));
        hist_lookups[n * 4 + 2] = (uint8_t)((n - left) * 255 / (right - left));
        hist_lookups[n * 4 + 3] = (uint8_t)((n - left) * 255 / (right - left));
    }

    memset(hist_lookups + right * 4, 255, (256 - right) * 4);

    for (n = 0; n < area; n += 4)
    {
        int p0 = (src_img[n + 0] << 2) + 0;
        int p1 = (src_img[n + 1] << 2) + 1;
        int p2 = (src_img[n + 2] << 2) + 2;
        int p3 = (src_img[n + 3] << 2) + 3;

        dst_img[n + 0] = hist_lookups[p0];
        dst_img[n + 1] = hist_lookups[p1];
        dst_img[n + 2] = hist_lookups[p2];
        dst_img[n + 3] = hist_lookups[p3];
    }

    for (n = ((area >> 2) << 2); n < area; n++)
    {
        int p0 = (src_img[n + 0] << 2) + 0;

        dst_img[n + 0] = hist_lookups[p0];
    }

#endif

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_lookups, HIST_LOOKUPS_SIZE);
#else
    CHECKED_FREE(hist_lookups, HIST_LOOKUPS_SIZE);
    CHECKED_FREE(hist, SIZE_INT32_0256);
#endif

    return;
}

// 根据提供的阈值二值化图像
#ifdef WIN321
void thresholding_fix(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t area, uint8_t thresh)
{
    int32_t k;
    uint8_t avg128[16];
    int32_t n;

    memset(avg128, thresh, (16u));
    n = area >> 5;

    if (n > 0)
    {
        _asm
        {
            mov     ecx, n          // put count to ecx reg
            mov     esi, gray_img   // put src address to esi reg
            mov     edi, bina_img   // put dst address to edi reg

            movdqu  xmm4, avg128    // xmm4 = tttt tttt tttt tttt H
            movdqu  xmm5, avg128    // xmm5 = tttt tttt tttt tttt H

            pxor    xmm6, xmm6      // xmm6 = 0000 0000 0000 0000 H
            pcmpeqb xmm7, xmm7      // xmm7 = FFFF FFFF FFFF FFFF H

            code_loop:
            movdqu   xmm0, [esi+0x00]
            movdqu   xmm1, [esi+0x10]

            // psubusb 两个压缩无符号字节整数的做饱和减法，如果单个字节结果小于0（负值）则将饱和值00H写入目标操作数
            psubusb xmm0, xmm4
            psubusb xmm1, xmm5
            // pcmpeqb 将目的寄存器与源存储器按字节比较，如果对应字节相等，就置目的寄存器对应字节为FFH，否则为00H
            pcmpeqb xmm0, xmm6      // xmm6 = 0000 0000 0000 0000 H
            pcmpeqb xmm1, xmm6      // xmm6 = 0000 0000 0000 0000 H
            // pandn 目的寄存器128个二进制位先取'非'，再'与'源存储器128个二进制位，内存变量必须16字节对齐
            pandn   xmm0, xmm7
            pandn   xmm1, xmm7

            movdqu  [edi+0x00], xmm0
            movdqu  [edi+0x10], xmm1

            add     esi, 0x20       // add src pointer by 32 bytes
            add     edi, 0x20       // add dst pointer by 32 bytes
            dec     ecx             // decrement count by 1
            jnz     code_loop       // jump to code_loop if not Zero
        }
    }

    for (k = n << 5; k < area; k++)
    {
        bina_img[k] = (uint8_t)((gray_img[k] > thresh) ? 255 : 0);
    }

    return;
}
#elif defined _TMS320C6X_
void thresholding_fix(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t area, uint8_t thresh)
{
    int32_t k;
    int32_t pix;
    uint32_t t0, t1, t2, t3;
    uint32_t t4 = 0;
    uint32_t edge0, edge1, edge2, edge3;
    uint32_t comp0, comp1, comp2, comp3;
    uint32_t ret0, ret1, ret2, ret3;

    t0 = t1 = thresh;
    t2 = _pack2(t0, t1);
    t3 = t2 << 8;
    t4 = t2 | t3;

    for (k = 0; k < (area >> 4); k++)
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

        _amem4(bina_img + (k << 4) +  0) = ret0;
        _amem4(bina_img + (k << 4) +  4) = ret1;
        _amem4(bina_img + (k << 4) +  8) = ret2;
        _amem4(bina_img + (k << 4) + 12) = ret3;
    }

    for (pix = (k << 4); pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] > thresh) ? 255 : 0;
    }

    return;
}
#else
void thresholding_fix(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t area, uint8_t thresh)
{
    int32_t pix;

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] > thresh) ? 255 : 0;
    }

    return;
}
#endif

int32_t thresholding_percent_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                 int32_t img_w, int32_t img_h, int32_t percent)
{
    int32_t i;
    const int32_t area = img_w * img_h;
    int32_t p0, p1, p2, p3;
    uint8_t t;
    int32_t *restrict hist = NULL;
    int32_t *restrict hist_sum = NULL;
    int32_t *restrict hist_tmp = NULL;

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_sum = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_tmp = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_1024, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_sum, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_tmp, SIZE_INT32_1024, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);
    memset(hist_tmp, 0, SIZE_INT32_1024);
//  memset(bina_img, 0, area);

    // 不考虑最后的余量，以保证内存访问不越界
    for (i = 0; i < area - 3; i += 4)
    {
        p0 = (gray_img[i + 0] << 2) + 0;
        p1 = (gray_img[i + 1] << 2) + 1;
        p2 = (gray_img[i + 2] << 2) + 2;
        p3 = (gray_img[i + 3] << 2) + 3;

        hist_tmp[p0]++;
        hist_tmp[p1]++;
        hist_tmp[p2]++;
        hist_tmp[p3]++;
    }

    for (i = 0; i < 256; i++)
    {
        hist[i] += (hist_tmp[(i << 2) + 0]
                    + hist_tmp[(i << 2) + 1]
                    + hist_tmp[(i << 2) + 2]
                    + hist_tmp[(i << 2) + 3]);
    }

    hist_sum[0] = hist[0];

    for (i = 1; i < 256; i++)
    {
        hist_sum[i] = hist_sum[i - 1] + hist[i];
    }

    // 按前景在图像中所占的比例percent%调整阈值
    for (i = 0; i < 256; i += 2)
    {
        if ((area - hist_sum[i]) * 100 < percent * area)
        {
            break;
        }
    }

    t = (uint8_t)MIN2(i, 250);

    thresholding_fix(gray_img, bina_img, area, t);

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_sum, SIZE_INT32_0256);
    MEM_free(IRAM, hist_tmp, SIZE_INT32_1024);
#else
    CHECKED_FREE(hist_tmp, SIZE_INT32_1024);
    CHECKED_FREE(hist_sum, SIZE_INT32_0256);
    CHECKED_FREE(hist, SIZE_INT32_0256);
#endif

    return t;
}

int32_t thresholding_percent_opt(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                 int32_t img_w, int32_t img_h,
                                 int32_t w0, int32_t w1, int32_t h0, int32_t h1, int32_t percent)
{
    int32_t w, h;
    int32_t i;
    const int32_t cnt = ((w1 - w0 + 1) >> 1) * ((h1 - h0 + 1));
    const int32_t area = img_w * img_h;
    int32_t p0, p1, p2, p3;
    int32_t *restrict hist = NULL;
    int32_t *restrict hist_sum = NULL;
    int32_t *restrict hist_tmp = NULL;
    uint8_t t;

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_sum = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_tmp = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_1024, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_sum, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_tmp, SIZE_INT32_1024, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);
//  memset(hist_sum, 0, SIZE_INT32_0256);
    memset(hist_tmp, 0, SIZE_INT32_1024);

    // 构建直方图，只有图像的中心部分参与构建直方图，排除外界干扰
    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1 - 7; w += 8)
        {
            p0 = (gray_img[img_w * h + w + 0] << 2) + 0;
            p1 = (gray_img[img_w * h + w + 2] << 2) + 1;
            p2 = (gray_img[img_w * h + w + 4] << 2) + 2;
            p3 = (gray_img[img_w * h + w + 6] << 2) + 3;

            hist_tmp[p0]++;
            hist_tmp[p1]++;
            hist_tmp[p2]++;
            hist_tmp[p3]++;
        }
    }

    for (i = 0; i < 256; i++)
    {
        hist[i] += (hist_tmp[(i << 2) + 0]
                    + hist_tmp[(i << 2) + 1]
                    + hist_tmp[(i << 2) + 2]
                    + hist_tmp[(i << 2) + 3]);
    }

    // 计算累积直方图，加速求阈值过程
    hist_sum[0] = hist[0];

    for (i = 1; i < 256; i++)
    {
        hist_sum[i] = hist[i] + hist_sum[i - 1];
    }

    // 按前景在图像中所占的比例percent%调整阈值
    for (i = 0; i < 256; i += 2)
    {
        if ((cnt - hist_sum[i]) * 100 < percent * cnt)
        {
            break;
        }
    }

    t = (uint8_t)i;

    thresholding_fix(gray_img, bina_img, area, t);

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_sum, SIZE_INT32_0256);
    MEM_free(IRAM, hist_tmp, SIZE_INT32_1024);
#else
    CHECKED_FREE(hist, SIZE_INT32_0256);
    CHECKED_FREE(hist_sum, SIZE_INT32_0256);
    CHECKED_FREE(hist_tmp, SIZE_INT32_1024);
#endif

    return t;
}

// 根据先验知识进行图像二值化，去除图像的左右边界干扰
int32_t thresholding_percent(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                             int32_t w0, int32_t w1, int32_t h0, int32_t h1, int32_t percent)
{
    int32_t w, h;
    int32_t i;
    const int32_t cnt = (w1 - w0 + 1) * (h1 - h0 + 1);
    const int32_t area = img_w * img_h;
    int32_t p0, p1, p2, p3;
    int32_t *restrict hist = NULL;
    int32_t *restrict hist_sum = NULL;
    int32_t *restrict hist_tmp = NULL;
    uint8_t t;
    const int32_t w4 = w0 + ((w1 - w0 + 1) >> 2) * 4;

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_sum = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_tmp = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_1024, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_sum, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_tmp, SIZE_INT32_1024, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);
//  memset(hist_sum, 0, SIZE_INT32_0256);
    memset(hist_tmp, 0, SIZE_INT32_1024);

    // 构建直方图，只有图像的中心部分参与构建直方图，排除外界干扰
#if 0

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            hist[gray_img[img_w * h + w]]++;
        }
    }

#else

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w < w4; w += 4)
        {
            p0 = (gray_img[img_w * h + w + 0] << 2) + 0;
            p1 = (gray_img[img_w * h + w + 1] << 2) + 1;
            p2 = (gray_img[img_w * h + w + 2] << 2) + 2;
            p3 = (gray_img[img_w * h + w + 3] << 2) + 3;

            hist_tmp[p0]++;
            hist_tmp[p1]++;
            hist_tmp[p2]++;
            hist_tmp[p3]++;
        }
    }

    for (h = h0; h <= h1; h++)
    {
        for (w = w4; w <= w1; w++)
        {
            p0 = (gray_img[img_w * h + w + 0] << 2) + 0;

            hist_tmp[p0]++;
        }
    }

    for (i = 0; i < 256; i++)
    {
        hist[i] += (hist_tmp[(i << 2) + 0]
                    + hist_tmp[(i << 2) + 1]
                    + hist_tmp[(i << 2) + 2]
                    + hist_tmp[(i << 2) + 3]);
    }

#endif

    // 计算累积直方图，加速求阈值过程
    hist_sum[0] = hist[0];

    for (i = 1; i < 256; i++)
    {
        hist_sum[i] = hist_sum[i - 1] + hist[i];
    }

    // 按前景在图像中所占的比例percent%调整阈值
    for (i = 0; i < 256; i += 2)
    {
        if ((cnt - hist_sum[i]) * 100 < percent * cnt)
        {
            break;
        }
    }

    t = (uint8_t)MIN2(i, 250);

    thresholding_fix(gray_img, bina_img, area, t);

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_sum, SIZE_INT32_0256);
    MEM_free(IRAM, hist_tmp, SIZE_INT32_1024);
#else
    CHECKED_FREE(hist_tmp, SIZE_INT32_1024);
    CHECKED_FREE(hist_sum, SIZE_INT32_0256);
    CHECKED_FREE(hist, SIZE_INT32_0256);
#endif

    return t;
}

// 用位操作代替字节操作
#if 0
int32_t thresholding_percent_opt_bits(uint8_t *restrict edge_img, uint8_t *restrict bina_img,
                                      int32_t img_w, int32_t img_h,
                                      int32_t w0, int32_t w1, int32_t h0, int32_t h1, int32_t percent)
{
    int32_t i;
    int32_t w, h;
    int32_t pix;
    int32_t cnt;
//    int32_t kk = 0;
//    const int32_t area = img_w * img_h;
    int32_t p0, p1, p2, p3;
    uint8_t t;
    uint32_t t0, t1, t2, t3;
    uint32_t t4 = 0u;
    uint32_t edge0, edge1, edge2, edge3;
    uint32_t edge4/*, edge5, edge6, edge7*/;
    uint32_t comp0, comp1, comp2, comp3;
    uint32_t comp4/*, comp5, comp6, comp7*/;
    uint32_t ret0;
    int32_t *restrict hist = NULL;
    int32_t *restrict hist_sum = NULL;
    int32_t *restrict hist_tmp = NULL;
    int32_t wbit_num;

//    FILE *txt = fopen("opt_bits.txt", "w");
//    if(txt == NULL)
//  {
//      printf("can't open file!\n");
//      while(1);
//  }

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_sum = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_tmp = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_1024, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_sum, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_tmp, SIZE_INT32_1024, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);
    memset(hist_sum, 0, SIZE_INT32_0256);
    memset(hist_tmp, 0, SIZE_INT32_1024);

    cnt = ((w1 - w0 + 1) / 2) * (h1 - h0 + 1);

    // 构建直方图，只有图像的中心部分参与构建直方图，排除外界干扰
    for (h = h0; h <= h1 - 1; h ++)
    {
#ifdef _TMS320C6X_
        touch((const void *)(edge_img + img_w * h), img_w);
#endif

        for (w = w0; w <= w1 - 7; w += 8)
        {
            p0 = edge_img[img_w * h + w + 0] * 4 + 0;
            p1 = edge_img[img_w * h + w + 2] * 4 + 1;
            p2 = edge_img[img_w * h + w + 4] * 4 + 2;
            p3 = edge_img[img_w * h + w + 6] * 4 + 3;

//            fprintf(txt, "%3d - %3d \n", img_w * h + w + 0, edge_img[img_w * h + w + 0]);
//            fprintf(txt, "%3d - %3d \n", img_w * h + w + 2, edge_img[img_w * h + w + 2]);
//            fprintf(txt, "%3d - %3d \n", img_w * h + w + 4, edge_img[img_w * h + w + 4]);
//            fprintf(txt, "%3d - %3d \n", img_w * h + w + 6, edge_img[img_w * h + w + 6]);

            hist_tmp[p0]++;
            hist_tmp[p1]++;
            hist_tmp[p2]++;
            hist_tmp[p3]++;
        }
    }

    for (i = 0; i < 256; i++)
    {
        hist[i] += (hist_tmp[(i << 2) + 0]
                    + hist_tmp[(i << 2) + 1]
                    + hist_tmp[(i << 2) + 2]
                    + hist_tmp[(i << 2) + 3]);
    }

//  fclose(txt);

    // 计算累积直方图，加速求阈值过程
    hist_sum[0] = hist[0];

    for (pix = 1; pix < 256; pix++)
    {
        hist_sum[pix] = hist_sum[pix - 1] + hist[pix];
    }

    // 按前景在图像中所占的比例percent%调整阈值
    for (i = 0; i < 256; i += 2)
    {
        if ((cnt - hist_sum[i]) * 100 < percent * cnt)
        {
            break;
        }
    }

    t = (uint8_t)i;

    t0 = t1 = t;
    t2 = _pack2(t0, t1);
    t3 = t2 << 8;
    t4 = t2 | t3;

    // 车牌连线find_all_runs()中lamda是20，所以每个（寄存器）中只存20位数据，高位补0
    wbit_num = img_w / 20;

    for (h = 0; h < img_h; h++)
    {
#ifdef _TMS320C6X_
        touch((const void *)(edge_img + img_w * h), img_w);
#endif

        for (w = 0; w < wbit_num; w++)
        {
            edge0 = _amem4(edge_img + img_w * h + w * 20 +  0);
            edge1 = _amem4(edge_img + img_w * h + w * 20 +  4);
            edge2 = _amem4(edge_img + img_w * h + w * 20 +  8);
            edge3 = _amem4(edge_img + img_w * h + w * 20 + 12);
            edge4 = _amem4(edge_img + img_w * h + w * 20 + 16);

            comp0 = _cmpgtu4(edge0, t4);
            comp1 = _cmpgtu4(edge1, t4);
            comp2 = _cmpgtu4(edge2, t4);
            comp3 = _cmpgtu4(edge3, t4);
            comp4 = _cmpgtu4(edge4, t4);

            ret0 = (comp4 << 16) | (comp3 << 12) | (comp2 << 8) | (comp1 << 4) | comp0;

            _amem4(bina_img + (wbit_num * h + w) * 4) = ret0;
        }
    }

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_sum, SIZE_INT32_0256);
    MEM_free(IRAM, hist_tmp, SIZE_INT32_1024);
#else
    CHECKED_FREE(hist_tmp, SIZE_INT32_1024);
    CHECKED_FREE(hist_sum, SIZE_INT32_0256);
    CHECKED_FREE(hist, SIZE_INT32_0256);
#endif

    return t;
}
//#else
int32_t thresholding_percent_opt_bits_all(uint8_t *restrict edge_img, uint8_t *restrict bina_img,
                                          int32_t img_w, int32_t img_h, int32_t percent)
{
    int32_t i;
    int32_t w, h;
    int32_t pix;
    const int32_t area = img_w * img_h;
    int32_t cnt = area;
    int32_t p0, p1, p2, p3;
    uint8_t t;
    uint32_t t0, t1, t2, t3;
    uint32_t t4 = 0;
    uint32_t edge0, edge1, edge2, edge3;
    uint32_t edge4/*, edge5, edge6, edge7*/;
    uint32_t comp0, comp1, comp2, comp3;
    uint32_t comp4/*, comp5, comp6, comp7*/;
    uint32_t ret0;
    int32_t *restrict hist = NULL;
    int32_t *restrict hist_sum = NULL;
    int32_t *restrict hist_tmp = NULL;
    int32_t wbit_num;

//    FILE *txt = fopen("1.txt", "w");
//    if(txt == NULL)
//  {
//      printf("can't open file!\n");
//      while(1);
//  }

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_sum = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_tmp = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_1024, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_sum, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_tmp, SIZE_INT32_1024, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);
    memset(hist_sum, 0, SIZE_INT32_0256);
    memset(hist_tmp, 0, SIZE_INT32_1024);

    for (i = 0; i < area; i += 4)
    {
#ifdef _TMS320C6X_

        if (i % (1024 * 8) == 0)
        {
            touch((const void *)(edge_img + i), 1024 * 8);
        }

#endif
        p0 = (edge_img[i + 0] << 2) + 0;
        p1 = (edge_img[i + 1] << 2) + 1;
        p2 = (edge_img[i + 2] << 2) + 2;
        p3 = (edge_img[i + 3] << 2) + 3;

        hist_tmp[p0]++;
        hist_tmp[p1]++;
        hist_tmp[p2]++;
        hist_tmp[p3]++;
    }

    for (i = 0; i < 256; i++)
    {
        hist[i] += (hist_tmp[(i << 2) + 0]
                    + hist_tmp[(i << 2) + 1]
                    + hist_tmp[(i << 2) + 2]
                    + hist_tmp[(i << 2) + 3]);
    }

//  fclose(txt);

    // 计算累积直方图，加速求阈值过程
    hist_sum[0] = hist[0];

    for (pix = 1; pix < 256; pix++)
    {
        hist_sum[pix] = hist_sum[pix - 1] + hist[pix];
    }

    // 按前景在图像中所占的比例percent%调整阈值
    for (i = 0; i < 256; i += 2)
    {
        if ((cnt - hist_sum[i]) * 100 < percent * cnt)
        {
            break;
        }
    }

    t = (uint8_t)i;

    t0 = t1 = t;
    t2 = _pack2(t0, t1);
    t3 = t2 << 8;
    t4 = t2 | t3;

    // 车牌连线find_all_runs()中lamda是20，所以每个（寄存器）中只存20位数据，高位补0
    wbit_num = img_w / 20;

    for (h = 0; h < img_h; h++)
    {
#ifdef _TMS320C6X_
        touch((const void *)(edge_img + img_w * h), img_w);
#endif

        for (w = 0; w < wbit_num; w++)
        {
            edge0 = _mem4(edge_img + img_w * h + w * 20 +  0);
            edge1 = _mem4(edge_img + img_w * h + w * 20 +  4);
            edge2 = _mem4(edge_img + img_w * h + w * 20 +  8);
            edge3 = _mem4(edge_img + img_w * h + w * 20 + 12);
            edge4 = _mem4(edge_img + img_w * h + w * 20 + 16);

            comp0 = _cmpgtu4(edge0, t4);
            comp1 = _cmpgtu4(edge1, t4);
            comp2 = _cmpgtu4(edge2, t4);
            comp3 = _cmpgtu4(edge3, t4);
            comp4 = _cmpgtu4(edge4, t4);

            ret0 = (comp4 << 16) | (comp3 << 12) | (comp2 << 8) | (comp1 << 4) | comp0;

            _mem4(bina_img + (wbit_num * h + w) * 4) = ret0;
        }
    }

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_sum, SIZE_INT32_0256);
    MEM_free(IRAM, hist_tmp, SIZE_INT32_1024);
#else
    CHECKED_FREE(hist_tmp, SIZE_INT32_1024);
    CHECKED_FREE(hist_sum, SIZE_INT32_0256);
    CHECKED_FREE(hist, SIZE_INT32_0256);
#endif

    return t;
}
#endif

#if 0
int32_t thresholding_iteration(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                               int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    int32_t k;
    int32_t w, h;
    int32_t sum, sum1, sum2;
    int32_t cnt, cnt1, cnt2;
    uint8_t avg, avg1, avg2;
    uint8_t thresh, prev_thresh;
    int32_t iteration;
    const int32_t area = img_w * img_h;
    int32_t *hist = NULL;
    const int32_t max_iteration = 4;

    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);

    memset(hist, 0, SIZE_INT32_0256);

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            hist[gray_img[img_w * h + w]]++;
        }
    }

    hist[0] = 0;
    sum = 0;
    cnt = 0;

    for (k = 0; k < 256; k++)
    {
        sum += hist[k] * k;
        cnt += hist[k];
    }

    avg = sum / cnt;
    thresh = avg;
    prev_thresh = avg;

    iteration = 0;

    while (iteration < max_iteration)
    {
        sum1 = 0;
        cnt1 = 0;
        sum2 = 0;
        cnt2 = 0;

        for (k = 0; k < thresh; k++)
        {
            sum1 += hist[k] * k;
            cnt1 += hist[k];
        }

        avg1 = sum1 / cnt1;

        for (k = thresh; k < 256; k++)
        {
            sum2 += hist[k] * k;
            cnt2 += hist[k];
        }

        avg2 = sum2 / cnt2;

        thresh = (avg1 + avg2) / 2;

        if (abs(thresh - prev_thresh) <= 2)
        {
            break;
        }

        prev_thresh = thresh;

        iteration++;
    }

    thresh = MIN2(255, thresh - 16);

    thresholding_fix(gray_img, bina_img, area, thresh);

    CHECKED_FREE(hist, SIZE_INT32_0256);

    return thresh;
}
#endif

// 根据均值进行图像二值化，取图像的中心部分去除干扰
#ifdef _TMS320C6X_
int32_t thresholding_avg(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                         int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    int32_t w, h;
    const int32_t area = img_w * img_h;
    uint32_t cnt = (w1 - w0 + 1) * (h1 - h0 + 1);
    uint32_t avg;
    uint32_t sum = 0u;

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            sum += gray_img[img_w * h + w];
        }
    }

    avg = sum / cnt;

    thresholding_fix(gray_img, bina_img, area, avg);

    return avg;
}
#else
int32_t thresholding_avg(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                         int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    int32_t w, h;
    const int32_t area = img_w * img_h;
    const uint32_t cnt = (uint32_t)((w1 - w0 + 1) * (h1 - h0 + 1));
    uint32_t avg;
    uint32_t sum = 0u;

    for (h = h0; h <= h1; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            sum += (uint32_t)gray_img[img_w * h + w];
        }
    }

    avg = sum / cnt;

    thresholding_fix(gray_img, bina_img, area, (uint8_t)avg);

    return avg;
}
#endif

#ifdef _TMS320C6X_
int32_t thresholding_avg_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    const int32_t area = img_w * img_h;
    int32_t sum;
    int32_t avg;
    int32_t k;
    int32_t n;
    uint32_t t0, t1, t2, t3;
    uint32_t t4 = 0;
    uint32_t edge0, edge1, edge2, edge3;
    uint32_t comp0, comp1, comp2, comp3;
    uint32_t ret0, ret1, ret2, ret3;

    sum = 0;

    for (pix = 0; pix < area; pix++)
    {
        sum += gray_img[pix];
    }

    avg = sum / (area + 1);

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

        comp0 = _cmpgtu4(t4, edge0);
        comp1 = _cmpgtu4(t4, edge1);
        comp2 = _cmpgtu4(t4, edge2);
        comp3 = _cmpgtu4(t4, edge3);

        ret0  = _xpnd4(comp0);
        ret1  = _xpnd4(comp1);
        ret2  = _xpnd4(comp2);
        ret3  = _xpnd4(comp3);

        _amem4(bina_img + (k << 4) +  0) = ret0;
        _amem4(bina_img + (k << 4) +  4) = ret1;
        _amem4(bina_img + (k << 4) +  8) = ret2;
        _amem4(bina_img + (k << 4) + 12) = ret3;
    }

    for (pix = n * 16; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] >= avg) ? 0 : 255;
    }

    return avg;
}
#elif WIN321
int32_t thresholding_avg_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    const uint32_t area = (uint32_t)(img_w * img_h);
    uint32_t sum;
    uint32_t avg;
    int32_t n;
    uint8_t avg128[16];

    sum = (uint32_t)0;

    for (pix = 0; pix < (int32_t)area; pix++)
    {
        sum += (uint32_t)gray_img[pix];
    }

    avg = (uint32_t)(sum / (area + 1u));

    memset(avg128, avg, (uint32_t)16);

    n = area >> 5;

    _asm
    {
        mov  ecx, n          // put count to ecx reg
        mov  esi, gray_img   // put src address to esi reg
        mov  edi, bina_img   // put dst address to edi reg

        movdqu   xmm1, avg128
        movdqu   xmm4, avg128

        code_loop:
        movdqu   xmm0, [esi + 0x00]
        movdqu   xmm3, [esi + 0x10]

        movdqu   xmm2, xmm1
        movdqu   xmm5, xmm4

        // psubusb 两个压缩无符号字节整数的做饱和减法，如果单个字节结果小于0（负值）则将饱和值00H写入目标操作数
        psubusb  xmm2, xmm0
        psubusb  xmm5, xmm3
        // 为了加快运行速度目前大于均值时结果中存放的是差值而不是255

        movdqu   [edi + 0x00], xmm2
        movdqu   [edi + 0x10], xmm5

        add      esi, 0x20       // add src pointer by 32 bytes
        add      edi, 0x20       // add dst pointer by 32 bytes
        dec      ecx             // decrement count by 1
        jnz      code_loop       // jump to code_loop if not Zero
    }

    for (pix = n << 5; pix < (int32_t)area; pix++)
    {
        bina_img[pix] = (uint8_t)((gray_img[pix] >= avg) ? 0 : 255);
    }

    return avg;
}
#else
int32_t thresholding_avg_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    const int32_t area = img_w * img_h;
    uint32_t sum;
    uint32_t avg;

    sum = 0;

    for (pix = 0; pix < area; pix++)
    {
        sum += (uint32_t)gray_img[pix];
    }

    avg = sum / (area + 1);

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] >= avg) ? 0 : 255;
    }

    return avg;
}
#endif

int32_t thresholding_avg_hist_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int32_t i;
    int32_t pix;
    int32_t threshold;
    int32_t temp;
    const int32_t area = img_w * img_h;
    uint32_t sum;
    uint32_t avg;
    int32_t p0, p1, p2, p3;
    int32_t *restrict hist = NULL;
    int32_t *restrict hist_sum = NULL;
    int32_t *restrict hist_tmp = NULL;

    sum = (0u);

    for (pix = 0; pix < area; pix++)
    {
        sum += (uint32_t)gray_img[pix];
    }

    avg = sum / (uint32_t)(area + 1);

#ifdef _TMS320C6X_
    hist = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_sum = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_0256, 128);
    hist_tmp = (int32_t *)MEM_alloc(IRAM, SIZE_INT32_1024, 128);
#else
    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_sum, SIZE_INT32_0256, CACHE_SIZE);
    CHECKED_MALLOC(hist_tmp, SIZE_INT32_1024, CACHE_SIZE);
#endif

    memset(hist, 0, SIZE_INT32_0256);
    memset(hist_sum, 0, SIZE_INT32_0256);
    memset(hist_tmp, 0, SIZE_INT32_1024);

    for (i = 0; i < area; i += 4)
    {
        p0 = (gray_img[i + 0] << 2) + 0;
        p1 = (gray_img[i + 1] << 2) + 1;
        p2 = (gray_img[i + 2] << 2) + 2;
        p3 = (gray_img[i + 3] << 2) + 3;

        hist_tmp[p0]++;
        hist_tmp[p1]++;
        hist_tmp[p2]++;
        hist_tmp[p3]++;
    }

    for (i = 0; i < 32; i++)
    {
        hist[i] += (hist_tmp[(i << 4) +  0]
                    + hist_tmp[(i << 4) +  1]
                    + hist_tmp[(i << 4) +  2]
                    + hist_tmp[(i << 4) +  3]
                    + hist_tmp[(i << 4) +  4]
                    + hist_tmp[(i << 4) +  5]
                    + hist_tmp[(i << 4) +  6]
                    + hist_tmp[(i << 4) +  7]
                    + hist_tmp[(i << 4) +  8]
                    + hist_tmp[(i << 4) +  9]
                    + hist_tmp[(i << 4) + 10]
                    + hist_tmp[(i << 4) + 11]
                    + hist_tmp[(i << 4) + 12]
                    + hist_tmp[(i << 4) + 13]
                    + hist_tmp[(i << 4) + 14]
                    + hist_tmp[(i << 4) + 15]
                    + hist_tmp[(i << 4) + 16]
                    + hist_tmp[(i << 4) + 17]
                    + hist_tmp[(i << 4) + 18]
                    + hist_tmp[(i << 4) + 19]
                    + hist_tmp[(i << 4) + 20]
                    + hist_tmp[(i << 4) + 21]
                    + hist_tmp[(i << 4) + 22]
                    + hist_tmp[(i << 4) + 23]
                    + hist_tmp[(i << 4) + 24]
                    + hist_tmp[(i << 4) + 25]
                    + hist_tmp[(i << 4) + 26]
                    + hist_tmp[(i << 4) + 27]
                    + hist_tmp[(i << 4) + 28]
                    + hist_tmp[(i << 4) + 29]
                    + hist_tmp[(i << 4) + 30]
                    + hist_tmp[(i << 4) + 31]
                   );
    }

    threshold = avg >> 3;

    temp = 0;

    for (i = 0; i < 5; i++)
    {
        if (threshold + i < 32)
            if (hist[threshold + i] < (hist[threshold] * 6 / 10))
            {
                temp = i;
                break;
            }
    }

    avg = (uint32_t)((threshold + temp) << 3);

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] >= avg) ? 0u : 255u;
    }

#ifdef _TMS320C6X_
    MEM_free(IRAM, hist, SIZE_INT32_0256);
    MEM_free(IRAM, hist_sum, SIZE_INT32_0256);
    MEM_free(IRAM, hist_tmp, SIZE_INT32_1024);
#else
    CHECKED_FREE(hist_tmp, SIZE_INT32_1024);
    CHECKED_FREE(hist_sum, SIZE_INT32_0256);
    CHECKED_FREE(hist, SIZE_INT32_0256);
#endif

    return avg;
}

// 利用梯度信息二值化
int32_t thresholding_by_grads(uint8_t *gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h)
{
    int32_t pix;
    int32_t area = img_w * img_h;
    int32_t sum1, sum2;
    int32_t w, h;
    int32_t ex, ey, exy;
    int32_t thr;

    sum1 = 0;
    sum2 = 0;

    for (h = 1; h < img_h - 1; h++)
    {
        for (w = 1; w < img_w - 1; w++)
        {
            ex = (int32_t)(gray_img[h * img_w + w + 1] - gray_img[h * img_w + w - 1]);
            ey = (int32_t)(gray_img[(h + 1) * img_w + w] - gray_img[(h - 1) * img_w + w]);
            exy = abs(ex) + abs(ey);
//            exy = exy > 255 ? 255 : exy;
            exy = (exy * (int32_t)gray_img[h * img_w + w]) >> 8;

            sum1 += exy * (int32_t)gray_img[h * img_w + w];
            sum2 += exy;
        }
    }

    if (sum2 == 0)
    {
        sum2 = 1;
    }

    thr = sum1 / sum2;

    for (pix = 0; pix < area; pix++)
    {
        bina_img[pix] = (gray_img[pix] < (uint8_t)thr) ? 0u : 255u;
    }

    return thr;
}

// 车牌二值化(根据笔划宽度)
int32_t thresholding_stroke_width(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                  int32_t img_w, int32_t img_h,
                                  int32_t w0, int32_t w1/*, int32_t h0, int32_t h1*/)
{
    int32_t w, h, k;
    int32_t st, ed;
    int32_t stroke_width = 0;
    int32_t stroke_width_min = 1;
    int32_t stroke_width_max = 3;
    int32_t stroke_thresh = 20;
    int32_t stroke_cnt = 0;
    int32_t thresh = 0;
    int32_t iteration = 0;
    uint8_t *restrict sub_gray_img = NULL;
    uint8_t *restrict sub_bina_img = NULL;
    int32_t sub_w = 0;
    int32_t sub_h = 0;
    uint32_t sum = (uint32_t)0;
    uint32_t avg = (uint32_t)0;
    uint32_t cnt = (uint32_t)0;

    sub_h = 5;
    sub_w = img_w;

#ifdef _TMS320C6X_
    sub_gray_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * sub_w * sub_h, 128);
    sub_bina_img = (uint8_t *)MEM_alloc(IRAM, sizeof(uint8_t) * sub_w * sub_h, 128);
#else
    CHECKED_MALLOC(sub_gray_img, sizeof(uint8_t) * sub_w * sub_h, CACHE_SIZE);
    CHECKED_MALLOC(sub_bina_img, sizeof(uint8_t) * sub_w * sub_h, CACHE_SIZE);
#endif

    for (k = 0; k < 5; k++)
    {
        h = img_h / 2 - 4 + k * 2;
        memcpy(sub_gray_img + sub_w * k, gray_img + img_w * h, img_w);
    }

    sum = 0u;
    cnt = 0u;

    for (h = 0; h < sub_h; h++)
    {
        for (w = w0; w <= w1; w++)
        {
            sum += (uint32_t)sub_gray_img[sub_w * h + w];
            cnt++;
        }
    }

    avg = sum / (cnt + 1u);

    thresh = (int32_t)(avg / (2u));

    thresholding_fix(sub_gray_img, sub_bina_img, sub_w * sub_h, (uint8_t)thresh);

    iteration = 0;

    while (iteration < 20)
    {
        stroke_cnt = 0;
        cnt = 0u;

        for (h = 0; h < sub_h; h++)
        {
            for (w = w0; w <= w1; w++)
            {
                cnt += (uint32_t)(sub_bina_img[sub_w * h + w] & (uint8_t)0x01);
            }
        }

        if ((int32_t)cnt < ((w1 - w0 + 1) * sub_h / 2))
        {
            for (h = 0; h < sub_h; h++)
            {
                for (w = 0; w < sub_w; w++)
                {
                    if (sub_bina_img[sub_w * h + w])
                    {
                        cnt++;
                        st = w;
                        ed = w;

                        for (; w < sub_w; w++)
                        {
                            if (sub_bina_img[sub_w * h + w] == 0u)
                            {
                                ed = w - 1;
                                break;
                            }
                        }

                        stroke_width = ed - st;

                        if ((stroke_width >= stroke_width_min) && (stroke_width <= stroke_width_max))
                        {
                            stroke_cnt++;
                        }
                    }
                }
            }

            if (stroke_cnt > stroke_thresh)
            {
                break;
            }
        }

        thresh += 10;
        iteration++;

        if ((thresh >= 255))
        {
            break;
        }

        thresholding_fix(sub_gray_img, sub_bina_img, sub_w * sub_h, (uint8_t)thresh);
    }

    if (stroke_cnt <= stroke_thresh)
    {
        thresh = (int32_t)avg;
    }

    thresholding_fix(gray_img, bina_img, img_w * img_h, (uint8_t)thresh);

#ifdef _TMS320C6X_
    MEM_free(IRAM, sub_gray_img, sizeof(uint8_t) * sub_w * sub_h);
    MEM_free(IRAM, sub_bina_img, sizeof(uint8_t) * sub_w * sub_h);
#else
    CHECKED_FREE(sub_gray_img, sizeof(uint8_t) * sub_w * sub_h);
    CHECKED_FREE(sub_bina_img, sizeof(uint8_t) * sub_w * sub_h);
#endif

    return thresh;
}

#if 0
/*
OTSU 算法可以说是自适应计算单阈值（用来转换灰度图像为二值图像）的简单高效方法。

下面的代码最早由 Ryan Dibble提供，此后经过多人Joerg.Schulenburg, R.Z.Liu 等修改，补正。

算法对输入的灰度图像的直方图进行分析，将直方图分成两个部分，使得两部分之间的距离最大。

划分点就是求得的阈值。
*/
int32_t thresholding_otsu(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                          int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    uint8_t *np = NULL;     // 图像指针
    int32_t threshold = 1;  // 阈值
    int32_t w, h, k;
    int32_t n, n1, n2, gmin, gmax;
    int32_t *hist = NULL;
    double m1, m2, sum, csum, fmax, sb;
    const int32_t area = img_w * img_h;

    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);

    memset(hist, 0, SIZE_INT32_0256);

    // 生成直方图
    gmin = 255;
    gmax = 0;

    for (h = h0; h <= h1; h++)
    {
        np = &gray_img[img_w * h + w0];

        for (w = w0; w <= w1; w++)
        {
            hist[*np]++;

            if (*np > gmax)
            {
                gmax = *np;
            }

            if (*np < gmin)
            {
                gmin = *np;
            }

            np++;
        }
    }

    // set up everything
    sum = csum = 0.0;
    n = 0;

    for (k = 0; k <= 255; k++)
    {
        sum += (double) k * (double) hist[k];   /* x*f(x) 质量矩 */
        n += hist[k];                           /* f(x) 质量 */
    }

    if (!n) // if n has no value, there is problems...
    {
        CHECKED_FREE(hist);
        printf("not normal threshold = 160\n");
        return (160);
    }

    fmax = -1.0;
    n1 = 0;

    for (k = 0; k < 255; k++)
    {
        n1 += hist[k];

        if (!n1)
        {
            continue;
        }

        n2 = n - n1;

        if (n2 == 0)
        {
            break;
        }

        csum += (double)k * hist[k];

        m1 = csum / n1;
        m2 = (sum - csum) / n2;
        sb = (double)n1 * (double)n2 * (m1 - m2) * (m1 - m2);

        if (sb > fmax)  /* bbg: note: can be optimized. */
        {
            fmax = sb;
            threshold = k;
        }
    }

    //printf("#OTSU: threshold = %d gmin = %d gmax = %d\n", threshold, gmin, gmax);

    for (n = 0; n < area; n++)
    {
        bina_img[n] = gray_img[n] > threshold ? 255u : 0u;
    }

    CHECKED_FREE(hist, SIZE_INT32_0256);

    return threshold;
}
#else
int32_t thresholding_otsu(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                          int32_t w0, int32_t w1, int32_t h0, int32_t h1)
{
    uint8_t *np = NULL;     // 图像指针
    int32_t threshold = 1;  // 阈值
    int32_t w, h, k;
    int32_t n, n1, n2, gmin, gmax;
    int32_t *hist = NULL;
    int64_t m1, /*m2, */sum, csum, fmax, sb;   // 为加快速度，采用长整型计算
    const int32_t area = img_w * img_h;

    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);

    memset(hist, 0, SIZE_INT32_0256);

    // 生成直方图
    gmin = 255;
    gmax = 0;

    for (h = h0; h <= h1; h++)
    {
        np = &gray_img[img_w * h + w0];

        for (w = w0; w <= w1; w++)
        {
            hist[*np]++;

            if (*np > gmax)
            {
                gmax = *np;
            }

            if (*np < gmin)
            {
                gmin = *np;
            }

            np++;
        }
    }

    // set up everything
    sum = 0;
    csum = 0;
    n = 0;

    for (k = 0; k <= 255; k++)
    {
        sum += (int64_t)k * hist[k];   /* x*f(x) 质量矩 */
        n += hist[k];                           /* f(x) 质量 */
    }

    if (!n) // if n has no value, there is problems...
    {
        CHECKED_FREE(hist, SIZE_INT32_0256);
        return (160);
    }

    fmax = -1;
    n1 = 0;

    for (k = 0; k < 255; k++)
    {
        n1 += hist[k];

        if (!n1)
        {
            continue;
        }

        n2 = n - n1;

        if (n2 == 0)
        {
            break;
        }

        csum += (int64_t)k * hist[k];

//         m1 = csum / n1;
//         m2 = (sum - csum) / n2;
//         sb = (float)(n1 * n2) * (m1 - m2) * (m1 - m2);

        m1 = csum * n - sum * n1;
        // float型除法要比长整型除法速度快，因此临时转为float型计算
        sb = (int64_t)(m1 * m1 / (float)(n1 * (n - n1)));

        if (sb > fmax)  /* bbg: note: can be optimized. */
        {
            fmax = sb;
            threshold = k;
        }
    }

    //printf("#OTSU: threshold = %d gmin = %d gmax = %d\n", threshold, gmin, gmax);

    for (n = 0; n < area; n++)
    {
        bina_img[n] = gray_img[n] > (uint8_t)threshold ? 255u : 0u;
    }

    CHECKED_FREE(hist, SIZE_INT32_0256);

    return threshold;
}
#endif

#if 1
/*************************************************************************
 *                     以下为几种二值化求阈值的算法                      *
 *************************************************************************/
// GSA灰度均衡法图像增强处理
// 先求得图像的平均灰度，然后根据公式Fout = (Fin -avg) * lumda + Fn计算得到新的灰度
// lumda为经验值1.92, Fn为经验值100
void gray_tatistical_approach(uint8_t *yuv420, int32_t img_w, int32_t img_h)
{
    int32_t w, h;
    int32_t avg = 0;
    int32_t gray_out;
    int32_t *hist = NULL;

    CHECKED_MALLOC(hist, SIZE_INT32_0256, CACHE_SIZE);

    memset(hist, 0, sizeof(int32_t) * 256);

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            avg += (int32_t)(yuv420[img_w * h + w]);
            hist[yuv420[img_w * h + w]]++;
        }
    }

    avg /= (img_w * img_h);

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            // 1.92和100这两个系数为经验值
            gray_out = (int32_t)(((int32_t)yuv420[img_w * h + w] - avg) * 1.92) + 100;
            gray_out = gray_out > 0 ? gray_out : 0;
            yuv420[img_w * h + w] = gray_out < 255 ? (uint8_t)gray_out : (255u);
        }
    }

    CHECKED_FREE(hist, SIZE_INT32_0256);
}

// bernsen二值化, len为尺度
// 对图像的每一个像素点，在len尺度范围内分别求原图像和高斯平滑图像的阀值，然后根据加权得到最终的阀值，
// 根据此阀值对像素点进行二值化。
void thresholding_bernsen(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                          int32_t img_w, int32_t img_h, int32_t len)
{
    int32_t w, h, m, n;
    int32_t max_t, min_t;
    int32_t threshold1, threshold2;
    uint8_t *gray_tmp = NULL;
    float arfa = 0.5;

    CHECKED_MALLOC(gray_tmp, sizeof(uint8_t) * img_w * img_h, CACHE_SIZE);

    memset(bina_img, 0, img_w * img_h);

    // 求得高斯平滑图像
    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            int32_t avg = 0;
            int32_t top = MAX2(h - len, 0);
            int32_t bottom = MIN2(h + len, img_h - 1);
            int32_t left = MAX2(w - len, 0);
            int32_t right = MIN2(w + len, img_w - 1);
            int32_t smooth_num = 0;

            for (m = top; m <= bottom; m++)
            {
                for (n = left; n <= right; n++)
                {
                    avg += (int32_t)(gray_img[img_w * m + n]);
                    smooth_num++;
                }
            }

            if (smooth_num != 0)
            {
                avg = avg / smooth_num;
                gray_tmp[img_w * h + w] = (uint8_t)avg;
            }
        }
    }

    for (h = 0; h < img_h; h++)
    {
        for (w = 0; w < img_w; w++)
        {
            int32_t top = MAX2(h - len, 0);
            int32_t bottom = MIN2(h + len, img_h - 1);
            int32_t left = MAX2(w - len, 0);
            int32_t right = MIN2(w + len, img_w - 1);

            max_t = 0;
            min_t = 0;

            for (m = top; m <= bottom; m++)
            {
                for (n = left; n <= right; n++)
                {
                    max_t = MAX2(gray_img[img_w * m + n], max_t);
                    min_t = MIN2(gray_img[img_w * m + n], min_t);
                }
            }

            // 求得原图像的阈值
            threshold1 = (max_t + min_t) / 2;

            for (m = top; m <= bottom; m++)
            {
                for (n = left; n <= right; n++)
                {
                    max_t = MAX2(gray_tmp[img_w * m + n], max_t);
                    min_t = MIN2(gray_tmp[img_w * m + n], min_t);
                }
            }

            // 求得高斯平滑图像的阈值
            threshold2 = (max_t + min_t) / 2;

            // 根据两个图像的阈值加权计算得到最终阈值，加权值arfa为经验值0.5
            threshold1 = (int32_t)((1 - arfa) * threshold1 + arfa * threshold2);

            bina_img[img_w * h + w] = (int32_t)(gray_img[img_w * h + w]) < threshold1 ? (0u) : (255u);
        }
    }

    CHECKED_FREE(gray_tmp, sizeof(uint8_t) * img_w * img_h);
}

int Huang(int32_t *data)
{
    // Implements Huang's fuzzy thresholding method
    // Uses Shannon's entropy function (one can also use Yager's entropy function)
    // Huang L.-K. and Wang M.-J.J. (1995) "Image Thresholding by Minimizing
    // the Measures of Fuzziness" Pattern Recognition, 28(1): 41-51
    // M. Emre Celebi  06.15.2007
    // Ported to ImageJ plugin by G. Landini from E Celebi's fourier_0.8 routines
    int threshold = -1;
    int ih, it;
    int first_bin;
    int last_bin;
    double sum_pix;
    double num_pix;
    double term;
    double ent;  // entropy
    double min_ent; // min entropy
    double mu_x;
    double mu_0[256];
    double mu_1[256];

    /* Determine the first non-zero bin */
    first_bin = 0;

    for (ih = 0; ih < 256; ih++)
    {
        if (data[ih] != 0)
        {
            first_bin = ih;
            break;
        }
    }

    /* Determine the last non-zero bin */
    last_bin = 255;

    for (ih = 255; ih >= first_bin; ih--)
    {
        if (data[ih] != 0)
        {
            last_bin = ih;
            break;
        }
    }

    term = 1.0 / (double)(last_bin - first_bin);

    sum_pix = 0;
    num_pix = 0;

    for (ih = first_bin; ih < 256; ih++)
    {
        sum_pix += (double)ih * data[ih];
        num_pix += data[ih];
        /* NUM_PIX cannot be zero ! */
        mu_0[ih] = sum_pix / num_pix;
    }

    sum_pix = 0;
    num_pix = 0;

    for (ih = last_bin; ih > 0; ih--)
    {
        sum_pix += (double)ih * data[ih];
        num_pix += data[ih];
        /* NUM_PIX cannot be zero ! */
        mu_1[ih - 1] = sum_pix / (double) num_pix;
    }

    /* Determine the threshold that minimizes the fuzzy entropy */
    threshold = -1;
    min_ent = 100000.0;

    for (it = 0; it < 256; it++)
    {
        ent = 0.0;

        for (ih = 0; ih <= it; ih++)
        {
            /* Equation (4) in Ref. 1 */
            mu_x = 1.0 / (1.0 + term * fabs(ih - mu_0[it]));

            if (!((mu_x < 1e-06) || (mu_x > 0.999999)))
            {
                /* Equation (6) & (8) in Ref. 1 */
                ent += data[ih] * (-mu_x * log(mu_x) - (1.0 - mu_x) * log(1.0 - mu_x));
            }
        }

        for (ih = it + 1; ih < 256; ih++)
        {
            /* Equation (4) in Ref. 1 */
            mu_x = 1.0 / (1.0 + term * fabs(ih - mu_1[it]));

            if (!((mu_x < 1e-06) || (mu_x > 0.999999)))
            {
                /* Equation (6) & (8) in Ref. 1 */
                ent += data[ih] * (-mu_x * log(mu_x) - (1.0 - mu_x) * log(1.0 - mu_x));
            }
        }

        /* No need to divide by NUM_ROWS * NUM_COLS * LOG(2) ! */
        if (ent < min_ent)
        {
            min_ent = ent;
            threshold = it;
        }
    }

    return threshold;
}

int Yen(int *data)
{
    // Implements Yen  thresholding method
    // 1) Yen J.C., Chang F.J., and Chang S. (1995) "A New Criterion
    //    for Automatic Multilevel Thresholding" IEEE Trans. on Image
    //    Processing, 4(3): 370-378
    // 2) Sezgin M. and Sankur B. (2004) "Survey over Image Thresholding
    //    Techniques and Quantitative Performance Evaluation" Journal of
    //    Electronic Imaging, 13(1): 146-165
    //    http://citeseer.ist.psu.edu/sezgin04survey.html
    //
    // M. Emre Celebi
    // 06.15.2007
    // Ported to ImageJ plugin by G.Landini from E Celebi's fourier_0.8 routines
    int threshold;
    int ih, it;
    double crit;
    double max_crit;
    double norm_histo[256]; /* normalized histogram */
    double P1[256]; /* cumulative normalized histogram */
    double P1_sq[256];
    double P2_sq[256];

    double total = 0;

    for (ih = 0; ih < 256; ih++)
    {
        total += data[ih];
    }

    for (ih = 0; ih < 256; ih++)
    {
        norm_histo[ih] = data[ih] / total;
    }

    P1[0] = norm_histo[0];

    for (ih = 1; ih < 256; ih++)
    {
        P1[ih] = P1[ih - 1] + norm_histo[ih];
    }

    P1_sq[0] = norm_histo[0] * norm_histo[0];

    for (ih = 1; ih < 256; ih++)
    {
        P1_sq[ih] = P1_sq[ih - 1] + norm_histo[ih] * norm_histo[ih];
    }

    P2_sq[255] = 0.0;

    for (ih = 254; ih >= 0; ih--)
    {
        P2_sq[ih] = P2_sq[ih + 1] + norm_histo[ih + 1] * norm_histo[ih + 1];
    }

    /* Find the threshold that maximizes the criterion */
    threshold = -1;
    max_crit = 0.0;

    for (it = 0; it < 256; it++)
    {
        crit = -1.0 * ((P1_sq[it] * P2_sq[it]) > 0.0 ? log(P1_sq[it] * P2_sq[it]) : 0.0) +  2 * ((P1[it] * (1.0 - P1[it])) > 0.0 ? log(P1[it] * (1.0 - P1[it])) : 0.0);

        if (crit > max_crit)
        {
            max_crit = crit;
            threshold = it;
        }
    }

    return threshold;
}

int Triangle(int *data)
{
    //  Zack, G. W., Rogers, W. E. and Latt, S. A., 1977,
    //  Automatic Measurement of Sister Chromatid Exchange Frequency,
    // Journal of Histochemistry and Cytochemistry 25 (7), pp. 741-753
    //
    //  modified from Johannes Schindelin plugin
    //
    // find min and max
    int min = 0, dmax = 0, max = 0, min2 = 0;
    int i;
    int inverted;
    int split;
    double splitDistance;

    // describe line by nx * x + ny * y - d = 0
    double nx, ny, d;

    for (i = 0; i < 256; i++)
    {
        if (data[i] > 0)
        {
            min = i;
            break;
        }
    }

    if (min > 0)
    {
        min--;    // line to the (p==0) point, not to data[min]
    }

    // The Triangle algorithm cannot tell whether the data is skewed to one side or another.
    // This causes a problem as there are 2 possible thresholds between the max and the 2 extremes
    // of the histogram.
    // Here I propose to find out to which side of the max point the data is furthest, and use that as
    //  the other extreme.
    for (i = 255; i > 0; i--)
    {
        if (data[i] > 0)
        {
            min2 = i;
            break;
        }
    }

    if (min2 < 255)
    {
        min2++;    // line to the (p==0) point, not to data[min]
    }

    for (i = 0; i < 256; i++)
    {
        if (data[i] > dmax)
        {
            max = i;
            dmax = data[i];
        }
    }

    // find which is the furthest side
    //IJ.log(""+min+" "+max+" "+min2);
    inverted = 0;

    if ((max - min) < (min2 - max))
    {
        // reverse the histogram
        //IJ.log("Reversing histogram.");
        int left  = 0;          // index of leftmost element
        int right = 255; // index of rightmost element

        inverted = 1;

        while (left < right)
        {
            // exchange the left and right elements
            int temp = data[left];
            data[left]  = data[right];
            data[right] = temp;
            // move the bounds toward the center
            left++;
            right--;
        }

        min = 255 - min2;
        max = 255 - max;
    }

    if (min == max)
    {
        //IJ.log("Triangle:  min == max.");
        return min;
    }


    // nx is just the max frequency as the other point has freq=0
    nx = data[max];   //-min; // data[min]; //  lowest value bmin = (p=0)% in the image
    ny = min - max;
    d = sqrt(nx * nx + ny * ny);
    nx /= d;
    ny /= d;
    d = nx * min + ny * data[min];

    // find split point
    split = min;
    splitDistance = 0;

    for (i = min + 1; i <= max; i++)
    {
        double newDistance = nx * i + ny * data[i] - d;

        if (newDistance > splitDistance)
        {
            split = i;
            splitDistance = newDistance;
        }
    }

    split--;

    if (inverted)
    {
        // The histogram might be used for something else, so let's reverse it back
        int left  = 0;
        int right = 255;

        while (left < right)
        {
            int temp = data[left];
            data[left]  = data[right];
            data[right] = temp;
            left++;
            right--;
        }

        return (255 - split);
    }
    else
    {
        return split;
    }
}

int Shanbhag(int *data)
{
    // Shanhbag A.G. (1994) "Utilization of Information Measure as a Means of
    //  Image Thresholding" Graphical Models and Image Processing, 56(5): 414-419
    // Ported to ImageJ plugin by G.Landini from E Celebi's fourier_0.8 routines
    int threshold;
    int ih, it;
    int first_bin;
    int last_bin;
    double term;
    double tot_ent;  /* total entropy */
    double min_ent;  /* max entropy */
    double ent_back; /* entropy of the background pixels at a given threshold */
    double ent_obj;  /* entropy of the object pixels at a given threshold */
    double norm_histo[256]; /* normalized histogram */
    double P1[256]; /* cumulative normalized histogram */
    double P2[256];

    double total = 0;

    for (ih = 0; ih < 256; ih++)
    {
        total += data[ih];
    }

    for (ih = 0; ih < 256; ih++)
    {
        norm_histo[ih] = data[ih] / total;
    }

    P1[0] = norm_histo[0];
    P2[0] = 1.0 - P1[0];

    for (ih = 1; ih < 256; ih++)
    {
        P1[ih] = P1[ih - 1] + norm_histo[ih];
        P2[ih] = 1.0 - P1[ih];
    }

    /* Determine the first non-zero bin */
    first_bin = 0;

    for (ih = 0; ih < 256; ih++)
    {
        if (!(fabs(P1[ih]) < /*2.220446049250313E-16*/0.000000000000000222))
        {
            first_bin = ih;
            break;
        }
    }

    /* Determine the last non-zero bin */
    last_bin = 255;

    for (ih = 255; ih >= first_bin; ih--)
    {
        if (!(fabs(P2[ih]) < 2.220446049250313E-16))
        {
            last_bin = ih;
            break;
        }
    }

    // Calculate the total entropy each gray-level
    // and find the threshold that maximizes it
    threshold = -1;
    min_ent = 100000.0;

    for (it = first_bin; it <= last_bin; it++)
    {
        /* Entropy of the background pixels */
        ent_back = 0.0;
        term = 0.5 / P1[it];

        for (ih = 1; ih <= it; ih++)      //0+1?
        {
            ent_back -= norm_histo[ih] * log(1.0 - term * P1[ih - 1]);
        }

        ent_back *= term;

        /* Entropy of the object pixels */
        ent_obj = 0.0;
        term = 0.5 / P2[it];

        for (ih = it + 1; ih < 256; ih++)
        {
            ent_obj -= norm_histo[ih] * log(1.0 - term * P2[ih]);
        }

        ent_obj *= term;

        /* Total entropy */
        tot_ent = fabs(ent_back - ent_obj);

        if (tot_ent < min_ent)
        {
            min_ent = tot_ent;
            threshold = it;
        }
    }

    return threshold;
}
#endif

// An efficient implementation of the Sauvola's document binarization algorithm based on integral images.
#if 0
#define integral_image_pix(w, h) \
    integral_image[(img_w) * (h) + (w)]

#define integral_sqimg_pix(w, h) \
    integral_sqimg[(img_w) * (h) + (w)]

#define rowsum_image_pix(w, h) \
    rowsum_image[(img_w) * (h) + (w)]

#define rowsum_sqimg_pix(w, h) \
    rowsum_sqimg[(img_w) * (h) + (w)]

#define gray_img_pix(w, h) \
    gray_img[(img_w) * (h) + (w)]

#define bina_img_pix(w, h) \
    bina_img[(img_w) * (h) + (w)]

void thresholding_sauvola(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int w, h;
    float sauvola_k = 0.3f; // Weighting factor
    int sauvola_w = 40;  // Local window size, Should always be positive

    int xamin, yamin, xamax, yamax;
    int xbmin, ybmin, xbmax, ybmax;
    double areaa, areab;
    double diagsum, idiagsum, diff, sqdiagsum, sqidiagsum, sqdiff;
    double meana, stda, threshold;
    double meanb, stdb;

    double restrict* integral_image = NULL;
    double restrict* integral_sqimg = NULL;
    double restrict* rowsum_image = NULL;
    double restrict* rowsum_sqimg = NULL;

    int x1 = 10; // Half of window size
    int y1 = 4;
    int x2 = 5; // Half of window size
    int y2 = 2;

    // Calculate the integral image, and integral of the squared image
    CHECKED_MALLOC(integral_image, sizeof(double) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(integral_sqimg, sizeof(double) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(rowsum_image, sizeof(double) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(rowsum_sqimg, sizeof(double) * img_w * img_h, CACHE_SIZE);

    memset(integral_image, 0, sizeof(double) * img_w * img_h);
    memset(integral_sqimg, 0, sizeof(double) * img_w * img_h);
    memset(rowsum_image, 0, sizeof(double) * img_w * img_h);
    memset(rowsum_sqimg, 0, sizeof(double) * img_w * img_h);

    for (h = 0; h < img_h; h++)
    {
        rowsum_image_pix(0, h) = gray_img_pix(0, h);
        rowsum_sqimg_pix(0, h) = gray_img_pix(0, h) * gray_img_pix(0, h);
    }

    for (w = 1; w < img_w; w++)
    {
        for (h = 0; h < img_h; h++)
        {
            rowsum_image_pix(w, h) = rowsum_image_pix(w - 1, h) + gray_img_pix(w, h);
            rowsum_sqimg_pix(w, h) = rowsum_sqimg_pix(w - 1, h) + gray_img_pix(w, h) * gray_img_pix(w, h);
        }
    }

    for (w = 0; w < img_w; w++)
    {
        integral_image_pix(w, 0) = rowsum_image_pix(w, 0);
        integral_sqimg_pix(w, 0) = rowsum_sqimg_pix(w, 0);
    }

    for (w = 0; w < img_w; w++)
    {
        for (h = 1; h < img_h; h++)
        {
            integral_image_pix(w, h) = integral_image_pix(w, h - 1) + rowsum_image_pix(w, h);
            integral_sqimg_pix(w, h) = integral_sqimg_pix(w, h - 1) + rowsum_sqimg_pix(w, h);
        }
    }

    // Calculate the meana and standard deviation using the integral image
    for (w = 0; w < img_w; w++)
    {
        for (h = 0; h < img_h; h++)
        {
            xamin = MAX2(0, w - x1);
            yamin = MAX2(0, h - y1);
            xamax = MIN2(img_w - 1, w + x1);
            yamax = MIN2(img_h - 1, h + y1);

            areaa = (xamax - xamin + 1) * (yamax - yamin + 1);

            // areaa can't be 0 here
            // proof (assuming whalfa >= 0):
            // we'll prove that (xamax-xamin+1) > 0,
            // (yamax-yamin+1) is analogous
            // It's the same as to prove: xamax >= xamin
            // img_w - 1 >= 0         since img_w > w >= 0
            // w + whalfa >= 0         since w >= 0, whalfa >= 0
            // w + whalfa >= w - whalfa since whalfa >= 0
            // img_w - 1 >= w - whalfa since img_w > w
            // --IM

            if (!xamin && !yamin) // Point at origin
            {
                diff   = integral_image_pix(xamax, yamax);
                sqdiff = integral_sqimg_pix(xamax, yamax);
            }
            else if (!xamin && yamin) // first column
            {
                diff   = integral_image_pix(xamax, yamax) - integral_image_pix(xamax, yamin - 1);
                sqdiff = integral_sqimg_pix(xamax, yamax) - integral_sqimg_pix(xamax, yamin - 1);
            }
            else if (xamin && !yamin) // first row
            {
                diff   = integral_image_pix(xamax, yamax) - integral_image_pix(xamin - 1, yamax);
                sqdiff = integral_sqimg_pix(xamax, yamax) - integral_sqimg_pix(xamin - 1, yamax);
            }
            else  // rest of the image
            {
                diagsum    = integral_image_pix(xamax, yamax) + integral_image_pix(xamin - 1, yamin - 1);
                idiagsum   = integral_image_pix(xamax, yamin - 1) + integral_image_pix(xamin - 1, yamax);
                diff       = diagsum - idiagsum;
                sqdiagsum  = integral_sqimg_pix(xamax, yamax) + integral_sqimg_pix(xamin - 1, yamin - 1);
                sqidiagsum = integral_sqimg_pix(xamax, yamin - 1) + integral_sqimg_pix(xamin - 1, yamax);
                sqdiff     = sqdiagsum - sqidiagsum;
            }

            meana = diff / areaa;
            stda  = sqrt((sqdiff - diff * diff / areaa) / (areaa - 1));




            xbmin = MAX2(0, w - x2);
            ybmin = MAX2(0, h - y2);
            xbmax = MIN2(img_w - 1, w + x2);
            ybmax = MIN2(img_h - 1, h + y2);

            areab = (xbmax - xbmin + 1) * (ybmax - ybmin + 1);

            if (!xbmin && !ybmin) // Point at origin
            {
                diff   = integral_image_pix(xbmax, ybmax);
                sqdiff = integral_sqimg_pix(xbmax, ybmax);
            }
            else if (!xbmin && ybmin) // first column
            {
                diff   = integral_image_pix(xbmax, ybmax) - integral_image_pix(xbmax, ybmin - 1);
                sqdiff = integral_sqimg_pix(xbmax, ybmax) - integral_sqimg_pix(xbmax, ybmin - 1);
            }
            else if (xbmin && !ybmin) // first row
            {
                diff   = integral_image_pix(xbmax, ybmax) - integral_image_pix(xbmin - 1, ybmax);
                sqdiff = integral_sqimg_pix(xbmax, ybmax) - integral_sqimg_pix(xbmin - 1, ybmax);
            }
            else  // rest of the image
            {
                diagsum    = integral_image_pix(xbmax, ybmax) + integral_image_pix(xbmin - 1, ybmin - 1);
                idiagsum   = integral_image_pix(xbmax, ybmin - 1) + integral_image_pix(xbmin - 1, ybmax);
                diff       = diagsum - idiagsum;
                sqdiagsum  = integral_sqimg_pix(xbmax, ybmax) + integral_sqimg_pix(xbmin - 1, ybmin - 1);
                sqidiagsum = integral_sqimg_pix(xbmax, ybmin - 1) + integral_sqimg_pix(xbmin - 1, ybmax);
                sqdiff     = sqdiagsum - sqidiagsum;
            }

            meanb = diff / areab;
            stdb  = sqrt((sqdiff - diff * diff / areab) / (areab - 1));


            if (meanb / meana > 1.2)
            {
                bina_img_pix(w, h) = 255;
            }
            else
            {
                bina_img_pix(w, h) = 0;
            }
        }
    }

    CHECKED_FREE(integral_image, sizeof(double) * img_w * img_h);
    CHECKED_FREE(integral_sqimg, sizeof(double) * img_w * img_h);
    CHECKED_FREE(rowsum_image, sizeof(double) * img_w * img_h);
    CHECKED_FREE(rowsum_sqimg, sizeof(double) * img_w * img_h);

    return;
}
#else
void thresholding_sauvola(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h)
{
    int w = 0;
    int h = 0;
    float sauvola_k = 0.5f; // Weighting factor
    int xmin, ymin, xmax, ymax;
    double diagsum, idiagsum, diff, sqdiagsum, sqidiagsum, sqdiff, area;
    double mean, std, threshold;
    double *restrict integral_image = NULL;
    double *restrict integral_sqimg = NULL;
    double *restrict rowsum_image = NULL;
    double *restrict rowsum_sqimg = NULL;
    int x1 = 10;
    int y1 = 10;

    // Calculate the integral image, and integral of the squared image
    CHECKED_MALLOC(integral_image, sizeof(double) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(integral_sqimg, sizeof(double) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(rowsum_image, sizeof(double) * img_w * img_h, CACHE_SIZE);
    CHECKED_MALLOC(rowsum_sqimg, sizeof(double) * img_w * img_h, CACHE_SIZE);

    memset(integral_image, 0, sizeof(double) * img_w * img_h);
    memset(integral_sqimg, 0, sizeof(double) * img_w * img_h);
    memset(rowsum_image, 0, sizeof(double) * img_w * img_h);
    memset(rowsum_sqimg, 0, sizeof(double) * img_w * img_h);

    for (h = 0; h < img_h; h++)
    {
        rowsum_image[img_w * h + 0] = gray_img[img_w * h + 0];
        rowsum_sqimg[img_w * h + 0] = gray_img[img_w * h + 0] * gray_img[img_w * h + 0];
    }

    for (w = 1; w < img_w; w++)
    {
        for (h = 0; h < img_h; h++)
        {
            rowsum_image[img_w * h + w] = rowsum_image[img_w * h + (w - 1)] + gray_img[img_w * h + w];
            rowsum_sqimg[img_w * h + w] = rowsum_sqimg[img_w * h + (w - 1)] + gray_img[img_w * h + w] * gray_img[img_w * h + w];
        }
    }

    for (w = 0; w < img_w; w++)
    {
        integral_image[img_w * 0 + w] = rowsum_image[img_w * 0 + w];
        integral_sqimg[img_w * 0 + w] = rowsum_sqimg[img_w * 0 + w];
    }

    for (w = 0; w < img_w; w++)
    {
        for (h = 1; h < img_h; h++)
        {
            integral_image[img_w * h + w] = integral_image[img_w * (h - 1) + w] + rowsum_image[img_w * h + w];
            integral_sqimg[img_w * h + w] = integral_sqimg[img_w * (h - 1) + w] + rowsum_sqimg[img_w * h + w];
        }
    }

    // Calculate the mean and standard deviation using the integral image
    for (w = 0; w < img_w; w++)
    {
        for (h = 0; h < img_h; h++)
        {
            xmin = MAX2(0, w - x1);
            ymin = MAX2(0, h - y1);
            xmax = MIN2(img_w - 1, w + x1);
            ymax = MIN2(img_h - 1, h + y1);

            area = (xmax - xmin + 1) * (ymax - ymin + 1);

            // area can't be 0 here
            // proof (assuming whalf >= 0):
            // we'll prove that (xmax-xmin+1) > 0,
            // (ymax-ymin+1) is analogous
            // It's the same as to prove: xmax >= xmin
            // img_w - 1 >= 0         since img_w > w >= 0
            // w + whalf >= 0         since w >= 0, whalf >= 0
            // w + whalf >= w - whalf since whalf >= 0
            // img_w - 1 >= w - whalf since img_w > w
            // --IM
//            ASSERT(area);

            if (!xmin && !ymin) // Point at origin
            {
                diff   = integral_image[img_w * ymax + xmax];
                sqdiff = integral_sqimg[img_w * ymax + xmax];
            }
            else if (!xmin && ymin) // first column
            {
                diff   = integral_image[img_w * ymax + xmax] - integral_image[img_w * (ymin - 1) + xmax];
                sqdiff = integral_sqimg[img_w * ymax + xmax] - integral_sqimg[img_w * (ymin - 1) + xmax];
            }
            else if (xmin && !ymin) // first row
            {
                diff   = integral_image[img_w * ymax + xmax] - integral_image[img_w * ymax + (xmin - 1)];
                sqdiff = integral_sqimg[img_w * ymax + xmax] - integral_sqimg[img_w * ymax + (xmin - 1)];
            }
            else  // rest of the image
            {
                diagsum    = integral_image[img_w * (ymax + 0) + xmax] + integral_image[img_w * (ymin - 1) + (xmin - 1)];
                idiagsum   = integral_image[img_w * (ymin - 1) + xmax] + integral_image[img_w * (ymax - 0) + (xmin - 1)];
                diff       = diagsum - idiagsum;
                sqdiagsum  = integral_sqimg[img_w * (ymax + 0) + xmax] + integral_sqimg[img_w * (ymin - 1) + (xmin - 1)];
                sqidiagsum = integral_sqimg[img_w * (ymin - 1) + xmax] + integral_sqimg[img_w * (ymax - 0) + (xmin - 1)];
                sqdiff     = sqdiagsum - sqidiagsum;
            }

            mean = diff / area;
            std  = sqrt((sqdiff - diff * diff / area) / (area - 1));

            threshold = mean * (1 + sauvola_k * ((std / 128) - 1));

            if ((double)gray_img[img_w * h + w] > threshold)
            {
                bina_img[img_w * h + w] = 255u;
            }
            else
            {
                bina_img[img_w * h + w] = 0u;
            }
        }
    }

    CHECKED_FREE(integral_image, sizeof(double) * img_w * img_h);
    CHECKED_FREE(integral_sqimg, sizeof(double) * img_w * img_h);
    CHECKED_FREE(rowsum_image, sizeof(double) * img_w * img_h);
    CHECKED_FREE(rowsum_sqimg, sizeof(double) * img_w * img_h);

    return;
}
#endif

