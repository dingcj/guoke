#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../malloc_aligned.h"
#include "label.h"

/********************************************************************
 Function:      image_label
 Description:   ��Ŀ����б��

 1. Param:
    bina_img    Ŀ����ṹ��
    label_img   ��ű�ǽ��ͼ�� --- 0 ��ʾ����, 1,2,3...��ʾĿ��ı��ֵ

 2. Return:     ����״̬���ر�ǵ�Ŀ�������������״̬����-1
    int32_t
********************************************************************/
int32_t image_label(uint8_t *restrict bina_img, uint16_t *restrict label_img,
                    Rects *restrict obj_pos, int32_t *restrict obj_area,
                    int32_t img_w, int32_t img_h, int32_t label_max)
{
    int32_t sum;
    int32_t w, h;
    int32_t i, j, k, m;
    int32_t pix;
    int32_t label;
    int32_t index = 1;  //  ��ʼֵ��Ϊ1
    int32_t pixel;
    uint16_t NN, NW, NE, WW;
    uint16_t *equivalent_table = NULL;  // ��ά��ŵȼ۱�
    uint16_t *label_list0 = NULL;       // һά��ŵȼ۱�
    uint16_t *label_list1 = NULL;       // һά�����ʱ�ȼ۱�
    uint16_t *sort_array = NULL;        // �������������

    CHECKED_MALLOC(equivalent_table, sizeof(uint16_t) * label_max * label_max, CACHE_SIZE);
    CHECKED_MALLOC(label_list0, sizeof(uint16_t) * label_max, CACHE_SIZE);
    CHECKED_MALLOC(label_list1, sizeof(uint16_t) * label_max, CACHE_SIZE);
    CHECKED_MALLOC(sort_array, sizeof(uint16_t) * label_max, CACHE_SIZE);

    memset(label_img, 0, sizeof(uint16_t) * img_w * img_h);
    memset(obj_pos, 0, sizeof(Rects) * label_max);
    memset(obj_area, 0, sizeof(int32_t) * label_max);

    memset(bina_img,                       0, img_w); //  ������2����Ϊ0
    memset(bina_img + img_w * (img_h - 1), 0, img_w); //  ������2����Ϊ0

    for (h = 0;  h < img_h; h++) //  �ֱ���ߺ��ұߵ�2�ж���Ϊ0
    {
        bina_img[img_w * (h + 0) + 0] = (uint8_t)0;
        bina_img[img_w * (h + 1) - 1] = (uint8_t)0;
    }

    memset(equivalent_table, 0, sizeof(uint16_t) * label_max * label_max);  // ��ά��ŵȼ۱�
    memset(label_list0, 0, sizeof(uint16_t) * label_max);               // һά��ŵȼ۱�
    memset(label_list1, 0, sizeof(uint16_t) * label_max);               // һά�����ʱ�ȼ۱�
    memset(sort_array, 0, sizeof(uint16_t) * label_max);                // �������������

    for (h = 1; h < img_h - 1; h++)
    {
        for (w = 1; w < img_w - 1; w++)
        {
            pixel = (int32_t)bina_img[img_w * h + w];

            if (pixel == 255)   //  Ŀ������
            {
                sum  = (int32_t)bina_img[(h - 1) * img_w + w - 1];
                sum += (int32_t)bina_img[(h - 1) * img_w + w + 0];
                sum += (int32_t)bina_img[(h - 1) * img_w + w + 1];
                sum += (int32_t)bina_img[(h + 0) * img_w + w - 1];

                NW = label_img[(h - 1) * img_w + w - 1];
                NN = label_img[(h - 1) * img_w + w + 0];
                NE = label_img[(h - 1) * img_w + w + 1];
                WW = label_img[(h + 0) * img_w + w - 1];

                if (sum == 0)   //  ����Աߵ�4���㶼Ϊ0
                {
                    label_img[img_w * h + w] = (uint16_t)index;
                    equivalent_table[label_max * index + index] = (uint16_t)1;
                    index++;

                    if (index == label_max) //  �ܿ���������ͷ������ת��
                    {
                        CHECKED_FREE(sort_array, sizeof(uint16_t) * label_max);
                        CHECKED_FREE(label_list1, sizeof(uint16_t) * label_max);
                        CHECKED_FREE(label_list0, sizeof(uint16_t) * label_max);
                        CHECKED_FREE(equivalent_table, sizeof(uint16_t) * label_max * label_max);


                        return -1;
                    }
                }
                else if (sum == 255) //  �����һ���㲻Ϊ0
                {
                    label_img[img_w * h + w] = (uint16_t)(NN + NW + NE + WW);
                }
                else if (sum > 255) //  ����ж���㲻Ϊ0
                {
                    if (NE != (uint16_t)0) //  ����
                    {
                        label_img[img_w * h + w] = NE;
                        equivalent_table[(int32_t)NE * label_max + (int32_t)NN] |= NN;
                        equivalent_table[(int32_t)NN * label_max + (int32_t)NE] |= NN;
                        equivalent_table[(int32_t)NE * label_max + (int32_t)NW] |= NW;
                        equivalent_table[(int32_t)NW * label_max + (int32_t)NE] |= NW;
                        equivalent_table[(int32_t)NE * label_max + (int32_t)WW] |= WW;
                        equivalent_table[(int32_t)WW * label_max + (int32_t)NE] |= WW;
                        continue;
                    }

                    if (NN != (uint16_t)0) //  ����
                    {
                        label_img[img_w * h + w] = NN;
                        equivalent_table[(int32_t)NN * label_max + (int32_t)NW] |= NW;
                        equivalent_table[(int32_t)NW * label_max + (int32_t)NN] |= NW;
                        equivalent_table[(int32_t)NN * label_max + (int32_t)WW] |= WW;
                        equivalent_table[(int32_t)WW * label_max + (int32_t)NN] |= WW;
                        continue;
                    }

                    if (NW != (uint16_t)0) //  ����
                    {
                        label_img[img_w * h + w] = NW;
                        equivalent_table[(int32_t)NW * label_max + (int32_t)WW] |= WW;
                        equivalent_table[(int32_t)WW * label_max + (int32_t)NW] |= WW;
                    }
                }
            }
        }
    }

    if (index > 1)
    {
        for (i = 1; i < index; i++) //  �����ά��ŵȼ۱��õ���ż�����еȼ۹�ϵ
        {
            for (j = 1; j < index; j++)
            {
                if ((equivalent_table[label_max * i + j] != (uint16_t)0) && (i != j)) // ���������ͬ��ż���ڵȼ۹�ϵ
                {
                    for (k = 1; k < index; k++)
                    {
                        equivalent_table[label_max * i + k] |= equivalent_table[label_max * j + k];
                        equivalent_table[label_max * j + k] |= equivalent_table[label_max * i + k];
                    }
                }
            }
        }

        for (i = 1; i < index; i++) //  ����ά�ȼ۱�ת��Ϊһά�ȼ۱�
        {
            for (j = 1; j < i + 1; j++)
            {
                if (equivalent_table[label_max * i + j] != (uint16_t)0)
                {
                    label_list0[i] = (uint16_t)j;
                    break;
                }
            }
        }

        sort_array[1] = label_list0[1];
        k = 2;

        for (i = 2; i < index; i++) //  1~index�����õı����Щ���ظ��ģ���ȡ���õ��Ĳ�ͬ���
        {
            m = 0;

            for (j = 1; j < k; j++)
            {
                if (label_list0[i] == sort_array[j])
                {
                    m = 1;
                    continue;
                }
            }

            if (m == 0)
            {
                sort_array[k] = label_list0[i];
                k++;
            }
        }

        for (i = 1; i < k; i++) //  �����ֵ����1,2,3.....��˳������
        {
            label_list1[sort_array[i]] = (uint16_t)i;
        }

        for (i = 1; i < index; i++)
        {
            label_list0[i] = label_list1[label_list0[i]];
        }

        index = k - 1;  //  Ŀ����

        if (index > 0)  //  Ŀ������Ϊ0ʱ
        {
            for (i = 0; i < index; i++)
            {
                obj_pos[i].x0 = (int16_t)img_w;  //  ��
                obj_pos[i].x1 = (int16_t)0;      //  ��
                obj_pos[i].y0 = (int16_t)img_h;  //  ��
                obj_pos[i].y1 = (int16_t)0;      //  ��
            }

            for (h = 0; h < img_h; h++)
            {
                for (w = 0; w < img_w; w++)
                {
                    pix = img_w * h + w;

                    if (label_img[pix] != (uint16_t)0) //  ��ԭ����δ������ı��ֵת����lable_list������õı��ֵ
                    {
                        label_img[pix] = label_list0[label_img[pix]];
                        label = (int32_t)label_img[pix];

                        label--;    //  ���(1,2,3,4...)��obj_pos�ж�Ӧ�����Ϊ(0,1,2,3...)

                        obj_area[label]++;

                        obj_pos[label].x0 = (int16_t)MIN2(obj_pos[label].x0, w); //  ��
                        obj_pos[label].x1 = (int16_t)MAX2(obj_pos[label].x1, w); //  ��
                        obj_pos[label].y0 = (int16_t)MIN2(obj_pos[label].y0, h); //  ��
                        obj_pos[label].y1 = (int16_t)MAX2(obj_pos[label].y1, h); //  ��
                    }
                }
            }
        }
    }
    else
    {
        index = 0;
    }

    CHECKED_FREE(sort_array, sizeof(uint16_t) * label_max);
    CHECKED_FREE(label_list1, sizeof(uint16_t) * label_max);
    CHECKED_FREE(label_list0, sizeof(uint16_t) * label_max);
    CHECKED_FREE(equivalent_table, sizeof(uint16_t) * label_max * label_max);


    return index;
}

