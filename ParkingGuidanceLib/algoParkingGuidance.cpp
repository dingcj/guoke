#include<string.h>
#include "algoParkingGuidance.h"

algoParkingGuidance::algoParkingGuidance()
{

}

algoParkingGuidance::~algoParkingGuidance()
{

}

int algoParkingGuidance::algoInit()
{
    // 车辆检测初始化
    //VehicleDetectManageInit();
    VehicleDetectManageInit();
    VehicleDetectManageConfig();

    // 车牌识别初始化

    return 0;
}

int algoParkingGuidance::algoConfig(PARKING_GUIDANCE_CONFIG* config)
{
    memcpy(&stConfig, config, sizeof(PARKING_GUIDANCE_CONFIG));

    // 车辆检测配置
    //VehicleDetectManageConfig();

    return 0;
}

void resize_NV21_BGR(unsigned char* pbNV21, int nImgSrcW, int nImgSrcH, unsigned char* pbBGR, int nDstW, int nDstH)
{
    int32_t w, h;

    for (h = 0; h < nDstH; h++)
    {
        int nH = h * nImgSrcH / nDstH;
        for (w = 0; w < nDstW; w++)
        {
            int nW = w * nImgSrcW / nDstW;

            unsigned char y = pbNV21[nH * nImgSrcW + nW];
            unsigned char u = pbNV21[nImgSrcW * nImgSrcH + nH / 2 * nImgSrcW + nW / 2 * 2 + 1];
            unsigned char v = pbNV21[nImgSrcW * nImgSrcH + nH / 2 * nImgSrcW + nW / 2 * 2 + 0];

            int cb = (int32_t)u - (int32_t)128;
            int cr = (int32_t)v - (int32_t)128;

            int y_1 = y << 16;
            int r = (y_1 + cr * 92241) / 65536;
            int g = (y_1 - cb * 25559 - cr * 38010) / 65536;
            int b = (y_1 + cb * 133038) / 65536;

            r = r > 255 ? 255 : r;
            g = g > 255 ? 255 : g;
            b = b > 255 ? 255 : b;

            r = r < 0 ? 0 : r;
            g = g < 0 ? 0 : g;
            b = b < 0 ? 0 : b;

            pbBGR[h * nDstW * 3 + w * 3 + 0] = b;
            pbBGR[h * nDstW * 3 + w * 3 + 1] = g;
            pbBGR[h * nDstW * 3 + w * 3 + 2] = r;
        }
    }

    return;
}

