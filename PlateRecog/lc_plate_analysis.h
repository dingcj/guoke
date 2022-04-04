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

    typedef void* Plate_handle; // ���Ʒ������

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
    /*                ����ʶ������������ֵ����                      */
    /******************************************************************/
#define LC_PLATE_SUCCESS                0                         // �����ɹ�
#define LC_PLATE_FAIL                   (LC_PLATE_SUCCESS + 100)  // ����ʧ��
#define LC_PLATE_ERR_INVALID_HANDLE     (LC_PLATE_SUCCESS + 101)  // ��������Ч
#define LC_PLATE_ERR_CREATE_HANDLE      (LC_PLATE_SUCCESS + 102)  // �������ʧ��
#define LC_PLATE_ERR_THREADNUM_LIMIT    (LC_PLATE_SUCCESS + 103)  // �߳���Ŀ����
#define LC_PLATE_ERR_TIME_LIMIT         (LC_PLATE_SUCCESS + 104)  // ʹ��ʱ������
#define LC_PLATE_ERR_THREAD_CHECK       (LC_PLATE_SUCCESS + 105)  // �̼߳��ʧ��
#define LC_PLATE_ERR_BUFF_OVERFLOW      (LC_PLATE_SUCCESS + 106)  // ͼ�񻺴����
#define LC_PLATE_ERR_DOG_CHECK          (LC_PLATE_SUCCESS + 107)  // ���ܹ����ʧ��
#define LC_PLATE_ERR_PARAM              (LC_PLATE_SUCCESS + 108)  // �����������
#define LC_PLATE_ERR_VIDEO_FORMAT       (LC_PLATE_SUCCESS + 109)  // �����ļ����ݸ�ʽ��֧��
#define LC_PLATE_ERR_VIDEO_SIZE         (LC_PLATE_SUCCESS + 110)  // ͼ���С�����п�����ͼ��Ŀ�ȡ��߶Ȳ�����16�ı�����
    /******************************************************************/

    /******************************************************************/
    /*                        ������Ƶ���Ͷ���                        */
    /******************************************************************/
// ��Ƶ������ʽ---����֡������(û��ͷ��)��Ϊ����
//=============================================================
#define LC_VIDEO_FORMAT_PLANAR          (1<< 0) // 4:2:0 planar
#define LC_VIDEO_FORMAT_I420            (1<< 1) // 4:2:0 planar (Y U V������˳��)
#define LC_VIDEO_FORMAT_YV12            (1<< 2) // 4:2:0 planar (Y V U������˳��)

#define LC_VIDEO_FORMAT_YUY2            (1<< 3) // 4:2:2 packed
#define LC_VIDEO_FORMAT_YUYV            (1<< 3) // 4:2:2 packed
#define LC_VIDEO_FORMAT_UYVY            (1<< 4) // 4:2:2 packed
#define LC_VIDEO_FORMAT_YVYU            (1<< 5) // 4:2:2 packed

#define LC_VIDEO_FORMAT_BGRA            (1<< 6) // 32-bit BGRA packed
#define LC_VIDEO_FORMAT_ABGR            (1<< 7) // 32-bit ABGR packed
#define LC_VIDEO_FORMAT_RGBA            (1<< 8) // 32-bit RGBA packed
#define LC_VIDEO_FORMAT_ARGB            (1<<15) // 32-bit ARGB packed

#define LC_VIDEO_FORMAT_BGR             (1<< 9) // 24-bit BGR packed (B G R������˳��)
#define LC_VIDEO_FORMAT_BGR24           (1<< 9) // 24-bit BGR packed (B G R������˳��)
#define LC_VIDEO_FORMAT_RGB             (1<<10) // 24-bit RGB packed (R G B������˳��)
#define LC_VIDEO_FORMAT_RGB24           (1<<10) // 24-bit RGB packed (R G B������˳��)

