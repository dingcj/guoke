#ifndef _LC_PLATE_ANALYSIS_H_
#define _LC_PLATE_ANALYSIS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WIN32
#ifndef DLL_API
#define DLL_API _declspec(dllimport)
#endif
#else
#define DLL_API
#endif

#include "portab.h"

    typedef void* Plate_handle; // 车牌分析句柄

#ifdef _TMS320C6X_
#pragma CODE_SECTION(lc_plate_get_version_date, ".text:lc_plate_get_version_date")
#pragma CODE_SECTION(lc_plate_set_plate_width, ".text:lc_plate_set_plate_width")
#pragma CODE_SECTION(lc_plate_get_plate_width, ".text:lc_plate_get_plate_width")
#pragma CODE_SECTION(lc_plate_set_rect_detect, ".text:lc_plate_set_rect_detect")
#pragma CODE_SECTION(lc_plate_get_rect_detect, ".text:lc_plate_get_rect_detect")
#pragma CODE_SECTION(lc_plate_set_chinese_mask, ".text:lc_plate_set_chinese_mask")
#pragma CODE_SECTION(lc_plate_get_chinese_mask, ".text:lc_plate_get_chinese_mask")
#pragma CODE_SECTION(lc_plate_get_chinese_default, ".text:lc_plate_get_chinese_default")
#pragma CODE_SECTION(lc_plate_set_chinese_default, ".text:lc_plate_set_chinese_default")
#pragma CODE_SECTION(lc_plate_get_chinese_trust_thresh, ".text:lc_plate_get_chinese_trust_thresh")
#pragma CODE_SECTION(lc_plate_set_chinese_trust_thresh, ".text:lc_plate_set_chinese_trust_thresh")
#pragma CODE_SECTION(lc_plate_set_car_color_enable, ".text:lc_plate_set_car_color_enable")
#pragma CODE_SECTION(lc_plate_get_car_color_enable, ".text:lc_plate_get_car_color_enable")
#pragma CODE_SECTION(lc_plate_set_car_window_enable, ".text:lc_plate_set_car_window_enable")
#pragma CODE_SECTION(lc_plate_get_car_window_enable, ".text:lc_plate_get_car_window_enable")
#pragma CODE_SECTION(lc_plate_set_video_format, ".text:lc_plate_set_video_format")
#pragma CODE_SECTION(lc_plate_get_video_format, ".text:lc_plate_get_video_format")
#pragma CODE_SECTION(lc_plate_set_position_flag, ".text:lc_plate_set_position_flag")
#pragma CODE_SECTION(lc_plate_get_position_flag, ".text:lc_plate_get_position_flag")
#pragma CODE_SECTION(lc_plate_set_OSD_flag, ".text:lc_plate_set_OSD_flag")
#pragma CODE_SECTION(lc_plate_get_OSD_flag, ".text:lc_plate_get_OSD_flag")
#pragma CODE_SECTION(lc_plate_set_OSD_scale, ".text:lc_plate_set_OSD_scale")
#pragma CODE_SECTION(lc_plate_get_OSD_scale, ".text:lc_plate_get_OSD_scale")
#pragma CODE_SECTION(lc_plate_set_OSD_position, ".text:lc_plate_set_OSD_position")
#pragma CODE_SECTION(lc_plate_get_OSD_position, ".text:lc_plate_get_OSD_position")
#pragma CODE_SECTION(lc_plate_set_deskew_flag, ".text:lc_plate_set_deskew_flag")
#pragma CODE_SECTION(lc_plate_get_deskew_flag, ".text:lc_plate_get_deskew_flag")
#pragma CODE_SECTION(lc_plate_set_image_quality, ".text:lc_plate_set_image_quality")
#pragma CODE_SECTION(lc_plate_get_image_quality, ".text:lc_plate_get_image_quality")
#pragma CODE_SECTION(lc_plate_get_current_plate_number, ".text:lc_plate_get_current_plate_number")
#pragma CODE_SECTION(lc_plate_get_current_plate_position, ".text:lc_plate_get_current_plate_position")
#pragma CODE_SECTION(lc_plate_get_plate_number, ".text:lc_plate_get_plate_number")
#pragma CODE_SECTION(lc_plate_get_plate_character_number, ".text:lc_plate_get_plate_character_number")
#pragma CODE_SECTION(lc_plate_get_plate_position, ".text:lc_plate_get_plate_position")
#pragma CODE_SECTION(lc_plate_get_plate_picture_size, ".text:lc_plate_get_plate_picture_size")
#pragma CODE_SECTION(lc_plate_get_plate_picture, ".text:lc_plate_get_plate_picture")
#pragma CODE_SECTION(lc_plate_get_plate_picture_pointer, ".text:lc_plate_get_plate_picture_pointer")
#pragma CODE_SECTION(lc_plate_get_small_picture_size, ".text:lc_plate_get_small_picture_size")
#pragma CODE_SECTION(lc_plate_get_small_picture, ".text:lc_plate_get_small_picture")
#pragma CODE_SECTION(lc_plate_get_small_picture_pointer, ".text:lc_plate_get_small_picture_pointer")
#pragma CODE_SECTION(lc_plate_get_window_position, ".text:lc_plate_get_window_position")
#pragma CODE_SECTION(lc_plate_get_window_picture_size, ".text:lc_plate_get_window_picture_size")
#pragma CODE_SECTION(lc_plate_get_window_picture, ".text:lc_plate_get_window_picture")
#pragma CODE_SECTION(lc_plate_get_window_picture_pointer, ".text:lc_plate_get_window_picture_pointer")
#pragma CODE_SECTION(lc_plate_get_plate_name, ".text:lc_plate_get_plate_name")
#pragma CODE_SECTION(lc_plate_get_plate_color_id, ".text:lc_plate_get_plate_color_id")
#pragma CODE_SECTION(lc_plate_get_plate_color_name, ".text:lc_plate_get_plate_color_name")
#pragma CODE_SECTION(lc_plate_get_plate_reliability, ".text:lc_plate_get_plate_reliability")
#pragma CODE_SECTION(lc_plate_get_character_reliability, ".text:lc_plate_get_character_reliability")
#pragma CODE_SECTION(lc_plate_get_plate_count, ".text:lc_plate_get_plate_count")
#pragma CODE_SECTION(lc_plate_get_plate_brightness, ".text:lc_plate_get_plate_brightness")
#pragma CODE_SECTION(lc_plate_get_plate_type_id, ".text:lc_plate_get_plate_type_id")
#pragma CODE_SECTION(lc_plate_get_plate_type_name, ".text:lc_plate_get_plate_type_name")
#pragma CODE_SECTION(lc_plate_get_car_speed, ".text:lc_plate_get_car_speed")
#pragma CODE_SECTION(lc_plate_get_car_color_id, ".text:lc_plate_get_car_color_id")
#pragma CODE_SECTION(lc_plate_get_car_color_name, ".text:lc_plate_get_car_color_name")
#pragma CODE_SECTION(lc_plate_get_car_color_reliability, ".text:lc_plate_get_car_color_reliability")
#pragma CODE_SECTION(lc_plate_get_car_direction, ".text:lc_plate_get_car_direction")
#endif

    /******************************************************************/
    /*                车牌识别函数常见返回值类型                      */
    /******************************************************************/
