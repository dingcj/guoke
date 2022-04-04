#ifndef _ALGO_PARING_GUIDANCE_H_
#define _ALGO_PARING_GUIDANCE_H_

#ifdef WIN32
#include "opencv2/opencv.hpp"
//using namespace cv;
#endif
#include <vector>
#include "VehicleDetectManage.h"
#include "lc_plate_analysis.h"
#include "ParkingGuidanceDefine.h"

#define MAX_VOTE_FRAME_NUM  (10)

using namespace std;

typedef struct _PLATE_INFO_ 
{
    RECT_P rtPlatePos;
    char   pcPlateName[10];
    uint8_t plateType;
    uint8_t plateReliability;
} PLATE_INFO;

class algoParkingGuidance
{
public:
    algoParkingGuidance();
    ~algoParkingGuidance();

    int algoInit();
    int algoConfig(PARKING_GUIDANCE_CONFIG* config);
    int algoProcess(unsigned char* pbNV21, int nImgW, int nImgH);
    int algoGetInfo(PARKING_GUIDANCE_OUT_INFO* pOutInfo);
    int algoRelease();

private:
    void GetRectPlate(int nImgW, int nImgH, POINT_P* pCoord, RECT_P& rtPlate);
    void RectNV21toYUV420(unsigned char* pbNV21, int nImgW, int nImgH, unsigned char* pbYUVPlace, RECT_P rtPlace);
    int IntegrateInfo(int nVehicleNum, RECT_V* pstRectObj, vector<PLATE_INFO>* pPlateInfo, PARKING_GUIDANCE_CONFIG* config);

private:
    PARKING_GUIDANCE_CONFIG stConfig;

    vector<PLATE_INFO> pstPlateInfo[MAX_PLACE_NUM];

    vector<PARKING_GUIDANCE_OUT_INFO> m_pParkingOutInfo;
};

#endif