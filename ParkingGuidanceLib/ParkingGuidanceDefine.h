#ifndef _PARKING_GUIDANCE_DEFINE_H_
#define _PARKING_GUIDANCE_DEFINE_H_

#include <stdint.h>

#ifndef MIN
#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#define MAX_PLACE_NUM   (10)    // ���λ��
#define MAX_PLATE_NUM   (10)    // �������
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
    int nPlaceNum;                              // ��λ����
    POINT_P stPlaceCoord[MAX_PLACE_NUM * 4];      // ��λλ�����꣬ÿ����λλ�ÿ����ı��Σ���4�����ʾ
    char pDefaultProvince[4];                   // Ĭ��ʡ�ݴ���

} PARKING_GUIDANCE_CONFIG;

typedef struct _PARKING_SINGLE_INFO_ 
{
    int bIsAvailible;       // ��λ�Ƿ���Ч   0: ��Ч   1: ��Ч
    int bIsVehicle;         // ��λ�Ƿ��г�   0: �޳�   1: �г�
    int bIsPlate;           // �Ƿ��⵽���� 0: û��⵽  1: ��⵽
    char pcPlateInfo[10];   // ������Ϣ
    RECT_P rtPlate;         // ����λ��    
    unsigned char plateType;
    unsigned char plateReliability;

} PARKING_SINGLE_INFO;

typedef struct _PARKING_GUIDANCE_OUT_INFO_ 
{
    PARKING_SINGLE_INFO stParkInfo[MAX_PLACE_NUM];
} PARKING_GUIDANCE_OUT_INFO;

#endif
