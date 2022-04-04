#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "malloc_aligned.h"
#include "time_test.h"
//#include "libjpeg_turbo_interface.h"

#ifdef WIN32
#define DLL_API _declspec(dllexport)
#else
#define DLL_API
#endif

#ifdef PALTE_THREADS
#include <windows.h>
#include <process.h>
#elif PLATE_KEY
#include <windows.h>
#include <process.h>
#endif

#ifdef WIN32
//#include <assert.h>
//#include "./color_space_src/yuv2jpg.h"
#include "./color_space_src/rgb2yuv.h"
#include "./color_space_src/convert.h"
#include "bmp2rgb/bmp2rgb.h"
//#include "vld.h"
#endif
#include "draw_debug_src/draw.h"
#include "lc_plate_analysis.h"
#include "lc_plate_params.h"
#include "common/image.h"
#include "location_interface.h"
#include "recognition_interface.h"
#include "plate_tracking_src/plate_tracking.h"
#include "plate_recognition_src/plate_deskewing.h"
#include "plate_recognition_src/car_body_color.h"
#include "plate_recognition_src/car_window.h"
#include "yuv_osd_string.h"
#include "debug_show_image.h"

#ifdef PLATE_KEY
#include "plate_key_src/SentinelKeysLicense.h"
char limit_time[] = {"2012-12-21 08:00:00"};
#endif

#define JPEG_OUTPUT  0  // 是否压缩JPEG图片，调试时置为0可以节约时间，发布版本时必须置为1

#define PLATE_TRUST_MIN  (75)

#define WIN32_PLATE_OUT 0

#define DEBUG_MODULE_INFOMATION 0

#ifdef WIN32
#define DEBUG_THREAD 0
#define SAVE_PLATE 0
#endif

#if DEBUG_THREAD
#include "stdio.h"
#include "Fcntl.h"
#include "io.h"
#include <Windows.h>
#endif

#ifdef _TMS320C6X_
#pragma CODE_SECTION(lc_plate_analysis_create, ".text:lc_plate_analysis_create")
#pragma CODE_SECTION(lc_plate_analysis_create_thread, ".text:lc_plate_analysis_create_thread")
#pragma CODE_SECTION(lc_plate_analysis, ".text:lc_plate_analysis")
#pragma CODE_SECTION(lc_plate_analysis_wait, ".text:lc_plate_analysis_wait")
#pragma CODE_SECTION(lc_plate_analysis_recreate, ".text:lc_plate_analysis_recreate")
#pragma CODE_SECTION(lc_plate_analysis_proce, ".text:lc_plate_analysis_proce")
#pragma CODE_SECTION(lc_plate_recognition_interface, ".text:lc_plate_recognition_interface")
#pragma CODE_SECTION(plate_result_clustering, ".text:plate_result_clustering")
#pragma CODE_SECTION(lc_plate_tracking_interface, ".text:lc_plate_tracking_interface")
#pragma CODE_SECTION(get_plate_type_from_plate_color, ".text:get_plate_type_from_plate_color")
#pragma CODE_SECTION(lc_plate_analysis_destroy, ".text:lc_plate_analysis_destroy")
//#pragma CODE_SECTION(lc_plate_analysis_create_no_wait, ".text:lc_plate_analysis_create_no_wait")
#endif

#define SIZE_YUV420(img_w, img_h) ((sizeof(uint8_t)) * (img_w) * (img_h) * 3 / 2)

static int32_t location_setup = 1;
static int32_t recognition_setup = 1;

void convert_yuv_to_jpg(uint8_t *pic_buf, int32_t img_w, int32_t img_h, int32_t quality, int32_t *pic_lens);

#ifdef PLATE_KEY

#define SFNT_NUM_CHECK 3    // 加密狗检测失败重复次数

typedef struct
{
    SP_HANDLE       licHandle;//Out param
    SP_DWORD        threadnum;
    int32_t         bCheckThread;
    HANDLE          hCheckLock;
    uint40_t        hCheckThread;
} SFNT_CHECK;

SFNT_CHECK sfnt_check;
static uint32_t threadsnum = 0;

SP_STATUS SFNTCheckTimeAndThreadLimit(char *stop_time)
{
    SP_STATUS   status;
    SYSTEMTIME  systime;
    SP_DWORD    max_value = 0;
    SP_FEATURE_INFO    featureInfo;

    FILE *ferr_out = NULL;
    int32_t  i;
    uint32_t sys_day;
    uint32_t sys_time;
    uint32_t info_day;
    uint32_t info_time;

    // Ecc信息获取
    status = SFNTGetFeatureInfo(sfnt_check.licHandle, SP_ECC_TIME_LIMIT_ECC, &featureInfo);

    for (i = 0; status == SP_ERR_UNIT_NOT_FOUND && i < SFNT_NUM_CHECK; i++)
    {
        Sleep(100);
        status = SFNTGetFeatureInfo(sfnt_check.licHandle, SP_ECC_TIME_LIMIT_ECC, &featureInfo);
    }

    if (status != SP_SUCCESS)
    {
        return status;
    }

    // Ecc信息中如果启用了时间限制功能，则检验时间是否到期
    if (featureInfo.bEnableStopTime)
    {
        GetLocalTime(&systime);
        sys_day = systime.wYear * 10000 + systime.wMonth * 100 + systime.wDay;
        sys_time = systime.wHour * 10000 + systime.wMinute * 100 + systime.wSecond;

        info_day = featureInfo.timeControl.stopTime.year * 10000
                   + featureInfo.timeControl.stopTime.month * 100
                   + featureInfo.timeControl.stopTime.dayOfMonth;
        info_time = featureInfo.timeControl.stopTime.hour * 10000
                    + featureInfo.timeControl.stopTime.minute * 100
                    + featureInfo.timeControl.stopTime.second;

        if ((sys_day > info_day) || (sys_day == info_day && sys_time >= info_time))
        {
            ferr_out = fopen("Plate_Dll_Error.txt", "a+");
            fprintf(ferr_out, "%04d-%02d-%02d %02d:%02d:%02d Error 1!\n",
                    systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                    systime.wMinute, systime.wSecond);
            fclose(ferr_out);

            return SP_FAIL;
        }

        if (stop_time != NULL)
        {
            sprintf(stop_time, "%04d-%02d-%02d %02d:%02d:%02d",
                    featureInfo.timeControl.stopTime.year,
                    featureInfo.timeControl.stopTime.month,
                    featureInfo.timeControl.stopTime.dayOfMonth,
                    featureInfo.timeControl.stopTime.hour,
                    featureInfo.timeControl.stopTime.minute,
                    featureInfo.timeControl.stopTime.second);
        }
    }

    // 获取加密狗中存储的线程数限制
    status = SFNTReadInteger(sfnt_check.licHandle, SP_THREAD_NUM_INTEGER, &max_value);

    for (i = 0; status == SP_ERR_UNIT_NOT_FOUND && i < SFNT_NUM_CHECK; i++)
    {
        Sleep(100);
        status = SFNTReadInteger(sfnt_check.licHandle, SP_THREAD_NUM_INTEGER, &max_value);
    }

    if (status != SP_SUCCESS)
    {
        return status;
    }

    if (max_value <= 0)
    {
        GetLocalTime(&systime);
        ferr_out = fopen("Plate_Dll_Error.txt", "a+");
        fprintf(ferr_out, "%04d-%02d-%02d %02d:%02d:%02d Error 2!\n",
                systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                systime.wMinute, systime.wSecond);
        fclose(ferr_out);

        return SP_FAIL;
    }
    else if (sfnt_check.threadnum == 0)
    {
        sfnt_check.threadnum = max_value;
    }
    else if (sfnt_check.threadnum != max_value)
    {
        GetLocalTime(&systime);
        ferr_out = fopen("Plate_Dll_Error.txt", "a+");
        fprintf(ferr_out, "%04d-%02d-%02d %02d:%02d:%02d Error 3!\n",
                systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                systime.wMinute, systime.wSecond);
        fclose(ferr_out);

        return SP_FAIL;
    }

#if DEBUG_THREAD
    printf("%04d-%02d-%02d %02d:%02d:%02d SFNT Check Successed!\n",
           systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
           systime.wMinute, systime.wSecond);
#endif

    return status;
}

