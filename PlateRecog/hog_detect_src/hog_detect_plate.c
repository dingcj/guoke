#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hog_detect_plate.h"
#include "../common/image.h"
#include "../lc_plate_analysis.h"
#include "../portab.h"

#ifdef WIN32
#include "../debug_show_image.h"
#endif

int32_t hog_detect_proc(uint8_t *src_img, int32_t img_w, int32_t img_h,
                        Rects* plate_rect, Rects* head_rect)
{
    int32_t h;
    uint8_t *plate_img = NULL;
    uint8_t *resize_img = NULL;
    int32_t plate_w, plate_h;
    int32_t sta_w, sta_h;      // 缩放后的宽度和高度
    int32_t found_num;
    hogfeature_type hog_feature[MAX_HOG_FEATURE_LEN];
    MyRect foundbox[MAX_HOG_FEATURE_LEN];
    int32_t hog_feature_len = 0;
    Rects plate_rect_tmp;

    plate_w = plate_rect->x1 - plate_rect->x0 + 1;
    plate_h = plate_rect->y1 - plate_rect->y0 + 1;

    plate_rect_tmp.x0 = MAX2(plate_rect->x0 - plate_w / 5, 0);
    plate_rect_tmp.x1 = MIN2(plate_rect->x1 + plate_w / 5, img_w - 1);
    plate_rect_tmp.y0 = MAX2(plate_rect->y0 - plate_h / 2, 0);
    plate_rect_tmp.y1 = MIN2(plate_rect->y1 + plate_h / 2, img_h - 1);

    plate_w = plate_rect_tmp.x1 - plate_rect_tmp.x0 + 1;
    plate_h = plate_rect_tmp.y1 - plate_rect_tmp.y0 + 1;

    CHECKED_MALLOC(plate_img, plate_w * plate_h * sizeof(uint8_t), CACHE_SIZE);

    for (h = 0; h < plate_h; h++)
    {
        memcpy(plate_img + plate_w * h, src_img + (h + plate_rect_tmp.y0) * img_w + plate_rect_tmp.x0, plate_w);
    }

    {
        sta_w = 64;
        sta_h = 32;

        CHECKED_MALLOC(resize_img, sta_w * sta_h * sizeof(uint8_t), CACHE_SIZE);

        gray_image_resize_linear(plate_img, resize_img, plate_w, plate_h, sta_w, sta_h);

        found_num = 0;
        calculate_hog_features_and_detect_c(resize_img,
                                            hog_feature, &hog_feature_len,
                                            mySize(64, 32),
                                            mySize(16, 16),
                                            mySize(8, 8),
                                            mySize(8, 8),
                                            9,
                                            4,
                                            sta_w, sta_h,
                                            foundbox, &found_num);

//         {
//             int32_t w, h;
//             IplImage *hog_img = NULL;
//             char plate_save_name[50];
//             static int32_t plate_num = 0;
//             hog_img = cvCreateImage(cvSize(sta_w, sta_h), IPL_DEPTH_8U, 1);
//             _IMAGE1_(hog_img, resize_img, sta_w, sta_h, 0);
//
//             if (found_num > 0)
//             {
//                 sprintf(plate_save_name, "right_plate//img%05d.jpg", plate_num);
//                 cvSaveImage(plate_save_name, hog_img, 0);
//             }
//             else
//             {
//                 sprintf(plate_save_name, "err_plate//img%05d.jpg", plate_num);
//                 cvSaveImage(plate_save_name, hog_img, 0);
//             }
//             plate_num++;
//             cvReleaseImage(&hog_img);
//         }

#if 0
        {
            int32_t w, h;
            IplImage *hog_img = NULL;
            hog_img = cvCreateImage(cvSize(sta_w, sta_h), IPL_DEPTH_8U, 3);
            _IMAGE3_(hog_img, resize_img, sta_w, sta_h, 0);

            for (h = 0; h < found_num; h++)
            {
                cvRectangle(hog_img,
                            cvPoint(foundbox[h].x, foundbox[h].y),
                            cvPoint(foundbox[h].x + foundbox[h].width - 1, foundbox[h].y + foundbox[h].height - 1),
                            cvScalar(0, 255, 0, 0), 2, 2, 0);
            }

            cvNamedWindow("show", 1);
            cvShowImage("show", hog_img);
            cvWaitKey(0);
            cvDestroyWindow("show");
            cvReleaseImage(&hog_img);
        }
#endif

        CHECKED_FREE(resize_img, sta_w * sta_h * sizeof(uint8_t));
    }

    CHECKED_FREE(plate_img, plate_w * plate_h * sizeof(uint8_t));

    return found_num;
}