int algoParkingGuidance::algoProcess(unsigned char* pbNV21, int nImgW, int nImgH)
{
    // 车辆检测
    unsigned char* pbBGR = new unsigned char[224 * 224 * 3];
    resize_NV21_BGR(pbNV21, nImgW, nImgH, pbBGR, 224, 224);
    VehicleDetectManageProcess(pbBGR, 224, 224);

    int nVehicleNum = 0;
    RECT_V* pstRectObj = new RECT_V[MAX_VEHICLE_DET_NUM];
    VehicleDetectManageGetInfo(nVehicleNum, pstRectObj);

    for (int i = 0; i < nVehicleNum; i++)
    {
        pstRectObj[i].x0 = pstRectObj[i].x0 * nImgW / 224;
        pstRectObj[i].x1 = pstRectObj[i].x1 * nImgW / 224;
        pstRectObj[i].y0 = pstRectObj[i].y0 * nImgH / 224;
        pstRectObj[i].y1 = pstRectObj[i].y1 * nImgH / 224;
    }

    delete[] pbBGR;

//     {
//         cv::Mat imShow(nImgH, nImgW, CV_8UC1);
//         memcpy(imShow.data, pbNV21, nImgW * nImgH);
//         cv::cvtColor(imShow, imShow, CV_GRAY2BGR);
//         for (int i = 0; i < nVehicleNum; i++)
//         {
//             int ww = pstRectObj[i].x1 - pstRectObj[i].x0 + 1;
//             int hh = pstRectObj[i].y1 - pstRectObj[i].y0 + 1;
//             cv::rectangle(imShow, cv::Rect(pstRectObj[i].x0, pstRectObj[i].y0, ww, hh), cv::Scalar(0, 255, 0), 2);
//         }
// 
//         cv::resize(imShow, imShow, cv::Size(nImgW / 2, nImgH / 2));
//         cv::imshow("show_", imShow);
//         cv::waitKey(0);
//     }

    // 车牌识别(由于每次按车位进行车牌识别，所以每次分别初始化，初始化耗时较小，可以暂不考虑对效率的影响)
    for (int i = 0; i < stConfig.nPlaceNum; i++)
    {
        pstPlateInfo[i].clear();

        RECT_P rtPlace;
        GetRectPlate(nImgW, nImgH, stConfig.stPlaceCoord + i * 4, rtPlace);

        int nPlaceWidth = rtPlace.x1 - rtPlace.x0 + 1;
        int nPlaceHeight = rtPlace.y1 - rtPlace.y0 + 1;

        unsigned char* pbYUVPlace = new unsigned char[nPlaceWidth * nPlaceHeight * 3 / 2];
        RectNV21toYUV420(pbNV21, nImgW, nImgH, pbYUVPlace, rtPlace);

        Plate_handle plate_handle = NULL;
        int proce_type = LC_PLATE_PROC_WAIT_PIC;
        lc_plate_analysis_create(&plate_handle, proce_type, nPlaceWidth, nPlaceHeight, NULL, &plate_handle);
        lc_plate_set_chinese_default(plate_handle, stConfig.pDefaultProvince);
        lc_plate_set_chinese_trust_thresh(plate_handle, 70);
        lc_plate_set_deskew_flag(plate_handle, 1);

        lc_plate_analysis(&plate_handle, pbYUVPlace, nPlaceWidth, nPlaceHeight, nPlaceWidth * nPlaceHeight * 3 / 2);

        int ret = 0;
        lc_plate_get_current_plate_number(plate_handle, &ret);
        for (int num = 0; num < ret; num++)
        {
            PLATE_INFO pInfo;
            Rects plate_pos;
            lc_plate_get_current_plate_position(plate_handle, num, &plate_pos);
            pInfo.rtPlatePos.x0 = plate_pos.x0 + rtPlace.x0;
            pInfo.rtPlatePos.x1 = plate_pos.x1 + rtPlace.x0;
            pInfo.rtPlatePos.y0 = plate_pos.y0 + rtPlace.y0;
            pInfo.rtPlatePos.y1 = plate_pos.y1 + rtPlace.y0;

            int8_t* plate_name = NULL;
			uint8_t plateType = 0u;
			uint8_t plateReliability = 0u;
			lc_plate_get_plate_color_id(plate_handle, num, &plateType);
			lc_plate_get_plate_reliability(plate_handle, num, &plateReliability);
            lc_plate_get_plate_name(plate_handle, num, &plate_name);

			pInfo.plateType = plateType;
			pInfo.plateReliability = plateReliability;
            memcpy(pInfo.pcPlateName, plate_name, 10);

            pstPlateInfo[i].push_back(pInfo);
        }

        lc_plate_analysis_destroy(plate_handle);

        delete[] pbYUVPlace;
    }

    // 统计车辆检测和车牌识别的信息
    IntegrateInfo(nVehicleNum, pstRectObj, pstPlateInfo, &stConfig);

    delete[] pstRectObj;

    return 0;
}

void algoParkingGuidance::GetRectPlate(int nImgW, int nImgH, POINT_P* pCoord, RECT_P& rtPlate)
{
    int minX = nImgW - 1;
    int maxX = 0;
    int minY = nImgH - 1;
    int maxY = 0;
    for (int i = 0; i < 4; i++)
    {
        minX = MIN(minX, pCoord[i].x);
        maxX = MAX(maxX, pCoord[i].x);
        minY = MIN(minY, pCoord[i].y);
        maxY = MAX(maxY, pCoord[i].y);
    }

    if (1)
    {
        rtPlate.x0 = minX;
        rtPlate.x1 = minX + (maxX - minX + 1) / 16 * 16 - 1;
        rtPlate.y0 = minY;
        rtPlate.y1 = minY + (maxY - minY + 1) / 16 * 16 - 1;
    }
    else
    {
        minX = MAX(0, minX - 50);
        maxX = MIN(nImgW - 1, maxX + 66);
        int width = (maxX - minX + 1) / 16 * 16;

        int height = maxY - minY + 1;
        minY = minY + height / 3;
        maxY = MIN(nImgH - 1, maxY + 66);
        height = (maxY - minY + 1) / 16 * 16;
        while (width * height > 500000)
        {
            height -= 16;
        }

        rtPlate.x0 = minX;
        rtPlate.x1 = minX + width - 1;
        rtPlate.y0 = maxY - height + 1;
        rtPlate.y1 = maxY;
    }

    return;
}

