#ifndef yuv_osd_string_h__
#define yuv_osd_string_h__

#ifdef __cplusplus
extern "C"
{
#endif

    /********************************************************************
     Function:      osd_mask_string
     Description:   YUV420p图像OSD叠加

     1. Param:
        szString    字符串指针
        ixPos       字符串叠加起始位置的横坐标
        *iyPos      字符串叠加起始位置的纵坐标（将返回下次叠加字符串的纵坐标）
        *yuvData    YUV420p图像数据
        yuvWidth    图像宽度
        yuvHeight   图像高度
        colorR      叠加字符串颜色的红色分量
        colorG      叠加字符串颜色的绿色分量
        colorB      叠加字符串颜色的蓝色分量

     2. Return:
        void
    ********************************************************************/
    void osd_mask_string(char *szString, int ixPos, int *iyPos, unsigned char *yuvData, int yuvWidth,
                         int yuvHeight, unsigned char colorR, unsigned char colorG, unsigned char colorB);

#ifdef __cplusplus
};
#endif

#endif // yuv_osd_string_h__
