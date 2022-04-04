#ifndef _LABEL_H_
#define _LABEL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define LABEL_MAX   (150)

#include "../portab.h"


    int32_t image_label(uint8_t *restrict bina_img, uint16_t *restrict label_img,
                        Rects *restrict obj_pos, int32_t *restrict obj_area,
                        int32_t img_w, int32_t img_h, int32_t label_max);


#ifdef __cplusplus
};
#endif

#endif //  _IMAGEPROC_H_


