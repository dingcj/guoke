/********************************************************************
FileName:       lc_plate_analysis.c
Description:    完成视频流车牌分析
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef WIN32
#ifndef DLL_API
#define DLL_API _declspec(dllexport)
#endif
#else
#define DLL_API
#endif

#include "lc_plate_analysis.h"
#include "lc_plate_params.h"
#include "draw_debug_src/draw.h"

#include "location_interface.h"
#include "recognition_interface.h"
#include "plate_tracking_src/plate_tracking.h"
#include "plate_recognition_src/plate_deskewing.h"

#define ENABLE_FLAG  (1u)
#define DISABLE_FLAG (0u)

// 程序版本号与日期
char g_code_version[] = {"V03.00.01.22"};
char g_code_date[] = {"2014-03-19"};

// 注意："北" 前面不要加空格
static char chinese[] =
{
//   00  02  04  06  08  10  12  14  16  18  20  22  24  26  28  30
    "云京冀吉宁川新晋桂沪津浙渝湘琼甘皖粤苏蒙藏豫贵赣辽鄂闽陕青鲁黑"
//   31  33  35  37  39  41
    "北成广海济京军空兰南沈WJ"
//   43  45  47  49  51
    "学警领港澳台挂试超使"
//   53  55  57  59  61
    "消边通森电金林特农武"
//  未知
    "??"
};

// 得到当前车牌识别算法的版本或者日期 1: 版本 0: 日期
int32_t lc_plate_get_version_date(uint8_t bVersion, int8_t **ver_date)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (bVersion == 1u)
    {
        *ver_date = g_code_version;
    }
    else if (bVersion == 0u)
    {
        *ver_date = g_code_date;
    }
    else
    {
        err_ret = LC_PLATE_ERR_PARAM;
    }

    return err_ret;
}

// 获取待分析的图片流或者图片的格式
int32_t lc_plate_get_video_format(Plate_handle plate_handle, int32_t *video_format)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *video_format = pa->video_format;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置待分析的图片流或者图片的格式
int32_t lc_plate_set_video_format(Plate_handle plate_handle, int32_t video_format)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        if ((video_format >= LC_PLATE_PROC_WAIT_PIC) || (video_format <= LC_PLATE_PROC_NOWAIT_VIDEO))
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->video_format = video_format;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->video_format = video_format;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取是否设置车牌红色矩形框标志,  0: 不设置, 1: 设置
int32_t lc_plate_get_position_flag(Plate_handle plate_handle, uint8_t *position_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *position_flag = pa->position_flag;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 是否设置车牌红色矩形框，0: 不设置, 1: 设置
int32_t lc_plate_set_position_flag(Plate_handle plate_handle, uint8_t enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        if ((enable_flag == ENABLE_FLAG) || (enable_flag == DISABLE_FLAG))
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->position_flag = enable_flag;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->position_flag = enable_flag;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取是否设置叠加车牌字符标志, 0: 不叠加，1: 叠加
int32_t lc_plate_get_OSD_flag(Plate_handle plate_handle, uint8_t *OSD_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *OSD_flag = pa->OSD_flag;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置抓拍的车牌图片上是否叠加识别出的车牌字符，0: 不叠加，1: 叠加
int32_t lc_plate_set_OSD_flag(Plate_handle plate_handle, uint8_t enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        if ((enable_flag == ENABLE_FLAG) || (enable_flag == DISABLE_FLAG))
        {
            PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->OSD_flag = enable_flag;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->OSD_flag = enable_flag;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取保存的图片上叠加的车牌字符大小
int32_t lc_plate_get_OSD_scale(Plate_handle plate_handle, double *scale)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *scale = pa->OSD_scale;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置保存的图片上叠加的车牌字符大小(scale >= 0.5 && scale <= 2.0)
int32_t lc_plate_set_OSD_scale(Plate_handle plate_handle, double scale)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        if ((scale >= 0.5) && (scale <= 2.0))
        {
            PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->OSD_scale = scale;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->OSD_scale = scale;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取保存图片上叠加车牌字符的左上角位置
int32_t lc_plate_get_OSD_position(Plate_handle plate_handle, int32_t *x, int32_t *y)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        *x = pa->OSD_pos_x;
        *y = pa->OSD_pos_y;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置保存图片上叠加车牌字符的左上角位置
int32_t lc_plate_set_OSD_position(Plate_handle plate_handle, int32_t x, int32_t y)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        if ((x >= 0) && (x < pa->img_w)
            && (y >= 0) && (y < pa->img_h))
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->OSD_pos_x = x;
                pa->OSD_pos_y = y;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->OSD_pos_x = x;
                    pa->OSD_pos_y = y;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取车牌倾斜校正是否开启标志 0: 关闭 1: 开启
int32_t lc_plate_get_deskew_flag(Plate_handle plate_handle, uint8_t *enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *enable_flag = pa->deskew_flag;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置车牌倾斜校正是否开启 0: 关闭 1: 开启
int32_t lc_plate_set_deskew_flag(Plate_handle plate_handle, uint8_t enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        if ((enable_flag == ENABLE_FLAG) || (enable_flag == DISABLE_FLAG))
        {
            PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->deskew_flag = enable_flag;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->deskew_flag = enable_flag;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取车牌的最小宽度和最大宽度(单位为像素)
int32_t lc_plate_get_plate_width(Plate_handle plate_handle, int32_t *plate_w_min, int32_t *plate_w_max)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        if (pa->loc != NULL)
        {
            lc_plate_location_get_plate_width(pa->loc, plate_w_min, plate_w_max);
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置车牌的最小宽度和最大宽度（单位为像素）
int32_t lc_plate_set_plate_width(Plate_handle plate_handle, int32_t plate_w_min, int32_t plate_w_max)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if ((plate_w_min >= 50) && (plate_w_min <= 100)
        && (plate_w_max >= 150) && (plate_w_max <= 200))
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        if ((plate_handle != NULL) && (pa->loc != NULL))
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                // 调用车牌定位模块的参数设置
                lc_plate_location_set_plate_width(pa->loc, plate_w_min, plate_w_max);
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    // 调用车牌定位模块的参数设置
                    lc_plate_location_set_plate_width(pa->loc, plate_w_min, plate_w_max);
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_PARAM;
    }

    return err_ret;
}

// 获取车牌分析的区域
int32_t lc_plate_get_rect_detect(Plate_handle plate_handle, Rects *rect_detect)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if ((plate_handle != NULL) && (pa->loc != NULL))
    {
        lc_plate_location_get_rect_detect(pa->loc, rect_detect);
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置车牌分析的区域
int32_t lc_plate_set_rect_detect(Plate_handle plate_handle, Rects rect_detect)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if ((plate_handle == NULL) || (pa->loc == NULL)/* || (pa->trk == NULL)*/)
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }
    else if (rect_detect.x0 >= 0
             && rect_detect.y0 >= 0
             && rect_detect.x1 < pa->img_w
             && rect_detect.y1 < pa->img_h
             && rect_detect.x0 < rect_detect.x1
             && rect_detect.y0 < rect_detect.y1)
    {
        if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
        {
            // 调用车牌定位模块的参数设置
            lc_plate_location_set_rect_detect(pa->loc, rect_detect);
            lc_plate_tracking_set_rect_detect(pa->trk, rect_detect);
        }

#ifdef PALTE_THREADS
        else
        {
            DWORD wait_ret;
            wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

            if (wait_ret != (DWORD)WAIT_TIMEOUT)
            {
                // 调用车牌定位模块的参数设置
                lc_plate_location_set_rect_detect(pa->loc, rect_detect);
                lc_plate_tracking_set_rect_detect(pa->trk, rect_detect);
                SetEvent(pa->handle_param);
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }
        }

#endif
    }
    else
    {
        err_ret = LC_PLATE_ERR_PARAM;
    }

    return err_ret;
}

