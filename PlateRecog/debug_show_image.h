#ifndef _DEBUG_SHOW_IMAGE_
#define _DEBUG_SHOW_IMAGE_

#ifdef WIN32
#include "opencv/cv.h"
#include "opencv/cxcore.h"
#include "opencv/highgui.h"
#endif

#ifdef WIN32
#define _IMAGE1_(image, plate_img, plate_w, plate_h, h_offset)                                                      \
    for (h = 0; h < plate_h; h++)                                                                                   \
    {                                                                                                               \
        for (w = 0; w < plate_w; w++)                                                                               \
        {                                                                                                           \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (1) + 0] = plate_img[(plate_w) * (h) + w]; \
        }                                                                                                           \
    }
#define _IMAGE3_(image, plate_img, plate_w, plate_h, h_offset)                                                      \
    for (h = 0; h < plate_h; h++)                                                                                   \
    {                                                                                                               \
        for (w = 0; w < plate_w; w++)                                                                               \
        {                                                                                                           \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 0] = plate_img[(plate_w) * (h) + w]; \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 1] = plate_img[(plate_w) * (h) + w]; \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 2] = plate_img[(plate_w) * (h) + w]; \
        }                                                                                                           \
    }
#define _IMAGE3_RGB_(image, plate_img, plate_w, plate_h, h_offset)                                                                \
    for (h = 0; h < plate_h; h++)                                                                                                 \
    {                                                                                                                             \
        for (w = 0; w < plate_w; w++)                                                                                                 \
        {                                                                                                                         \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 0] = plate_img[(plate_w) * (h) * (3) + (w) * (3) + 0]; \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 1] = plate_img[(plate_w) * (h) * (3) + (w) * (3) + 1]; \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 2] = plate_img[(plate_w) * (h) * (3) + (w) * (3) + 2]; \
        }                                                                                                                         \
    }
#define _IMAGE3_BGR_(image, plate_img, plate_w, plate_h, h_offset)                                                                \
    for (h = 0; h < plate_h; h++)                                                                                                 \
    {                                                                                                                             \
    for (w = 0; w < plate_w; w++)                                                                                                 \
        {                                                                                                                         \
        image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 0] = plate_img[(plate_w) * (h) * (3) + (w) * (3) + 2]; \
        image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 1] = plate_img[(plate_w) * (h) * (3) + (w) * (3) + 1]; \
        image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 2] = plate_img[(plate_w) * (h) * (3) + (w) * (3) + 0]; \
        }                                                                                                                         \
    }
#define _IMAGE3_HALF_(image, plate_img, plate_w, plate_h, h_offset)                                                       \
    for (h = 0; h < plate_h; h++)                                                                                         \
    {                                                                                                                     \
        for (w = 0; w < plate_w; w++)                                                                                     \
        {                                                                                                                 \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 0] = plate_img[(plate_w) * (h) * (2) + w]; \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 1] = plate_img[(plate_w) * (h) * (2) + w]; \
            image->imageData[(image->widthStep) * (h + h_offset) + (w) * (3) + 2] = plate_img[(plate_w) * (h) * (2) + w]; \
        }                                                                                                                 \
    }
#endif

#endif // _DEBUG_SHOW_IMAGE_
