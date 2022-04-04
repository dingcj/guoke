#include "ParkingGuiddanceIO.h"
#include "ParkingGuidance.h"

DLL_API int ParkingGuidanceIOInit()
{
    return ParkingGudanceInit();
}

DLL_API int ParkingGuidanceIOConfig(PARKING_GUIDANCE_CONFIG* config)
{
    return ParkingGudanceConfig(config);
}

DLL_API int ParkingGuidanceIOProcess(unsigned char* pbNV21, int nImgW, int nImgH)
{
    return ParkingGudanceProcess(pbNV21, nImgW, nImgH);
}

DLL_API int ParkingGuidanceIOGetInfo(PARKING_GUIDANCE_OUT_INFO* pOutInfo)
{
    return ParkingGudanceGetInfo(pOutInfo);
}

DLL_API int ParkingGuidanceIORelease()
{
    return ParkingGudanceRelease();
}