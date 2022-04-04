#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "sample_comm.h"


HI_S32 SAMPLE_2BPP_DrawLine_CheckParam(HI_U8 *pVirAddr,SIZE_S *pstSize,SAMPLE_LINE_S *pstLine)
{
    if(pstLine->u32Thick == 0){
        SAMPLE_PRT("u32Thick is illegal!\n");
        return HI_FAILURE;
    }

    if( (pstLine->bDisplay != HI_TRUE) && (pstLine->bDisplay != HI_FALSE) ){
        SAMPLE_PRT("bDisplay is illegal!\n");
        return HI_FAILURE;
    }

    if( pstLine->stPoint1.s32X < 0 || pstLine->stPoint2.s32X < 0 ||
        pstLine->stPoint1.s32Y < 0 || pstLine->stPoint2.s32Y < 0){
        SAMPLE_PRT("The coordinates of the points cannot be less than 0!\n");
        return HI_FAILURE;
    }

    if( pstLine->stPoint1.s32X >= pstSize->u32Width ||
        pstLine->stPoint2.s32X >= pstSize->u32Width){
        SAMPLE_PRT("Coordinate x cannot be greater than width-1!\n");
        return HI_FAILURE;
    }

    if( pstLine->stPoint1.s32Y >= pstSize->u32Height ||
        pstLine->stPoint2.s32Y >= pstSize->u32Height){
        SAMPLE_PRT("Coordinate y cannot be greater than height-1!\n");
        return HI_FAILURE;
    }

    if( (pstLine->stPoint1.s32X == pstLine->stPoint2.s32X) &&
        (pstLine->stPoint1.s32Y == pstLine->stPoint2.s32Y)){
        SAMPLE_PRT("stPoint1 is valued to stPoint2!\n");
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}


HI_S32 SAMPLE_2BPP_DrawLine_Vertical(HI_U8 *pVirAddr,SIZE_S *pstSize,SAMPLE_LINE_S *pstLine)
{
    POINT_S *pStartPoint, *pEndPoint;
    HI_S32 i;
    HI_U8 *pu8Temp;
    HI_U8 point_in_bit;
    HI_U32 cnt;

    /* Vertical line, all x are the same, y increment */
    if(pstLine->stPoint1.s32Y < pstLine->stPoint2.s32Y){
         pStartPoint = &pstLine->stPoint1;
         pEndPoint   = &pstLine->stPoint2;
    }
    else{
         pStartPoint = &pstLine->stPoint2;
         pEndPoint   = &pstLine->stPoint1;
    }

    if(pStartPoint->s32X + pstLine->u32Thick > pstSize->u32Width){
        SAMPLE_PRT("(s32X + u32Thick) is illegal!\n");
        return HI_FAILURE;
    }

    for (i = pStartPoint->s32Y; i < pEndPoint->s32Y;i++)
    {
        pu8Temp = pVirAddr + (pStartPoint->s32X + i*pstSize->u32Width)/4;
        point_in_bit = pStartPoint->s32X % 4;

        for(cnt=0; cnt<pstLine->u32Thick; cnt++)
        {
            if(pstLine->bDisplay == HI_TRUE){
                *pu8Temp = *pu8Temp | (0x1 << (2*point_in_bit +1));
            }
            else{
                *pu8Temp = *pu8Temp & ~(0x1 << (2*point_in_bit +1));
            }
            point_in_bit++;
            if(0 == point_in_bit%4)
            {
                pu8Temp++;
                point_in_bit = 0;
            }
        }
    }

    return HI_SUCCESS;
}



HI_S32 SAMPLE_2BPP_DrawLine_Horizontal(HI_U8 *pVirAddr,SIZE_S *pstSize,SAMPLE_LINE_S *pstLine)
{

    POINT_S *pStartPoint, *pEndPoint;
    HI_S32 i;
    HI_U8 *pu8Temp;
    HI_U8 point_in_bit;
    HI_U32 x1_in_bytes, x2_in_bytes;

    /* Horizontal line, all x are the same, y increment */

    if(pstLine->stPoint1.s32X < pstLine->stPoint2.s32X){
        pStartPoint = &pstLine->stPoint1;
        pEndPoint   = &pstLine->stPoint2;
    }
    else{
        pStartPoint = &pstLine->stPoint1;
        pEndPoint   = &pstLine->stPoint1;
    }
    x1_in_bytes = ALIGN_UP( (pStartPoint->s32X + pStartPoint->s32Y*pstSize->u32Width), 4)/4;
    pu8Temp = pVirAddr + (pStartPoint->s32X + pStartPoint->s32Y*pstSize->u32Width)/4;
    point_in_bit = pStartPoint->s32X%4;
    if(point_in_bit == 1){
        if(pstLine->bDisplay == HI_TRUE){
            *pu8Temp = *pu8Temp | 0xa8;
        }
        else{
            *pu8Temp = *pu8Temp & ~0xa8;
        }
    }
    else if(point_in_bit == 2){
        if(pstLine->bDisplay == HI_TRUE){
            *pu8Temp = *pu8Temp | 0xa0;
        }
        else{
            *pu8Temp = *pu8Temp & ~0xa0;
        }
    }
    else if(point_in_bit == 3){
        if(pstLine->bDisplay == HI_TRUE){
            *pu8Temp = *pu8Temp | 0x80;
        }
        else{
            *pu8Temp = *pu8Temp & ~0x80;
        }
    }
    else{
    }

    if(pEndPoint->s32X + pstLine->u32Thick >= pstSize->u32Width)
    {
        x2_in_bytes = (pstSize->u32Width + pEndPoint->s32Y*pstSize->u32Width)/4;
    }
    else{
        x2_in_bytes = ALIGN_DOWN( (pEndPoint->s32X + pstLine->u32Thick + pEndPoint->s32Y*pstSize->u32Width), 4)/4;
        pu8Temp = pVirAddr + (pEndPoint->s32X + pstLine->u32Thick + pEndPoint->s32Y*pstSize->u32Width)/4;
        point_in_bit = pEndPoint->s32X%4;
        if(point_in_bit == 1){
            if(pstLine->bDisplay == HI_TRUE){
                *pu8Temp = *pu8Temp | 0x2;
            }
            else{
                *pu8Temp = *pu8Temp & ~0x2;
            }
        }
        else if(point_in_bit == 2){
            if(pstLine->bDisplay == HI_TRUE){
                *pu8Temp = *pu8Temp | 0xa;
            }
            else{
                *pu8Temp = *pu8Temp & ~0xa;
            }
        }
        else if(point_in_bit == 3){
            if(pstLine->bDisplay == HI_TRUE){
                *pu8Temp = *pu8Temp | 0x2a;
            }
            else{
                *pu8Temp = *pu8Temp & ~0x2a;
            }
        }
        else{
        }
    }
    for(i= x1_in_bytes; i<x2_in_bytes; i++ ){
        pu8Temp  = pVirAddr + i;
        if(pstLine->bDisplay == HI_TRUE){
            *pu8Temp = *pu8Temp | 0xaa;
        }
        else{
            *pu8Temp = *pu8Temp & ~0xaa;
        }
        pu8Temp++;
    }

    return HI_SUCCESS;
}





HI_S32 SAMPLE_2BPP_DrawLine_LowSlope(HI_U8 *pVirAddr,SIZE_S *pstSize,SAMPLE_LINE_S *pstLine)
{
    POINT_S *pStartPoint, *pEndPoint;
    HI_FLOAT k, b, x;
    HI_S32 i;
    HI_U8 *pu8Temp;
    HI_U8 point_in_bit;
    HI_S32 s32PointY;

    if(pstLine->stPoint1.s32X < pstLine->stPoint2.s32X){
        pStartPoint = &pstLine->stPoint1;
        pEndPoint   = &pstLine->stPoint2;
    }
    else{
        pStartPoint = &pstLine->stPoint2;
        pEndPoint   = &pstLine->stPoint1;
    }
    k = (HI_FLOAT)(pEndPoint->s32Y - pStartPoint->s32Y)/(HI_FLOAT)(pEndPoint->s32X - pStartPoint->s32X);
    b = pStartPoint->s32Y - k * pStartPoint->s32X;

    s32PointY    = pStartPoint->s32Y;
    point_in_bit = pStartPoint->s32X%4;
    for (i = pStartPoint->s32X; i < pEndPoint->s32X;)
    {
        HI_U32 cnt;
        HI_FLOAT y;

        pu8Temp = pVirAddr + (i + s32PointY*pstSize->u32Width)/4;
        point_in_bit = i % 4;
        for(cnt=0; cnt<pstLine->u32Thick; cnt++)
        {
            if(i + cnt >= pstSize->u32Width){
                break;
            }
            if(pstLine->bDisplay == HI_TRUE){
                *pu8Temp = *pu8Temp | (0x1 << (2*point_in_bit +1));
            }
            else{
                *pu8Temp = *pu8Temp & ~(0x1 << (2*point_in_bit +1));
            }
            point_in_bit++;
            if(0 == point_in_bit%4)
            {
                pu8Temp++;
                point_in_bit = 0;
            }
        }
        i++;
        x = i;
        y = k*x + b;
        s32PointY = (HI_S32)y;
        if(s32PointY < 0)
        {
            SAMPLE_PRT("s32PointY is illegal!\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}




HI_S32 SAMPLE_2BPP_DrawLine_HighSlope(HI_U8 *pVirAddr, SIZE_S *pstSize, SAMPLE_LINE_S *pstLine)
{
    POINT_S *pStartPoint, *pEndPoint;
    HI_FLOAT k, b, x;
    HI_S32 i;
    HI_U8 *pu8Temp;
    HI_U8 point_in_bit;
    HI_S32 s32PointX;

    if(pstLine->stPoint1.s32Y < pstLine->stPoint2.s32Y){
        pStartPoint = &pstLine->stPoint1;
        pEndPoint   = &pstLine->stPoint2;
    }
    else{
        pStartPoint = &pstLine->stPoint2;
        pEndPoint   = &pstLine->stPoint1;
    }
    k = (HI_FLOAT)(pEndPoint->s32Y - pStartPoint->s32Y)/(HI_FLOAT)(pEndPoint->s32X - pStartPoint->s32X);
    b = pStartPoint->s32Y - k * pStartPoint->s32X;

    s32PointX    = pStartPoint->s32X;
    point_in_bit = pStartPoint->s32X%4;
    for (i = pStartPoint->s32Y; i < pEndPoint->s32Y;)
    {
        HI_U32 cnt;
        HI_FLOAT y;

        pu8Temp = pVirAddr + (s32PointX + i*pstSize->u32Width)/4;
        point_in_bit = s32PointX % 4;
        for(cnt=0; cnt<pstLine->u32Thick; cnt++)
        {
            if(s32PointX + cnt >= pstSize->u32Width){
                break;
            }
            if(pstLine->bDisplay == HI_TRUE){
                *pu8Temp = *pu8Temp | (0x1 << (2*point_in_bit +1));
            }
            else{
                *pu8Temp = *pu8Temp & ~(0x1 << (2*point_in_bit +1));
            }
            point_in_bit++;
            if(0 == point_in_bit%4)
            {
                pu8Temp++;
                point_in_bit = 0;
            }
        }
        i++;
        y = i;
        x = (y - b)/k;
        s32PointX = (HI_S32)x;
        if(s32PointX < 0)
        {
            SAMPLE_PRT("s32PointX is illegal!\n");
            return HI_FAILURE;
        }
    }

    return HI_SUCCESS;
}



HI_S32 SAMPLE_2BPP_DrawLine(HI_U8 *pVirAddr, SIZE_S *pstSize, SAMPLE_LINE_S *pstLine)
{
    HI_S32 ret;

    ret = SAMPLE_2BPP_DrawLine_CheckParam(pVirAddr, pstSize, pstLine);
    if(ret != HI_SUCCESS){
        return HI_FAILURE;
    }

    if( (pstLine->stPoint1.s32X == pstLine->stPoint2.s32X) &&
        (pstLine->stPoint1.s32Y != pstLine->stPoint2.s32Y)){
        return SAMPLE_2BPP_DrawLine_Vertical(pVirAddr, pstSize, pstLine);
    }
    else if( (pstLine->stPoint1.s32X != pstLine->stPoint2.s32X) &&
             (pstLine->stPoint1.s32Y == pstLine->stPoint2.s32Y)){
        return SAMPLE_2BPP_DrawLine_Horizontal(pVirAddr, pstSize, pstLine);
    }
    else if(ABS(pstLine->stPoint2.s32Y - pstLine->stPoint1.s32Y) >=
            ABS(pstLine->stPoint2.s32X - pstLine->stPoint1.s32X)){
        return SAMPLE_2BPP_DrawLine_HighSlope(pVirAddr, pstSize, pstLine);
    }
    else
    {
        return SAMPLE_2BPP_DrawLine_LowSlope(pVirAddr, pstSize, pstLine);
    }

    return HI_SUCCESS;
}




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