void algoParkingGuidance::RectNV21toYUV420(unsigned char* pbNV21, int nImgW, int nImgH, unsigned char* pbYUVPlace, RECT_P rtPlace)
{
    int nPlaceW = rtPlace.x1 - rtPlace.x0 + 1;
    int nPlaceH = rtPlace.y1 - rtPlace.y0 + 1;
    unsigned char* pbPlaceU = pbYUVPlace + nPlaceW * nPlaceH;
    unsigned char* pbPlaceV = pbYUVPlace + nPlaceW * nPlaceH * 5 / 4;
    unsigned char* pbUV = pbNV21 + nImgW * nImgH;

    for (int h = 0; h < nPlaceH; h++)
    {
        for (int w = 0; w <nPlaceW; w++)
        {
            pbYUVPlace[h * nPlaceW + w] = pbNV21[(h + rtPlace.y0) * nImgW + w + rtPlace.x0];
            pbPlaceU[(h / 2) * nPlaceW / 2 + w / 2] = pbUV[(h + rtPlace.y0) / 2 * nImgW + (w + rtPlace.x0) / 2 * 2 + 1];
            pbPlaceV[(h / 2) * nPlaceW / 2 + w / 2] = pbUV[(h + rtPlace.y0) / 2 * nImgW + (w + rtPlace.x0) / 2 * 2 + 0];
        }
    }

    return;
}

bool RectIntersect(RECT_V obj0, RECT_V obj1)
{
    int x0 = MAX(obj0.x0, obj1.x0);
    int x1 = MIN(obj0.x1, obj1.x1);
    int y0 = MAX(obj0.y0, obj1.y0);
    int y1 = MIN(obj0.y1, obj1.y1);

    if (x1 - x0 > 0 && y1 - y0 > 0)
    {
        int area0 = (obj0.x1 - obj0.x0 + 1) * (obj0.y1 - obj0.y0 + 1);
        int area1 = (obj1.x1 - obj1.x0 + 1) * (obj1.y1 - obj1.y0 + 1);
        int area = (x1 - x0 + 1) * (y1 - y0 + 1);

        if (area > area0 * 0.3 || area > area1 * 0.3)
        {
            return true;
        }
    }
    
    return false;
}

int algoParkingGuidance::IntegrateInfo(int nVehicleNum, RECT_V* pstRectObj, vector<PLATE_INFO>* pPlateInfo, PARKING_GUIDANCE_CONFIG* config)
{
    PARKING_GUIDANCE_OUT_INFO pParkingOutInfo;
    memset(&pParkingOutInfo, 0, sizeof(PARKING_GUIDANCE_OUT_INFO));

    for (int i = 0; i < config->nPlaceNum; i++)
    {
        RECT_V center;
        RECT_V rectangleOut;
        POINT_P* pPt = config->stPlaceCoord + i * 4;
        rectangleOut.x0 = MIN(MIN(pPt[0].x, pPt[1].x), MIN(pPt[2].x, pPt[3].x));
        rectangleOut.x1 = MAX(MAX(pPt[0].x, pPt[1].x), MAX(pPt[2].x, pPt[3].x));
        rectangleOut.y0 = MIN(MIN(pPt[0].y, pPt[1].y), MIN(pPt[2].y, pPt[3].y));
        rectangleOut.y1 = MAX(MAX(pPt[0].y, pPt[1].y), MAX(pPt[2].y, pPt[3].y));

        center.x0 = rectangleOut.x0 + (rectangleOut.x1 - rectangleOut.x0 + 1) / 4 - 1;
        center.x1 = rectangleOut.x0 + (rectangleOut.x1 - rectangleOut.x0 + 1) / 4 * 3 - 1;
        center.y0 = rectangleOut.y0 + (rectangleOut.y1 - rectangleOut.y0 + 1) / 4 - 1;
        center.y1 = rectangleOut.y0 + (rectangleOut.y1 - rectangleOut.y0 + 1) / 4 * 3 - 1;

        pParkingOutInfo.stParkInfo[i].bIsAvailible = 1;
        for (int j = 0; j < nVehicleNum; j++)
        {
            bool bIntersect = RectIntersect(center, pstRectObj[j]);
            if (bIntersect)
            {
                pParkingOutInfo.stParkInfo[i].bIsVehicle = 1;
                break;
            }
        }

        if (pPlateInfo[i].size() > 0)
        {
            int dist = rectangleOut.x1 - rectangleOut.x0 + 1;
            for (int j = 0; j < pPlateInfo[i].size(); j++)
            {
                int center_plate = (pPlateInfo[i][j].rtPlatePos.x1 + pPlateInfo[i][j].rtPlatePos.x0) / 2;
                int center_place = (rectangleOut.x1 + rectangleOut.x0) / 2;
                //if (abs(center_plate - center_place) < dist)
                {
                    memcpy(pParkingOutInfo.stParkInfo[i].pcPlateInfo, pPlateInfo[i][j].pcPlateName, 10);
                    pParkingOutInfo.stParkInfo[i].rtPlate = pPlateInfo[i][j].rtPlatePos;
					pParkingOutInfo.stParkInfo[i].plateType = pPlateInfo[i][j].plateType;
					pParkingOutInfo.stParkInfo[i].plateReliability = pPlateInfo[i][j].plateReliability;
                    pParkingOutInfo.stParkInfo[i].bIsPlate = 1;
                }
            }
        }
    }

    if (m_pParkingOutInfo.size() >= MAX_VOTE_FRAME_NUM)
    {
        m_pParkingOutInfo.erase(m_pParkingOutInfo.begin());
        m_pParkingOutInfo.push_back(pParkingOutInfo);
    }
    else
    {
        m_pParkingOutInfo.push_back(pParkingOutInfo);
    }

    return 0;
}

