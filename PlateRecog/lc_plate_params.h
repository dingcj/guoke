#ifndef _LC_PLATE_PARAMS_H_
#define _LC_PLATE_PARAMS_H_

#ifdef PALTE_THREADS
#include <windows.h>
#include <process.h>
#endif

#include "portab.h"
#include "location_interface.h"
#include "recognition_interface.h"
#include "plate_tracking_src/plate_tracking.h"

#define RECTS_NUM   (5)           // 每帧图像中检测车牌的最大数目
#define NUM_FRAMES  (25)

#define PLATE_MAX_WIDTH  400      // 定位的车牌小图片的宽度
#define PLATE_MAX_HEIGHT 160      // 定位的车牌小图片的高度

/********************************************************************
 车牌字符chs的编码规则:
 如果chs>=100说明是汉字，例如chs=136对应汉字"云"，chs=150对应汉字"甘"
 如果chs<100说明是数字或字母，chs存放的是数字和大写字母对应的ASCII码
********************************************************************/
typedef struct
{
    int32_t plate_num;              // 当前帧检测得到的车牌数目
    Rects   plate_list[RECTS_NUM];  // 当前帧检测得到的车牌在原始图像中的位置
    Rects   plate_rect[RECTS_NUM];  // 车牌在抓拍的车牌图片中的位置
    Rects   car_window_rect[RECTS_NUM]; //车窗在抓拍图片中的位置
    uint8_t *buf_flage;             // 抓拍的车牌图片0: 最后一张图片 1: 最清晰的一张图片

    uint8_t chs[RECTS_NUM][CH_NUMS];    // 识别出来的车牌字符代码（为支持绿牌，将字符数修改为8，最后一个是字符串结束符）
    int8_t  character[RECTS_NUM][2 + 5 + 2 + 1]; // 识别出来的车牌字符的字符串（注意：最后一个车牌字符也有可能是汉字，汉字占2个字符，最后一个是字符串结束符）
    uint8_t chs_trust[RECTS_NUM][CH_NUMS];  // 单个车牌字符的置信度
    uint8_t trust[RECTS_NUM];       // 车牌字符的置信度
    uint8_t color[RECTS_NUM];       // 车牌颜色 0:蓝底 1:黄底 2:白底－军牌 3:白底－警牌 4:黑牌 10:新能源车牌99:未知
    uint8_t type[RECTS_NUM];        // 车牌类型 1:大型汽车号牌 2:小型汽车号牌 3:新能源车牌 16:教练汽车号牌 23:警用汽车号牌 99:未知
    uint8_t ch_num[RECTS_NUM];      // 确定车牌字符数目
    uint8_t brightness[RECTS_NUM];  // 车牌亮度值1 ~ 255

    int32_t count[RECTS_NUM];       // 车牌计数值，同一车牌在视频中出现的次数
    uint8_t directions[RECTS_NUM];  // 车牌运动方向 0:未知方向 1:车牌从上往下运动 2:车牌从下往上运动
    uint8_t speeds[RECTS_NUM][2];   // 车牌运动速度
    uint8_t car_color[RECTS_NUM];   // 车身颜色 0:白色 1:银灰 2:黄色 3:粉色 4:红色 5:紫色 6:绿色 7:蓝色 8:棕色 9:黑色 99:未知 255:未识别
    uint8_t car_color_trust[RECT_MAX];  //车身颜色置信度
    int32_t save_num;               // 用户需要保存的车牌号数目
    uint8_t *pic_buff;              // 抓拍的车牌图片(目前是JPG格式)的起始地址
    uint8_t *pic_small[RECTS_NUM];  // 抓拍的车牌图片中定位的车牌小图片(目前是JPG格式)的起始地址
    uint8_t *pic_window[RECTS_NUM]; // 抓拍的车牌图片中定位的车窗区域小图片(目前的JPG格式)的起始地址
    int32_t pbuff_lens;             // 抓拍的车牌图片的大小(单位是字节)
    int32_t psmall_lens[RECTS_NUM]; // 抓拍的车牌图片中定位的车牌小图片的大小(单位是字节)
    int32_t pwindow_lens[RECTS_NUM];// 抓拍的车牌图片中定位的车窗区域小图片的大小(单位是字节)
} PlateInfo;

typedef struct
{
    int32_t       img_w;                     // 图像宽度
    int32_t       img_h;                     // 图像高度

    int32_t       video_format;              // 视频格式
    uint8_t       position_flag;             // 是否设置车牌红色矩形框 0: 不设置(默认) 1: 设置
    uint8_t       car_color_enable;          // 是否设置车身颜色识别 0:不识别(默认) 1:识别

    int32_t       location_enable;
    int32_t       recognition_enable;
    int32_t       tracking_enable;
    uint8_t       video_flag;                 // 视频(或图片)标识
    uint8_t       trust_thresh;               // 车牌识别置信度的阈值

    Loc_handle    loc;                        // 车牌定位句柄
    Reg_handle    reg;                        // 车牌字符识别句柄
    Trk_handle    trk;                        // 车牌跟踪句柄
//  Rects         *plate_pos;                 // 用户需要保存的图片中车牌的位置
    uint8_t       *srcyuv420;                 // 用户待处理的图片(yuv420格式，输入图像都要转换成yuv420格式)
//  uint8_t       *pic_buff;                  // 用户需要保存的图片(抓拍的车牌图片，目前是JPG格式)
    int32_t       img_quality;                // 抓拍车牌图片(JPG)的压缩质量，1 ~ 100，值越大，质量越好、图片的文件也越大

#ifdef PALTE_THREADS
    unsigned long hThread;                    // 视频处理线程句柄
    uint8_t       bThread;                    // 视频处理线程运行标志
    int32_t       index_save;                 // 缓存中视频帧的保存索引
    int32_t       index_proce;                // 缓存中视频帧的处理索引
    HANDLE        handle_wait;                // 用于等待读写frame_flag标志
    HANDLE        handle_frame;               // 用于通知视频处理线程有视频需要处理
    uint8_t       frame_flag[NUM_FRAMES];     // 用于标志缓存中的视频帧是否处理
    uint8_t       *frame420[NUM_FRAMES];      // 用于缓存视频帧
    PLATECALLBACK plateCallBack;              // 用户回调函数
    void          *context;                   // 用户上下文指针
    HANDLE        handle_param;               // 用于缓存方式中等待设置参数
#endif

    int32_t       proce_type;                 // 0.无缓存图片处理，1.无缓存视频处理，2.有缓存图片处理，3.有缓存视频处理
    PlateInfo     *pinfo;                     // 识别结果信息
    LocationInfo  *linfo;                     // 车牌定位信息

    uint8_t       OSD_flag;                   // 是否在图片上叠加车牌字符
    int32_t       OSD_pos_x;                  // 车牌字符叠加位置
    int32_t       OSD_pos_y;                  // 车牌字符叠加位置
    double        OSD_scale;                  // 叠加车牌缩放比例
    uint8_t       deskew_flag;                // 是否对车牌做倾斜校正
    uint8_t       car_window_enable;          // 是否开启车窗区域识别 0: 不识别 1: 识别
} PlateAnalysis;


#endif  // _LC_PLATE_PARAMS_H_

