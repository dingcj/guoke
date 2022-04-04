#ifndef yuv_osd_string_h__
#define yuv_osd_string_h__

#ifdef __cplusplus
extern "C"
{
#endif

    /********************************************************************
     Function:      osd_mask_string
     Description:   YUV420pͼ��OSD����

     1. Param:
        szString    �ַ���ָ��
        ixPos       �ַ���������ʼλ�õĺ�����
        *iyPos      �ַ���������ʼλ�õ������꣨�������´ε����ַ����������꣩
        *yuvData    YUV420pͼ������
        yuvWidth    ͼ����
        yuvHeight   ͼ��߶�
        colorR      �����ַ�����ɫ�ĺ�ɫ����
        colorG      �����ַ�����ɫ����ɫ����
        colorB      �����ַ�����ɫ����ɫ����

     2. Return:
        void
    ********************************************************************/
    void osd_mask_string(char *szString, int ixPos, int *iyPos, unsigned char *yuvData, int yuvWidth,
                         int yuvHeight, unsigned char colorR, unsigned char colorG, unsigned char colorB);

#ifdef __cplusplus
};
#endif

#endif // yuv_osd_string_h__