// 获取汉字(省份)识别掩膜 0: 开启该汉字识别 1: 屏蔽该汉字识别
int32_t lc_plate_get_chinese_mask(Plate_handle plate_handle, uint8_t *chinese_mask)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if (chinese_mask == NULL)
    {
        err_ret = LC_PLATE_ERR_PARAM;
    }
    else if ((plate_handle != NULL) && (pa->reg != NULL))
    {
        lc_plate_recognition_get_chinese_mask(pa->reg, chinese_mask);
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置汉字(省份)识别掩膜(提高字符识别的精度) 0: 开启该汉字识别 1: 屏蔽该汉字识别
int32_t lc_plate_set_chinese_mask(Plate_handle plate_handle, uint8_t *chinese_mask)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if (chinese_mask == NULL)
    {
        err_ret = LC_PLATE_ERR_PARAM;
    }
    else if ((plate_handle != NULL) && (pa->reg != NULL))
    {
        int32_t i = 0;
        uint8_t flag = 0u;

        // 判断掩码中是否存在非0和非1的数据
        for (i = 0; i < 31; i++)
        {
            flag |= (chinese_mask[i] & (uint8_t)0xFE);
        }

        // 如果存在非0和非1的数据则属于参数出错
        if (flag)
        {
            return LC_PLATE_ERR_PARAM;
        }

        if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
        {
            // 调用车牌字符识别模块的参数设置
            lc_plate_recognition_set_chinese_mask(pa->reg, chinese_mask);
        }

#ifdef PALTE_THREADS
        else
        {
            DWORD wait_ret;
            wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

            if (wait_ret != (DWORD)WAIT_TIMEOUT)
            {
                // 调用车牌字符识别模块的参数设置
                lc_plate_recognition_set_chinese_mask(pa->reg, chinese_mask);
                SetEvent(pa->handle_param);
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }
        }

#endif
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取默认的汉字(省份)
int32_t lc_plate_get_chinese_default(Plate_handle plate_handle, char *chinese_default)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if ((plate_handle != NULL) && (pa->reg != NULL))
    {
        uint8_t dist = 0u;

        lc_plate_recognition_get_chinese_default(pa->reg, &dist);

        dist -= LABEL_START_OF_CHINESE;

        if (dist < 31u)
        {
            chinese_default[0] = chinese[2u * dist + 0u];
            chinese_default[1] = chinese[2u * dist + 1u];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置默认的汉字(省份)，当汉字的置信度不高时(不太肯定时)，采用默认汉字
int32_t lc_plate_set_chinese_default(Plate_handle plate_handle, char *chinese_default)
{
    int32_t err_ret = LC_PLATE_ERR_PARAM;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if ((plate_handle != NULL) && (pa->reg != NULL))
    {
        int32_t i;

        // 总共31个省份汉字
        for (i = 0; i < 31; i++)
        {
            if (chinese_default[0] == chinese[i * 2 + 0]
                && chinese_default[1] == chinese[i * 2 + 1])
            {
                err_ret = LC_PLATE_SUCCESS;
                break;
            }
        }

        if (err_ret == LC_PLATE_SUCCESS)
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                // 调用车牌字符识别模块的参数设置
                lc_plate_recognition_set_chinese_default(pa->reg, (uint8_t)((uint32_t)i + LABEL_START_OF_CHINESE));
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    // 调用车牌字符识别模块的参数设置
                    lc_plate_recognition_set_chinese_default(pa->reg, (uint8_t)((uint32_t)i + LABEL_START_OF_CHINESE));
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取汉字的置信度阈值
int32_t lc_plate_get_chinese_trust_thresh(Plate_handle plate_handle, uint8_t *trust_thresh)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if ((plate_handle != NULL) && (pa->reg != NULL))
    {
        lc_plate_recognition_get_chinese_trust_thresh(pa->reg, trust_thresh);
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置汉字的置信度阈值，当低于该阈值时采用默认的省份汉字
int32_t lc_plate_set_chinese_trust_thresh(Plate_handle plate_handle, uint8_t trust_thresh)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

    if ((plate_handle != NULL) && (pa->reg != NULL))
    {
        if ((trust_thresh > 0u) && (trust_thresh < 101u))
        {
//          trust_thresh = MIN2(MAX2(0, trust_thresh), 100);

            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                lc_plate_recognition_set_chinese_trust_thresh(pa->reg, trust_thresh);
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    lc_plate_recognition_set_chinese_trust_thresh(pa->reg, trust_thresh);
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍车牌图片(JPG)的压缩质量，1 ~ 100，值越大，质量越好、图片的文件也越大
int32_t lc_plate_get_image_quality(Plate_handle plate_handle, int32_t *img_quality)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *img_quality = pa->img_quality;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置抓拍车牌图片(JPG)的压缩质量，1 ~ 100，值越大，质量越好、图片的文件也越大
int32_t lc_plate_set_image_quality(Plate_handle plate_handle, int32_t img_quality)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle == NULL)
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }
    else if ((img_quality > 0) && (img_quality < 101))
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

#ifdef _TMS320C6X_
        img_quality = MIN2(img_quality, 85);
#endif

        if (pa->img_quality != img_quality)
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->img_quality = img_quality;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->img_quality = img_quality;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_PARAM;
    }

    return err_ret;
}

// 获取车身颜色识别标志，0:不识别 1:识别
int32_t lc_plate_get_car_color_enable(Plate_handle plate_handle, uint8_t *enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        *enable_flag = pa->car_color_enable;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置车身颜色识别标志，0:不识别 1:识别
int32_t lc_plate_set_car_color_enable(Plate_handle plate_handle, uint8_t enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle == NULL)
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }
    else
    {
        if ((enable_flag == ENABLE_FLAG) || (enable_flag == DISABLE_FLAG))
        {
            PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->car_color_enable = enable_flag;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->car_color_enable = enable_flag;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }

    return err_ret;
}

// 获取车窗区域识别标志，0:不识别 1:识别
int32_t lc_plate_get_car_window_enable(Plate_handle plate_handle, uint8_t *enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;
        *enable_flag = pa->car_window_enable;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 设置车窗区域识别标志，0:不识别 1:识别
int32_t lc_plate_set_car_window_enable(Plate_handle plate_handle, uint8_t enable_flag)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle == NULL)
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }
    else
    {
        PlateAnalysis *pa = (PlateAnalysis *)plate_handle;

        if ((enable_flag == ENABLE_FLAG) || (enable_flag == DISABLE_FLAG))
        {
            if ((pa->proce_type == LC_PLATE_PROC_WAIT_PIC) || (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO))
            {
                pa->car_window_enable = enable_flag;
            }

#ifdef PALTE_THREADS
            else
            {
                DWORD wait_ret;
                wait_ret = WaitForSingleObject(pa->handle_param, (DWORD)100);

                if (wait_ret != (DWORD)WAIT_TIMEOUT)
                {
                    pa->car_window_enable = enable_flag;
                    SetEvent(pa->handle_param);
                }
                else
                {
                    err_ret = LC_PLATE_FAIL;
                }
            }

#endif
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }

    return err_ret;
}

// 获取成功识别车牌的数目
int32_t lc_plate_get_plate_number(Plate_handle plate_handle, int32_t *plate_num)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        *plate_num = pinfo->save_num;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的字符数目
int32_t lc_plate_get_plate_character_number(Plate_handle plate_handle, int32_t index, uint8_t *ch_num)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *ch_num = pinfo->ch_num[index];
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的车牌号字符串
int32_t lc_plate_get_plate_name(Plate_handle plate_handle, int32_t index, int8_t **plate_name)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;
        int8_t *ptr = NULL;
        int32_t dist = 0;

        if (index < pinfo->save_num)
        {
            if (pinfo->ch_num[index] == (uint8_t)8)
            {
                dist = (int32_t)(pinfo->chs[index][0] - LABEL_START_OF_CHINESE) * 2;

                pinfo->character[index][0] = (int8_t)chinese[dist + 0];
                pinfo->character[index][1] = (int8_t)chinese[dist + 1];
                pinfo->character[index][2] = (int8_t)pinfo->chs[index][1];
                pinfo->character[index][3] = (int8_t)pinfo->chs[index][2];
                pinfo->character[index][4] = (int8_t)pinfo->chs[index][3];
                pinfo->character[index][5] = (int8_t)pinfo->chs[index][4];
                pinfo->character[index][6] = (int8_t)pinfo->chs[index][5];
                pinfo->character[index][7] = (int8_t)pinfo->chs[index][6];
                pinfo->character[index][8] = (int8_t)pinfo->chs[index][7];
                pinfo->character[index][9] = (int8_t)'\0';
            }
            else
            {
                dist = (int32_t)(pinfo->chs[index][0] - LABEL_START_OF_CHINESE) * 2;

                pinfo->character[index][0] = (int8_t)chinese[dist + 0];
                pinfo->character[index][1] = (int8_t)chinese[dist + 1];
                pinfo->character[index][2] = (int8_t)pinfo->chs[index][1];
                pinfo->character[index][3] = (int8_t)pinfo->chs[index][2];
                pinfo->character[index][4] = (int8_t)pinfo->chs[index][3];
                pinfo->character[index][5] = (int8_t)pinfo->chs[index][4];
                pinfo->character[index][6] = (int8_t)pinfo->chs[index][5];
                pinfo->character[index][7] = (int8_t)pinfo->chs[index][6];
                pinfo->character[index][8] = (int8_t)'\0';

                // 最后一个车牌字符是汉字的情况
                ptr = &pinfo->character[index][7];

                switch (pinfo->chs[index][6])
                {
                    case CH_XUE:
                        strcpy(ptr, "学");
                        break;

                    case CH_JING:
                        strcpy(ptr, "警");
                        break;

                    case CH_LING:
                        strcpy(ptr, "领");
                        break;

                    case CH_GANG:
                        strcpy(ptr, "港");
                        break;

                    case CH_AO:
                        strcpy(ptr, "澳");
                        break;

                    case CH_TAI:
                        strcpy(ptr, "台");
                        break;

                    case CH_GUA:
                        strcpy(ptr, "挂");
                        break;

                    case CH_SHIY:
                        strcpy(ptr, "试");
                        break;

                    case CH_CHAO:
                        strcpy(ptr, "超");
                        break;

                    case CH_SHIG:
                        strcpy(ptr, "使");
                        break;

                    default:
                        break;
                }
            }

            *plate_name = pinfo->character[index];
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取当前帧检测到车牌的数目(用于阻塞方式中得到当前帧的信息)
int32_t lc_plate_get_current_plate_number(Plate_handle plate_handle, int32_t *current_num)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;
        *current_num = pinfo->plate_num;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取当前帧检测到车牌的位置(用于阻塞方式中得到当前帧的信息)，index表示车牌序号，从0开始
int32_t lc_plate_get_current_plate_position(Plate_handle plate_handle, int32_t index, Rects *current_position)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->plate_num)
        {
            current_position->x0 = pinfo->plate_list[index].x0;
            current_position->x1 = pinfo->plate_list[index].x1;
            current_position->y0 = pinfo->plate_list[index].y0;
            current_position->y1 = pinfo->plate_list[index].y1;
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的车牌位置，index表示车牌序号，从0开始
int32_t lc_plate_get_plate_position(Plate_handle plate_handle, int32_t index, Rects *plate_position)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            plate_position->x0 = pinfo->plate_rect[index].x0;
            plate_position->x1 = pinfo->plate_rect[index].x1;
            plate_position->y0 = pinfo->plate_rect[index].y0;
            plate_position->y1 = pinfo->plate_rect[index].y1;
        }
        else
        {
            err_ret = LC_PLATE_ERR_PARAM;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍的车牌图片的数据长度(单位字节byte)
int32_t lc_plate_get_plate_picture_size(Plate_handle plate_handle, int32_t *pic_len)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (pinfo->pbuff_lens > 0)
        {
            *pic_len = pinfo->pbuff_lens;
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍的车牌图片，pbuff用于存放图像数据的缓存(JPG格式)，buffsize表示缓存的大小（缓存最大为：image_w*image_h*3/2）
// 函数返回后，buffsize的值为图像数据的长度；成功则pbuff中有数据，失败则无数据(一般是缓存过小)。(三张图片暂时未考虑)
int32_t lc_plate_get_plate_picture(Plate_handle plate_handle, uint8_t *pbuff, int32_t *buffsize)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (pinfo->pbuff_lens <= *buffsize)
        {
            memcpy(pbuff, pinfo->pic_buff, pinfo->pbuff_lens);
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }

        *buffsize = pinfo->pbuff_lens;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍的车牌图片的数据指针(起始地址)
int32_t lc_plate_get_plate_picture_pointer(Plate_handle plate_handle, uint8_t **pic_pointer)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        *pic_pointer = pinfo->pic_buff;
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

//==================================================================================================
// 获取抓拍的车牌区域图片的数据长度(单位字节byte)
int32_t lc_plate_get_small_picture_size(Plate_handle plate_handle, int32_t index, int32_t *small_pic_len)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if ((pinfo != NULL) && (index < pinfo->save_num) && (pinfo->psmall_lens[index] > 0))
        {
            *small_pic_len = pinfo->psmall_lens[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍的车牌图片，pbuff用于存放图像数据的缓存(JPG格式)，buffsize表示缓存的大小（缓存最大为：image_w*image_h*3/2）
// 函数返回后，buffsize的值为车牌区域图像数据的长度；成功则pbuff中有数据，失败则无数据(一般是缓存过小)。(三张图片暂时未考虑)
int32_t lc_plate_get_small_picture(Plate_handle plate_handle, uint8_t *pbuff, int32_t *buffsize, int32_t index)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if ((pinfo != NULL))
        {
            if ((index < pinfo->save_num) && (pinfo->psmall_lens[index] <= *buffsize))
            {
                memcpy(pbuff, pinfo->pic_small[index], pinfo->psmall_lens[index]);
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }

            *buffsize = pinfo->psmall_lens[index];
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}
// 获取抓拍的车牌区域图片的数据指针(起始地址)
int32_t lc_plate_get_small_picture_pointer(Plate_handle plate_handle, int32_t index, uint8_t **small_pic_pointer)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if ((pinfo != NULL) && (index < pinfo->save_num))
        {
            *small_pic_pointer = pinfo->pic_small[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}
//==================================================================================================

// 获取已成功识别车牌的车辆车窗区域位置，index表示车牌序号，从0开始
int32_t lc_plate_get_window_position(Plate_handle plate_handle, int32_t index, Rects *window_position)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            window_position->x0 = pinfo->car_window_rect[index].x0;
            window_position->x1 = pinfo->car_window_rect[index].x1;
            window_position->y0 = pinfo->car_window_rect[index].y0;
            window_position->y1 = pinfo->car_window_rect[index].y1;
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍的车窗区域图片的数据长度(单位字节byte)
int32_t lc_plate_get_window_picture_size(Plate_handle plate_handle, int32_t index, int32_t *window_pic_len)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if ((pinfo != NULL) && (index < pinfo->save_num) && (pinfo->pwindow_lens[index] > 0))
        {
            *window_pic_len = pinfo->pwindow_lens[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取抓拍的车窗区域图片，pbuff用于存放图像数据的缓存(JPG格式)，buffsize表示缓存的大小（缓存最大为：image_w*image_h*3/2）
// 函数返回后，buffsize的值为图像数据的长度；成功则pbuff中有数据，失败则无数据(一般是缓存过小)。(三张图片暂时未考虑)
int32_t lc_plate_get_window_picture(Plate_handle plate_handle, uint8_t *pbuff, int32_t *buffsize, int32_t index)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if ((pinfo != NULL))
        {
            if ((index < pinfo->save_num) && (pinfo->pwindow_lens[index] <= *buffsize))
            {
                memcpy(pbuff, pinfo->pic_window[index], pinfo->pwindow_lens[index]);
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }

            *buffsize = pinfo->pwindow_lens[index];
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}
// 获取抓拍的车窗区域图片的数据指针(起始地址)
int32_t lc_plate_get_window_picture_pointer(Plate_handle plate_handle, int32_t index, uint8_t **window_pic_pointer)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if ((pinfo != NULL) && (index < pinfo->save_num))
        {
            *window_pic_pointer = pinfo->pic_window[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}
//==================================================================================================

// 获取已成功识别车牌的颜色代码，index表示车牌序号，从0开始
// (车牌颜色 0:蓝底 1:黄底 2:白底－军牌 3:白底－警牌 4:黑牌 5:红牌 6:绿牌 99:未知)
int32_t lc_plate_get_plate_color_id(Plate_handle plate_handle, int32_t index, uint8_t *plate_color_id)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *plate_color_id = pinfo->color[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的颜色字符串，index表示车牌序号，从0开始
int32_t lc_plate_get_plate_color_name(Plate_handle plate_handle, int32_t index, int8_t **plate_color)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            switch ((int32_t)pinfo->color[index])
            {
                case 0:
                    *plate_color = "蓝牌";
                    break;

                case 1:
                    *plate_color = "黄牌";
                    break;

                case 2:
                    *plate_color = "白牌";
                    break;

                case 3:
                    *plate_color = "白牌";
                    break;

                case 4:
                    *plate_color = "黑牌";
                    break;

                case 5:
                    *plate_color = "红牌";
                    break;

                case 6:
                    *plate_color = "绿牌";
                    break;
                case 10:
                    *plate_color = "新能源";
                    break;
                default:
                    *plate_color = "未知颜色";
                    break;
            }
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的置信度，index表示车牌序号，从0开始(置信度越高，识别结果越可信)
int32_t lc_plate_get_plate_reliability(Plate_handle plate_handle, int32_t index, uint8_t *plate_reliability)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *plate_reliability = pinfo->trust[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的单个字符置信度(置信度越高，识别结果越可信)
int32_t lc_plate_get_character_reliability(Plate_handle plate_handle, int32_t index, int32_t ch_num, uint8_t *ch_reliability)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            if ((uint8_t)ch_num < pinfo->ch_num[index])
            {
                *ch_reliability = pinfo->chs_trust[index][ch_num];
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的计数值，index表示车牌序号，从0开始(同一车牌连续出现的次数)
int32_t lc_plate_get_plate_count(Plate_handle plate_handle, int32_t index, int32_t *plate_cnt)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *plate_cnt = pinfo->count[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的亮度值，index表示车牌序号，从0开始
int32_t lc_plate_get_plate_brightness(Plate_handle plate_handle, int32_t index, uint8_t *brightness)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *brightness = pinfo->brightness[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的号牌类型代码，index表示车牌序号，从0开始
// (车牌类型 1:大型汽车号牌 2:小型汽车号牌 3:新能源车牌 16:教练汽车号牌 23:警用汽车号牌
// 25:军用汽车号牌 26:境外汽车号牌 27: 境外商用汽车号牌 28: 农用汽车号牌 99:未知)
int32_t lc_plate_get_plate_type_id(Plate_handle plate_handle, int32_t index, uint8_t *plate_type_id)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *plate_type_id = (int32_t)pinfo->type[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的号牌类型字符串，index表示车牌序号，从0开始
int32_t lc_plate_get_plate_type_name(Plate_handle plate_handle, int32_t index, int8_t **plate_type)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            switch ((int32_t)pinfo->type[index])
            {
                case 1:
                    *plate_type = "大型汽车号牌";
                    break;

                case 2:
                    *plate_type = "小型汽车号牌";
                    break;

                case 16:
                    *plate_type = "教练汽车号牌";
                    break;

                case 23:
                    *plate_type = "警用汽车号牌";
                    break;

                case 25:
                    *plate_type = "军用汽车号牌";
                    break;

                case 26:
                    *plate_type = "境外汽车号牌";
                    break;

                case 27:
                    *plate_type = "境外商用汽车号牌";
                    break;

                case 28:
                    *plate_type = "农用汽车号牌";
                    break;

                default:
                    *plate_type = "未知号牌";
                    break;
            }
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的车辆速度，index表示车牌序号，从0开始
int32_t lc_plate_get_car_speed(Plate_handle plate_handle, int32_t index, int32_t *car_speed)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            int32_t vx = 0;
            int32_t vy = 0;

            vx = (int32_t)pinfo->speeds[index][0];
            vy = (int32_t)pinfo->speeds[index][1];

            *car_speed = (int32_t)sqrt((vx * vx) + (vy * vy));
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的车身颜色代码，index表示车牌序号，从0开始
// 车身颜色 0:白色 1:银灰 2:黄色 3:粉色 4:红色 5:紫色 6:绿色 7:蓝色 8:棕色 9: 黑色 99:未知 255:未识别
int32_t lc_plate_get_car_color_id(Plate_handle plate_handle, int32_t index, uint8_t *car_color_id)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        if (((PlateAnalysis *)plate_handle)->car_color_enable)
        {
            PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

            if (index < pinfo->save_num)
            {
                *car_color_id = pinfo->car_color[index];
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
            *car_color_id = (255u);
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车身颜色的置信度，index表示车牌序号，从0开始(置信度越高，识别结果越可信)
int32_t lc_plate_get_car_color_reliability(Plate_handle plate_handle, int32_t index, uint8_t *car_color_reliability)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        if (((PlateAnalysis *)plate_handle)->car_color_enable)
        {
            PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

            if (index < pinfo->save_num)
            {
                *car_color_reliability = pinfo->car_color_trust[index];
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的车身颜色字符串，index表示车牌序号，从0开始
// 车身颜色 0:白色 1:银灰 2:黄色 3:粉色 4:红色 5:紫色 6:绿色 7:蓝色 8:棕色 9: 黑色 99:未知 255:未识别
int32_t lc_plate_get_car_color_name(Plate_handle plate_handle, int32_t index, int8_t **car_color)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        if (((PlateAnalysis *)plate_handle)->car_color_enable)
        {
            PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

            if (index < pinfo->save_num)
            {
                switch ((int32_t)pinfo->car_color[index])
                {
                    case 0:
                        *car_color = "白色";
                        break;

                    case 1:
                        *car_color = "银灰";
                        break;

                    case 2:
                        *car_color = "黄色";
                        break;

                    case 3:
                        *car_color = "粉色";
                        break;

                    case 4:
                        *car_color = "红色";
                        break;

                    case 5:
                        *car_color = "紫色";
                        break;

                    case 6:
                        *car_color = "绿色";
                        break;

                    case 7:
                        *car_color = "蓝色";
                        break;

                    case 8:
                        *car_color = "棕色";
                        break;

                    case 9:
                        *car_color = "黑色";
                        break;

                    case 99:
                        *car_color = "未知";
                        break;

                    default:
                        *car_color = "未知";
                        break;
                }
            }
            else
            {
                err_ret = LC_PLATE_FAIL;
            }
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

// 获取已成功识别车牌的车辆运动方向，index表示车牌序号，从0开始
// (车牌运动方向 0:未知方向 1:车牌从上往下运动 2:车牌从下往上运动)
int32_t lc_plate_get_car_direction(Plate_handle plate_handle, int32_t index, uint8_t *car_direction)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        PlateInfo *pinfo = ((PlateAnalysis *)plate_handle)->pinfo;

        if (index < pinfo->save_num)
        {
            *car_direction = pinfo->directions[index];
        }
        else
        {
            err_ret = LC_PLATE_FAIL;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}
