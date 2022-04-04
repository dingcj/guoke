#ifndef _PARKING_GUIDANCE_DEFINE_H_
#define _PARKING_GUIDANCE_DEFINE_H_

#include <stdint.h>

#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#define MAX_PLACE_NUM   (10)    // 最大车位数
#define MAX_PLATE_NUM   (10)    // 最大车牌数
typedef struct _POINT_P_
{
    int x;
    int y;
} POINT_P;

typedef struct _RECT_P_
{
    int x0;
    int y0;
    int x1;
    int y1;
} RECT_P;

typedef struct _PARKING_GUIDANCE_CONFIG_ 
{
    int nPlaceNum;                              // 车位个数
    POINT_P stPlaceCoord[MAX_PLACE_NUM * 4];      // 车位位置坐标，每个车位位置框都是四边形，用4个点表示
    char pDefaultProvince[4];                   // 默认省份代码

} PARKING_GUIDANCE_CONFIG;

typedef struct _PARKING_SINGLE_INFO_ 
{
    int bIsAvailible;       // 车位是否有效   0: 无效   1: 有效
    int bIsVehicle;         // 车位是否有车   0: 无车   1: 有车
    int bIsPlate;           // 是否检测到车牌 0: 没检测到  1: 检测到
    char pcPlateInfo[10];   // 车牌信息
    RECT_P rtPlate;         // 车牌位置    
    unsigned char plateType;
    unsigned char plateReliability;

} PARKING_SINGLE_INFO;

typedef struct _PARKING_GUIDANCE_OUT_INFO_ 
{
    PARKING_SINGLE_INFO stParkInfo[MAX_PLACE_NUM];
} PARKING_GUIDANCE_OUT_INFO;

#endif