uint32_t __stdcall SFNTCheckThread(void *para)
{
    SP_BYTE     signResult[42];
    SP_BYTE     signbuff[SP_SOFTWARE_KEY_LEN];
    SP_STATUS   status = SP_SUCCESS;
    SYSTEMTIME  systime;

    FILE *ferr_out = NULL;
    int32_t  i, tmp;

    while (sfnt_check.bCheckThread > 0)
    {
        // 生成Ecc检测随机码
        for (i = 0; i < SP_SOFTWARE_KEY_LEN; i++)
        {
            tmp = rand();
            signbuff[i] = tmp - ((tmp >> 8) << 8);
        }

        // Ecc随机码输入
        status = SFNTSign(sfnt_check.licHandle, SP_ECC_TIME_LIMIT_ECC, signbuff,
                          SP_SOFTWARE_KEY_LEN, signResult);

        for (i = 0; status == SP_ERR_UNIT_NOT_FOUND && i < SFNT_NUM_CHECK; i++)
        {
            Sleep(100);
            status = SFNTSign(sfnt_check.licHandle, SP_ECC_TIME_LIMIT_ECC, signbuff,
                              SP_SOFTWARE_KEY_LEN, signResult);
        }

        if (status != SP_SUCCESS)
        {
            break;
        }

        status = SFNTCheckTimeAndThreadLimit(NULL);

        if (status != SP_SUCCESS)
        {
            break;
        }

        // Ecc随机码算法校验
        status = SFNTVerify(sfnt_check.licHandle, PUBLIC_KEY_SP_ECC_TIME_LIMIT_ECC,
                            signbuff, SP_SOFTWARE_KEY_LEN, signResult);

        for (i = 0; status == SP_ERR_UNIT_NOT_FOUND && i < SFNT_NUM_CHECK; i++)
        {
            Sleep(100);
            status = SFNTVerify(sfnt_check.licHandle, PUBLIC_KEY_SP_ECC_TIME_LIMIT_ECC,
                                signbuff, SP_SOFTWARE_KEY_LEN, signResult);
        }

        if (status != SP_SUCCESS)
        {
            break;
        }

        Sleep(3000);
    }

    sfnt_check.bCheckThread = 0;
#if DEBUG_THREAD
    printf("SFNTCheckThread Over, Error: %d !!!!!!!!!!!!!!!\n", status);
#endif

    if (status != SP_SUCCESS)
    {
        // 加密狗检验错误时，输出错误号到文件
        GetLocalTime(&systime);
        ferr_out = fopen("Plate_Dll_Error.txt", "a+");
        fprintf(ferr_out, "%04d-%02d-%02d %02d:%02d:%02d Error %d!\n",
                systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                systime.wMinute, systime.wSecond, status);
        fclose(ferr_out);
    }

    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,  // handle to DLL module
                    DWORD fdwReason,     // reason for calling function
                    LPVOID lpvReserved)  //(void *, unsigned long fdwReason, void *)
{
    int32_t i;
    FILE *ferr_out = NULL;
    SYSTEMTIME  systime;
    SP_STATUS   status = SP_SUCCESS;

    switch (fdwReason)
    {
        case 0:
#if DEBUG_THREAD
            printf("SFNTReleaseLicense! bCheckThread: %d, hCheckThread: %d, hCheckLock: %d, licHandle: %d\n",
                   sfnt_check.bCheckThread, sfnt_check.hCheckThread, sfnt_check.hCheckLock, sfnt_check.licHandle);
#endif

            if (sfnt_check.bCheckThread != 0)
            {
                sfnt_check.bCheckThread = 0;
                WaitForSingleObject((HANDLE)sfnt_check.hCheckThread, INFINITE);
            }

            CloseHandle(sfnt_check.hCheckLock);

            // 释放加密狗
            SFNTReleaseLicense(sfnt_check.licHandle);
            break;

        case 1:
            memset(&sfnt_check, 0, sizeof(SFNT_CHECK));

            // 检测加密狗
            status = SFNTGetLicense(DEVELOPERID, SOFTWARE_KEY, LICENSEID, SP_ATTR_ACTIVE,
                                    &sfnt_check.licHandle);

            for (i = 0; status == SP_ERR_UNIT_NOT_FOUND && i < SFNT_NUM_CHECK; i++)
            {
                Sleep(500);
                status = SFNTGetLicense(DEVELOPERID, SOFTWARE_KEY, LICENSEID, SP_ATTR_ACTIVE,
                                        &sfnt_check.licHandle);
            }

#if DEBUG_THREAD
            printf("SFNTGetLicense %s!\n", (status == SP_SUCCESS ? "Successed" : "Failed"));
#endif

            if (status == SP_SUCCESS)
            {
                status = SFNTCheckTimeAndThreadLimit(limit_time);
            }

            if (status != SP_SUCCESS)
            {
                // 加密狗检验错误时，输出错误号到文件
                GetLocalTime(&systime);
                ferr_out = fopen("Plate_Dll_Error.txt", "a+");
                fprintf(ferr_out, "%04d-%02d-%02d %02d:%02d:%02d Error %d!\n",
                        systime.wYear, systime.wMonth, systime.wDay, systime.wHour,
                        systime.wMinute, systime.wSecond, status);
                fclose(ferr_out);
            }

            if (status == SP_SUCCESS)
            {
                sfnt_check.bCheckThread = 1;
                sfnt_check.hCheckLock = CreateEvent(NULL, FALSE, TRUE, NULL);

                if (sfnt_check.hCheckLock == NULL)
                {
                    sfnt_check.bCheckThread = 0;
                    status = SP_FAIL;
                }
                else
                {
                    sfnt_check.hCheckThread = _beginthreadex(NULL, 0, SFNTCheckThread, NULL, 0, NULL);

                    if (0 == sfnt_check.hCheckThread)
                    {
                        CloseHandle(sfnt_check.hCheckLock);
                        sfnt_check.bCheckThread = 0;
                        status = SP_FAIL;
                    }
                }
            }

            break;

        default:
            break;
    }

    if (status == SP_SUCCESS)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

uint32_t SNTFCheckOutputBool()
{
    int32_t   i;
    SP_STATUS status;
    SP_DWORD  bcheck = 0;
    uint32_t  err_ret = LC_PLATE_SUCCESS;

    if (sfnt_check.bCheckThread > 0)
    {
        // 加密狗检测
        status = SFNTReadInteger(sfnt_check.licHandle, SP_OUTPUT_BOOL_BOOLEAN, &bcheck);

        for (i = 0; status == SP_ERR_UNIT_NOT_FOUND && i < SFNT_NUM_CHECK; i++)
        {
            status = SFNTReadInteger(sfnt_check.licHandle, SP_OUTPUT_BOOL_BOOLEAN, &bcheck);
        }

        if (status != SP_SUCCESS || bcheck <= 0)
        {
            err_ret = LC_PLATE_ERR_DOG_CHECK;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_DOG_CHECK;
    }

    return err_ret;
}

int32_t lc_plate_get_limit_time(int8_t **limit_time_back)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (limit_time[0] != '\0')
    {
        *limit_time_back = limit_time;
    }
    else
    {
        err_ret = LC_PLATE_FAIL;
    }

    return err_ret;
}

int32_t lc_plate_get_limit_thread_num(uint32_t *num)
{
    *num = sfnt_check.threadnum;

    return LC_PLATE_SUCCESS;
}
#endif

void lc_plate_analysis_create_thread(Plate_handle *plate_handle, int32_t proce_type, int32_t img_w, int32_t img_h)
{
    PlateAnalysis *pa = NULL;
    int32_t i;

    CHECKED_MALLOC(pa, sizeof(PlateAnalysis), CACHE_SIZE);
    memset(pa, 0, sizeof(PlateAnalysis));

    pa->img_w = img_w;
    pa->img_h = img_h;

    if (location_setup)
    {
        lc_plate_location_create(&pa->loc, img_w, img_h);
        lc_plate_location_param_default(pa->loc);

        if (proce_type == LC_PLATE_PROC_WAIT_PIC || proce_type == LC_PLATE_PROC_NOWAIT_PIC)
        {
            lc_plate_location_set_video_flag(pa->loc, (uint8_t)0);
        }
        else
        {
            lc_plate_location_set_video_flag(pa->loc, (uint8_t)1);
        }

        pa->location_enable = 1;
    }

    if (recognition_setup)
    {
        lc_plate_recognition_create(&pa->reg);
        pa->recognition_enable = 1;

        if (proce_type == LC_PLATE_PROC_WAIT_VIDEO || proce_type == LC_PLATE_PROC_NOWAIT_VIDEO)
        {
            lc_plate_tracking_create(&pa->trk, img_w, img_h);
        }
    }

#ifdef _TMS320C6X1_
    pa->pinfo = (PlateInfo *)MEM_alloc(IRAM, sizeof(PlateInfo), 128);
    pa->linfo = (LocationInfo *)MEM_alloc(IRAM, sizeof(LocationInfo), 128);
#else
    CHECKED_MALLOC(pa->pinfo, sizeof(PlateInfo), CACHE_SIZE);
    CHECKED_MALLOC(pa->linfo, sizeof(LocationInfo), CACHE_SIZE);
#endif

    memset(pa->pinfo, 0, sizeof(PlateInfo));
    memset(pa->linfo, 0, sizeof(LocationInfo));

    // 不管进来的是什么格式的图像，最后都要转成YUV420格式来分析
    CHECKED_MALLOC(pa->srcyuv420, SIZE_YUV420(img_w, img_h), CACHE_SIZE);
//    CHECKED_MALLOC(pa->pic_buff, SIZE_YUV420(img_w, img_h), CACHE_SIZE);
    CHECKED_MALLOC(pa->pinfo->pic_buff, SIZE_YUV420(img_w, img_h), CACHE_SIZE);

    for (i = 0; i < RECTS_NUM; i++)
    {
        CHECKED_MALLOC(pa->pinfo->pic_small[i],
                       SIZE_YUV420(PLATE_MAX_WIDTH, PLATE_MAX_HEIGHT),
                       CACHE_SIZE);

        CHECKED_MALLOC(pa->pinfo->pic_window[i],
                       SIZE_YUV420(MIN2(img_w, PLATE_MAX_WIDTH * 7), MIN2(img_h, PLATE_MAX_HEIGHT * 25)),
                       CACHE_SIZE);
    }

    pa->position_flag = (uint8_t)0;             // 默认不框出车牌位置
    pa->proce_type = proce_type;
    pa->video_format = LC_VIDEO_FORMAT_I420;
    pa->car_color_enable = (uint8_t)0;          // 默认不识别车身颜色
    pa->OSD_flag = (uint8_t)0;
    pa->OSD_scale = 1.0;
    pa->OSD_pos_x = 0;
    pa->OSD_pos_y = 0;
    pa->car_window_enable = (uint8_t)0;         // 默认不识别车窗位置
    pa->deskew_flag = (uint8_t)1;    // 车牌倾斜校正(默认打开)
    pa->img_quality = 75;
    pa->trust_thresh = (uint8_t)PLATE_TRUST_MIN;

    if (proce_type == LC_PLATE_PROC_WAIT_PIC || proce_type == LC_PLATE_PROC_NOWAIT_PIC)
    {
        pa->video_flag = (uint8_t)0;
    }
    else
    {
        pa->video_flag = (uint8_t)1;
    }

    *plate_handle = (Plate_handle)pa;   // 将句柄返回

#ifdef WIN32
    //colors_pace_convert_init();
#endif

    return;
}

// 对车牌候选区域的识别结果进行聚类，去除2个相邻候选区域中的伪车牌
static void plate_result_clustering(LocationInfo *plate)
{
    int32_t m, n;
    int32_t xcm, ycm;  // 矩形rect_real[m]的中心点坐标
    int32_t xcn, ycn;  // 矩形rect_real[n]的中心点坐标
    int32_t w, h;

    if (plate->real_num <= 1)
    {
        return;
    }

    // 对车牌进行y方向聚类，只保留y坐标值最大的一个车牌
    for (m = 0; m < plate->real_num; m++)
    {
        if (plate->rect_real[m].x0 >= 0)
        {
            xcm = (plate->rect_real[m].x0 + plate->rect_real[m].x1) / 2;
            ycm = (plate->rect_real[m].y0 + plate->rect_real[m].y1) / 2;

            w = plate->rect_real[m].x1 - plate->rect_real[m].x0 + 1;
            h = plate->rect_real[m].y1 - plate->rect_real[m].y0 + 1;

            for (n = 0; n < plate->real_num; n++)
            {
                if (plate->rect_real[n].x0 >= 0 && (n != m))
                {
                    if (two_rectanges_intersect_or_not(plate->rect_real[m], plate->rect_real[n]))
                    {
                        if (plate->trust[m] > plate->trust[n])
                        {
                            plate->rect_real[n].x0 = -1;
                            continue;
                        }
                        else
                        {
                            plate->rect_real[m].x0 = -1;
                            break;
                        }
                    }

                    xcn = (plate->rect_real[n].x0 + plate->rect_real[n].x1) / 2;
                    ycn = (plate->rect_real[n].y0 + plate->rect_real[n].y1) / 2;

                    // 如果2个车牌候选区域在垂直和水平方向上的距离都比较近，则很可能
                    // 上面的车牌候选区域是保险杠引起的误报，下面的才是真正的车牌
                    if ((abs(ycn - ycm) < h * 5) && (abs(xcn - xcm) < w))
                    {
                        if (ycn > ycm)
                        {
                            plate->rect_real[m].x0 = -1;
                            break;
                        }
                        else
                        {
                            plate->rect_real[n].x0 = -1;
                            continue;
                        }
                    }
                }
            }
        }
    }

    n = 0;

    for (m = 0; m < plate->real_num; m++)
    {
        if (plate->rect_real[m].x0 >= 0 /*&& (n != m)*/)
        {
            plate->rect_cand[n] = plate->rect_cand[m];
            plate->rect_real[n] = plate->rect_real[m];

            plate->trust[n] = plate->trust[m];
            plate->color[n] = plate->color[m];
            plate->car_color[n] = plate->car_color[m];
            plate->car_color_trust[n] = plate->car_color_trust[m];
            plate->save_flag[n] = plate->save_flag[m];

            plate->ch_num[n] = plate->ch_num[m];
            memcpy(plate->chs[n], plate->chs[m], (uint32_t)8);
            memcpy(plate->chs_trust[n], plate->chs_trust[m], (uint32_t)7);
            memcpy(plate->ch_buf[n], plate->ch_buf[m], (uint32_t)(7 * 32 * 24));

            n++;
        }
    }

    plate->real_num = n;

    return;
}

#if WIN32_PLATE_OUT
static void yuv2bgr1(uint8_t *yuv420, uint8_t *buf_bgr, int32_t img_w, int32_t img_h)
{
    int32_t i, j;
    int32_t r, g, b;
    int32_t y, cb, cr;
    uint8_t *u;
    uint8_t *v;

    u = yuv420 + img_w * img_h;
    v = yuv420 + img_w * img_h + img_w * img_h / 4;

    for (i = 0; i < img_h; i++)
    {
        for (j = 0; j < img_w; j++)
        {
            cb = (uint8_t)u[(i / 2) * (img_w / 2) + (j / 2)] - 128;
            cr = (uint8_t)v[(i / 2) * (img_w / 2) + (j / 2)] - 128;

            y = yuv420[i * img_w + j];

            //          r = (int32_t)(y + cb * 0.956f + cr * 0.621f);
            //          g = (int32_t)(y + cb * 0.272f + cr * 0.647f);
            //          b = (int32_t)(y + cb * 1.106f + cr * 1.703f);

            r = (int32_t)(y + cr * 1.4075f);
            g = (int32_t)(y - cb * 0.39f - cr * 0.58f);
            b = (int32_t)(y + cb * 2.03f);

            r = r > 255 ? 255 : r;
            g = g > 255 ? 255 : g;
            b = b > 255 ? 255 : b;

            r = r < 0 ? 0 : r;
            g = g < 0 ? 0 : g;
            b = b < 0 ? 0 : b;

            buf_bgr[i * img_w * 3 + j * 3 + 0] = (uint8_t)b;
            buf_bgr[i * img_w * 3 + j * 3 + 1] = (uint8_t)g;
            buf_bgr[i * img_w * 3 + j * 3 + 2] = (uint8_t)r;
        }
    }
}
static void get_body_area0(uint8_t *restrict yuv_data, uint8_t *restrict bodyimg, int32_t img_w, int32_t img_h,
                           int32_t body_w, int32_t body_h, Rects *restrict rect_body)
{
    int32_t w, h;
    uint8_t *restrict yuv_y = yuv_data;
    uint8_t *restrict yuv_u = yuv_data + img_w * img_h;
    uint8_t *restrict yuv_v = yuv_data + img_w * img_h * 5 / 4;
    uint8_t *restrict bodyimg_y = bodyimg;
    uint8_t *restrict bodyimg_u = bodyimg + body_w * body_h;
    uint8_t *restrict bodyimg_v = bodyimg + body_w * body_h * 5 / 4;

    for (h = 0; h < body_h; h++)
    {
        for (w = 0; w < body_w; w++)
        {
            bodyimg_y[body_w * h + w] = yuv_y[img_w * (h + rect_body->y0) + rect_body->x0 + w];
        }
    }

    for (h = 0; h < body_h / 2; h++)
    {
        for (w = 0; w < body_w / 2; w++)
        {
            bodyimg_u[body_w / 2 * h + w] = yuv_u[img_w / 2 * (h + rect_body->y0 / 2) + rect_body->x0 / 2 + w];
            bodyimg_v[body_w / 2 * h + w] = yuv_v[img_w / 2 * (h + rect_body->y0 / 2) + rect_body->x0 / 2 + w];
        }
    }

    return;
}
#endif

static void lc_plate_recognition_interface(PlateAnalysis *pa, uint8_t *restrict yuv_img,
                                           LocationInfo *plate, int32_t img_w, int32_t img_h)
{
    int32_t i, k;
    int32_t n = 0;
//    int32_t w;
    int32_t h;
    int32_t plate_w;
    int32_t plate_h;
    int32_t real_h;
    int32_t cmp_y0 = 0;
    int32_t cmp_y1 = 0;
    int16_t x0 = 0;
    int16_t x1 = 0;
    int16_t y0 = 0;
    int16_t y1 = 0;
    Rects ch_pos[15];
    int32_t ch_num;
    uint8_t *restrict pgray = NULL;
    int32_t plate_area;
    uint8_t valid = (uint8_t)0;

    CLOCK_INITIALIZATION();

#if DEBUG_MODULE_INFOMATION
    // 打印定位模块输出信息
    {
        int32_t i;
        FILE* fmodule_info_out = fopen("module_info_out.txt", "a+");

        fprintf(fmodule_info_out, "======================================\n");
        fprintf(fmodule_info_out, "location num: %d\n", plate->real_num);

        for (i = 0; i < plate->real_num; i++)
        {
            fprintf(fmodule_info_out, "Rect%d: %d, %d, %d, %d\n", i,
                    plate->rect_real[i].x0, plate->rect_real[i].x1,
                    plate->rect_real[i].y0, plate->rect_real[i].y1);
        }

        fclose(fmodule_info_out);
    }
#endif

    for (i = 0; i < plate->real_num; i++)
    {
        ch_num = 0;

        valid = (uint8_t)1;

        if (pa->video_flag)
        {
            for (k = 0; k < n; k++)
            {
                if (two_rectanges_intersect_or_not(plate->rect_real[i], plate->rect_real[k]))
                {
                    valid = (uint8_t)0;
                    break;
                }
            }

            if (!valid)
            {
                continue;
            }
        }

        x0 = plate->rect_real[i].x0;
        x1 = plate->rect_real[i].x1;
        y0 = plate->rect_real[i].y0;
        y1 = plate->rect_real[i].y1;

        real_h = y1 - y0 + 1;

#if WIN32_PLATE_OUT
        {
            int32_t w;
            static int32_t numSave1 = 1;
            uint8_t nameSave1[260];
            int32_t save_w = (MIN2(x1 + 1, img_w - 1) - x0 + 1) / 2 * 2;
            int32_t save_h = MIN2(y1 + 16, img_h - 1) - MAX2(y0 - 16, 0) + 1;
            uint8_t *save_img;
            Rects rect_save;
            uint8_t *save_rgb;
            IplImage *imageSave = NULL;

            imageSave = cvCreateImage(cvSize(save_w, save_h), 8, 3);

            rect_save.x0 = x0;
            rect_save.x1 = x0 + save_w - 1;
            rect_save.y0 = MAX2(y0 - 16, 0);
            rect_save.y1 = MIN2(y1 + 16, img_h - 1);

//            if (!(x0 > 1050 && y0 > 650))
            {
                CHECKED_MALLOC(save_img, save_w * save_h * 3 / 2, CACHE_SIZE);
                CHECKED_MALLOC(save_rgb, save_w * save_h * 3, CACHE_SIZE);

                get_body_area0(yuv_img, save_img, img_w, img_h, save_w, save_h, &rect_save);

                yuv2bgr1(save_img, save_rgb, save_w, save_h);

                _IMAGE3_RGB_(imageSave, save_rgb, save_w, save_h, 0);

                sprintf(nameSave1, "double02\\tttstrrmg%06d.bmp", numSave1++);
                cvSaveImage(nameSave1, imageSave);
                cvReleaseImage(&imageSave);

                CHECKED_FREE(save_img);
                CHECKED_FREE(save_rgb);
            }
        }
#endif

        if (pa->deskew_flag) // 为车牌旋转留出足够的空间
        {
            cmp_y0 = MIN2(y0, 16);
            cmp_y1 = MIN2(img_h - 1 - y1, 16);

            y0 = (int16_t)MAX2(y0 - 16, 0);
            y1 = (int16_t)MIN2(y1 + 16, img_h - 1);
        }

        plate_w = x1 - x0 + 1;
        plate_h = y1 - y0 + 1;
        plate_area = plate_w * plate_h * 1;

        CHECKED_MALLOC(pgray, sizeof(uint8_t) * plate_area, CACHE_SIZE);

        for (h = 0; h < plate_h; h++)
        {
            memcpy(pgray + plate_w * h, yuv_img + img_w * (y0 + h) + x0, plate_w);
        }

        if (pa->deskew_flag)
        {
            if (plate_w > 2 * real_h)
            {
                if (plate->plate_double_flag[i] == (0u))  // 单层牌车牌倾斜校正
                {
                    plate_deskewing_single(pgray, cmp_y0, cmp_y1, plate_w, plate_h, real_h, plate_w - real_h);
                    plate_h = plate_h - cmp_y0 - cmp_y1;
                }
                else                                   // 双层牌车牌倾斜校正
                {
                    int32_t plate_h_up = plate->rect_double_up[i].y1 - plate->rect_double_up[i].y0 + 1;

                    plate_deskewing_double(pgray, cmp_y0, cmp_y1, plate_w, plate_h, plate_h_up, real_h, plate_w - real_h);
                    plate_h = plate_h - cmp_y0 - cmp_y1;
                }
            }
            else
            {
                memcpy(pgray, pgray + plate_w * cmp_y0, plate_w * real_h);
                plate_h = plate_h - cmp_y0 - cmp_y1;
            }
        }

        {
#if SAVE_PLATE
            static numSave = 0;
            IplImage *imageSave = cvCreateImage(cvSize(plate_w, plate_h), 8, 1);
            char nameSave[260];

            for (h = 0; h < plate_h; h++)
            {
                memcpy(imageSave->imageData + imageSave->widthStep * h, pgray + plate_w * h, plate_w);
            }

            sprintf(nameSave, "pp\\%d.bmp", numSave++);
            cvSaveImage(nameSave, imageSave);
            cvReleaseImage(&imageSave);
#endif
        }

        CLOCK_START(lc_plate_recognition)
#ifdef _TMS320C6X_1
        printf("plate_w = %d, plate_h = %d\n", plate_w, plate_h);
#endif

        if (plate->plate_double_flag[i] == (0u))  // 单层牌识别
        {
            lc_plate_recognition_single(pa->reg, pgray, plate_w, plate_h, ch_pos, &ch_num,
                                        plate->ch_buf[n], plate->chs[n], plate->chs_trust[n], &plate->brightness[n],
                                        &plate->trust[n], pa->trust_thresh, &plate->color[i], &plate->color[n],
                                        yuv_img, img_w, img_h, &plate->rect_real[i], &plate->rect_real[n]);
        }
        else                                   // 双层牌识别
        {
            // 计算上层区域高度，将其传入识别函数
            int32_t plate_h_up = plate->rect_double_up[i].y1 - plate->rect_double_up[i].y0 + 1;

            lc_plate_recognition_double(pa->reg, pgray, plate_h_up, plate_w, plate_h, ch_pos, &ch_num,
                                        plate->ch_buf[n], plate->chs[n], plate->chs_trust[n], &plate->brightness[n],
                                        &plate->trust[n], pa->trust_thresh, &plate->color[i], &plate->color[n],
                                        yuv_img, img_w, img_h, &plate->rect_real[i], &plate->rect_real[n]);
        }

        CLOCK_END("lc_plate_recognition");

        //        printf("\nch_num: %d, trust: %2d, trust_thresh: %d", ch_num, plate->trust[n], pa->trust_thresh);

        if (ch_num == 7)
        {
            //printf("%s\n", plate->chs[n]);
            plate->ch_num[n] = (uint8_t)7;

            if ((pa->car_color_enable) && (plate->plate_double_flag[i] != (1u))) // 只有单层牌才进行车身颜色识别，双层牌为后牌
            {
                car_body_color(yuv_img, img_w, img_h, &plate->rect_real[i],
                               &plate->car_color[n], &plate->car_color_trust[n]);
            }
            else
            {
                plate->car_color[n] = (99u);
                plate->car_color_trust[n] = (0u);
            }

            //printf("%s\n", plate->chs);
            n++;
        }
        else if (ch_num == 8)
        {
            //printf("%s\n", plate->chs[n]);
            plate->ch_num[n] = (uint8_t)8;

            // 暂时不做农用车的车身颜色识别
            plate->car_color[n] = (99u);
            plate->car_color_trust[n] = (0u);
//             if (pa->car_color_enable)
//             {
//                 car_body_color(yuv_img, img_w, img_h, &plate->rect_real[i],
//                     &plate->car_color[n], &plate->car_color_trust[n]);
//             }
            n++;
        }

        CHECKED_FREE(pgray, sizeof(uint8_t) * plate_area);
    }

#if DEBUG_MODULE_INFOMATION
    // 打印识别模块输出信息
    {
        int32_t i;
        FILE* fmodule_info_out = fopen("module_info_out.txt", "a+");

        fprintf(fmodule_info_out, "======================================\n");
        fprintf(fmodule_info_out, "recog number: %d\n", n);

        for (i = 0; i < n; i++)
        {
            int32_t j;

            fprintf(fmodule_info_out, "plate%d---trust: %d, brightness: %d, color: %d\n",
                    i, plate->trust[i], plate->brightness[i], plate->color[i]);
            fprintf(fmodule_info_out, "Rect: (%d, %d, %d, %d)\n",
                    plate->rect_real[i].x0, plate->rect_real[i].x1,
                    plate->rect_real[i].y0, plate->rect_real[i].y1);
            fprintf(fmodule_info_out, "ch_num: %d, chs: %s\n", plate->ch_num[i], plate->chs[i]);
            fprintf(fmodule_info_out, "chs_trust: ");

            for (j = 0; j < plate->ch_num[i]; j++)
            {
                fprintf(fmodule_info_out, "%d ", plate->chs_trust[i][j]);
            }

            fprintf(fmodule_info_out, "\n");
        }

        fclose(fmodule_info_out);
    }
#endif


#if DEBUG_MODULE_INFOMATION
    // 打印车身颜色识别模块输出信息
    {
        FILE* fcar_color_out = NULL;

        fcar_color_out = fopen("module_info_out.txt", "a+");

        fprintf(fcar_color_out, "======================================\n");
        fprintf(fcar_color_out, "car_color number: %d\n", n);

        for (i = 0; i < n; i++)
        {
            fprintf(fcar_color_out, "plate%d---color: %d\n", i, plate->car_color[i]);
        }

        fclose(fcar_color_out);
    }
#endif

    plate->real_num = n;

    plate_result_clustering(plate);

    return;
}

#if 0
static void set_character_as_unkown_while_no_convinced(uint8_t *restrict chs, uint8_t *restrict chs_trust)
{
    int32_t k = 0;

    if (chs_trust[0] < 85)
    {
        chs[0] = 163; // 省份汉字未知
    }

    for (k = 1; k < 7; k++)
    {
        if (chs_trust[k] < 90)
        {
            chs[k] = '?';
        }
    }

    return;
}
#endif

static void lc_plate_tracking_interface(Plate_handle* plate_handle, Trk_handle trk_handle,
                                        PlateInfo *restrict pinfo, LocationInfo *restrict plate,
                                        uint8_t *restrict yuv420, int32_t img_w, int32_t img_h)
{
    int32_t i;
    int32_t index;
    int32_t snum = 0;
    int32_t onum;
    ObjectInfo *restrict oinfo = NULL;
    ObjectInfo *restrict sinfo = NULL;  // 用于保存的车牌信息

    onum = plate->real_num;

    CHECKED_MALLOC(oinfo, sizeof(ObjectInfo) * onum, CACHE_SIZE);
    CHECKED_MALLOC(sinfo, sizeof(ObjectInfo) * TRACK_MAX, CACHE_SIZE);

    for (i = 0; i < onum; i++)
    {
        oinfo[i].pos = plate->rect_real[i];
        oinfo[i].trust = plate->trust[i];
        oinfo[i].brightness = plate->brightness[i];
        oinfo[i].color = plate->color[i];
        oinfo[i].car_color = plate->car_color[i];
        oinfo[i].car_color_trust = plate->car_color_trust[i];
        oinfo[i].directions = (uint8_t)0; // 所有目标的初始值为方向未知
        oinfo[i].trk_idx = (uint8_t)INVALID_ID;
        oinfo[i].ch_num = plate->ch_num[i];
        memcpy(oinfo[i].chs, plate->chs[i], sizeof(plate->chs[i]));
        memcpy(oinfo[i].chs_trust, plate->chs_trust[i], sizeof(plate->chs_trust[i]));
    }

    lc_plate_tracking(trk_handle, pinfo->pic_buff, yuv420, oinfo, onum, sinfo, &snum,
                      plate->save_flag, img_w, img_h);

    plate->real_num = MIN2(plate->real_num, RECTS_NUM);

    index = 0;

    for (i = 0; i < onum; i++)
    {
        if (plate->save_flag[i])
        {
            pinfo->plate_list[index] = plate->rect_real[i];
            index++;
        }
    }

    pinfo->plate_num = (int32_t)index;
    pinfo->save_num = snum;

    if (snum > 0)   // 保存抓拍信息
    {
        // 保存车牌识别信息
        pinfo->trust[0] = sinfo[0].trust;
        pinfo->color[0] = sinfo[0].color;
        pinfo->brightness[0] = sinfo[0].brightness;
        pinfo->car_color[0] = sinfo[0].car_color;
        pinfo->car_color_trust[0] = sinfo[0].car_color_trust;
        pinfo->plate_rect[0] = sinfo[0].pos;
        pinfo->ch_num[0] = sinfo[0].ch_num;
        pinfo->directions[0] = sinfo[0].directions;

//      set_character_as_unkown_while_no_convinced(sinfo[0].chs, sinfo[0].chs_trust);

        memcpy(pinfo->chs[0], sinfo[0].chs, sizeof(sinfo[0].chs));
        memcpy(pinfo->chs_trust[0], sinfo[0].chs_trust, sizeof(sinfo[0].chs_trust));

        // 保存车牌图像
//        memcpy(pinfo->pic_buff, pa->pic_buff, SIZE_YUV420(img_w, img_h));
    }

#if DEBUG_MODULE_INFOMATION
    // 打印跟踪模块输出信息
    {
        FILE* fmodule_info_out = fopen("module_info_out.txt", "a+");

        fprintf(fmodule_info_out, "======================================\n");
        fprintf(fmodule_info_out, "track num: %d\n", snum);

        if (snum > 0)
        {
            fprintf(fmodule_info_out, "trust: %d, color: %d, brightness: %d, car_color: %d, direction: %d\n",
                    pinfo->trust[0], pinfo->color[0], pinfo->brightness[0], pinfo->car_color[0], pinfo->directions[0]);
            fprintf(fmodule_info_out, "Rect: (%d, %d, %d, %d)\n", pinfo->plate_rect[0].x0, pinfo->plate_rect[0].x1,
                    pinfo->plate_rect[0].y0, pinfo->plate_rect[0].y1);
            fprintf(fmodule_info_out, "ch_num: %d, chs: %s\n", pinfo->ch_num[0], pinfo->chs[0]);
            fprintf(fmodule_info_out, "chs_trust: ");

            for (i = 0; i < pinfo->ch_num[0]; i++)
            {
                fprintf(fmodule_info_out, "%d ", pinfo->chs_trust[0][i]);
            }

            fprintf(fmodule_info_out, "\n");
        }

        fclose(fmodule_info_out);
    }
#endif

    CHECKED_FREE(sinfo, sizeof(ObjectInfo) * TRACK_MAX);
    CHECKED_FREE(oinfo, sizeof(ObjectInfo) * onum);

    return;
}

// 根据车牌颜色推断号牌类型
static void get_plate_type_from_plate_color(PlateInfo *restrict pinfo/*, LocationInfo *restrict linfo*/)
{
    int32_t i;

    for (i = 0; i < pinfo->save_num; i++)
    {
        if (pinfo->color[i] == (uint8_t)0)       // 蓝牌
        {
            pinfo->type[i] = (uint8_t)2;         // 小型汽车号牌
        }
        else if (pinfo->color[i] == (uint8_t)1)  // 黄牌
        {
            // 大型汽车和教练车的汉字范围必须小于131或者为163(未知)
            if ((pinfo->chs[i][0] < (131u)) || (pinfo->chs[i][0] == (163u)))
            {
                if (pinfo->chs[i][6] == CH_XUE) // 最后一个车牌字符是'学'
                {
                    pinfo->type[i] = (uint8_t)16;    // 教练汽车号牌
                }
                else
                {
                    pinfo->type[i] = (uint8_t)1;     // 大型汽车号牌
                }
            }
            else                                    // 如果汉字为军牌汉字，则车牌颜色和车牌类型要相应修改为白牌和军用汽车类型
            {
                pinfo->color[i] = (uint8_t)2;
                pinfo->type[i] = (uint8_t)25;
            }

        }
        else if (pinfo->color[i] == (uint8_t)2)  // 白牌
        {
            if (pinfo->chs[i][0] >= (131u))
            {
                pinfo->type[i] = (uint8_t)25;        // 军用汽车号牌
            }
            else                                     // 如果汉字为普通省份汉字，则车牌颜色和车牌类型要相应修改为黄牌和大型汽车类型
            {
                pinfo->color[i] = (uint8_t)1;
                pinfo->type[i] = (uint8_t)1;
            }
        }
        else if (pinfo->color[i] == (uint8_t)3)  // 白牌
        {
            pinfo->type[i] = (uint8_t)23;        // 警用汽车号牌
        }
        else if (pinfo->color[i] == (uint8_t)4)  // 黑牌
        {
            pinfo->type[i] = (uint8_t)26;        // 境外汽车号牌
        }
        else if (pinfo->color[i] == (uint8_t)5)  // 红牌
        {
            pinfo->type[i] = (uint8_t)27;        //
        }
        else if (pinfo->color[i] == (uint8_t)6)  // 绿牌
        {
            pinfo->type[i] = (uint8_t)28;        // 农用汽车号牌
        }
        else if (pinfo->color[i] == (uint8_t)10)
        {
            pinfo->type[i] = (uint8_t)3;
        }
        else
        {
            pinfo->type[i] = (uint8_t)99;        // 未知
        }
    }

    return;
}

int32_t lc_plate_analysis_proce(Plate_handle *plate_handle, uint8_t *restrict yuv420,
                                PlateInfo *restrict pinfo, LocationInfo *restrict linfo)
{
    int32_t       i;
    int32_t      err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = (PlateAnalysis *)(*plate_handle);
    int32_t       img_w = pa->img_w;
    int32_t       img_h = pa->img_h;

    CLOCK_INITIALIZATION();

    if (location_setup && pa->location_enable)
    {
        CLOCK_START(lc_plate_location);
        lc_plate_location(pa->loc, yuv420, linfo);
        CLOCK_END("lc_plate_location");
    }

    if (recognition_setup && pa->recognition_enable)
    {
        CLOCK_START(lc_plate_recognition_interface);
        lc_plate_recognition_interface(pa, yuv420, linfo, img_w, img_h);
        CLOCK_END("lc_plate_recognition_interface");
    }

    pinfo->save_num = 0;

    if (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO || pa->proce_type == LC_PLATE_PROC_NOWAIT_VIDEO)
    {
#if 1
        CLOCK_START(lc_plate_tracking_interface);
        lc_plate_tracking_interface(plate_handle, pa->trk, pinfo, linfo, yuv420, img_w, img_h);
        CLOCK_END("lc_plate_tracking_interface");

        if (pinfo->save_num > 0)
        {
            pinfo->save_num = 1;
        }

#else
        pinfo->plate_num = 0;//linfo->real_num;

        for (i = 0; i < pinfo->plate_num; i++)
        {
            pinfo->plate_list[i] = linfo->rect_real[i];
        }

#endif
    }
    else
    {
        pinfo->plate_num = linfo->real_num;
        pinfo->save_num = linfo->real_num;

        memcpy(pinfo->pic_buff, yuv420, SIZE_YUV420(img_w, img_h));

        for (i = 0; i < pinfo->save_num; i++)
        {
//          printf("ch_num: %d\n", pinfo->ch_num[i]);
            pinfo->plate_list[i] = linfo->rect_real[i];
            pinfo->plate_rect[i] = linfo->rect_real[i];
            pinfo->trust[i] = linfo->trust[i];
            pinfo->color[i] = linfo->color[i];
            pinfo->ch_num[i] = linfo->ch_num[i];
            pinfo->brightness[i] = linfo->brightness[i];
            pinfo->car_color[i] = linfo->car_color[i];
            pinfo->car_color_trust[i] = linfo->car_color_trust[i];
            memcpy(pinfo->chs[i], linfo->chs[i], sizeof(linfo->chs[i]));
            memcpy(pinfo->chs_trust[i], linfo->chs_trust[i], sizeof(linfo->chs[i]));
        }
    }

    if (pinfo->save_num > 0)
    {
#ifdef PLATE_KEY
        err_ret = SNTFCheckOutputBool();
#endif

        if (err_ret == LC_PLATE_SUCCESS)
        {
            // 根据车牌颜色推断号牌类型
            get_plate_type_from_plate_color(pinfo/*, linfo*/);

            if (pa->car_window_enable)
            {
                for (i = 0; i < pinfo->save_num; i++)
                {
                    int32_t ret;
                    int32_t h;
                    int32_t x0, x1, y0, y1;
                    int32_t window_w, window_h;
                    uint8_t *restrict picy = pinfo->pic_buff;
                    uint8_t *restrict picu = pinfo->pic_buff + img_w * img_h;
                    uint8_t *restrict picv = pinfo->pic_buff + (img_w * img_h * 5 >> 2);
                    uint8_t *restrict winy = NULL;
                    uint8_t *restrict winu = NULL;
                    uint8_t *restrict winv = NULL;

                    //车窗区域识别
                    ret = car_window_area_recog(pinfo->pic_buff, img_w, /*img_h, */
                                                &pinfo->plate_rect[i], &pinfo->car_window_rect[i]);

                    if (ret == 0)
                    {
                        continue;
                    }

                    x0 = pinfo->car_window_rect[i].x0;
                    x1 = x0 + (((pinfo->car_window_rect[i].x1 - x0) >> 4) << 4);
                    y0 = pinfo->car_window_rect[i].y0;
                    y1 = y0 + (((pinfo->car_window_rect[i].y1 - y0) >> 4) << 4);
                    window_w = x1 - x0;
                    window_h = y1 - y0;
                    winy = pinfo->pic_window[i];
                    winu = pinfo->pic_window[i] + window_w * window_h;
                    winv = pinfo->pic_window[i] + (window_w * window_h * 5 >> 2);

                    for (h = y0; h < y1; h++)
                    {
                        memcpy(winy + window_w * (h - y0), picy + img_w * h + x0, window_w);
                    }

                    for (h = (y0 >> 1); h < (y1 >> 1); h++)
                    {
                        memcpy(winu + (window_w >> 1) * (h - (y0 >> 1)), picu + (img_w >> 1) * h + (x0 >> 1), window_w >> 1);
                        memcpy(winv + (window_w >> 1) * (h - (y0 >> 1)), picv + (img_w >> 1) * h + (x0 >> 1), window_w >> 1);
                    }

#if JPEG_OUTPUT1
                    convert_yuv_to_jpg(pinfo->pic_window[i], window_w, window_h,
                                       pa->img_quality, &pinfo->pwindow_lens[i]);
#endif
                }
            }

            // 对进行车牌矩形框适当扩展
            for (i = 0; i < pinfo->save_num; i++)
            {
                pinfo->plate_rect[i].x0 = (int16_t)MAX2(pinfo->plate_rect[i].x0 - 10, 0);
                pinfo->plate_rect[i].x1 = (int16_t)MIN2(pinfo->plate_rect[i].x1 + 10, img_w - 1);
                pinfo->plate_rect[i].y0 = (int16_t)MAX2(pinfo->plate_rect[i].y0 - 6, 0);
                pinfo->plate_rect[i].y1 = (int16_t)MIN2(pinfo->plate_rect[i].y1 + 6, img_h - 1);
            }

            for (i = 0; i < pinfo->save_num; i++)
            {
                uint8_t *restrict sy = pinfo->pic_buff;
                uint8_t *restrict su = pinfo->pic_buff + img_w * img_h;
                uint8_t *restrict sv = pinfo->pic_buff + img_w * img_h * 5 / 4;
                uint8_t *restrict dy = NULL;
                uint8_t *restrict du = NULL;
                uint8_t *restrict dv = NULL;
                int32_t h;
                int32_t small_w;
                int32_t small_h;
                // 使小图像的宽度、高度都是16的倍数
                int32_t x0 = pinfo->plate_rect[i].x0;
                int32_t x1 = x0 + ((pinfo->plate_rect[i].x1 - x0 + 15) & 0xFFF0);
                int32_t y0 = pinfo->plate_rect[i].y0;
                int32_t y1 = y0 + ((pinfo->plate_rect[i].y1 - y0 + 15) & 0xFFF0);

                // 如果越界的话左移或者上移一点
                if (x1 > img_w - 1)
                {
                    x0 -= 16;
                    x1 -= 16;
                }

                if (y1 > img_h - 1)
                {
                    y0 -= 16;
                    y1 -= 16;
                }

                small_w = MIN2((x1 - x0), PLATE_MAX_WIDTH);
                small_h = MIN2((y1 - y0), PLATE_MAX_HEIGHT);
                dy = pinfo->pic_small[i];
                du = pinfo->pic_small[i] + small_w * small_h;
                dv = pinfo->pic_small[i] + small_w * small_h * 5 / 4;

                for (h = y0; h < y1; h++)
                {
                    memcpy(dy + small_w * (h - y0), sy + img_w * h + x0, small_w);
                }

                for (h = (y0 >> 1); h < (y1 >> 1); h++)
                {
                    memcpy(du + (small_w >> 1) * (h - (y0 >> 1)), su + (img_w >> 1) * h + (x0 >> 1), small_w >> 1);
                    memcpy(dv + (small_w >> 1) * (h - (y0 >> 1)), sv + (img_w >> 1) * h + (x0 >> 1), small_w >> 1);
                }

                pinfo->psmall_lens[i] = SIZE_YUV420(small_w, small_h);
#ifdef _TMS320C6X_
                {
                    int32_t small_quality = 80;           // 车牌区域小图片编码质量初始值设置为80
                    uint8_t *small_pic = NULL;

                    CHECKED_MALLOC(small_pic, SIZE_YUV420(small_w, small_h), CACHE_SIZE);
                    memcpy(small_pic, pinfo->pic_small[i], SIZE_YUV420(small_w, small_h));
                    convert_yuv_to_jpg(small_pic, small_w, small_h, small_quality, &pinfo->psmall_lens[i]);

                    // 编码后的车牌区域小图片长度不能超过12K
                    while ((pinfo->psmall_lens[i] > 12288) && ((small_quality -= 5) > 0))
                    {
                        memcpy(small_pic, pinfo->pic_small[i], SIZE_YUV420(small_w, small_h));

                        convert_yuv_to_jpg(small_pic, small_w, small_h, small_quality, &pinfo->psmall_lens[i]);
                    }

                    if (small_quality <= 0)
                    {
                        pinfo->psmall_lens[i] = 0;
                    }

                    memcpy(pinfo->pic_small[i], small_pic, pinfo->psmall_lens[i]);

                    CHECKED_FREE(small_pic, SIZE_YUV420(small_w, small_h));
                }
#else
#if JPEG_OUTPUT
                {
                    uint8_t *image_pic = NULL;

                    CHECKED_MALLOC(image_pic, SIZE_YUV420(small_w, small_h), CACHE_SIZE);

                    memcpy(image_pic, pinfo->pic_small[i], SIZE_YUV420(small_w, small_h));

                    yuv420_to_jpeg_buffer(pinfo->pic_small[i], image_pic, small_w, small_h, 95, &pinfo->psmall_lens[i]);

                    CHECKED_FREE(image_pic, SIZE_YUV420(small_w, small_h));
                }
#endif // JPEG_OUPUT
#endif
            }

            for (i = 0; i < pinfo->save_num; i++)
            {
                if (pa->position_flag)
                {
                    draw_plate_window(pinfo->pic_buff, img_w, img_h,
                                      pinfo->plate_rect[i].x0, pinfo->plate_rect[i].x1,
                                      pinfo->plate_rect[i].y0, pinfo->plate_rect[i].y1);
                }

// #ifdef WIN321 // 对外关闭OSD叠加功能
//                 if (pa->OSD_flag)
//                 {
//                     int8_t *plate_name = NULL;
//                     int8_t *plate_color_name = NULL;
//                     int8_t OSD_string[1024];
//                     int32_t y;
//
//                     y = pa->OSD_pos_y;
//
//                     lc_plate_get_plate_name(*plate_handle, i, &plate_name);
//                     lc_plate_get_plate_color_name(*plate_handle, i, &plate_color_name);
//
//                     sprintf(OSD_string, "%s %s", plate_name, plate_color_name);
//
//                     osd_mask_string(OSD_string, pa->OSD_pos_x, &y, pinfo->pic_buff, img_w, img_h, 255, 0, 0);
//                 }
// #endif
            }

            pinfo->pbuff_lens = SIZE_YUV420(img_w, img_h);

#if JPEG_OUTPUT
#ifdef _TMS320C6X_
            {
                uint8_t *image_pic = NULL;
                int32_t image_quality = pa->img_quality;

                CHECKED_MALLOC(image_pic, SIZE_YUV420(img_w, img_h), CACHE_SIZE);

                memcpy(image_pic, pinfo->pic_buff, SIZE_YUV420(img_w, img_h));

                CLOCK_START(convert_yuv_to_jpg);
                convert_yuv_to_jpg(image_pic, img_w, img_h, image_quality, &pinfo->pbuff_lens);
                CLOCK_END("convert_yuv_to_jpg");

                // 抓拍图像大小不能超过85K
                while ((pinfo->pbuff_lens > 87040) && ((image_quality -= 5) > 0))
                {
                    memcpy(image_pic, pinfo->pic_buff, SIZE_YUV420(img_w, img_h));

                    convert_yuv_to_jpg(image_pic, img_w, img_h, image_quality, &pinfo->pbuff_lens);
                }

                if (image_quality <= 0)
                {
                    pinfo->pbuff_lens = 0;
                }

                memcpy(pinfo->pic_buff, image_pic, pinfo->pbuff_lens);

                CHECKED_FREE(image_pic, SIZE_YUV420(img_w, img_h));
            }
#else
            {
                uint8_t *image_pic = NULL;

                CHECKED_MALLOC(image_pic, SIZE_YUV420(img_w, ((img_h + 15) & 0xFFF0)), CACHE_SIZE);

                memcpy(image_pic, pinfo->pic_buff, SIZE_YUV420(img_w, img_h));

                // 采用libjpeg后编码一帧D1图像需要6ms（与原来相比节约了4ms）
                yuv420_to_jpeg_buffer(pinfo->pic_buff, image_pic, img_w, img_h, pa->img_quality, &pinfo->pbuff_lens);

                CHECKED_FREE(image_pic, SIZE_YUV420(img_w, ((img_h + 15) & 0xFFF0)));
            }
#endif
#endif // JPEG_OUPUT
        }
    }

    return err_ret;
}

// 图像大小发生变化后调用
void lc_plate_analysis_recreate(PlateAnalysis *pa, int32_t img_w, int32_t img_h)
{
    CHECKED_FREE(pa->srcyuv420, SIZE_YUV420(pa->img_w, pa->img_h));
//    CHECKED_FREE(pa->pic_buff);
    CHECKED_FREE(pa->pinfo->pic_buff, SIZE_YUV420(pa->img_w, pa->img_h));

    pa->img_w = img_w;
    pa->img_h = img_h;

    // 不管进来的是什么格式的图像，最后都要转成YUV420格式来分析
    CHECKED_MALLOC(pa->srcyuv420, SIZE_YUV420(img_w, img_h), CACHE_SIZE);
//    CHECKED_MALLOC(pa->pic_buff, SIZE_YUV420(img_w, img_h), CACHE_SIZE);
    CHECKED_MALLOC(pa->pinfo->pic_buff, SIZE_YUV420(img_w, img_h), CACHE_SIZE);

    lc_plate_location_recreate(pa->loc, img_w, img_h);
    lc_plate_tracking_recreate(pa->trk, img_w, img_h);
}

int32_t lc_plate_analysis_wait(Plate_handle *plate_handle, uint8_t *src_img,
                               int32_t img_w, int32_t img_h, int32_t img_size)
{
    uint8_t *yuv420 = NULL;
    PlateAnalysis *pa = NULL;
    PlateInfo *pinfo = NULL;
#ifdef WIN32
    int32_t video_format;
    int32_t img_hsize = 0;
#endif
//  FILE *f;
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (*plate_handle == NULL)
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
        return err_ret;
    }

    pa = (PlateAnalysis *)(*plate_handle);
    pinfo = pa->pinfo;

#ifdef WIN32
    video_format = pa->video_format;

    if (video_format == LC_VIDEO_FORMAT_BMP)
    {

    }
    else if (video_format == LC_VIDEO_FORMAT_JPG)
    {
        img_hsize = 0;
    }

#endif

    // 如果图像的大小发生了变化，系统重启并使用系统默认的参数
    if (pa->img_w != img_w || pa->img_h != img_h)
    {
        lc_plate_analysis_recreate(pa, img_w, img_h);
    }

#ifdef _TMS320C6X_
    yuv420 = src_img;
#else
    yuv420 = pa->srcyuv420;
    memcpy(yuv420, src_img, img_w * img_h * 3 / 2);
//     image_conver_to_yuv420(yuv420, yuv420 + img_w * img_h, yuv420 + img_w * img_h * 5 / 4,
//                            img_w, img_h, img_w, src_img + img_hsize, img_w, video_format, 0, img_size);
#endif

#if 0
    f = fopen("H:\\bmp_test\\1.yuv", "wb");

    if (f != NULL)
    {
        fwrite(yuv420, SIZE_YUV420(img_w, img_h), 1, f);
        fclose(f);
    }

#endif

    err_ret = lc_plate_analysis_proce(plate_handle, yuv420, pinfo, pa->linfo);

    return err_ret;
}

