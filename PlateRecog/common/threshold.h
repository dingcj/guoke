#ifndef _THRESHOLD_H_
#define _THRESHOLD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "../portab.h"

// �Աȶ�����
    void contrast_extend_for_gary_image(uint8_t *restrict dst_img, uint8_t *restrict src_img, int32_t img_w, int32_t img_h);

// GSA�ҶȾ��ⷨͼ����ǿ����
    void gray_tatistical_approach(uint8_t *yuv420, int32_t img_w, int32_t img_h);

// bernsen��ֵ����
    void thresholding_bernsen(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                              int32_t img_w, int32_t img_h, int32_t len);

// ���ݾ�ֵ����ͼ���ֵ����ȡͼ������Ĳ���ȥ������
    int32_t thresholding_avg(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                             int32_t w0, int32_t w1, int32_t h0, int32_t h1);

    int32_t thresholding_avg_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

    int32_t thresholding_avg_hist_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

// ���ݱʻ���Ƚ��ж�ֵ��
    int32_t thresholding_stroke_width(uint8_t *restrict gray_img, uint8_t *bina_img,
                                      int32_t img_w, int32_t img_h,
                                      int32_t w0, int32_t w1/*, int32_t h0, int32_t h1*/);
// ���õ�����ʽ����ͼ�����ֵ
    int32_t thresholding_iteration(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                   int32_t w0, int32_t w1, int32_t h0, int32_t h1);
// �����ṩ����ֵ��ֵ��ͼ��
    void thresholding_fix(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t area, uint8_t thresh);

// ����ǰ���ͱ����İٷֱȽ���ͼ���ֵ��
    int32_t thresholding_percent(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h,
                                 int32_t w0, int32_t w1, int32_t h0, int32_t h1, int32_t percent);

    int32_t thresholding_percent_opt(uint8_t *restrict gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h,
                                     int32_t w0, int32_t w1, int32_t h0, int32_t h1, int32_t percent);

    int32_t thresholding_percent_all(uint8_t *restrict gray_img, uint8_t *restrict bina_img,
                                     int32_t img_w, int32_t img_h, int32_t percent);

    int32_t thresholding_by_grads(uint8_t *gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h);

// ��λ���������ֽڲ���
    int32_t thresholding_percent_opt_bits(uint8_t *restrict edge_img, uint8_t *restrict bina_img,
                                          int32_t img_w, int32_t img_h,
                                          int32_t w0, int32_t w1, int32_t h0, int32_t h1, int32_t percent);
    int32_t thresholding_percent_opt_bits_all(uint8_t *restrict edge_img, uint8_t *restrict bina_img,
                                              int32_t img_w, int32_t img_h, int32_t percent);

    int32_t thresholding_otsu(uint8_t *restrict gray_img, uint8_t *bina_img, int32_t img_w, int32_t img_h,
                              int32_t w0, int32_t w1, int32_t h0, int32_t h1);

    void vertical_sobel(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h);
    void vertical_sobel_simple(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h);
void vertical_sobel_simple_rect(uint8_t *restrict gray_img, uint8_t *restrict edge_img, int32_t img_w, int32_t img_h, 
                                Rects rect, int32_t rect_w, int32_t rect_h, int32_t rect_h2);

    void thresholding_sauvola(uint8_t *restrict gray_img, uint8_t *restrict bina_img, int32_t img_w, int32_t img_h);

#ifdef __cplusplus
};
#endif

#endif // _THRESHOLD_H_



//uint8_t top_hat_mask[(9+1+9)*(9+1+9)] =
//{
//  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
//  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
//  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
//  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
//  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
//  0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
//  0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
//  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0
//};
//
///********************************************************************
// Function:        top_hat_trasform
// Description: �ö�ñ�任���Ʋ���������ͼ��
//
// 1.   Param:
//  gray_img    �Ҷ�ͼ��
//  min_img     ��ʴ���ͼ��
//  max_img     ���ͺ��ͼ��
//  img_w       ͼ��Ŀ��
//  img_h       ͼ��ĸ߶�
//
// 2.   Return:
//  void
//********************************************************************/
//void top_hat_trasform(uint8_t *restrict gray_img, uint8_t *min_img, uint8_t *max_img, int32_t img_w, int32_t img_h)
//{
//  int32_t w, h;
//  uint8_t min;
//  uint8_t max;
//  int32_t sum;
//  int32_t dif;
//  int32_t m, n;
//
//  // ��ʴ���㣬�������������ҵ���Сֵ
//  for(h = 9; h < img_h-9; h++)
//  {
//      for(w = 9; w < img_w-9; w++)
//      {
//          min = 255;
//          for(m = h-9; m <= h+9; m++)
//          {
//              for(n = w-9; n <= w+9; n++)
//              {
//                  min = top_hat_mask[img_w * (m-(h-9)) + (n-(w-9))] ? MIN2(gray_img[img_w * m + n], min) : min;
//              }
//          }
//          min_img[img_w * h + w] = min;
//      }
//  }
//
//  // �������㣬�������������ҵ����ֵ
//  for(h = 9; h < img_h-9; h++)
//  {
//      for(w = 9; w < img_w-9; w++)
//      {
//          max = 0;
//          for(m = h-9; m <= h+9; m++)
//          {
//              for(n = w-9; n <= w+9; n++)
//              {
//                  max = top_hat_mask[img_w * (m-(h-9)) + (n-(w-9))] ? MAX(min_img[img_w * m + n], max) : max;
//              }
//          }
//          max_img[img_w * h + w] = max;
//      }
//  }
//
//  // �õ���ñ����Ľ��
//  for(h = 9; h < img_h-9; h++)
//  {
//      for(w = 9; w < img_w-9; w++)
//      {
//          dif = abs(gray_img[img_w * h + w] - max_img[img_w * h + w]);
//
//          gray_img[img_w * h + w] = (uint8_t)dif;
//      }
//  }
//
//  return;
//}
