#ifndef _PARKING_GUIDANCE_H_
#define _PARKING_GUIDANCE_H_

#include "ParkingGuidanceDefine.h"

#ifdef  __cplusplus  
extern "C" {
#endif 

    int ParkingGudanceInit();
    int ParkingGudanceConfig(PARKING_GUIDANCE_CONFIG* config);
    int ParkingGudanceProcess(unsigned char* pbNV21, int nImgW, int nImgH);
    int ParkingGudanceGetInfo(PARKING_GUIDANCE_OUT_INFO* pOutInfo);
    int ParkingGudanceRelease();

#ifdef  __cplusplus  
}
#endif

#endif