#ifdef PALTE_THREADS
int32_t __stdcall lc_plate_analysis_thread(Plate_handle* plate_handle)
{
    PlateAnalysis *pa = (PlateAnalysis *)(*plate_handle);
    int32_t index;
    uint8_t bproce;
    uint8_t *yuv420 = NULL;
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateInfo *pinfo = pa->pinfo;
    PLATECALLBACK PlateCallBack = pa->plateCallBack;

#if DEBUG_THREAD
    printf("lc_plate_analysis_thread start ID : %d!\n", pa->hThread);
#endif

    while (pa->bThread > (uint8_t)0)
    {
        index = pa->index_proce;

        WaitForSingleObject(pa->handle_wait, INFINITE);
        bproce = pa->frame_flag[index];
        SetEvent(pa->handle_wait);

        if (bproce == (uint8_t)0)
        {
            // 等待写视频函数的消息通知
            WaitForSingleObject(pa->handle_frame, INFINITE);
            continue;
        }

        yuv420 = pa->frame420[index];

        // 需要等待参数设置完成才能开始处理
        WaitForSingleObject(pa->handle_param, INFINITE);
        err_ret = lc_plate_analysis_proce(plate_handle, yuv420, pinfo, pa->linfo);
        SetEvent(pa->handle_param);

        if (pinfo->save_num > 0)
        {
            (*PlateCallBack)(err_ret, pa->context);

            if (err_ret != LC_PLATE_SUCCESS)
            {
                pa->bThread = (uint8_t)0;
            }
        }

        WaitForSingleObject(pa->handle_wait, INFINITE);
        pa->frame_flag[index] = (uint8_t)0;
        SetEvent(pa->handle_wait);

        pa->index_proce++;

        if (pa->index_proce >= NUM_FRAMES)
        {
            pa->index_proce = 0;
        }
    }

#if DEBUG_THREAD
    printf("lc_plate_analysis_thread over ID : %d!!!!!!!!!!!!!!!\n", pa->hThread);
#endif

    return err_ret;
}
#endif