#define LC_PLATE_SUCCESS                0                         // 操作成功
#define LC_PLATE_FAIL                   (LC_PLATE_SUCCESS + 100)  // 操作失败
#define LC_PLATE_ERR_INVALID_HANDLE     (LC_PLATE_SUCCESS + 101)  // 输入句柄无效
#define LC_PLATE_ERR_CREATE_HANDLE      (LC_PLATE_SUCCESS + 102)  // 创建句柄失败
#define LC_PLATE_ERR_THREADNUM_LIMIT    (LC_PLATE_SUCCESS + 103)  // 线程数目限制
#define LC_PLATE_ERR_TIME_LIMIT         (LC_PLATE_SUCCESS + 104)  // 使用时间限制
#define LC_PLATE_ERR_THREAD_CHECK       (LC_PLATE_SUCCESS + 105)  // 线程检测失败
#define LC_PLATE_ERR_BUFF_OVERFLOW      (LC_PLATE_SUCCESS + 106)  // 图像缓存溢出
#define LC_PLATE_ERR_DOG_CHECK          (LC_PLATE_SUCCESS + 107)  // 加密狗检测失败
#define LC_PLATE_ERR_PARAM              (LC_PLATE_SUCCESS + 108)  // 输入参数有误
#define LC_PLATE_ERR_VIDEO_FORMAT       (LC_PLATE_SUCCESS + 109)  // 输入文件数据格式不支持
#define LC_PLATE_ERR_VIDEO_SIZE         (LC_PLATE_SUCCESS + 110)  // 图像大小错误（有可能是图像的宽度、高度不满足16的倍数）
    /******************************************************************/

    /******************************************************************/
    /*                        常见视频类型定义                        */
    /******************************************************************/
