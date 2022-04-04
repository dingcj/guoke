#ifndef PARKING_GUIDANCE_IO_H
#define PARKING_GUIDANCE_IO_H

#include "ParkingGuidanceDefine.h"

#ifdef _EXPORT_LIB_
#define DLL_API  __declspec(dllexport)
#else
#define DLL_API  //__declspec(dllimport)
#endif

#ifdef  __cplusplus  
extern "C" {
#endif 

/*************************************/
// 算法初始化
/*************************************/
DLL_API int ParkingGuidanceIOInit();

/*************************************/
// 算法配置
/*************************************/
DLL_API int ParkingGuidanceIOConfig(PARKING_GUIDANCE_CONFIG* config);

/*************************************/
// 算法处理主函数
/*************************************/
DLL_API int ParkingGuidanceIOProcess(unsigned char* pbNV21, int nImgW, int nImgH);

/*************************************/
// 算法结果获取
/*************************************/
DLL_API int ParkingGuidanceIOGetInfo(PARKING_GUIDANCE_OUT_INFO* pOutInfo);

/*************************************/
// 算法释放
/*************************************/
DLL_API int ParkingGuidanceIORelease();

#ifdef  __cplusplus  
}
#endif

#endif // !PARKING_GUIDANCE_IO_H