#define LC_VIDEO_FORMAT_RGB555          (1<<11) // 16-bit RGB555 packed (R G B������˳��)
#define LC_VIDEO_FORMAT_RGB565          (1<<12) // 16-bit RGB565 packed (R G B������˳��)
#define LC_VIDEO_FORMAT_BGR555          (1<<13) // 16-bit BGR555 packed (B G R������˳��)
#define LC_VIDEO_FORMAT_BGR565          (1<<14) // 16-bit BGR565 packed (B G R������˳��)
//=============================================================
// ����ͼƬ����ʽ---����֡ͼƬ(����ͷ��)��Ϊ����
//=============================================================
#define LC_VIDEO_FORMAT_JPG             (1<<29) // �Ե�֡JPGͼƬ��Ϊ����
#define LC_VIDEO_FORMAT_BMP             (1<<30) // �Ե�֡BMPͼƬ��Ϊ����
//=============================================================
#define LC_VIDEO_FORMAT_VFLIP           (1<<31) // vertical flip mask (ͼ���Ƿ�ֱ��ת)
    /******************************************************************/

    /******************************************************************/
    /*                       ����ʶ����ʽ                         */
    /******************************************************************/
#define LC_PLATE_PROC_WAIT_PIC          0   // ������(�ȴ����)��ʽ����ͼƬ
#define LC_PLATE_PROC_WAIT_VIDEO        1   // ������(�ȴ����)��ʽ������Ƶ
#define LC_PLATE_PROC_NOWAIT_PIC        2   // ������(���ȴ����)��ʽ����ͼƬ��������ڻص�����֪ͨ
#define LC_PLATE_PROC_NOWAIT_VIDEO      3   // ������(���ȴ����)��ʽ������Ƶ��������ڻص�����֪ͨ
    /******************************************************************/

    /********************************************************************
     Function:      PLATECALLBACK
     Description:   ʶ������ƺ���õĻص�����������֪ͨ�û���ʱ����ʶ����Ϣ��
                    ʶ����ͨ�����Ʒ������ȡ�á��ú���ֻ�ڲ��÷�������ʽʱʹ�á�

     Param:
        context     ����ʱ�������context
        param       ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    typedef void (*PLATECALLBACK)(uint32_t param, void *context);

    /********************************************************************
     Function:      lc_plate_get_version_date
     Description:   ��ȡ����ʶ���㷨��汾��������

     1. Param:
        bVersion    0����ȡ���ڣ�1����ȡ�汾��
        ver_date    �㷨��汾�Ż��������ַ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_version_date(uint8_t bVersion, int8_t **ver_date);

    /********************************************************************
     Function:      lc_plate_get_limit_time
     Description:   ��ȡ����ʶ���㷨��ʹ������ʱ��

     1. Param
        limit_time_back ����ʶ���㷨��ʹ������ʱ���ַ���(�磺2012-01-01 00:00:00)

        Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_limit_time(int8_t **limit_time_back);

    /********************************************************************
     Function:      lc_plate_get_limit_thread_num
     Description:   ��ȡ����ʶ���㷨��ʹ������·��

     1. Param
        num ����ʶ���㷨��ʹ������·��

        Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_limit_thread_num(uint32_t *num);

    /********************************************************************
     Function:      lc_plate_analysis_create
     Description:   �������Ʒ������

     1. Param:
        plate_handle    ���Ʒ������
        proce_type      ��鿴����ʶ����ʽ
        img_w           ͼ����
        img_h           ͼ��߶�
        plateCallBack   ʶ��ص�����  (����������ʽʱ�ò�����ΪNULL)
        context         �û�������ָ��(����������ʽʱ�ò�����ΪNULL)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_analysis_create(Plate_handle *plate_handle, int32_t proce_type, int32_t img_w,
                                             int32_t img_h, PLATECALLBACK plateCallBack, void *context);

    /********************************************************************
    Function:       lc_plate_analysis
     Description:   ���Ʒ�������������Ƶʶ����ʹ��

     1. Param:
        plate_handle    ���Ʒ������
        src_img         ����������Ƶ����(Ĭ��ΪYUV420��ʽ)
        img_w           ͼ����
        img_h           ͼ��߶�
        img_size        ͼ�����ݴ�С(���ݸ�ʽΪLC_VIDEO_FORMAT_JPG��LC_VIDEO_FORMAT_BMPʱ��Ч)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_analysis(Plate_handle *plate_handle, uint8_t *src_img, int32_t img_w, int32_t img_h, int32_t img_size);

    /********************************************************************
     Function:      lc_plate_analysis_destroy
     Description:   �������Ʒ������

     1. Param:
        plate_handle    ���Ʒ������

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_analysis_destroy(Plate_handle plate_handle);

    /********************************************************************
     Function:      lc_plate_set_plate_width
     Description:   ���ü�⳵�Ƶ���С��Ⱥ������(��λΪ����)

     1. Param:
        plate_handle    ���Ʒ������
        width_min       ������С���(Ĭ��ֵΪ60)
        width_max       ���������(Ĭ��ֵΪ180)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_plate_width(Plate_handle plate_handle, int32_t width_min, int32_t width_max);
    DLL_API int32_t lc_plate_get_plate_width(Plate_handle plate_handle, int32_t *width_min, int32_t *width_max);

    /********************************************************************
     Function:      lc_plate_set_rect_detect
     Description:   ���ó��Ʒ����ľ�������

     1. Param:
        plate_handle    ���Ʒ������
        rect            ���Ʒ�����������(Ĭ��ȥ�����¸�ʮ��֮һ)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_rect_detect(Plate_handle plate_handle, Rects rect);
    DLL_API int32_t lc_plate_get_rect_detect(Plate_handle plate_handle, Rects *rect);

    /********************************************************************
     Function:      lc_plate_set_chinese_mask
     Description:   ���ú���(ʡ��)ʶ����Ĥ(����ַ�ʶ��ľ���) 0: �����ú���ʶ�� 1: ���θú���ʶ��

    uint8_t chinese_mask[] =
    {
    //  �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� ԥ �� �� �� �� �� �� �� ³ �� (ʡ��)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

     1. Param:
        plate_handle    ���Ʒ������
        mask            ����ʶ����Ĥ(Ĭ��ȫ�����ֿ���ʶ��)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_chinese_mask(Plate_handle plate_handle, uint8_t *mask);
    DLL_API int32_t lc_plate_get_chinese_mask(Plate_handle plate_handle, uint8_t *mask);

    /********************************************************************
     Function:      lc_plate_set_chinese_default
     Description:   ����Ĭ��ʡ�ݺ���

      ʡ�ݺ��֣��� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� �� ԥ �� �� �� �� �� �� �� ³ ��

     1. Param:
        plate_handle    ���Ʒ������
        chinese_default ʡ�ݺ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_chinese_default(Plate_handle plate_handle, char *chinese_default);
    DLL_API int32_t lc_plate_get_chinese_default(Plate_handle plate_handle, char *chinese_default);

    /********************************************************************
     Function:      lc_plate_set_chinese_trust_thresh
     Description:   �������Ŷ���ֵ��������ʶ������Ŷȵ������õ���ֵʱ����Ĭ��ʡ�ݺ���

     1. Param:
        plate_handle    ���Ʒ������
        trust_thresh    ���ֵ����Ŷ���ֵ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_chinese_trust_thresh(Plate_handle plate_handle, uint8_t trust_thresh);
    DLL_API int32_t lc_plate_get_chinese_trust_thresh(Plate_handle plate_handle, uint8_t *trust_thresh);

    /********************************************************************
     Function:      lc_plate_set_car_color_enable
     Description:   ���ó�����ɫʶ���־

     1. Param:
        plate_handle    ���Ʒ������
        bEnable         ������ɫʶ���־��0:��ʶ��1:ʶ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_car_color_enable(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_car_color_enable(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_car_window_enable
     Description:   ���ó�������ʶ���־

     1. Param:
        plate_handle    ���Ʒ������
        bEnable         ��������ʶ���־��0:��ʶ��1:ʶ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_car_window_enable(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_car_window_enable(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_video_format
     Description:   ���ô���������Ƶ������ͼƬ�ĸ�ʽ

     1. Param:
        plate_handle    ���Ʒ������
        format          ������Ƶ����(��ο�������Ƶ�����б�Ķ���)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_video_format(Plate_handle plate_handle, int32_t format);
    DLL_API int32_t lc_plate_get_video_format(Plate_handle plate_handle, int32_t *format);

    /********************************************************************
     Function:      lc_plate_set_position_flag
     Description:   ����ץ�ĵĳ���ͼƬ���Ƿ񻭳����Ƶ�λ��

     1. Param:
        plate_handle    ���Ʒ������
        bEnable         �Ƿ񻭳����Ƶ�λ�ã�0:������(Ĭ��)��1:����

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_position_flag(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_position_flag(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_OSD_flag
     Description:   ����ץ�ĵĳ���ͼƬ���Ƿ����ʶ����ĳ����ַ�

     1. Param:
        plate_handle    ���Ʒ������
        bEnable         �Ƿ����ʶ����ĳ����ַ���0:������(Ĭ��)��1:����

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_OSD_flag(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_OSD_flag(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_OSD_scale
     Description:   ����ץ�ĵĳ���ͼƬ�ϵ��ӵĳ����ַ���С(Ĭ�Ͽ��Ϊ100������)

     1. Param:
        plate_handle    ���Ʒ������
        scale           ���ӵĳ����ַ�����ʾ����(scale >= 0.5 && scale <= 2.0)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_OSD_scale(Plate_handle plate_handle, double scale);
    DLL_API int32_t lc_plate_get_OSD_scale(Plate_handle plate_handle, double *scale);

    /********************************************************************
     Function:      lc_plate_set_OSD_position
     Description:   ����ץ�ĵĳ���ͼƬ�ϵ��ӳ����ַ������Ͻ�λ��

     1. Param:
        plate_handle    ���Ʒ������
        x               ���ӳ����ַ������Ͻ�X����(Ĭ��Ϊ0)
        y               ���ӳ����ַ������Ͻ�Y����(Ĭ��Ϊ0)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_OSD_position(Plate_handle plate_handle, int32_t x, int32_t y);
    DLL_API int32_t lc_plate_get_OSD_position(Plate_handle plate_handle, int32_t *x, int32_t *y);

    /********************************************************************
     Function:      lc_plate_set_deskew_flag
     Description:   �����Ƿ�Գ��ƽ�����бУ��

     1. Param:
        plate_handle    ���Ʒ������
        bEnable         ������бУ��ʹ�ܱ�־ 0:��У����1:У��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_deskew_flag(Plate_handle plate_handle, uint8_t bEnable);
    DLL_API int32_t lc_plate_get_deskew_flag(Plate_handle plate_handle, uint8_t *bEnable);

    /********************************************************************
     Function:      lc_plate_set_image_quality
     Description:   ����ץ�ĳ���ͼƬ(JPG)��ѹ������

     1. Param:
        plate_handle    ���Ʒ������
        quality         ͼƬ(JPG)��ѹ��������1 ~ 100��ֵԽ������Խ�á�ͼƬ���ļ�ҲԽ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_set_image_quality(Plate_handle plate_handle, int32_t quality);
    DLL_API int32_t lc_plate_get_image_quality(Plate_handle plate_handle, int32_t *quality);

    /********************************************************************
     Function:      lc_plate_get_current_plate_number
     Description:   ��ȡ��ǰ֡��⵽���Ƶ���Ŀ(����������ʽ�еõ���ǰ֡����Ϣ)

     1. Param:
        plate_handle    ���Ʒ������
        current_num     ��ǰ֡��⵽���Ƶ���Ŀ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_current_plate_number(Plate_handle plate_handle, int32_t *current_num);

    /********************************************************************
     Function:      lc_plate_get_current_plate_position
     Description:   ��ȡ��ǰ֡��⵽���Ƶ�λ��(����������ʽ�еõ���ǰ֡����Ϣ)

     1. Param:
        plate_handle        ���Ʒ������
        index               ��ʾ������ţ���0��ʼ
        current_position    ����λ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_current_plate_position(Plate_handle plate_handle, int32_t index, Rects *current_position);

    /********************************************************************
     Function:      lc_plate_get_plate_number
     Description:   ��ȡ�ɹ�ʶ���Ƶ���Ŀ

     1. Param:
        plate_handle    ���Ʒ������
        plate_num       �ɹ�ʶ���Ƶ���Ŀ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_number(Plate_handle plate_handle, int32_t *plate_num);

    /********************************************************************
     Function:      lc_plate_get_plate_position
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ���λ��(��ץ�ĵĳ���ͼƬ�е�λ��)
                    ע�⣺ץ�ĵĳ���ͼƬһ�㲻�ǵ�ǰ֡��ͼƬ

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_position  ����λ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_position(Plate_handle plate_handle, int32_t index, Rects *plate_position);

    /********************************************************************
     Function:      lc_plate_get_plate_picture_size
     Description:   ��ȡץ�ĵĳ���ͼƬ�����ݳ���(��λ�ֽ�byte)

     1. Param:
        plate_handle    ���Ʒ������
        pic_len         ץ��ͼƬ�����ݳ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_picture_size(Plate_handle plate_handle, int32_t *pic_len);

    /********************************************************************
     Function:      lc_plate_get_plate_picture
     Description:   ��ȡץ�ĵĳ���ͼƬ���������غ�buffsize��ֵΪͼ��
                    ���ݵĳ��ȣ��ɹ���pbuff�������ݣ�ʧ����������(һ���ǻ����С)

     1. Param:
        plate_handle    ���Ʒ������
        pbuff           ���ڴ��ͼ�����ݵĻ���(JPG��ʽ)
        buffsize        ����Ĵ�С(�������Ϊ��image_w*image_h*3/2)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_picture(Plate_handle plate_handle, uint8_t *pbuff, int32_t *buffsize);

    /********************************************************************
     Function:      lc_plate_get_plate_picture_pointer
     Description:   ��ȡץ�ĵĳ���ͼƬ������ָ��(��ʼ��ַ)

     1. Param:
        plate_handle    ���Ʒ������
        pic_pointer     ץ��ͼƬ������ָ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_picture_pointer(Plate_handle plate_handle, uint8_t **pic_pointer);

    /********************************************************************
     Function:      lc_plate_get_small_picture_size
     Description:   ��ȡץ�ĵĳ�������ͼƬ�����ݳ���(��λ�ֽ�byte)

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        small_pic_len   ץ�ĵĳ�������ͼƬ�����ݳ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_small_picture_size(Plate_handle plate_handle, int32_t index, int32_t *small_pic_len);

    /********************************************************************
     Function:      lc_plate_get_small_picture
     Description:   ��ȡץ�ĵĳ�������ͼƬ���������غ�buffsize��ֵΪͼ�����ݵĳ��ȣ�
                    �ɹ���pbuff�������ݣ�ʧ����������(һ���ǻ����С)��

     1. Param:
        plate_handle    ���Ʒ������
        pbuff           ���ڴ��ͼ�����ݵĻ���(JPG��ʽ)
        buffsize        ��ʾ����Ĵ�С(�������Ϊ��image_w*image_h*3/2)
        index           ��ʾ������ţ���0��ʼ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_small_picture(Plate_handle plate_handle, uint8_t *pbuff,
                                               int32_t *buffsize, int32_t index);

    /********************************************************************
     Function:      lc_plate_get_small_picture_pointer
     Description:   ��ȡץ�ĵĳ�������ͼƬ������ָ��(��ʼ��ַ)

     1. Param:
        plate_handle        ���Ʒ������
        index               ��ʾ������ţ���0��ʼ
        small_pic_pointer   ץ�ĵĳ�������ͼƬ������ָ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_small_picture_pointer(Plate_handle plate_handle, int32_t index, uint8_t **small_pic_pointer);

    /********************************************************************
     Function:      lc_plate_get_window_position
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ�����������λ��(��ץ�ĵ�ͼƬ�е�λ��)
                    ע�⣺ץ�ĵ�ͼƬһ�㲻�ǵ�ǰ֡��ͼƬ

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        window_position ��������λ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_position(Plate_handle plate_handle, int32_t index, Rects *window_position);

    /********************************************************************
     Function:      lc_plate_get_window_picture_size
     Description:   ��ȡץ�ĵĳ�������ͼƬ�����ݳ���(��λ�ֽ�byte)

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        window_pic_len  ��������ͼƬ�����ݳ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_picture_size(Plate_handle plate_handle, int32_t index, int32_t *window_pic_len);

    /********************************************************************
     Function:      lc_plate_get_window_picture
     Description:   ��ȡץ�ĵĳ�������ͼƬ���������غ�buffsize��ֵΪͼ�����ݵĳ��ȣ�
                    �ɹ���pbuff�������ݣ�ʧ����������(һ���ǻ����С)��

     1. Param:
        plate_handle    ���Ʒ������
        pbuff           ���ڴ��ͼ�����ݵĻ���(JPG��ʽ)
        buffsize        ��ʾ����Ĵ�С(�������Ϊ��image_w*image_h*3/2)
        index           ��ʾ������ţ���0��ʼ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_picture(Plate_handle plate_handle, uint8_t *pbuff,
                                                int32_t *buffsize, int32_t index);

    /********************************************************************
     Function:      lc_plate_get_window_picture_pointer
     Description:   ��ȡץ�ĵĳ�������ͼƬ������ָ��(��ʼ��ַ)

     1. Param:
        plate_handle        ���Ʒ������
        index               ��ʾ������ţ���0��ʼ
        window_pic_pointer  ��������ͼƬ������ָ��

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_window_picture_pointer(Plate_handle plate_handle, int32_t index, uint8_t **window_pic_pointer);

    /********************************************************************
     Function:      lc_plate_get_plate_character_number
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ��ַ���Ŀ

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        ch_num          �����ַ���Ŀ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_character_number(Plate_handle plate_handle, int32_t index, uint8_t *ch_num);

    /********************************************************************
     Function:      lc_plate_get_plate_name
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ��ƺ��ַ���

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_name      ���ƺ��ַ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_name(Plate_handle plate_handle, int32_t index, int8_t **plate_name);

    /********************************************************************
     Function:      lc_plate_get_plate_color_id
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ���ɫ����

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_color_id  ������ɫ����(0:���� 1:�Ƶ� 2:�׵ף����� 3:�׵ף����� 4:���� 5:���� 6:���� 99:δ֪)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_color_id(Plate_handle plate_handle, int32_t index, uint8_t *plate_color_id);

    /********************************************************************
     Function:      lc_plate_get_plate_color_name
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ���ɫ�ַ���

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_color     ������ɫ�ַ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_color_name(Plate_handle plate_handle, int32_t index, int8_t **plate_color);

    /********************************************************************
     Function:      lc_plate_get_plate_reliability
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ����Ŷ�(���Ŷ�Խ�ߣ�ʶ����Խ����)

     1. Param:
        plate_handle        ���Ʒ������
        index               ��ʾ������ţ���0��ʼ
        plate_reliability   �������Ŷ�(��Χ��1��100)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_reliability(Plate_handle plate_handle, int32_t index, uint8_t *plate_reliability);

    /********************************************************************
     Function:      lc_plate_get_character_reliability
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĵ����ַ����Ŷ�(���Ŷ�Խ�ߣ�ʶ����Խ����)

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        ch_num          ��ʾ�ַ���ţ���0��ʼ
        ch_reliability  �����ַ����Ŷ�(��Χ: 1-1001)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_character_reliability(Plate_handle plate_handle, int32_t index, int32_t ch_num, uint8_t *ch_reliability);

    /********************************************************************
     Function:      lc_plate_get_plate_count
     Description:   ��ȡ�ѳɹ�ʶ���Ƶļ���ֵ(ͬһ�����������ֵĴ���)

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_cnt       ���Ƽ���ֵ(ͬһ�����������ֵĴ���)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_count(Plate_handle plate_handle, int32_t index, int32_t *plate_cnt);

    /********************************************************************
     Function:      lc_plate_get_plate_brightness
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ�����ֵ

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        brightness      ��������ֵ

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_brightness(Plate_handle plate_handle, int32_t index, uint8_t *brightness);

    /********************************************************************
     Function:      lc_plate_get_plate_type_id
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ����ʹ���

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_type_id   �������ʹ���(1:������������ 2:С���������� 16:������������ 23:������������
                        25:������������ 26:������������ 27: ���������������� 28: ũ���������� 99:δ֪)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_type_id(Plate_handle plate_handle, int32_t index, uint8_t *plate_type_id);

    /********************************************************************
     Function:      lc_plate_get_plate_type_name
     Description:   ��ȡ�ѳɹ�ʶ���Ƶ������ַ���

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        plate_type      ���������ַ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_plate_type_name(Plate_handle plate_handle, int32_t index, int8_t **plate_type);

    /********************************************************************
     Function:      lc_plate_get_car_speed
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ����ٶ�

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        car_speed       �����ٶ�

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_speed(Plate_handle plate_handle, int32_t index, int32_t *car_speed);

    /********************************************************************
     Function:      lc_plate_get_car_color_id
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ�����ɫ����

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        car_color_id    ������ɫ����(0:��ɫ 1:���� 2:��ɫ 3:��ɫ 4:��ɫ 5:��ɫ 6:��ɫ 7:��ɫ 8:��ɫ 9:��ɫ 99:δ֪ 255:δʶ��)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_color_id(Plate_handle plate_handle, int32_t index, uint8_t *car_color_id);

    /*********************************************************************
     Function:      lc_plate_get_car_color_reliability
     Description:   ��ȡ�ѳɹ�ʶ������ɫ�����Ŷ�

     1. Param:
        plate_handle            ���Ʒ������
        index                   ��ʾ������ţ���0��ʼ
        car_color_reliability   ������ɫ���Ŷ�(��Χ��1��100)

      2. Return
        ��鿴����ʶ������������ֵ���͵Ķ���
    **********************************************************************/
    DLL_API int32_t lc_plate_get_car_color_reliability(Plate_handle plate_handle, int32_t index, uint8_t *car_color_reliability);

    /********************************************************************
     Function:      lc_plate_get_car_color_name
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ�����ɫ�ַ���

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        car_color       ������ɫ�ַ���

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_color_name(Plate_handle plate_handle, int32_t index, int8_t **car_color);

    /********************************************************************
     Function:      lc_plate_get_car_direction
     Description:   ��ȡ�ѳɹ�ʶ���Ƶĳ����˶�����

     1. Param:
        plate_handle    ���Ʒ������
        index           ��ʾ������ţ���0��ʼ
        car_direction   �����˶�����(0:δ֪���� 1:���ƴ��ϵ����˶� 2:��ʾ���ƴ��µ����˶�)

     2. Return:
        ��鿴����ʶ������������ֵ���͵Ķ���
    ********************************************************************/
    DLL_API int32_t lc_plate_get_car_direction(Plate_handle plate_handle, int32_t index, uint8_t *car_direction);


#ifdef __cplusplus
};
#endif

#endif  // _LC_PLATE_ANALYSIS_H_