#ifdef PALTE_THREADS
// 图像大小发生变化后调用
void lc_plate_analysis_recreate_no_wait(PlateAnalysis *pa, int32_t img_w, int32_t img_h)
{
    int32_t i;

    for (i = 0; i < NUM_FRAMES; i++)
    {
        CHECKED_FREE(pa->frame420[i], SIZE_YUV420(pa->img_w, pa->img_h));
    }

    for (i = 0; i < NUM_FRAMES; i++)
    {
        CHECKED_MALLOC(pa->frame420[i], SIZE_YUV420(img_w, img_h), CACHE_SIZE);
    }

    pa->index_save = 0;
    pa->index_proce = 0;

    return;
}

int32_t lc_plate_analysis_create_no_wait(Plate_handle *plate_handle, int32_t proce_type,
                                         int32_t img_w, int32_t img_h, PLATECALLBACK plateCallBack, void *context)
{
    int32_t i;
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = NULL;

    if (*plate_handle == NULL)
    {
        lc_plate_analysis_create_thread(plate_handle, proce_type, img_w, img_h);

        if (*plate_handle != NULL)
        {
            pa = (PlateAnalysis *)(*plate_handle);

            for (i = 0; i < NUM_FRAMES; i++)
            {
                CHECKED_MALLOC(pa->frame420[i], SIZE_YUV420(img_w, img_h), CACHE_SIZE);
            }

            pa->bThread = (uint8_t)1;
            pa->index_save = 0;
            pa->index_proce = 0;
            pa->context = context;
            pa->hThread = (unsigned long) - 1;
            pa->plateCallBack = plateCallBack;
            memset(pa->frame_flag, 0, sizeof(uint8_t) * NUM_FRAMES);

            pa->handle_wait = CreateEvent(NULL, FALSE, TRUE, NULL);
            pa->handle_frame = CreateEvent(NULL, FALSE, FALSE, NULL);
            pa->handle_param = CreateEvent(NULL, FALSE, TRUE, NULL);

            if (pa->handle_wait == NULL || pa->handle_frame == NULL || pa->handle_param == NULL)
            {
#if DEBUG_THREAD
                printf("create_no_wait handle == NULL\n");
#endif
                lc_plate_analysis_destroy(plate_handle);

                *plate_handle = NULL;
                err_ret = LC_PLATE_ERR_CREATE_HANDLE;
            }
            else
            {
                pa->hThread = _beginthreadex(NULL, (uint32_t)0, lc_plate_analysis_thread, plate_handle, (uint32_t)0, NULL);

                if (pa->hThread == (unsigned long)0)
                {
#if DEBUG_THREAD
                    printf("create_no_wait hThread == -1\n");
#endif
                    lc_plate_analysis_destroy(plate_handle);

                    *plate_handle = NULL;
                    err_ret = LC_PLATE_ERR_CREATE_HANDLE;
                }
            }
        }
        else
        {
            err_ret = LC_PLATE_ERR_INVALID_HANDLE;

#if DEBUG_THREAD
            printf("create_no_wait plate_handle == NULL\n");
#endif

#ifdef PLATE_KEY
            WaitForSingleObject(sfnt_check.hCheckLock, INFINITE);

            if (threadsnum > 0)
            {
                threadsnum--;
            }

            SetEvent(sfnt_check.hCheckLock);
#endif
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;

#ifdef PLATE_KEY
        WaitForSingleObject(sfnt_check.hCheckLock, INFINITE);

        if (threadsnum > 0)
        {
            threadsnum--;
        }

        SetEvent(sfnt_check.hCheckLock);
#endif
    }

    return err_ret;
}
#endif

#ifdef PALTE_THREADS
int32_t lc_plate_analysis_no_wait(Plate_handle* plate_handle, uint8_t *src_img,
                                  int32_t img_w, int32_t img_h, int32_t img_size)
{
    uint8_t bproce;
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = NULL;
    uint8_t *yuv420 = NULL;
    int32_t video_format;
    int32_t img_hsize = 0;
//  FILE *f = NULL;

    if (*plate_handle != NULL)
    {
        pa = (PlateAnalysis *)(*plate_handle);

        if (pa->bThread == (uint8_t)0)
        {
            err_ret = LC_PLATE_ERR_THREAD_CHECK;
        }
        else
        {
            if (pa->img_w != img_w || pa->img_h != img_h)
            {
                //err_ret = LC_PLATE_ERR_VIDEO_SIZE;
                WaitForSingleObject(pa->handle_param, INFINITE);
                WaitForSingleObject(pa->handle_wait, INFINITE);

                lc_plate_analysis_recreate_no_wait(pa, img_w, img_h);
                lc_plate_analysis_recreate(pa, img_w, img_h);

                SetEvent(pa->handle_wait);
                SetEvent(pa->handle_param);
            }

            WaitForSingleObject(pa->handle_wait, INFINITE);
            bproce = pa->frame_flag[pa->index_save];
            SetEvent(pa->handle_wait);

            if (bproce == (uint8_t)0)
            {
//                 img_w = pa->img_w;
//                 img_h = pa->img_h;

                video_format = pa->video_format;

                if (video_format == LC_VIDEO_FORMAT_BMP)
                {
                    img_hsize = bmp_decode_header(src_img, img_size, &img_w, &img_h, &video_format);

                    if (img_hsize <= 0)
                    {
                        err_ret = LC_PLATE_ERR_VIDEO_FORMAT;
                        return err_ret;
                    }
                    else if (pa->img_w != img_w || pa->img_h != img_h)
                    {
                        err_ret = LC_PLATE_ERR_VIDEO_SIZE;
                        return err_ret;
                    }
                }
                else if (video_format == LC_VIDEO_FORMAT_JPG)
                {
                    img_hsize = 0;
                }

                yuv420 = pa->frame420[pa->index_save];

                image_conver_to_yuv420(yuv420, yuv420 + img_w * img_h, yuv420 + img_w * img_h * 5 / 4,
                                       img_w, img_h, img_w, src_img + img_hsize, img_w, video_format, 0, img_size);

#if 0
                f = fopen("H:\\bmp_test\\33.yuv", "wb");

                if (f != NULL)
                {
                    fwrite(yuv420, SIZE_YUV420(img_w, img_h), 1, f);
                    fclose(f);
                }

#endif

                WaitForSingleObject(pa->handle_wait, INFINITE);
                pa->frame_flag[pa->index_save] = (uint8_t)1;
                SetEvent(pa->handle_wait);
                SetEvent(pa->handle_frame);

                pa->index_save++;

                if (pa->index_save >= NUM_FRAMES)
                {
                    pa->index_save = 0;
                }
            }
            else
            {
                err_ret = LC_PLATE_ERR_BUFF_OVERFLOW;
            }
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}
#endif

int32_t lc_plate_analysis(Plate_handle *plate_handle, uint8_t *src_img,
                          int32_t img_w, int32_t img_h, int32_t img_size)
{
    PlateAnalysis *pa = NULL;
    int32_t err_ret = LC_PLATE_SUCCESS;

    if (plate_handle != NULL)
    {
        pa = (PlateAnalysis *)(*plate_handle);

        switch (pa->proce_type)
        {
            case LC_PLATE_PROC_WAIT_PIC:
            case LC_PLATE_PROC_WAIT_VIDEO:
                err_ret = lc_plate_analysis_wait(plate_handle, src_img, img_w, img_h, img_size);
                break;
#ifdef PALTE_THREADS

            case LC_PLATE_PROC_NOWAIT_PIC:
            case LC_PLATE_PROC_NOWAIT_VIDEO:
                err_ret = lc_plate_analysis_no_wait(plate_handle, src_img, img_w, img_h, img_size);
                break;
#endif

            default:
                err_ret = LC_PLATE_ERR_INVALID_HANDLE;
                break;
        }
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

int32_t lc_plate_analysis_destroy(Plate_handle plate_handle)
{
    int32_t err_ret = LC_PLATE_SUCCESS;
    PlateAnalysis *pa = NULL;
    int32_t i;

    if (plate_handle != NULL)
    {
        pa = (PlateAnalysis *)plate_handle;

#ifdef PALTE_THREADS

        if (pa->proce_type == LC_PLATE_PROC_NOWAIT_PIC || pa->proce_type == LC_PLATE_PROC_NOWAIT_VIDEO)
        {
#if DEBUG_THREAD
            printf("lc_plate_analysis_destroy_no_wait ID : %d???????????\n", pa->hThread);
#endif
            pa->bThread = (unsigned long)0;

            if (pa->hThread != (unsigned long) - 1)
            {
                if (pa->handle_frame != NULL)
                {
                    SetEvent(pa->handle_frame);
                }

                WaitForSingleObject((HANDLE)pa->hThread, INFINITE);
            }

            if (pa->handle_wait != NULL)
            {
                CloseHandle(pa->handle_wait);
            }

            if (pa->handle_frame != NULL)
            {
                CloseHandle(pa->handle_frame);
            }

            if (pa->handle_param != NULL)
            {
                CloseHandle(pa->handle_param);
            }

#if DEBUG_THREAD
            printf("lc_plate_analysis_destroy_no_wait ID : %d@@@@@@@@@@@\n", pa->hThread);
#endif

            for (i = 0; i < NUM_FRAMES; i++)
            {
                CHECKED_FREE(pa->frame420[i], SIZE_YUV420(pa->img_w, pa->img_h));
            }
        }

#endif

        if (location_setup)
        {
            lc_plate_location_destroy(pa->loc, pa->img_w, pa->img_h);
        }

        if (recognition_setup)
        {
            lc_plate_recognition_destroy(pa->reg);

            if (pa->proce_type == LC_PLATE_PROC_WAIT_VIDEO || pa->proce_type == LC_PLATE_PROC_NOWAIT_VIDEO)
            {
                lc_plate_tracking_destroy(pa->trk, pa->img_w, pa->img_h);
            }
        }

        CHECKED_FREE(pa->linfo, sizeof(LocationInfo));
        CHECKED_FREE(pa->pinfo->pic_buff, SIZE_YUV420(pa->img_w, pa->img_h));

        for (i = 0; i < RECTS_NUM; i++)
        {
            CHECKED_FREE(pa->pinfo->pic_small[i], SIZE_YUV420(PLATE_MAX_WIDTH, PLATE_MAX_HEIGHT));
            CHECKED_FREE(pa->pinfo->pic_window[i], SIZE_YUV420(MIN2(pa->img_w, PLATE_MAX_WIDTH * 7), MIN2(pa->img_h, PLATE_MAX_HEIGHT * 25)));
        }

        CHECKED_FREE(pa->pinfo, sizeof(PlateInfo));
        CHECKED_FREE(pa->srcyuv420, SIZE_YUV420(pa->img_w, pa->img_h));
//        CHECKED_FREE(pa->pic_buff);
        CHECKED_FREE(pa, sizeof(PlateAnalysis));

#ifdef PLATE_KEY
        WaitForSingleObject(sfnt_check.hCheckLock, INFINITE);

        if (threadsnum > 0)
        {
            threadsnum--;
        }

        SetEvent(sfnt_check.hCheckLock);
#endif

#if DEBUG_THREAD
        printf("lc_plate_analysis_destroy\n");
#endif
    }
    else
    {
        err_ret = LC_PLATE_ERR_INVALID_HANDLE;
    }

    return err_ret;
}

int32_t lc_plate_analysis_create(Plate_handle *plate_handle, int32_t proce_type, int32_t img_w,
                                 int32_t img_h, PLATECALLBACK plateCallBack, void *context)
{
    int32_t err_ret = LC_PLATE_SUCCESS;

#if DEBUG_THREAD
    int32_t fd;
    FILE *file = NULL;

    AllocConsole();
    fd = _open_osfhandle((long)GetStdHandle((unsigned long) - 11), _O_TEXT);
    file = _fdopen(fd, "w");
    *stdout = *file;
    setvbuf(stdout, 0, _IONBF, 0);
#endif

    if (img_w <= 0 || img_h <= 0 || proce_type < LC_PLATE_PROC_WAIT_PIC || proce_type > LC_PLATE_PROC_NOWAIT_VIDEO)
    {
        err_ret = LC_PLATE_ERR_PARAM;
        return err_ret;
    }

    if (plateCallBack == NULL && (proce_type == LC_PLATE_PROC_NOWAIT_PIC || proce_type == LC_PLATE_PROC_NOWAIT_VIDEO))
    {
        err_ret = LC_PLATE_ERR_PARAM;
        return err_ret;
    }

#ifdef PLATE_KEY
    WaitForSingleObject(sfnt_check.hCheckLock, INFINITE);

    if (threadsnum >= sfnt_check.threadnum)
    {
        SetEvent(sfnt_check.hCheckLock);

#if DEBUG_THREAD
        printf("lc_plate_analysis_create thread limit: %d\n", sfnt_check.threadnum);
#endif
        err_ret = LC_PLATE_ERR_THREADNUM_LIMIT;
        return err_ret;
    }
    else
    {
#if DEBUG_THREAD
        printf("lc_plate_analysis_create threadsnum: %d\n", threadsnum);
#endif
        threadsnum++;
    }

    SetEvent(sfnt_check.hCheckLock);
#endif // PLATE_KEY

#if DEBUG_THREAD
    printf("lc_plate_analysis_create proce_type: %d\n", proce_type);
#endif

    switch (proce_type)
    {
        case LC_PLATE_PROC_WAIT_PIC:
        case LC_PLATE_PROC_WAIT_VIDEO:
            lc_plate_analysis_create_thread(plate_handle, proce_type, img_w, img_h);
            break;
#ifdef PALTE_THREADS

        case LC_PLATE_PROC_NOWAIT_PIC:
        case LC_PLATE_PROC_NOWAIT_VIDEO:
            err_ret = lc_plate_analysis_create_no_wait(plate_handle, proce_type, img_w, img_h, plateCallBack, context);
            break;
#endif

        default:
            err_ret = LC_PLATE_FAIL;
            break;
    }

    return err_ret;
}
