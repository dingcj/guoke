#include "ParkingGuidance.h"
#include "algoParkingGuidance.h"

algoParkingGuidance* m_algo = nullptr;

int ParkingGudanceInit()
{
    m_algo = new algoParkingGuidance;
    
    return m_algo->algoInit();
}

int ParkingGudanceConfig(PARKING_GUIDANCE_CONFIG* config)
{
    if (m_algo != nullptr)
    {
        return m_algo->algoConfig(config);
    }
    else
    {
        return -1;
    }
}

int ParkingGudanceProcess(unsigned char* pbNV21, int nImgW, int nImgH)
{
    if (m_algo != nullptr)
    {
        return m_algo->algoProcess(pbNV21, nImgW, nImgH);
    }
    else
    {
        return -1;
    }
}

int ParkingGudanceGetInfo(PARKING_GUIDANCE_OUT_INFO* pOutInfo)
{
    if (m_algo != nullptr)
    {
        return m_algo->algoGetInfo(pOutInfo);
    }
    else
    {
        return -1;
    }
}

int ParkingGudanceRelease()
{
    if (m_algo != nullptr)
    {
        int nErrFlag = m_algo->algoRelease();
        if (nErrFlag != 0)
        {
            return nErrFlag;
        }
        else
        {
            delete m_algo;
            return 0;
        }
    }
    else
    {
        return -1;
    }
}