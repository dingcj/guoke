#include "VehicleDetect.h"
//#include "opencv2/opencv.hpp"
//using namespace cv;

VehicleDetect::VehicleDetect()
{

}

VehicleDetect::~VehicleDetect()
{
    
}

int VehicleDetect::VehicleDetectInit()
{
    m_netVehicleDet.opt.use_packing_layout = true;
    m_netVehicleDet.opt.use_bf16_storage = true;
    m_netVehicleDet.load_param("vehicle0415.param");
    m_netVehicleDet.load_model("vehicle0415.bin");

    return 0;
}

int VehicleDetect::VehicleDetectConfig()
{
    return 0;
}

int VehicleDetect::VehicleDetectProcess(unsigned char* pbImg, int nImgW, int nImgH)
{
    vDetInfo.clear();

    ncnn::Mat ncnnData = ncnn::Mat::from_pixels_resize(pbImg, ncnn::Mat::PIXEL_BGR, nImgW, nImgH, m_nDetSize, m_nDetSize);
    ncnnData.substract_mean_normalize(pfMean, pfNorm);

    ncnn::Extractor ncnnExt = m_netVehicleDet.create_extractor();

    ncnnExt.input("data", ncnnData);

    ncnn::Mat ncnnOut;
    ncnnExt.extract("detection_out", ncnnOut);
    float* pfData = (float*)ncnnOut.data;
    for (int i = 0; i < min(ncnnOut.h, MAX_VEHICLE_DET_NUM); i++)
    {
        int label = pfData[i * 6 + 0];
        float score = pfData[i * 6 + 1];
        RECT_V rtObj;
        rtObj.x0 = pfData[i * 6 + 2] * nImgW;
        rtObj.y0 = pfData[i * 6 + 3] * nImgH;
        rtObj.x1 = pfData[i * 6 + 4] * nImgW;
        rtObj.y1 = pfData[i * 6 + 5] * nImgH;
        if (score > 0.9)
        {
            vDetInfo.push_back(rtObj);
        }
    }

//     {
//         cv::Mat imShow(nImgH, nImgW, CV_8UC1);
//         memcpy(imShow.data, pbImg, nImgW * nImgH);
//         cv::cvtColor(imShow, imShow, CV_GRAY2BGR);
//         for (int i = 0; i < vDetInfo.size(); i++)
//         {
//             int ww = vDetInfo[i].x1 - vDetInfo[i].x0 + 1;
//             int hh = vDetInfo[i].y1 - vDetInfo[i].y0 + 1;
//             cv::rectangle(imShow, cv::Rect(vDetInfo[i].x0, vDetInfo[i].y0, ww, hh), cv::Scalar(0, 255, 0), 2);
//         }
// 
//         cv::resize(imShow, imShow, cv::Size(nImgW / 2, nImgH / 2));
//         cv::imshow("show_", imShow);
//         cv::waitKey(0);
//     }

    return 0;
}

int VehicleDetect::VehicleDetectGetInfo(int& nVehicleNum, RECT_V* pstVehicleRect)
{
    nVehicleNum = vDetInfo.size();
    for (int i = 0; i < min(nVehicleNum, MAX_VEHICLE_DET_NUM); i++)
    {
        pstVehicleRect[i] = vDetInfo[i];
    }

    return 0;
}

int VehicleDetect::VehicleDetectRelease()
{
    m_netVehicleDet.clear();
    vDetInfo.clear();

    return 0;
}
