#include "VehicleDetectManage.h"
#include "VehicleDetect.h"

VehicleDetect* m_vehicleDetect = nullptr;

int VehicleDetectManageInit()
{
    m_vehicleDetect = new VehicleDetect;
    
    return m_vehicleDetect->VehicleDetectInit();
}

int VehicleDetectManageConfig()
{
    if (m_vehicleDetect != nullptr)
    {
        return m_vehicleDetect->VehicleDetectConfig();
    }
    else
    {
        return -1;
    }
}

int VehicleDetectManageProcess(unsigned char* pbImg, int nImgW, int nImgH)
{
    if (m_vehicleDetect != nullptr)
    {
        return m_vehicleDetect->VehicleDetectProcess(pbImg, nImgW, nImgH);
    }
    else
    {
        return -1;
    }
}

int VehicleDetectManageGetInfo(int& nVehicleNum, RECT_V* pstVehicleRect)
{
    if (m_vehicleDetect != nullptr)
    {
        return m_vehicleDetect->VehicleDetectGetInfo(nVehicleNum, pstVehicleRect);
    }
    else
    {
        return -1;
    }
}

int VehicleDetectManageRelease()
{
    if (m_vehicleDetect != nullptr)
    {
        int nErrFlag = m_vehicleDetect->VehicleDetectRelease();
        if (nErrFlag != 0)
        {
            return nErrFlag;
        }
        else
        {
            delete m_vehicleDetect;
            return 0;
        }
    }
    else
    {
        return -1;
    }
}
