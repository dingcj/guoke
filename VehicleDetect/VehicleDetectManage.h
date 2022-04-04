#ifndef _VEHICLE_DETECT_MANAGE_H_
#define _VEHICLE_DETECT_MANAGE_H_

#include "VehicleDetectDefine.h"

#ifdef _EXPORT_VEHICLE_LIB_
#define DLL_VEHICLE_API  __declspec(dllexport)
#else
#define DLL_VEHICLE_API  //__declspec(dllimport)
#endif

#ifdef  __cplusplus  
extern "C" {
#endif 

    DLL_VEHICLE_API int VehicleDetectManageInit();
    DLL_VEHICLE_API int VehicleDetectManageConfig();
    DLL_VEHICLE_API int VehicleDetectManageProcess(unsigned char* pbImg, int nImgW, int nImgH);
    DLL_VEHICLE_API int VehicleDetectManageGetInfo(int& nVehicleNum, RECT_V* pstVehicleRect);
    DLL_VEHICLE_API int VehicleDetectManageRelease();

#ifdef  __cplusplus  
}
#endif

#endif