void VotePlateName(vector<char*> pcPlate, PARKING_SINGLE_INFO* pInfo)
{
    if (pcPlate.size() < 1)
    {
        pInfo->bIsPlate = 0;
        memset(pInfo->pcPlateInfo, 0, 10);
        return;
    }

    for (int i = 0; i < 10; i++)
    {
        int num = pcPlate.size();
        vector<char> voteInfo;
        vector<int>  voteCnt;
        for (int j = 0; j < num; j++)
        {
            bool bMatch = false;
            for (int n = 0; n < voteInfo.size(); n++)
            {
                if (pcPlate[j][i] == voteInfo[n])
                {
                    voteCnt[n]++;
                    bMatch = true;
                    break;
                }
            }

            if (bMatch ==false)
            {
                voteInfo.push_back(pcPlate[j][i]);
                voteCnt.push_back(1);
            }
        }

        int nMaxCnt = 0;
        for (int j = 0; j < voteCnt.size(); j++)
        {
            if (voteCnt[j] > nMaxCnt)
            {
                nMaxCnt = voteCnt[j];
                pInfo->pcPlateInfo[i] = voteInfo[j];
            }
        }
    }

    pInfo->bIsPlate = 1;
    for (int i = 0; i < 6; i++)
    {
        if (pInfo->pcPlateInfo[i] == 0)
        {
            pInfo->bIsPlate = 0;
            memset(pInfo->pcPlateInfo, 0, 10);
            break;
        }
    }

    return;
}

int algoParkingGuidance::algoGetInfo(PARKING_GUIDANCE_OUT_INFO* pOutInfo)
{
    memset(pOutInfo, 0, sizeof(PARKING_GUIDANCE_OUT_INFO));

    for (int i = 0; i < stConfig.nPlaceNum; i++)
    {
        int nVehicleCnt = 0;
        vector<char*> pcPlate;
        for (int j = 0; j < m_pParkingOutInfo.size(); j++)
        {
            nVehicleCnt += m_pParkingOutInfo[j].stParkInfo[i].bIsVehicle == true;
            if (m_pParkingOutInfo[j].stParkInfo[i].bIsPlate)
            {
                pcPlate.push_back(m_pParkingOutInfo[j].stParkInfo[i].pcPlateInfo);
            }
        }

        VotePlateName(pcPlate, &pOutInfo->stParkInfo[i]);

        if (nVehicleCnt > m_pParkingOutInfo.size() / 2)
        {
            pOutInfo->stParkInfo[i].bIsVehicle = 1;
        }
        else
        {
            pOutInfo->stParkInfo[i].bIsVehicle = 0;
        }

        if (pOutInfo->stParkInfo[i].bIsPlate == 1)
        {
            PARKING_SINGLE_INFO stLast = m_pParkingOutInfo[m_pParkingOutInfo.size() - 1].stParkInfo[i];

            if (stLast.bIsPlate == 1)
            {
                pOutInfo->stParkInfo[i].rtPlate = stLast.rtPlate;
            }
            else
            {
                pOutInfo->stParkInfo[i].bIsPlate = 0;
                memset(pOutInfo->stParkInfo[i].pcPlateInfo, 0, 10);
            }
            pOutInfo->stParkInfo[i].bIsVehicle = 1;
        }

        /* 如果连续3帧都没检测到车，则认为车已经离开 */
        if (m_pParkingOutInfo.size() >= 3)
        {
            int nInfoNum = m_pParkingOutInfo.size();
            int nNoVehicle = 0;
            for (int j = 0; j < 3; j++)
            {
                nNoVehicle += m_pParkingOutInfo[nInfoNum - j - 1].stParkInfo[i].bIsVehicle == 0;
            }
            if (nNoVehicle >= 3)
            {
                pOutInfo->stParkInfo[i].bIsVehicle = 0;
                pOutInfo->stParkInfo[i].bIsPlate = 0;
                memset(pOutInfo->stParkInfo[i].pcPlateInfo, 0, 10);
            }
        }

        pOutInfo->stParkInfo[i].bIsAvailible = 1;
    }

    return 0;
}

int algoParkingGuidance::algoRelease()
{
    m_pParkingOutInfo.clear();
    pstPlateInfo->clear();

    //VehicleDetectManageRelease();
    VehicleDetectManageRelease();

    return 0;
}