// 视频流处理方式---以整帧裸数据(没有头部)作为输入
//=============================================================
#define LC_VIDEO_FORMAT_PLANAR          (1<< 0) // 4:2:0 planar
#define LC_VIDEO_FORMAT_I420            (1<< 1) // 4:2:0 planar (Y U V的排列顺序)
#define LC_VIDEO_FORMAT_YV12            (1<< 2) // 4:2:0 planar (Y V U的排列顺序)

#define LC_VIDEO_FORMAT_YUY2            (1<< 3) // 4:2:2 packed
#define LC_VIDEO_FORMAT_YUYV            (1<< 3) // 4:2:2 packed
#define LC_VIDEO_FORMAT_UYVY            (1<< 4) // 4:2:2 packed
#define LC_VIDEO_FORMAT_YVYU            (1<< 5) // 4:2:2 packed

#define LC_VIDEO_FORMAT_BGRA            (1<< 6) // 32-bit BGRA packed
#define LC_VIDEO_FORMAT_ABGR            (1<< 7) // 32-bit ABGR packed
#define LC_VIDEO_FORMAT_RGBA            (1<< 8) // 32-bit RGBA packed
#define LC_VIDEO_FORMAT_ARGB            (1<<15) // 32-bit ARGB packed

#define LC_VIDEO_FORMAT_BGR             (1<< 9) // 24-bit BGR packed (B G R的排列顺序)
#define LC_VIDEO_FORMAT_BGR24           (1<< 9) // 24-bit BGR packed (B G R的排列顺序)
#define LC_VIDEO_FORMAT_RGB             (1<<10) // 24-bit RGB packed (R G B的排列顺序)
#define LC_VIDEO_FORMAT_RGB24           (1<<10) // 24-bit RGB packed (R G B的排列顺序)

#define LC_VIDEO_FORMAT_RGB555          (1<<11) // 16-bit RGB555 packed (R G B的排列顺序)
#define LC_VIDEO_FORMAT_RGB565          (1<<12) // 16-bit RGB565 packed (R G B的排列顺序)
#define LC_VIDEO_FORMAT_BGR555          (1<<13) // 16-bit BGR555 packed (B G R的排列顺序)
#define LC_VIDEO_FORMAT_BGR565          (1<<14) // 16-bit BGR565 packed (B G R的排列顺序)
//=============================================================
// 单张图片处理方式---以整帧图片(包含头部)作为输入
//=============================================================
#define LC_VIDEO_FORMAT_JPG             (1<<29) // 以单帧JPG图片作为输入
#define LC_VIDEO_FORMAT_BMP             (1<<30) // 以单帧BMP图片作为输入
//=============================================================
#define LC_VIDEO_FORMAT_VFLIP           (1<<31) // vertical flip mask (图像是否垂直翻转)
    /******************************************************************/

    /******************************************************************/
    /*                       车牌识别处理方式                         */
    /******************************************************************/
