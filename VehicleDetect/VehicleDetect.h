#ifndef _VEHICLE_DETECT_H_
#define _VEHICLE_DETECT_H_

#include <vector>
#include "VehicleDetectDefine.h"
#include "net.h"
using namespace std;

class VehicleDetect
{
public:
    VehicleDetect();
    ~VehicleDetect();

    int VehicleDetectInit();
    int VehicleDetectConfig();
    int VehicleDetectProcess(unsigned char* pbImg, int nImgW, int nImgH);
    int VehicleDetectGetInfo(int& nVehicleNum, RECT_V* pstVehicleRect);
    int VehicleDetectRelease();


private:
    ncnn::Net m_netVehicleDet;

    int m_nDetSize = 224;
    float pfMean[3] = { 127.5f, 127.5f, 127.5f };
    float pfNorm[3] = { 0.007843f, 0.007843f, 0.007843f };  // 1/127.5

    vector<RECT_V> vDetInfo;
};

#endif