#define LC_PLATE_PROC_WAIT_PIC          0   // 以阻塞(等待结果)方式处理图片
#define LC_PLATE_PROC_WAIT_VIDEO        1   // 以阻塞(等待结果)方式处理视频
#define LC_PLATE_PROC_NOWAIT_PIC        2   // 非阻塞(不等待结果)方式处理图片，结果将在回调函数通知
#define LC_PLATE_PROC_NOWAIT_VIDEO      3   // 非阻塞(不等待结果)方式处理视频，结果将在回调函数通知
    /******************************************************************/

    /********************************************************************
     Function:      PLATECALLBACK
     Description:   识别出车牌后调用的回调函数，用于通知用户及时保存识别信息。
                    识别结果通过车牌分析句柄取得。该函数只在采用非阻塞方式时使用。

     Param:
        context     创建时传入参数context
        param       请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    typedef void (*PLATECALLBACK)(uint32_t param, void *context);

    /********************************************************************
     Function:      lc_plate_get_version_date
     Description:   获取车牌识别算法库版本号与日期

     1. Param:
        bVersion    0：获取日期；1：获取版本号
        ver_date    算法库版本号或者日期字符串

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_version_date(uint8_t bVersion, int8_t **ver_date);

    /********************************************************************
     Function:      lc_plate_get_limit_time
     Description:   获取车牌识别算法库使用限制时间

     1. Param
        limit_time_back 车牌识别算法库使用限制时间字符串(如：2012-01-01 00:00:00)

        Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_limit_time(int8_t **limit_time_back);

    /********************************************************************
     Function:      lc_plate_get_limit_thread_num
     Description:   获取车牌识别算法库使用限制路数

     1. Param
        num 车牌识别算法库使用限制路数

        Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_limit_thread_num(uint32_t *num);

    /********************************************************************
     Function:      lc_plate_analysis_create
     Description:   创建车牌分析句柄

     1. Param:
        plate_handle    车牌分析句柄
        proce_type      请查看车牌识别处理方式
        img_w           图像宽度
        img_h           图像高度
        plateCallBack   识别回调函数  (采用阻塞方式时该参数可为NULL)
        context         用户上下文指针(采用阻塞方式时该参数可为NULL)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_analysis_create(Plate_handle *plate_handle, int32_t proce_type, int32_t img_w,
                                             int32_t img_h, PLATECALLBACK plateCallBack, void *context);

    /********************************************************************
    Function:       lc_plate_analysis
     Description:   车牌分析主函数，视频识别中使用

     1. Param:
        plate_handle    车牌分析句柄
        src_img         待分析的视频数据(默认为YUV420格式)
        img_w           图像宽度
        img_h           图像高度
        img_size        图像数据大小(数据格式为LC_VIDEO_FORMAT_JPG或LC_VIDEO_FORMAT_BMP时有效)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_analysis(Plate_handle *plate_handle, uint8_t *src_img, int32_t img_w, int32_t img_h, int32_t img_size);

    /********************************************************************
     Function:      lc_plate_analysis_destroy
     Description:   撤销车牌分析句柄

     1. Param:
        plate_handle    车牌分析句柄

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_analysis_destroy(Plate_handle plate_handle);

    /********************************************************************
     Function:      lc_plate_set_plate_width
     Description:   设置检测车牌的最小宽度和最大宽度(单位为像素)

     1. Param:
        plate_handle    车牌分析句柄
        width_min       车牌最小宽度(默认值为60)
        width_max       车牌最大宽度(默认值为180)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_plate_width(Plate_handle plate_handle, int32_t width_min, int32_t width_max);
    DLL_API int32_t lc_plate_get_plate_width(Plate_handle plate_handle, int32_t *width_min, int32_t *width_max);

    /********************************************************************
     Function:      lc_plate_set_rect_detect
     Description:   设置车牌分析的矩形区域

     1. Param:
        plate_handle    车牌分析句柄
        rect            车牌分析矩形区域(默认去除上下各十分之一)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_rect_detect(Plate_handle plate_handle, Rects rect);
    DLL_API int32_t lc_plate_get_rect_detect(Plate_handle plate_handle, Rects *rect);

    /********************************************************************
     Function:      lc_plate_set_chinese_mask
     Description:   设置汉字(省份)识别掩膜(提高字符识别的精度) 0: 开启该汉字识别 1: 屏蔽该汉字识别

    uint8_t chinese_mask[] =
    {
    //  云 京 冀 吉 宁 川 新 晋 桂 沪 津 浙 渝 湘 琼 甘 皖 粤 苏 蒙 藏 豫 贵 赣 辽 鄂 闽 陕 青 鲁 黑 (省份)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

     1. Param:
        plate_handle    车牌分析句柄
        mask            车牌识别掩膜(默认全部汉字开启识别)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_chinese_mask(Plate_handle plate_handle, uint8_t *mask);
    DLL_API int32_t lc_plate_get_chinese_mask(Plate_handle plate_handle, uint8_t *mask);

    /********************************************************************
     Function:      lc_plate_set_chinese_default
     Description:   设置默认省份汉字

      省份汉字：云 京 冀 吉 宁 川 新 晋 桂 沪 津 浙 渝 湘 琼 甘 皖 粤 苏 蒙 藏 豫 贵 赣 辽 鄂 闽 陕 青 鲁 黑

     1. Param:
        plate_handle    车牌分析句柄
        chinese_default 省份汉字

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_chinese_default(Plate_handle plate_handle, char *chinese_default);
    DLL_API int32_t lc_plate_get_chinese_default(Plate_handle plate_handle, char *chinese_default);

    /********************************************************************
     Function:      lc_plate_set_chinese_trust_thresh
     Description:   设置置信度阈值，当汉字识别的置信度低于设置的阈值时采用默认省份汉字

     1. Param:
        plate_handle    车牌分析句柄
        trust_thresh    汉字的置信度阈值

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_chinese_trust_thresh(Plate_handle plate_handle, uint8_t trust_thresh);
    DLL_API int32_t lc_plate_get_chinese_trust_thresh(Plate_handle plate_handle, uint8_t *trust_thresh);

    /********************************************************************
     Function:      lc_plate_set_car_color_enable
     Description:   设置车身颜色识别标志

     1. Param:
        plate_handle    车牌分析句柄
        bEnable         车身颜色识别标志，0:不识别，1:识别

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_car_color_enable(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_car_color_enable(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_car_window_enable
     Description:   设置车窗区域识别标志

     1. Param:
        plate_handle    车牌分析句柄
        bEnable         车窗区域识别标志，0:不识别，1:识别

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_car_window_enable(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_car_window_enable(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_video_format
     Description:   设置待分析的视频流或者图片的格式

     1. Param:
        plate_handle    车牌分析句柄
        format          输入视频类型(请参看常见视频类型列表的定义)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_video_format(Plate_handle plate_handle, int32_t format);
    DLL_API int32_t lc_plate_get_video_format(Plate_handle plate_handle, int32_t *format);

    /********************************************************************
     Function:      lc_plate_set_position_flag
     Description:   设置抓拍的车牌图片上是否画出车牌的位置

     1. Param:
        plate_handle    车牌分析句柄
        bEnable         是否画出车牌的位置，0:不叠加(默认)，1:叠加

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_position_flag(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_position_flag(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_OSD_flag
     Description:   设置抓拍的车牌图片上是否叠加识别出的车牌字符

     1. Param:
        plate_handle    车牌分析句柄
        bEnable         是否叠加识别出的车牌字符，0:不叠加(默认)，1:叠加

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_OSD_flag(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_OSD_flag(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_OSD_scale
     Description:   设置抓拍的车牌图片上叠加的车牌字符大小(默认宽度为100个像素)

     1. Param:
        plate_handle    车牌分析句柄
        scale           叠加的车牌字符的显示比例(scale >= 0.5 && scale <= 2.0)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_OSD_scale(Plate_handle plate_handle, double scale);
    DLL_API int32_t lc_plate_get_OSD_scale(Plate_handle plate_handle, double *scale);

    /********************************************************************
     Function:      lc_plate_set_OSD_position
     Description:   设置抓拍的车牌图片上叠加车牌字符的左上角位置

     1. Param:
        plate_handle    车牌分析句柄
        x               叠加车牌字符的左上角X坐标(默认为0)
        y               叠加车牌字符的左上角Y坐标(默认为0)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_OSD_position(Plate_handle plate_handle, int32_t x, int32_t y);
    DLL_API int32_t lc_plate_get_OSD_position(Plate_handle plate_handle, int32_t *x, int32_t *y);

    /********************************************************************
     Function:      lc_plate_set_deskew_flag
     Description:   设置是否对车牌进行倾斜校正

     1. Param:
        plate_handle    车牌分析句柄
        bEnable         车牌倾斜校正使能标志 0:不校正，1:校正

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_deskew_flag(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_deskew_flag(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_image_quality
     Description:   设置抓拍车牌图片(JPG)的压缩质量

     1. Param:
        plate_handle    车牌分析句柄
        quality         图片(JPG)的压缩质量，1 ~ 100，值越大，质量越好、图片的文件也越大

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_set_image_quality(Plate_handle plate_handle, int32_t quality);
    DLL_API int32_t lc_plate_get_image_quality(Plate_handle plate_handle, int32_t *quality);

    /********************************************************************
     Function:      lc_plate_get_current_plate_number
     Description:   获取当前帧检测到车牌的数目(用于阻塞方式中得到当前帧的信息)

     1. Param:
        plate_handle    车牌分析句柄
        current_num     当前帧检测到车牌的数目

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_current_plate_number(Plate_handle plate_handle, int32_t *current_num);

    /********************************************************************
     Function:      lc_plate_get_current_plate_position
     Description:   获取当前帧检测到车牌的位置(用于阻塞方式中得到当前帧的信息)

     1. Param:
        plate_handle        车牌分析句柄
        index               表示车牌序号，从0开始
        current_position    车牌位置

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_current_plate_position(Plate_handle plate_handle, int32_t index, Rects *current_position);

    /********************************************************************
     Function:      lc_plate_get_plate_number
     Description:   获取成功识别车牌的数目

     1. Param:
        plate_handle    车牌分析句柄
        plate_num       成功识别车牌的数目

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_number(Plate_handle plate_handle, int32_t *plate_num);

    /********************************************************************
     Function:      lc_plate_get_plate_position
     Description:   获取已成功识别车牌的车牌位置(在抓拍的车牌图片中的位置)
                    注意：抓拍的车牌图片一般不是当前帧的图片

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_position  车牌位置

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_position(Plate_handle plate_handle, int32_t index, Rects *plate_position);

    /********************************************************************
     Function:      lc_plate_get_plate_picture_size
     Description:   获取抓拍的车牌图片的数据长度(单位字节byte)

     1. Param:
        plate_handle    车牌分析句柄
        pic_len         抓拍图片的数据长度

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_picture_size(Plate_handle plate_handle, int32_t *pic_len);

    /********************************************************************
     Function:      lc_plate_get_plate_picture
     Description:   获取抓拍的车牌图片。函数返回后，buffsize的值为图像
                    数据的长度；成功则pbuff中有数据，失败则无数据(一般是缓存过小)

     1. Param:
        plate_handle    车牌分析句柄
        pbuff           用于存放图像数据的缓存(JPG格式)
        buffsize        缓存的大小(缓存最大为：image_w*image_h*3/2)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_picture(Plate_handle plate_handle, uint8_t *pbuff, int32_t *buffsize);

    /********************************************************************
     Function:      lc_plate_get_plate_picture_pointer
     Description:   获取抓拍的车牌图片的数据指针(起始地址)

     1. Param:
        plate_handle    车牌分析句柄
        pic_pointer     抓拍图片的数据指针

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_picture_pointer(Plate_handle plate_handle, uint8_t **pic_pointer);

    /********************************************************************
     Function:      lc_plate_get_small_picture_size
     Description:   获取抓拍的车牌区域图片的数据长度(单位字节byte)

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        small_pic_len   抓拍的车牌区域图片的数据长度

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_small_picture_size(Plate_handle plate_handle, int32_t index, int32_t *small_pic_len);

    /********************************************************************
     Function:      lc_plate_get_small_picture
     Description:   获取抓拍的车牌区域图片，函数返回后，buffsize的值为图像数据的长度；
                    成功则pbuff中有数据，失败则无数据(一般是缓存过小)。

     1. Param:
        plate_handle    车牌分析句柄
        pbuff           用于存放图像数据的缓存(JPG格式)
        buffsize        表示缓存的大小(缓存最大为：image_w*image_h*3/2)
        index           表示车牌序号，从0开始

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_small_picture(Plate_handle plate_handle, uint8_t *pbuff,
                                               int32_t *buffsize, int32_t index);

    /********************************************************************
     Function:      lc_plate_get_small_picture_pointer
     Description:   获取抓拍的车牌区域图片的数据指针(起始地址)

     1. Param:
        plate_handle        车牌分析句柄
        index               表示车牌序号，从0开始
        small_pic_pointer   抓拍的车牌区域图片的数据指针

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_small_picture_pointer(Plate_handle plate_handle, int32_t index, uint8_t **small_pic_pointer);

    /********************************************************************
     Function:      lc_plate_get_window_position
     Description:   获取已成功识别车牌的车辆车窗区域位置(在抓拍的图片中的位置)
                    注意：抓拍的图片一般不是当前帧的图片

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        window_position 车窗区域位置

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_position(Plate_handle plate_handle, int32_t index, Rects *window_position);

    /********************************************************************
     Function:      lc_plate_get_window_picture_size
     Description:   获取抓拍的车窗区域图片的数据长度(单位字节byte)

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        window_pic_len  车窗区域图片的数据长度

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_picture_size(Plate_handle plate_handle, int32_t index, int32_t *window_pic_len);

    /********************************************************************
     Function:      lc_plate_get_window_picture
     Description:   获取抓拍的车窗区域图片，函数返回后，buffsize的值为图像数据的长度；
                    成功则pbuff中有数据，失败则无数据(一般是缓存过小)。

     1. Param:
        plate_handle    车牌分析句柄
        pbuff           用于存放图像数据的缓存(JPG格式)
        buffsize        表示缓存的大小(缓存最大为：image_w*image_h*3/2)
        index           表示车牌序号，从0开始

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_picture(Plate_handle plate_handle, uint8_t *pbuff,
                                                int32_t *buffsize, int32_t index);

    /********************************************************************
     Function:      lc_plate_get_window_picture_pointer
     Description:   获取抓拍的车窗区域图片的数据指针(起始地址)

     1. Param:
        plate_handle        车牌分析句柄
        index               表示车牌序号，从0开始
        window_pic_pointer  车窗区域图片的数据指针

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_picture_pointer(Plate_handle plate_handle, int32_t index, uint8_t **window_pic_pointer);

    /********************************************************************
     Function:      lc_plate_get_plate_character_number
     Description:   获取已成功识别车牌的字符数目

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        ch_num          车牌字符数目

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_character_number(Plate_handle plate_handle, int32_t index, uint8_t *ch_num);

    /********************************************************************
     Function:      lc_plate_get_plate_name
     Description:   获取已成功识别车牌的车牌号字符串

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_name      车牌号字符串

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_name(Plate_handle plate_handle, int32_t index, int8_t **plate_name);

    /********************************************************************
     Function:      lc_plate_get_plate_color_id
     Description:   获取已成功识别车牌的颜色代码

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_color_id  车牌颜色代码(0:蓝底 1:黄底 2:白底－军牌 3:白底－警牌 4:黑牌 5:红牌 6:绿牌 99:未知)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_color_id(Plate_handle plate_handle, int32_t index, uint8_t *plate_color_id);

    /********************************************************************
     Function:      lc_plate_get_plate_color_name
     Description:   获取已成功识别车牌的颜色字符串

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_color     车牌颜色字符串

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_color_name(Plate_handle plate_handle, int32_t index, int8_t **plate_color);

    /********************************************************************
     Function:      lc_plate_get_plate_reliability
     Description:   获取已成功识别车牌的置信度(置信度越高，识别结果越可信)

     1. Param:
        plate_handle        车牌分析句柄
        index               表示车牌序号，从0开始
        plate_reliability   车牌置信度(范围：1－100)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_reliability(Plate_handle plate_handle, int32_t index, uint8_t *plate_reliability);

    /********************************************************************
     Function:      lc_plate_get_character_reliability
     Description:   获取已成功识别车牌的单个字符置信度(置信度越高，识别结果越可信)

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        ch_num          表示字符序号，从0开始
        ch_reliability  单个字符置信度(范围: 1-1001)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_character_reliability(Plate_handle plate_handle, int32_t index, int32_t ch_num, uint8_t *ch_reliability);

    /********************************************************************
     Function:      lc_plate_get_plate_count
     Description:   获取已成功识别车牌的计数值(同一车牌连续出现的次数)

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_cnt       车牌计数值(同一车牌连续出现的次数)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_count(Plate_handle plate_handle, int32_t index, int32_t *plate_cnt);

    /********************************************************************
     Function:      lc_plate_get_plate_brightness
     Description:   获取已成功识别车牌的亮度值

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        brightness      车牌亮度值

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_brightness(Plate_handle plate_handle, int32_t index, uint8_t *brightness);

    /********************************************************************
     Function:      lc_plate_get_plate_type_id
     Description:   获取已成功识别车牌的类型代码

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_type_id   车牌类型代码(1:大型汽车号牌 2:小型汽车号牌 16:教练汽车号牌 23:警用汽车号牌
                        25:军用汽车号牌 26:境外汽车号牌 27: 境外商用汽车号牌 28: 农用汽车号牌 99:未知)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_type_id(Plate_handle plate_handle, int32_t index, uint8_t *plate_type_id);

    /********************************************************************
     Function:      lc_plate_get_plate_type_name
     Description:   获取已成功识别车牌的类型字符串

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        plate_type      车牌类型字符串

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_type_name(Plate_handle plate_handle, int32_t index, int8_t **plate_type);

    /********************************************************************
     Function:      lc_plate_get_car_speed
     Description:   获取已成功识别车牌的车辆速度

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        car_speed       车辆速度

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_speed(Plate_handle plate_handle, int32_t index, int32_t *car_speed);

    /********************************************************************
     Function:      lc_plate_get_car_color_id
     Description:   获取已成功识别车牌的车身颜色代码

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        car_color_id    车身颜色代码(0:白色 1:银灰 2:黄色 3:粉色 4:红色 5:紫色 6:绿色 7:蓝色 8:棕色 9:黑色 99:未知 255:未识别)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_color_id(Plate_handle plate_handle, int32_t index, uint8_t *car_color_id);

    /*********************************************************************
     Function:      lc_plate_get_car_color_reliability
     Description:   获取已成功识别车身颜色的置信度

     1. Param:
        plate_handle            车牌分析句柄
        index                   表示车牌序号，从0开始
        car_color_reliability   车身颜色置信度(范围：1－100)

      2. Return
        请查看车牌识别函数常见返回值类型的定义
    **********************************************************************/
    DLL_API int32_t lc_plate_get_car_color_reliability(Plate_handle plate_handle, int32_t index, uint8_t *car_color_reliability);

    /********************************************************************
     Function:      lc_plate_get_car_color_name
     Description:   获取已成功识别车牌的车身颜色字符串

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        car_color       车身颜色字符串

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_color_name(Plate_handle plate_handle, int32_t index, int8_t **car_color);

    /********************************************************************
     Function:      lc_plate_get_car_direction
     Description:   获取已成功识别车牌的车辆运动方向

     1. Param:
        plate_handle    车牌分析句柄
        index           表示车牌序号，从0开始
        car_direction   车辆运动方向(0:未知方向 1:车牌从上到下运动 2:表示车牌从下到上运动)

     2. Return:
        请查看车牌识别函数常见返回值类型的定义
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_direction(Plate_handle plate_handle, int32_t index, uint8_t *car_direction);


#ifdef __cplusplus
};
#endif

#endif  // _LC_PLATE_ANALYSIS_H_
