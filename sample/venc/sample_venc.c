
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "sample_comm.h"
#include "ParkingGuiddanceIO.h"
#include "httpinterface.h"
#include "rtputils.h"
#include "cJSON.h"
#include "ringfifo.h"
#include "rtspservice.h"
#include "tcpServer.h"
#include "base64.h"
#include "shareHeader.h"
#include "watchdog.h"

#define BIG_STREAM_SIZE     PIC_2688x1944
#define SMALL_STREAM_SIZE   PIC_VGA


#define VB_MAX_NUM            10
#define ONLINE_LIMIT_WIDTH    2304

#define WRAP_BUF_LINE_EXT     416

const int MAX_PLATE_WIDTH = 256;
const int MAX_PLATE_HEIGHT = 80;

#define MAX_URL_LEN (256)
#define MAX_ID_LEN (64)

typedef struct hiSAMPLE_VPSS_ATTR_S
{
    SIZE_S            stMaxSize;
    DYNAMIC_RANGE_E   enDynamicRange;
    PIXEL_FORMAT_E    enPixelFormat;
    COMPRESS_MODE_E   enCompressMode[VPSS_MAX_PHY_CHN_NUM];
    SIZE_S            stOutPutSize[VPSS_MAX_PHY_CHN_NUM];
    FRAME_RATE_CTRL_S stFrameRate[VPSS_MAX_PHY_CHN_NUM];
    HI_BOOL           bMirror[VPSS_MAX_PHY_CHN_NUM];
    HI_BOOL           bFlip[VPSS_MAX_PHY_CHN_NUM];
    HI_BOOL           bChnEnable[VPSS_MAX_PHY_CHN_NUM];

    SAMPLE_SNS_TYPE_E enSnsType;
    HI_U32            BigStreamId;
    HI_U32            SmallStreamId;
    VI_VPSS_MODE_E    ViVpssMode;
    HI_BOOL           bWrapEn;
    HI_U32            WrapBufLine;
} SAMPLE_VPSS_CHN_ATTR_S;

typedef struct hiSAMPLE_VB_ATTR_S
{
    HI_U32            validNum;
    HI_U64            blkSize[VB_MAX_NUM];
    HI_U32            blkCnt[VB_MAX_NUM];
    HI_U32            supplementConfig;
} SAMPLE_VB_ATTR_S;

typedef struct PlaceInterfaceStru_ {
    char id[MAX_ID_LEN];
    int enable;
    int plateCfgState;
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
    int x4;
    int y4;
} PlaceInterfaceStru;

typedef struct ParkingGuidanceParamStru_
{
    PARKING_GUIDANCE_CONFIG algParam;
    
    char postJpegUrl[MAX_URL_LEN];
    int postJpegInterval;
    
    char postHeartBeatUrl[MAX_URL_LEN];
    int heartBeatInterval;

    char postion[MAX_ID_LEN];
    char ControlIP[MAX_ID_LEN];
    char ControledIP[MAX_ID_LEN];
    char LightSlaveIP[MAX_ID_LEN];
    char LightSlaveMac[MAX_ID_LEN];
    PlaceInterfaceStru placeCfgList[MAX_PLACE_NUM];
} ParkingGuidanceParamStru;

#define MAX_CHECK_NUM 10
#define PALTE_BUF_LEN 10
typedef struct CheckPlateCredibleItmList_ {
    char plateList[MAX_CHECK_NUM][PALTE_BUF_LEN];
    uint32_t insert;
}CheckPlateCredibleItmList;

typedef enum UPGRADE_STATE_ {
    UPG_NOT_START = 0,
    UPG_PROCESS,
    UPG_SUCCESS,
    UPG_FAILED
} UPGRADE_STATE;

typedef struct UpgradeState_ {
    pthread_mutex_t mutex;
    UPGRADE_STATE state;
    int progress;
    int fileSize;
    int currSize;
} UpgradeState;

typedef enum PARKING_PLACE_STATE_ {
    PARKING_PLACE_EMPTY,
    PARKING_PLACE_FULL,
    PARKING_PLACE_INIT
} PARKING_PLACE_STATE;

#define FLASHING_PERIOD 1
#define LIGHT_CTRL_GPIO_NUM 3
#define LIGHT_STATIC_STAT_NUM (LIGHT_OFF + 1)
typedef struct Gpio_ {
    char *gpio;
    char *value;
} Gpio;

typedef struct LightStaticState_ {
    Gpio gpio[LIGHT_CTRL_GPIO_NUM];
}LightStaticState;

typedef struct ColorState_ {
    uint32_t flashing;
    uint32_t flashClose;
    uint32_t period;
    uint32_t currentColor;
} ColorState;

typedef enum PLATE_CONFIG_STATE_ {
    PLATE_CONFIG_AUTO = 0,
    PLATE_CONFIG_EMPTY,
    PLATE_CONFIG_FULL
} PLATE_CONFIG_STATE;


extern void *rtsp_main(void *p);
extern void exit_rtsp();

int     g_ReplaceYuvData = HI_FALSE;
char    *g_ReplaceYuvFile = "./replace.yuv";

ParkingGuidanceParamStru g_Allconfig = {0};

const char *g_NetDeviceName = "eth0";

volatile int g_exitMainThread = 0;
volatile int g_exitPostThread = 0;
volatile int g_rebootSystem = HI_FALSE;

UpgradeState g_upgState;
const char *g_upgradeTmpFile = "/tmp/upgrade.zip";

volatile int g_triggerUploadPic = HI_FALSE;

static PARKING_PLACE_STATE g_prevParkingPlaceState = PARKING_PLACE_INIT;
static uint8_t g_parkingPlaceStateBitmap = 0u;

volatile ColorState g_colorState = {HI_FALSE, HI_FALSE, FLASHING_PERIOD, LIGHT_GREEN};
LightStaticState g_lightStateGpioValTbl[LIGHT_STATIC_STAT_NUM] = {
    {{{"69", "1"},{"70", "0"},{"4", "0"}}},   // LIGHT_RED
    {{{"69", "0"},{"70", "0"},{"4", "1"}}},   // LIGHT_GREEN
    {{{"69", "0"},{"70", "1"},{"4", "0"}}},   // LIGHT_BLUE
    {{{"69", "1"},{"70", "1"},{"4", "1"}}},   // LIGHT_WHITE
    {{{"69", "1"},{"70", "0"},{"4", "1"}}},   // LIGHT_YELLOW
    {{{"69", "1"},{"70", "1"},{"4", "0"}}},   // LIGHT_PURPLE
    {{{"69", "0"},{"70", "1"},{"4", "1"}}},   // LIGHT_CYANBLUE
    {{{"69", "0"},{"70", "0"},{"4", "0"}}},   // LIGHT_OFF
};

const int g_maxPostFifoLen = 4;
FifoRing *g_jpgePostFifo = NULL;
FifoRing *g_othPostFifo = NULL;
FifoRing *g_CtrLightFifo = NULL;

static char curPlateInfo[MAX_PLACE_NUM][10] = {0};

int     g_TestAlg = HI_FALSE;

#define WDT_DEV_FILE "/dev/watchdog"

typedef struct PostInfoItem {
    char *postUrl;
    char *postData;
} PostInfoItem;

void init_wdt()
{
    char buffer[256] = {0};
    const uint32_t margin = 180;
    snprintf(buffer, sizeof(buffer), "insmod /ko/gk7205v200_wdt.ko default_margin=%u nodeamon=1", margin);
    int ret = system(buffer);
    if (ret != 0) {
        SAMPLE_PRT("%s - %u\n", strerror(errno), ret);
    }
}

void feed_wdt()
{
    int fd = open(WDT_DEV_FILE, O_RDWR);
    if (fd < 0) {
        perror("feed_wdt open");
        return;
    }
    int ret = ioctl(fd, WDIOC_KEEPALIVE);
    if (ret != 0) {
        perror("ioctl err");
    }
    close(fd);
}

int WriteStringToFile(const char *str, const char *file)
{
    SAMPLE_PRT("STR: %s, file: %s\n", str, file);
    FILE *fp = fopen(file, "w");
    if (fp == NULL) {
        perror("WriteStringToFile");
        return -1;
    }

    fprintf(fp, "%s\n", str);
    fclose(fp);

    return 0;
}

void SetLightSateByGpio(uint32_t state)
{
    char buf[128] = {0};
    Gpio *gpio = g_lightStateGpioValTbl[state].gpio;
    for (int i = 0; i < LIGHT_CTRL_GPIO_NUM; i++) {
        WriteStringToFile(gpio[i].gpio, "/sys/class/gpio/export");
        snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%s/direction", gpio[i].gpio);
        WriteStringToFile("out", buf);
        snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%s/value", gpio[i].gpio);
        WriteStringToFile(gpio[i].value, buf);
    }
}
void SetLightState(uint32_t state)
{
    if (state >= LIGHT_RED && state <= LIGHT_OFF) {
        g_colorState.flashing = HI_FALSE;
        g_colorState.currentColor = state;
        SetLightSateByGpio(state);
    }

    if (state == LIGHT_FLASHING) {
        g_colorState.flashing = HI_TRUE;
    }
}

void ProcessLightFlash()
{
    static time_t prevFlashTime = 0;
    time_t now = CurrentTimestamp();
	uint32_t flashTimeOut = CheckIsTimeout(now, prevFlashTime, g_colorState.period);

    if (g_colorState.flashing && flashTimeOut) {
        if (g_colorState.flashClose) {
            SetLightSateByGpio(LIGHT_OFF);
            g_colorState.flashClose = HI_FALSE;
        } else {
            SetLightSateByGpio(g_colorState.currentColor);
            g_colorState.flashClose = HI_TRUE;
        }
        prevFlashTime = now;
    }
    return;
}


int ProcPostInfoToServer(FifoRing *fifo)
{
    void *tmp = NULL;
    int ret = fifo_ring_pop(fifo, &tmp);
    if (ret != 0) {
        return 0;
    }

    PostInfoItem *itm = (PostInfoItem *)tmp;
    //SAMPLE_PRT(itm->postData);
    if (strlen(itm->postUrl) != 0) {
        post_json_to_server(itm->postUrl, itm->postData, NULL, 0);
    }

    free(itm->postData);
    free(itm);

    return 1;
}

int GetSlaveParkingPlaceState(uint16_t *bitmap)
{
    int ret = PARKING_PLACE_FULL;
    char rspBuf[128] = {0};
    char url[128] = {0};
    cJSON *rootObj = NULL;

    *bitmap = MAX_PARKING_PLACE_NUM << 8;
    for (uint32_t i = 0; i < MAX_PARKING_PLACE_NUM; i++) {
        *bitmap |= (1u << i);
    }

    if (strlen(g_Allconfig.LightSlaveIP) == 0) {
        return PARKING_PLACE_FULL;
    }

    snprintf(url, sizeof(url), "http://%s:8080/cgi-bin/ctrcgi", g_Allconfig.LightSlaveIP);
    ret = post_json_to_server(url, "{\"com\":0}", rspBuf, sizeof(rspBuf));
    JUGE(ret == 0, PARKING_PLACE_FULL);

    rspBuf[sizeof(rspBuf) - 1] = '\0';
    SAMPLE_PRT("%s\n", rspBuf);

    rootObj = cJSON_Parse(rspBuf);
    JUGE(rootObj != NULL, PARKING_PLACE_FULL);
    cJSON *parkingStateObj = cJSON_GetObjectItem(rootObj, "ParkingPlaceState");
    JUGE(parkingStateObj != NULL, PARKING_PLACE_FULL);
    JUGE(parkingStateObj->type == cJSON_Number, PARKING_PLACE_FULL);
    
    cJSON *parkingBitmap = cJSON_GetObjectItem(rootObj, "ParkingPlaceStateBitmap");
    if ((parkingBitmap != NULL) && (parkingBitmap->type == cJSON_Number)) {
        *bitmap = (uint16_t)(parkingBitmap->valueint);
    } else {
        SAMPLE_PRT("Error response: %s\n", rspBuf);
    }

    ret = parkingStateObj->valueint;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    return ret;
}

uint8_t CheckSum(uint8_t *data, uint32_t len) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum += *data++;
    }

    return sum;
}

int SendDataTo485(uint8_t *data, uint32_t len)
{
    int ret = OK;
    uint32_t pos = 0;
    uint32_t totalLen = len * 2 + 64;
    char *postData = malloc(totalLen);
    JUGE(postData != NULL, ERROR);
    
    pos += snprintf(postData + pos, totalLen - pos, "{\"com\":%d, \"data\":\"", COMM_SEND_485);
    for (uint32_t i = 0; i < len; i++) {
        pos += snprintf(postData + pos, totalLen - pos, "%02X", data[i]);
    }
    pos += snprintf(postData + pos, totalLen - pos, "\"}");
    ret = pos;
    SAMPLE_PRT("SendDataTo485: %s\n", postData);

    char url[128] = {0};
    snprintf(url, sizeof(url), "http://%s:8080/cgi-bin/ctrcgi", g_Allconfig.ControlIP);
    post_json_to_server(url, postData, NULL, 0);
RET:
    if (postData != NULL) {
        free(postData);
    }
    return ret;
}

int SendParkingStateBitmapToLightBrd(uint8_t slaveBitMap, uint8_t parkNum)
{
    int eqmNum = 1;
    if (strlen(g_Allconfig.LightSlaveIP) != 0) {
        eqmNum = 2;
    }
    int cmdPos = 0;
    uint8_t cmdBinBuf[64] = {0};

    cmdBinBuf[cmdPos++] = TO_LIGHT_BRD;
    cmdBinBuf[cmdPos++] = 0xff;
    cmdBinBuf[cmdPos++] = TO_BRD_PARK_BITMAP;
    cmdBinBuf[cmdPos++] = eqmNum * 2;
    cmdBinBuf[cmdPos++] = MAX_PARKING_PLACE_NUM;
    cmdBinBuf[cmdPos++] = g_parkingPlaceStateBitmap;
    if (eqmNum == 2) {
        cmdBinBuf[cmdPos++] = parkNum;
        cmdBinBuf[cmdPos++] = slaveBitMap;
    }

    uint8_t sum = CheckSum(cmdBinBuf, cmdPos);
    cmdBinBuf[cmdPos++] = sum;
    
    SendDataTo485(cmdBinBuf, cmdPos);
    
    return OK;
}

int ProcCtrlLightStatus(FifoRing *fifo)
{
    void *tmp = NULL;
    int ret = fifo_ring_pop(fifo, &tmp);
    if (ret != 0) {
        return 0;
    }

    if (strlen(g_Allconfig.ControlIP) == 0) {
        return 1;
    }

    uint16_t slaveBitMap = 0u;
    int slaveState = GetSlaveParkingPlaceState(&slaveBitMap);
    SAMPLE_PRT("slaveState: %d, masterState: %d\n", slaveState, g_prevParkingPlaceState);
    
    char jsonBuf[128] = {0};
    char url[128] = {0};
    int color = ((g_prevParkingPlaceState == PARKING_PLACE_FULL) && (slaveState == PARKING_PLACE_FULL)) ? 
        (LIGHT_RED) : LIGHT_GREEN;
    snprintf(jsonBuf, sizeof(jsonBuf), "{\"com\":6, \"color\":%d, \"type\":%d}", LIGHT_MANDATORY, color + 1);
    SAMPLE_PRT("%s\n", jsonBuf);
    
    snprintf(url, sizeof(url), "http://%s:8080/cgi-bin/ctrcgi", g_Allconfig.ControlIP);
    SAMPLE_PRT("%s\n", url);
    post_json_to_server(url, jsonBuf, NULL, 0);

    uint8_t bitMap = (uint8_t)(slaveBitMap & 0xff);
    uint8_t parkNum = (uint8_t)(slaveBitMap >> 8);
    SendParkingStateBitmapToLightBrd(bitMap, parkNum);

    return 1;
}

void * PostInfoToServerThread(void *p)
{
    void *ret = NULL;
    g_jpgePostFifo = fifo_ring_create(g_maxPostFifoLen);
    JUGE(g_jpgePostFifo != NULL, NULL);
    g_othPostFifo = fifo_ring_create(g_maxPostFifoLen);
    JUGE(g_othPostFifo != NULL, NULL);
    g_CtrLightFifo = fifo_ring_create(g_maxPostFifoLen);
    JUGE(g_CtrLightFifo != NULL, NULL);

    while (!g_exitPostThread) {
        int procWork = 0;
        int ret = 0;
        ret = ProcPostInfoToServer(g_othPostFifo);
        if (ret != 0) { procWork++; }

        ret = ProcPostInfoToServer(g_jpgePostFifo);
        if (ret != 0) { procWork++; }

        ret = ProcCtrlLightStatus(g_CtrLightFifo);
        if (ret != 0) { procWork++; }
        
        if (!procWork) {
            sleep(1);
        } else {
            SAMPLE_PRT("PostInfoToServerThread procWork: %d\n", procWork);
        }
    }

RET:
    if (g_jpgePostFifo) {
        fifo_ring_destroy(g_jpgePostFifo);
    }
    if (g_othPostFifo) {
        fifo_ring_destroy(g_othPostFifo);
    }
    if (g_CtrLightFifo) {
        fifo_ring_destroy(g_CtrLightFifo);
    }
    
    return ret;
}

int GetIntValFromJson(cJSON *Obj, char *key, int *val)
{
    cJSON *itmObj = cJSON_GetObjectItem(Obj, key);
    if ((itmObj == NULL) || (itmObj->type != cJSON_Number)) {
        return -1;
    }

    *val = itmObj->valueint;
    
    return 0;
}

int GetStringValFromJson(cJSON *Obj, char *key, char *buf, int bufLen)
{
    cJSON *itmObj = cJSON_GetObjectItem(Obj, key);
    if ((itmObj == NULL) || (itmObj->type != cJSON_String)) {
        return -1;
    }
    strncpy(buf, itmObj->valuestring, bufLen - 1);
    buf[bufLen - 1] = '\0';
    
    return 0;
}

int GetPlaceCoordFromJson(cJSON *obj, PlaceInterfaceStru *placeCfgList)
{
    cJSON *itmObj = cJSON_GetObjectItem(obj, "stPlaceCoord");
    if ((itmObj == NULL) || (itmObj->type != cJSON_Array)) {
        return -4;
    }

    int placeNum = cJSON_GetArraySize(itmObj);
    if (placeNum > MAX_PLACE_NUM) {
        return -5;
    }

    for (int i = 0; i < placeNum; i++) {
        cJSON *placeObj = cJSON_GetArrayItem(itmObj, i);
        if (placeObj == NULL) {
            return -6;
        }
        GetStringValFromJson(placeObj, "carnum", placeCfgList[i].id, sizeof(placeCfgList[i].id));

        //placeCfgList[i].enable = 0;
        GetIntValFromJson(placeObj, "carnumEnable", &(placeCfgList[i].enable));

        GetIntValFromJson(placeObj, "plateCfgState", &(placeCfgList[i].plateCfgState));
        
        GetIntValFromJson(placeObj, "x1", &(placeCfgList[i].x1));
        GetIntValFromJson(placeObj, "y1", &(placeCfgList[i].y1));

        GetIntValFromJson(placeObj, "x2", &(placeCfgList[i].x2));
        GetIntValFromJson(placeObj, "y2", &(placeCfgList[i].y2));

        GetIntValFromJson(placeObj, "x3", &(placeCfgList[i].x3));
        GetIntValFromJson(placeObj, "y3", &(placeCfgList[i].y3));

        GetIntValFromJson(placeObj, "x4", &(placeCfgList[i].x4));
        GetIntValFromJson(placeObj, "y4", &(placeCfgList[i].y4));
    }

    return 0;
}

void TranslatePlacePosToAlg(PlaceInterfaceStru *src, int placeNum, PARKING_GUIDANCE_CONFIG *dst)
{
    dst->nPlaceNum = placeNum;
    for (int i = 0; i < placeNum; i++) {
        dst->stPlaceCoord[i * 4 + 0].x = src[i].x1;
        dst->stPlaceCoord[i * 4 + 0].y = src[i].y1;

        dst->stPlaceCoord[i * 4 + 1].x = src[i].x2;
        dst->stPlaceCoord[i * 4 + 1].y = src[i].y2;

        dst->stPlaceCoord[i * 4 + 2].x = src[i].x3;
        dst->stPlaceCoord[i * 4 + 2].y = src[i].y3;

        dst->stPlaceCoord[i * 4 + 3].x = src[i].x4;
        dst->stPlaceCoord[i * 4 + 3].y = src[i].y4;
    }
}

int GetParkingGuidanceCfg(cJSON *rootObj, ParkingGuidanceParamStru *allCfg)
{
    PARKING_GUIDANCE_CONFIG *param = &(allCfg->algParam);
    cJSON *parkingObj = cJSON_GetObjectItem(rootObj, "PARKING_GUIDANCE_CONFIG");
    if (parkingObj == NULL) {
        return -1;
    }
    
    if (GetIntValFromJson(parkingObj, "nPlaceNum", &(param->nPlaceNum)) != 0) {
        return -2;
    }
    
    if (GetStringValFromJson(parkingObj, "pDefaultProvince", param->pDefaultProvince, 
        sizeof(param->pDefaultProvince)) != 0) {
        return -3;
    }

    
    int ret = GetPlaceCoordFromJson(parkingObj, allCfg->placeCfgList);
    if (ret != 0) {
        return ret;
    }
    TranslatePlacePosToAlg(allCfg->placeCfgList, param->nPlaceNum, param);

    return 0;
}

int DumpConfigToJson(const ParkingGuidanceParamStru *cfg)
{
    int ret = 0;
    cJSON *rootObj = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);

    cJSON_AddStringToObject(rootObj, "postJpegUrl", cfg->postJpegUrl);
    cJSON_AddNumberToObject(rootObj, "postJpegInterval", cfg->postJpegInterval);

    cJSON_AddStringToObject(rootObj, "postHeartBeatUrl", cfg->postHeartBeatUrl);
    cJSON_AddNumberToObject(rootObj, "heartBeatInterval", cfg->heartBeatInterval);

    cJSON_AddStringToObject(rootObj, "postion", cfg->postion);

    cJSON_AddStringToObject(rootObj, "ControlIP", cfg->ControlIP);
    cJSON_AddStringToObject(rootObj, "ControledIP", cfg->ControledIP);

    cJSON_AddStringToObject(rootObj, "LightSlaveIP", cfg->LightSlaveIP);
    cJSON_AddStringToObject(rootObj, "LightSlaveMac", cfg->LightSlaveMac);

    //JUGE(cfg->postUrl != NULL, -1);
    //cJSON_AddStringToObject(rootObj, "postUrl", cfg->postUrl);

    cJSON *parkCfgObj = NULL;
    EXEC_NE((parkCfgObj = cJSON_CreateObject()), NULL, -1);
    cJSON_AddItemToObject(rootObj, "PARKING_GUIDANCE_CONFIG", parkCfgObj);

    cJSON_AddNumberToObject(parkCfgObj, "nPlaceNum", cfg->algParam.nPlaceNum);
    
    //JUGE(strlen(cfg->algParam.pDefaultProvince) != 0, -1);
    cJSON_AddStringToObject(parkCfgObj, "pDefaultProvince", cfg->algParam.pDefaultProvince);
    
    cJSON *pointArr = NULL;
    EXEC_NE((pointArr = cJSON_CreateArray()), NULL, -1);
    cJSON_AddItemToObject(parkCfgObj, "stPlaceCoord", pointArr);
    
    for (int i = 0; i < cfg->algParam.nPlaceNum; i++) {
        cJSON *point = NULL;
        EXEC_NE((point = cJSON_CreateObject()), NULL, -1);
        cJSON_AddItemToArray(pointArr, point);
        cJSON_AddStringToObject(point, "carnum", cfg->placeCfgList[i].id);
        cJSON_AddNumberToObject(point, "carnumEnable", cfg->placeCfgList[i].enable);
        cJSON_AddNumberToObject(point, "plateCfgState", cfg->placeCfgList[i].plateCfgState);
        cJSON_AddNumberToObject(point, "x1", cfg->algParam.stPlaceCoord[i * 4 + 0].x);
        cJSON_AddNumberToObject(point, "y1", cfg->algParam.stPlaceCoord[i * 4 + 0].y);
        cJSON_AddNumberToObject(point, "x2", cfg->algParam.stPlaceCoord[i * 4 + 1].x);
        cJSON_AddNumberToObject(point, "y2", cfg->algParam.stPlaceCoord[i * 4 + 1].y);
        cJSON_AddNumberToObject(point, "x3", cfg->algParam.stPlaceCoord[i * 4 + 2].x);
        cJSON_AddNumberToObject(point, "y3", cfg->algParam.stPlaceCoord[i * 4 + 2].y);
        cJSON_AddNumberToObject(point, "x4", cfg->algParam.stPlaceCoord[i * 4 + 3].x);
        cJSON_AddNumberToObject(point, "y4", cfg->algParam.stPlaceCoord[i * 4 + 3].y);
    }
    char *out = cJSON_Print(rootObj);
    if (out != NULL) {
        printf("out:\n%s\n", out);
        FILE *fp = fopen(g_ConfigFile, "w");
        if (fp != NULL) {
            fwrite(out, 1, strlen(out), fp);
            fclose(fp);
        } else {
            perror("fopen err");
        }
        free(out);
    }
    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    return ret;
}

int ParseConfigJson(const char *cfgfile, ParkingGuidanceParamStru *cfg)
{
    FILE *fp = NULL;
    long len = 0;
    char *data = NULL;
    int ret = 0;
    cJSON *rootObj = NULL;
    
    fp = fopen(cfgfile,"rb");
    if (fp == NULL) {
        SAMPLE_PRT("Open config file failed!\n");
        ret = -1;
        goto RET;
    }
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    data = (char*)malloc(len + 1);
    if (data == NULL) {
        SAMPLE_PRT("Malloc memory error!\n");
        ret = -1;
        goto RET;
    }
    
    fread(data, 1, len, fp);
    rootObj = cJSON_Parse(data);
    if (rootObj == NULL) {
        SAMPLE_PRT("cJSON_Parse error!\n");
        ret = -1;
        goto RET;
    }

    ret = GetStringValFromJson(rootObj, "postJpegUrl", cfg->postJpegUrl, sizeof(cfg->postJpegUrl));
    if (ret != 0) {
        SAMPLE_PRT("get postJpegUrl from config file failed!\n");
        memset(cfg->postJpegUrl, 0, sizeof(cfg->postJpegUrl));
    }

    cfg->postJpegInterval = 0;
    GetIntValFromJson(rootObj, "postJpegInterval", &(cfg->postJpegInterval));

    ret = GetStringValFromJson(rootObj, "postHeartBeatUrl", cfg->postHeartBeatUrl, sizeof(cfg->postHeartBeatUrl));
    if (ret != 0) {
        SAMPLE_PRT("get postHeartBeatUrl from config file failed!\n");
        memset(cfg->postHeartBeatUrl, 0, sizeof(cfg->postHeartBeatUrl));
    }

    cfg->heartBeatInterval = 0;
    GetIntValFromJson(rootObj, "heartBeatInterval", &(cfg->heartBeatInterval));

    memset(cfg->postion, 0, sizeof(cfg->postion));
    GetStringValFromJson(rootObj, "postion", cfg->postion, sizeof(cfg->postion));

    memset(cfg->ControlIP, 0, sizeof(cfg->ControlIP));
    GetStringValFromJson(rootObj, "ControlIP", cfg->ControlIP, sizeof(cfg->ControlIP));

    memset(cfg->ControledIP, 0, sizeof(cfg->ControledIP));
    GetStringValFromJson(rootObj, "ControledIP", cfg->ControledIP, sizeof(cfg->ControledIP));

    memset(cfg->LightSlaveIP, 0, sizeof(cfg->LightSlaveIP));
    GetStringValFromJson(rootObj, "LightSlaveIP", cfg->LightSlaveIP, sizeof(cfg->LightSlaveIP));

    memset(cfg->LightSlaveMac, 0, sizeof(cfg->LightSlaveMac));
    GetStringValFromJson(rootObj, "LightSlaveMac", cfg->LightSlaveMac, sizeof(cfg->LightSlaveMac));

    ret = GetParkingGuidanceCfg(rootObj, cfg);
RET:
    if (fp != NULL) {
        fclose(fp);
    }
    if (data != NULL) {
        free(data);
    }
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    return ret;
}

/******************************************************************************
* function : show usage
******************************************************************************/
void SAMPLE_VENC_Usage(char* sPrgNm)
{
    printf("Usage : %s [index] \n", sPrgNm);
    printf("index:\n");
    printf("\t  0) H265(Large Stream)+H264(Small Stream)+JPEG lowdelay encode with RingBuf.\n");
    printf("\t  1) H.265e + H264e.\n");
    printf("\t  2) Qpmap:H.265e + H264e.\n");
    printf("\t  3) IntraRefresh:H.265e + H264e.\n");
    printf("\t  4) RoiBgFrameRate:H.265e + H.264e.\n");
    printf("\t  5) Mjpeg +Jpeg snap.\n");

    return;
}

/******************************************************************************
* function : to process abnormal case
******************************************************************************/
void SAMPLE_VENC_HandleSig(HI_S32 signo)
{
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_VENC_StopSendQpmapFrame();
        SAMPLE_COMM_VENC_StopGetStream();
        SAMPLE_COMM_All_ISP_Stop();
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram termination abnormally!\033[0;39m\n");
    }
    exit(-1);
}

/******************************************************************************
* function : to process abnormal case - the case of stream venc
******************************************************************************/
void SAMPLE_VENC_StreamHandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTERM == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

VENC_GOP_MODE_E SAMPLE_VENC_GetGopMode(void)
{
    char c;
    VENC_GOP_MODE_E enGopMode = 0;

Begin_Get:

    printf("please input choose gop mode!\n");
    printf("\t 0) NORMALP.\n");
    printf("\t 1) DUALP.\n");
    printf("\t 2) SMARTP.\n");

    while((c = getchar()) != '\n' && c != EOF)
    switch(c)
    {
        case '0':
            enGopMode = VENC_GOPMODE_NORMALP;
            break;
        case '1':
            enGopMode = VENC_GOPMODE_DUALP;
            break;
        case '2':
            enGopMode = VENC_GOPMODE_SMARTP;
            break;
        default:
            SAMPLE_PRT("input rcmode: %c, is invaild!\n",c);
            goto Begin_Get;
    }

    return enGopMode;
}

SAMPLE_RC_E SAMPLE_VENC_GetRcMode(void)
{
    char c;
    SAMPLE_RC_E  enRcMode = 0;

Begin_Get:

    printf("please input choose rc mode!\n");
    printf("\t c) cbr.\n");
    printf("\t v) vbr.\n");
    printf("\t a) avbr.\n");
    printf("\t x) cvbr.\n");
    printf("\t q) qvbr.\n");
    printf("\t f) fixQp\n");

    c = 'c';
    //while((c = getchar()) != '\n' && c != EOF)
    switch(c)
    {
        case 'c':
            enRcMode = SAMPLE_RC_CBR;
            break;
        case 'v':
            enRcMode = SAMPLE_RC_VBR;
            break;
        case 'a':
            enRcMode = SAMPLE_RC_AVBR;
            break;
        case 'f':
            enRcMode = SAMPLE_RC_FIXQP;
            break;
        case 'x':
            enRcMode = SAMPLE_RC_CVBR;
            break;
        case 'q':
            enRcMode = SAMPLE_RC_QVBR;
            break;
        default:
            SAMPLE_PRT("input rcmode: %c, is invaild!\n",c);
            goto Begin_Get;
    }
    return enRcMode;
}

VENC_INTRA_REFRESH_MODE_E SAMPLE_VENC_GetIntraRefreshMode(void)
{
    char c;
    VENC_INTRA_REFRESH_MODE_E   enIntraRefreshMode = INTRA_REFRESH_ROW;

Begin_Get:

    printf("please input choose IntraRefresh mode!\n");
    printf("\t r) ROW.\n");
    printf("\t c) COLUMN.\n");

    while((c = getchar()) != '\n' && c != EOF)
    switch(c)
    {
        case 'r':
            enIntraRefreshMode = INTRA_REFRESH_ROW;
            break;
        case 'c':
            enIntraRefreshMode = INTRA_REFRESH_COLUMN;
            break;

        default:
            SAMPLE_PRT("input IntraRefresh Mode: %c, is invaild!\n",c);
            goto Begin_Get;
    }
    return enIntraRefreshMode;
}


static HI_U32 GetFrameRateFromSensorType(SAMPLE_SNS_TYPE_E enSnsType)
{
    HI_U32 FrameRate;

    SAMPLE_COMM_VI_GetFrameRateBySensor(enSnsType, &FrameRate);

    return FrameRate;
}

static HI_U32 GetFullLinesStdFromSensorType(SAMPLE_SNS_TYPE_E enSnsType)
{
    HI_U32 FullLinesStd = 0;

    switch (enSnsType)
    {
        case SONY_IMX327_MIPI_2M_30FPS_12BIT:
        case SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1:
            FullLinesStd = 1125;
            break;
        case SONY_IMX307_MIPI_2M_30FPS_12BIT:
        case SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SONY_IMX307_2L_MIPI_2M_30FPS_12BIT:
        case SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1:
        case SMART_SC2235_DC_2M_30FPS_10BIT:
        case SMART_SC2231_MIPI_2M_30FPS_10BIT:
            FullLinesStd = 1125;
            break;
        case SONY_IMX335_MIPI_5M_30FPS_12BIT:
        case SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1:
            FullLinesStd = 1875;
            break;
        case SONY_IMX335_MIPI_4M_30FPS_12BIT:
        case SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1:
            FullLinesStd = 1375;
            break;
        case SMART_SC4236_MIPI_3M_30FPS_10BIT:
        case SMART_SC4236_MIPI_3M_20FPS_10BIT:
            FullLinesStd = 1600;
            break;
        case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT:
        case GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT_FORCAR:
            FullLinesStd = 1108;
            break;
        case SMART_SC3235_MIPI_3M_30FPS_10BIT:
            FullLinesStd = 1350;                     /* 1350 sensor SC3235 full lines */
            break;
        case OMNIVISION_OS05A_MIPI_5M_30FPS_12BIT:
            SAMPLE_PRT("Error: sensor type %d resolution out of limits, not support ring!\n",enSnsType);
            break;
        default:
            SAMPLE_PRT("Error: Not support this sensor now! ==> %d\n",enSnsType);
            break;
    }

    return FullLinesStd;
}

static HI_VOID AdjustWrapBufLineBySnsType(SAMPLE_SNS_TYPE_E enSnsType, HI_U32 *pWrapBufLine)
{
    /*some sensor as follow need to expand the wrapBufLine*/
    if ((enSnsType == SMART_SC4236_MIPI_3M_30FPS_10BIT) ||
        (enSnsType == SMART_SC4236_MIPI_3M_20FPS_10BIT) ||
        (enSnsType == SMART_SC2235_DC_2M_30FPS_10BIT))
    {
        *pWrapBufLine += WRAP_BUF_LINE_EXT;
    }

    return;
}

static HI_VOID GetSensorResolution(SAMPLE_SNS_TYPE_E enSnsType, SIZE_S *pSnsSize)
{
    HI_S32          ret;
    SIZE_S          SnsSize;
    PIC_SIZE_E      enSnsSize;

    ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return;
    }
    ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &SnsSize);
    if (HI_SUCCESS != ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return;
    }

    *pSnsSize = SnsSize;

    return;
}

static VI_VPSS_MODE_E GetViVpssModeFromResolution(SAMPLE_SNS_TYPE_E SnsType)
{
    SIZE_S SnsSize = {0};
    VI_VPSS_MODE_E ViVpssMode;

    GetSensorResolution(SnsType, &SnsSize);

    if (SnsSize.u32Width > ONLINE_LIMIT_WIDTH)
    {
        ViVpssMode = VI_OFFLINE_VPSS_ONLINE;
    }
    else
    {
        ViVpssMode = VI_ONLINE_VPSS_ONLINE;
    }

    return ViVpssMode;
}

static HI_VOID SAMPLE_VENC_GetDefaultVpssAttr(SAMPLE_SNS_TYPE_E enSnsType, HI_BOOL *pChanEnable, SIZE_S stEncSize[], SAMPLE_VPSS_CHN_ATTR_S *pVpssAttr)
{
    HI_S32 i;

    memset(pVpssAttr, 0, sizeof(SAMPLE_VPSS_CHN_ATTR_S));

    pVpssAttr->enDynamicRange = DYNAMIC_RANGE_SDR8;
    pVpssAttr->enPixelFormat  = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    pVpssAttr->bWrapEn        = 0;
    pVpssAttr->enSnsType      = enSnsType;
    pVpssAttr->ViVpssMode     = GetViVpssModeFromResolution(enSnsType);

    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if (HI_TRUE == pChanEnable[i])
        {
            pVpssAttr->enCompressMode[i]          = (i == 0)? COMPRESS_MODE_SEG : COMPRESS_MODE_NONE;
            pVpssAttr->stOutPutSize[i].u32Width   = stEncSize[i].u32Width;
            pVpssAttr->stOutPutSize[i].u32Height  = stEncSize[i].u32Height;
            pVpssAttr->stFrameRate[i].s32SrcFrameRate  = -1;
            pVpssAttr->stFrameRate[i].s32DstFrameRate  = -1;
            pVpssAttr->bMirror[i]                      = HI_FALSE;
            pVpssAttr->bFlip[i]                        = HI_FALSE;

            pVpssAttr->bChnEnable[i]                   = HI_TRUE;
        }
    }

    return;
}

HI_S32 SAMPLE_VENC_SYS_Init(SAMPLE_VB_ATTR_S *pCommVbAttr)
{
    HI_S32 i;
    HI_S32 s32Ret;
    VB_CONFIG_S stVbConf;

    if (pCommVbAttr->validNum > VB_MAX_COMM_POOLS)
    {
        SAMPLE_PRT("SAMPLE_VENC_SYS_Init validNum(%d) too large than VB_MAX_COMM_POOLS(%d)!\n", pCommVbAttr->validNum, VB_MAX_COMM_POOLS);
        return HI_FAILURE;
    }

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    for (i = 0; i < pCommVbAttr->validNum; i++)
    {
        stVbConf.astCommPool[i].u64BlkSize   = pCommVbAttr->blkSize[i];
        stVbConf.astCommPool[i].u32BlkCnt    = pCommVbAttr->blkCnt[i];
        //printf("%s,%d,stVbConf.astCommPool[%d].u64BlkSize = %lld, blkSize = %d\n",__func__,__LINE__,i,stVbConf.astCommPool[i].u64BlkSize,stVbConf.astCommPool[i].u32BlkCnt);
    }

    stVbConf.u32MaxPoolCnt = pCommVbAttr->validNum;

    if(pCommVbAttr->supplementConfig == 0)
    {
        s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    }
    else
    {
        s32Ret = SAMPLE_COMM_SYS_InitWithVbSupplement(&stVbConf,pCommVbAttr->supplementConfig);
    }

    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    return HI_SUCCESS;
}

HI_VOID SAMPLE_VENC_SetDCFInfo(VI_PIPE ViPipe)
{
    ISP_DCF_INFO_S stIspDCF;

    HI_MPI_ISP_GetDCFInfo(ViPipe, &stIspDCF);

    strncpy((char *)stIspDCF.stIspDCFConstInfo.au8ImageDescription, "Thumbnail test", DCF_DRSCRIPTION_LENGTH);
    strncpy((char *)stIspDCF.stIspDCFConstInfo.au8Make, "Hisilicon", DCF_DRSCRIPTION_LENGTH);
    strncpy((char *)stIspDCF.stIspDCFConstInfo.au8Model, "IP Camera", DCF_DRSCRIPTION_LENGTH);
    strncpy((char *)stIspDCF.stIspDCFConstInfo.au8Software, "v.1.1.0", DCF_DRSCRIPTION_LENGTH);

    stIspDCF.stIspDCFConstInfo.u32FocalLength             = 0x00640001;
    stIspDCF.stIspDCFConstInfo.u8Contrast                 = 5;
    stIspDCF.stIspDCFConstInfo.u8CustomRendered           = 0;
    stIspDCF.stIspDCFConstInfo.u8FocalLengthIn35mmFilm    = 1;
    stIspDCF.stIspDCFConstInfo.u8GainControl              = 1;
    stIspDCF.stIspDCFConstInfo.u8LightSource              = 1;
    stIspDCF.stIspDCFConstInfo.u8MeteringMode             = 1;
    stIspDCF.stIspDCFConstInfo.u8Saturation               = 1;
    stIspDCF.stIspDCFConstInfo.u8SceneCaptureType         = 1;
    stIspDCF.stIspDCFConstInfo.u8SceneType                = 0;
    stIspDCF.stIspDCFConstInfo.u8Sharpness                = 5;
    stIspDCF.stIspDCFUpdateInfo.u32ISOSpeedRatings         = 500;
    stIspDCF.stIspDCFUpdateInfo.u32ExposureBiasValue       = 5;
    stIspDCF.stIspDCFUpdateInfo.u32ExposureTime            = 0x00010004;
    stIspDCF.stIspDCFUpdateInfo.u32FNumber                 = 0x0001000f;
    stIspDCF.stIspDCFUpdateInfo.u8WhiteBalance             = 1;
    stIspDCF.stIspDCFUpdateInfo.u8ExposureMode             = 0;
    stIspDCF.stIspDCFUpdateInfo.u8ExposureProgram          = 1;
    stIspDCF.stIspDCFUpdateInfo.u32MaxApertureValue        = 0x00010001;

    HI_MPI_ISP_SetDCFInfo(ViPipe, &stIspDCF);

    return;
}

HI_S32 SAMPLE_VENC_VI_Init( SAMPLE_VI_CONFIG_S *pstViConfig, VI_VPSS_MODE_E ViVpssMode)
{
    HI_S32              s32Ret;
    SAMPLE_SNS_TYPE_E   enSnsType;
    ISP_CTRL_PARAM_S    stIspCtrlParam;
    HI_U32              u32FrameRate;


    enSnsType = pstViConfig->astViInfo[0].stSnsInfo.enSnsType;

    pstViConfig->as32WorkingViId[0]                           = 0;

    pstViConfig->astViInfo[0].stSnsInfo.MipiDev            = SAMPLE_COMM_VI_GetComboDevBySensor(pstViConfig->astViInfo[0].stSnsInfo.enSnsType, 0);
    pstViConfig->astViInfo[0].stSnsInfo.s32BusId           = 0;
    pstViConfig->astViInfo[0].stDevInfo.enWDRMode          = WDR_MODE_NONE;
    pstViConfig->astViInfo[0].stPipeInfo.enMastPipeMode    = ViVpssMode;

    //pstViConfig->astViInfo[0].stPipeInfo.aPipe[0]          = ViPipe0;
    pstViConfig->astViInfo[0].stPipeInfo.aPipe[1]          = -1;

    //pstViConfig->astViInfo[0].stChnInfo.ViChn              = ViChn;
    //pstViConfig->astViInfo[0].stChnInfo.enPixFormat        = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    //pstViConfig->astViInfo[0].stChnInfo.enDynamicRange     = enDynamicRange;
    pstViConfig->astViInfo[0].stChnInfo.enVideoFormat      = VIDEO_FORMAT_LINEAR;
    pstViConfig->astViInfo[0].stChnInfo.enCompressMode     = COMPRESS_MODE_NONE;
    s32Ret = SAMPLE_COMM_VI_SetParam(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_SetParam failed with %d!\n", s32Ret);
        return s32Ret;
    }

    SAMPLE_COMM_VI_GetFrameRateBySensor(enSnsType, &u32FrameRate);

    s32Ret = HI_MPI_ISP_GetCtrlParam(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stIspCtrlParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_ISP_GetCtrlParam failed with %d!\n", s32Ret);
        return s32Ret;
    }
    stIspCtrlParam.u32StatIntvl  = u32FrameRate/30;
    if (stIspCtrlParam.u32StatIntvl == 0)
    {
        stIspCtrlParam.u32StatIntvl = 1;
    }

    s32Ret = HI_MPI_ISP_SetCtrlParam(pstViConfig->astViInfo[0].stPipeInfo.aPipe[0], &stIspCtrlParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_ISP_SetCtrlParam failed with %d!\n", s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_VI_StartVi(pstViConfig);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_COMM_SYS_Exit();
        SAMPLE_PRT("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        return s32Ret;
    }

    return HI_SUCCESS;
}

static HI_S32 SAMPLE_VENC_VPSS_CreateGrp(VPSS_GRP VpssGrp, SAMPLE_VPSS_CHN_ATTR_S *pParam)
{
    HI_S32          s32Ret;
    PIC_SIZE_E      enSnsSize;
    SIZE_S          stSnsSize;
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(pParam->enSnsType, &enSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    stVpssGrpAttr.enDynamicRange          = pParam->enDynamicRange;
    stVpssGrpAttr.enPixelFormat           = pParam->enPixelFormat;
    stVpssGrpAttr.u32MaxW                 = stSnsSize.u32Width;
    stVpssGrpAttr.u32MaxH                 = stSnsSize.u32Height;
    stVpssGrpAttr.bNrEn                   = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enNrType       = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;
    stVpssGrpAttr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, &stVpssGrpAttr);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_DestoryGrp(VPSS_GRP VpssGrp)
{
    HI_S32          s32Ret;

    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_StartGrp(VPSS_GRP VpssGrp)
{
    HI_S32          s32Ret;

    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_ChnEnable(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, SAMPLE_VPSS_CHN_ATTR_S *pParam, HI_BOOL bWrapEn)
{
    HI_S32 s32Ret;
    VPSS_CHN_ATTR_S     stVpssChnAttr;
    VPSS_CHN_BUF_WRAP_S stVpssChnBufWrap;

    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    stVpssChnAttr.u32Width                     = pParam->stOutPutSize[VpssChn].u32Width;
    stVpssChnAttr.u32Height                    = pParam->stOutPutSize[VpssChn].u32Height;
    stVpssChnAttr.enChnMode                    = VPSS_CHN_MODE_USER;
    stVpssChnAttr.enCompressMode               = pParam->enCompressMode[VpssChn];
    stVpssChnAttr.enDynamicRange               = pParam->enDynamicRange;
    stVpssChnAttr.enPixelFormat                = pParam->enPixelFormat;
    if (stVpssChnAttr.u32Width * stVpssChnAttr.u32Height > 2688 * 1520 ) {
        stVpssChnAttr.stFrameRate.s32SrcFrameRate  = 30;
        stVpssChnAttr.stFrameRate.s32DstFrameRate  = 20;
    } else {
        stVpssChnAttr.stFrameRate.s32SrcFrameRate  = pParam->stFrameRate[VpssChn].s32SrcFrameRate;
        stVpssChnAttr.stFrameRate.s32DstFrameRate  = pParam->stFrameRate[VpssChn].s32DstFrameRate;
    }
    stVpssChnAttr.u32Depth                     = 0;
    stVpssChnAttr.bMirror                      = pParam->bMirror[VpssChn];
    stVpssChnAttr.bFlip                        = pParam->bFlip[VpssChn];
    stVpssChnAttr.enVideoFormat                = VIDEO_FORMAT_LINEAR;
    stVpssChnAttr.stAspectRatio.enMode         = ASPECT_RATIO_NONE;

    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &stVpssChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr chan %d failed with %#x\n", VpssChn, s32Ret);
        goto exit0;
    }

    if (bWrapEn)
    {
        if (VpssChn != 0)   //vpss limit! just vpss chan0 support wrap
        {
            SAMPLE_PRT("Error:Just vpss chan 0 support wrap! Current chan %d\n", VpssChn);
            goto exit0;
        }


        HI_U32 WrapBufLen = 0;
        VPSS_VENC_WRAP_PARAM_S WrapParam;

        memset(&WrapParam, 0, sizeof(VPSS_VENC_WRAP_PARAM_S));
        WrapParam.bAllOnline      = (pParam->ViVpssMode == VI_ONLINE_VPSS_ONLINE) ? 1 : 0;
        WrapParam.u32FrameRate    = GetFrameRateFromSensorType(pParam->enSnsType);
        WrapParam.u32FullLinesStd = GetFullLinesStdFromSensorType(pParam->enSnsType);
        WrapParam.stLargeStreamSize.u32Width = pParam->stOutPutSize[pParam->BigStreamId].u32Width;
        WrapParam.stLargeStreamSize.u32Height= pParam->stOutPutSize[pParam->BigStreamId].u32Height;
        WrapParam.stSmallStreamSize.u32Width = pParam->stOutPutSize[pParam->SmallStreamId].u32Width;
        WrapParam.stSmallStreamSize.u32Height= pParam->stOutPutSize[pParam->SmallStreamId].u32Height;

        if (HI_MPI_SYS_GetVPSSVENCWrapBufferLine(&WrapParam, &WrapBufLen) == HI_SUCCESS)
        {
            AdjustWrapBufLineBySnsType(pParam->enSnsType, &WrapBufLen);

            stVpssChnBufWrap.u32WrapBufferSize = VPSS_GetWrapBufferSize(WrapParam.stLargeStreamSize.u32Width,
                WrapParam.stLargeStreamSize.u32Height, WrapBufLen, pParam->enPixelFormat, DATA_BITWIDTH_8,
                COMPRESS_MODE_NONE, DEFAULT_ALIGN);
            stVpssChnBufWrap.bEnable = 1;
            stVpssChnBufWrap.u32BufLine = WrapBufLen;
            s32Ret = HI_MPI_VPSS_SetChnBufWrapAttr(VpssGrp, VpssChn, &stVpssChnBufWrap);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VPSS_SetChnBufWrapAttr Chn %d failed with %#x\n", VpssChn, s32Ret);
                goto exit0;
            }
        }
        else
        {
            SAMPLE_PRT("Current sensor type: %d, not support BigStream(%dx%d) and SmallStream(%dx%d) Ring!!\n",
                pParam->enSnsType,
                pParam->stOutPutSize[pParam->BigStreamId].u32Width, pParam->stOutPutSize[pParam->BigStreamId].u32Height,
                pParam->stOutPutSize[pParam->SmallStreamId].u32Width, pParam->stOutPutSize[pParam->SmallStreamId].u32Height);
        }

    }

    s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VPSS_EnableChn (%d) failed with %#x\n", VpssChn, s32Ret);
        goto exit0;
    }

exit0:
    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_ChnDisable(VPSS_GRP VpssGrp, VPSS_CHN VpssChn)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);

    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return s32Ret;
}

static HI_S32 SAMPLE_VENC_VPSS_Init(VPSS_GRP VpssGrp, SAMPLE_VPSS_CHN_ATTR_S *pstParam)
{
    HI_S32 i,j;
    HI_S32 s32Ret;
    HI_BOOL bWrapEn;

    s32Ret = SAMPLE_VENC_VPSS_CreateGrp(VpssGrp, pstParam);
    if (s32Ret != HI_SUCCESS)
    {
        goto exit0;
    }

    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if (pstParam->bChnEnable[i] == HI_TRUE)
        {
            bWrapEn = (i==0)? pstParam->bWrapEn : 0;

            s32Ret = SAMPLE_VENC_VPSS_ChnEnable(VpssGrp, i, pstParam, bWrapEn);
            if (s32Ret != HI_SUCCESS)
            {
                goto exit1;
            }
        }
    }

    i--; // for abnormal case 'exit1' prossess;

    s32Ret = SAMPLE_VENC_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        goto exit1;
    }

    return s32Ret;

exit1:
    for (j = 0; j <= i; j++)
    {
        if (pstParam->bChnEnable[j] == HI_TRUE)
        {
            SAMPLE_VENC_VPSS_ChnDisable(VpssGrp, i);
        }
    }

    SAMPLE_VENC_VPSS_DestoryGrp(VpssGrp);
exit0:
    return s32Ret;
}

static HI_VOID SAMPLE_VENC_GetCommVbAttr(const SAMPLE_SNS_TYPE_E enSnsType, const SAMPLE_VPSS_CHN_ATTR_S *pstParam,
    HI_BOOL bSupportDcf, SAMPLE_VB_ATTR_S * pstcommVbAttr)
{
    if (pstParam->ViVpssMode != VI_ONLINE_VPSS_ONLINE)
    {
        SIZE_S snsSize = {0};
        GetSensorResolution(enSnsType, &snsSize);

        if (pstParam->ViVpssMode == VI_OFFLINE_VPSS_ONLINE || pstParam->ViVpssMode == VI_OFFLINE_VPSS_OFFLINE)
        {
            pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = VI_GetRawBufferSize(snsSize.u32Width, snsSize.u32Height,
                                                                                  PIXEL_FORMAT_RGB_BAYER_12BPP,
                                                                                  COMPRESS_MODE_NONE,
                                                                                  DEFAULT_ALIGN);
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
            pstcommVbAttr->validNum++;
        }

        if (pstParam->ViVpssMode == VI_OFFLINE_VPSS_OFFLINE)
        {
            pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(snsSize.u32Width, snsSize.u32Height,
                                                                                      PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                                      DATA_BITWIDTH_8,
                                                                                      COMPRESS_MODE_NONE,
                                                                                      DEFAULT_ALIGN);
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 2;
            pstcommVbAttr->validNum++;
        }

        if (pstParam->ViVpssMode == VI_ONLINE_VPSS_OFFLINE)
        {
            pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(snsSize.u32Width, snsSize.u32Height,
                                                                                      PIXEL_FORMAT_YVU_SEMIPLANAR_420,
                                                                                      DATA_BITWIDTH_8,
                                                                                      COMPRESS_MODE_NONE,
                                                                                      DEFAULT_ALIGN);
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
            pstcommVbAttr->validNum++;

        }
    }
    if(HI_TRUE == pstParam->bWrapEn)
    {
        pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = VPSS_GetWrapBufferSize(pstParam->stOutPutSize[pstParam->BigStreamId].u32Width,
                                                                                 pstParam->stOutPutSize[pstParam->BigStreamId].u32Height,
                                                                                 pstParam->WrapBufLine,
                                                                                 pstParam->enPixelFormat,DATA_BITWIDTH_8,COMPRESS_MODE_NONE,DEFAULT_ALIGN);
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 1;
        pstcommVbAttr->validNum++;
    }
    else
    {
        pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(pstParam->stOutPutSize[0].u32Width, pstParam->stOutPutSize[0].u32Height,
                                                                                  pstParam->enPixelFormat,
                                                                                  DATA_BITWIDTH_8,
                                                                                  pstParam->enCompressMode[0],
                                                                                  DEFAULT_ALIGN);

        if (pstParam->ViVpssMode == VI_ONLINE_VPSS_ONLINE)
        {
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
        }
        else
        {
            pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 2;
        }

        pstcommVbAttr->validNum++;
    }



    pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(pstParam->stOutPutSize[1].u32Width, pstParam->stOutPutSize[1].u32Height,
                                                                              pstParam->enPixelFormat,
                                                                              DATA_BITWIDTH_8,
                                                                              pstParam->enCompressMode[1],
                                                                              DEFAULT_ALIGN);

    if (pstParam->ViVpssMode == VI_ONLINE_VPSS_ONLINE)
    {
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 3;
    }
    else
    {
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 2;
    }
    pstcommVbAttr->validNum++;


    //vgs dcf use
    if(HI_TRUE == bSupportDcf)
    {
        pstcommVbAttr->blkSize[pstcommVbAttr->validNum] = COMMON_GetPicBufferSize(160, 120,
                                                                                  pstParam->enPixelFormat,
                                                                                  DATA_BITWIDTH_8,
                                                                                  COMPRESS_MODE_NONE,
                                                                                  DEFAULT_ALIGN);
        pstcommVbAttr->blkCnt[pstcommVbAttr->validNum]  = 1;
        pstcommVbAttr->validNum++;
    }

}

HI_S32 SAMPLE_VENC_CheckSensor(SAMPLE_SNS_TYPE_E   enSnsType,SIZE_S  stSize)
{
    HI_S32 s32Ret;
    SIZE_S          stSnsSize;
    PIC_SIZE_E      enSnsSize;

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    if((stSnsSize.u32Width < stSize.u32Width) || (stSnsSize.u32Height < stSize.u32Height))
    {
        //SAMPLE_PRT("Sensor size is (%d,%d), but encode chnl is (%d,%d) !\n",
            //stSnsSize.u32Width,stSnsSize.u32Height,stSize.u32Width,stSize.u32Height);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_S32 SAMPLE_VENC_ModifyResolution(SAMPLE_SNS_TYPE_E   enSnsType,PIC_SIZE_E *penSize,SIZE_S *pstSize)
{
    HI_S32 s32Ret;
    SIZE_S          stSnsSize;
    PIC_SIZE_E      enSnsSize;

    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(enSnsType, &enSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_VI_GetSizeBySensor failed!\n");
        return s32Ret;
    }
    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSnsSize, &stSnsSize);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
        return s32Ret;
    }

    *penSize = enSnsSize;
    pstSize->u32Width  = stSnsSize.u32Width;
    pstSize->u32Height = stSnsSize.u32Height;

    return HI_SUCCESS;
}

void PlateYuvCropping(VIDEO_FRAME_INFO_S *srcFrame, HI_U8 *dstYuvBuff, 
    HI_U32 x, HI_U32 y, HI_U32 width, HI_U32 height, HI_BOOL translate)
{
    if (x % 2 != 0) { x--; }
    if (y % 2 != 0) { y--; }
    
    width = (width < 32) ? 32 : width;
    height = (height < 32) ? 32 : height;
    if (width % 4 != 0)
    {
        width = ((width + 4) / 4) * 4;
        width = (width > MAX_PLATE_WIDTH) ? MAX_PLATE_WIDTH : width;
    }
    if (height % 4 != 0)
    {
        height = ((height + 4) / 4) * 4;
        height = (height > MAX_PLATE_HEIGHT) ? MAX_PLATE_HEIGHT : height;
    }
    
    HI_U32 u32BlkSize = (srcFrame->stVFrame.u32Stride[0]) * (srcFrame->stVFrame.u32Height) * 3 / 2;
    HI_U8 *psrcVirYaddr = (HI_U8 *)HI_MPI_SYS_Mmap(srcFrame->stVFrame.u64PhyAddr[0], u32BlkSize);    
    if (psrcVirYaddr == NULL)
    {
        SAMPLE_PRT("Plate_yuv_cropping HI_MPI_SYS_Mmap failed\n");
        return;
    }

    if (translate) {
        /* copy Y */
        HI_U8 *srcStart = psrcVirYaddr + y * srcFrame->stVFrame.u32Stride[0] + x;
        HI_U8 *dstStart = dstYuvBuff;
        for (int i = 0; i < height; i++)
        {
            memcpy(dstStart, srcStart, width);
            srcStart += srcFrame->stVFrame.u32Stride[0];
            dstStart += width;
        }
        /* copy vu */
        srcStart = psrcVirYaddr + srcFrame->stVFrame.u32Stride[0] * srcFrame->stVFrame.u32Height 
                    + (y / 2) * srcFrame->stVFrame.u32Stride[1] + x;
        HI_U8 *dstU = dstYuvBuff + width * height;
        HI_U8 *dstV = dstU + (width / 2) * (height / 2);
        for (int i = 0; i < height / 2; i++)
        {
            int j = 0;
            int k = 0;
            for (j = 0, k = 0; j < width / 2; j++, k += 2)
            {
                dstV[j] = srcStart[k];
                dstU[j] = srcStart[k + 1];
            }
            srcStart += srcFrame->stVFrame.u32Stride[1];
            dstU += width / 2;
            dstV += width / 2;
        }
    }else {
        /* copy Y */
        HI_U8 *srcStart = psrcVirYaddr + y * srcFrame->stVFrame.u32Stride[0] + x;
        HI_U8 *dstStart = dstYuvBuff;
        for (int i = 0; i < height; i++)
        {
            memcpy(dstStart, srcStart, width);
            srcStart += srcFrame->stVFrame.u32Stride[0];
            dstStart += width;
        }
        /* copy vu */
        srcStart = psrcVirYaddr + srcFrame->stVFrame.u32Stride[0] * srcFrame->stVFrame.u32Height 
                    + (y / 2) * srcFrame->stVFrame.u32Stride[1] + x;
        dstStart = dstYuvBuff + width * height;
        for (int i = 0; i < height / 2; i++)
        {
            memcpy(dstStart, srcStart, width);
            srcStart += srcFrame->stVFrame.u32Stride[1];
            dstStart += width;
        }
    }

    HI_MPI_SYS_Munmap(psrcVirYaddr, u32BlkSize);
}

HI_S32 PlateReplaceFrameFromFile(FILE *fp, VIDEO_FRAME_INFO_S *frame)
{
    HI_U32 dataSize = 0u;
    HI_U64 phy_addr = 0u;
    HI_U8* pUserVirtAddr = NULL;
    int i = 0;
    HI_U8* start = NULL;
    int ret = 0;

    if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YVU_SEMIPLANAR_420)
    {
        SAMPLE_PRT("Plate_yuv_cropping unsupport pixel format - %d - %d\n", frame->stVFrame.enPixelFormat, PIXEL_FORMAT_YVU_SEMIPLANAR_420);
        return -1;
    }

    (void) fseek(fp, 0L, SEEK_SET);
    SAMPLE_PRT("u32Stride[0]: %u, u32Stride[1]: %u, width: %u, height: %u\n", frame->stVFrame.u32Stride[0],
        frame->stVFrame.u32Stride[1], frame->stVFrame.u32Width, frame->stVFrame.u32Height);
    dataSize = (frame->stVFrame.u32Stride[0]) * (frame->stVFrame.u32Height) * 3 / 2;
    
    phy_addr = frame->stVFrame.u64PhyAddr[0];

    pUserVirtAddr = (HI_U8*) HI_MPI_SYS_Mmap(phy_addr, dataSize);
    if (HI_NULL == pUserVirtAddr)
    {
        SAMPLE_PRT("PlateReplaceFrameFromFile HI_MPI_SYS_Mmap failed.");
        return -1;
    }

    start = pUserVirtAddr;
    for (i = 0; i < frame->stVFrame.u32Height; i++)
    {
        ret = fread(start, 1, frame->stVFrame.u32Width, fp);
        if (ret != frame->stVFrame.u32Width)
        {
            perror("fread error");
            SAMPLE_PRT("PlateReplaceFrameFromFile fread1 failed, ret: %u\n.", ret);
            (void) fseek(fp, 0L, SEEK_SET);
            HI_MPI_SYS_Munmap(pUserVirtAddr, dataSize);
            return -1;
        }
        start += frame->stVFrame.u32Stride[0];
    }

    start = pUserVirtAddr + frame->stVFrame.u32Stride[0] * frame->stVFrame.u32Height;
    for (i = 0; i < frame->stVFrame.u32Height / 2; i++)
    {
        ret = fread(start, 1, frame->stVFrame.u32Width, fp);
        if (ret != frame->stVFrame.u32Width)
        {
            SAMPLE_PRT("PlateReplaceFrameFromFile fread2 failed.");
            (void) fseek(fp, 0L, SEEK_SET);
            HI_MPI_SYS_Munmap(pUserVirtAddr, dataSize);
            return -1;
        }
        start += frame->stVFrame.u32Stride[1];
    }
    
    HI_MPI_SYS_Munmap(pUserVirtAddr, dataSize);
    return 0;
}

HI_S32 FillPlateFrame(VIDEO_FRAME_INFO_S *srcFrame, VIDEO_FRAME_INFO_S *plateFrame,
    VB_POOL u32PoolId, HI_U64 phyAddr, HI_U32 x, HI_U32 y, HI_U32 width, HI_U32 height, int i)
{
    if (x % 2 != 0) { x--; }
    if (y % 2 != 0) { y--; }

    width = (width < 32) ? 32 : width;
    height = (height < 32) ? 32 : height;
    if (width % 4 != 0)
    {
        width = ((width + 4) / 4) * 4;
        width = (width > MAX_PLATE_WIDTH) ? MAX_PLATE_WIDTH : width;
    }
    if (height % 4 != 0)
    {
        height = ((height + 4) / 4) * 4;
        height = (height > MAX_PLATE_HEIGHT) ? MAX_PLATE_HEIGHT : height;
    }

    memcpy(plateFrame, srcFrame, sizeof(VIDEO_FRAME_INFO_S));
    plateFrame->stVFrame.u32TimeRef += (10 * (i + 1));
    plateFrame->u32PoolId = u32PoolId;
    plateFrame->stVFrame.u32Stride[0] = MAX_PLATE_WIDTH;
    plateFrame->stVFrame.u32Stride[1] = MAX_PLATE_WIDTH;
    plateFrame->stVFrame.u32Width = width;
    plateFrame->stVFrame.u32Height = height;

    plateFrame->stVFrame.u64PhyAddr[0] = phyAddr;
    plateFrame->stVFrame.u64PhyAddr[1] = phyAddr + MAX_PLATE_WIDTH * height;

    HI_U32 srcDataSize = srcFrame->stVFrame.u32Stride[0] * srcFrame->stVFrame.u32Height * 3 / 2;
    HI_U32 dstDataSize = MAX_PLATE_WIDTH * height * 3 / 2;
    
    HI_U8 *pSrcVirtAddr = (HI_U8*) HI_MPI_SYS_Mmap(srcFrame->stVFrame.u64PhyAddr[0], srcDataSize);
    if (pSrcVirtAddr == NULL) {
        SAMPLE_PRT("FillPlateFrame HI_MPI_SYS_Mmap src frame failed.\n");
        return -1;
    }
    HI_U8 *pDstVirtAddr = (HI_U8*) HI_MPI_SYS_Mmap(phyAddr, dstDataSize);
    if (pDstVirtAddr == NULL) {
        HI_MPI_SYS_Munmap(pSrcVirtAddr, srcDataSize);
        SAMPLE_PRT("FillPlateFrame HI_MPI_SYS_Mmap dst frame failed.\n");
        return -1;
    }

    /* copy Y */
    HI_U8 *srcStart = pSrcVirtAddr + y * srcFrame->stVFrame.u32Stride[0] + x;
    HI_U8 *dstStart = pDstVirtAddr;
    for (int i = 0; i < height; i++)
    {
        memcpy(dstStart, srcStart, width);
        srcStart += srcFrame->stVFrame.u32Stride[0];
        dstStart += plateFrame->stVFrame.u32Stride[0];
    }
    /* copy vu */
    srcStart = pSrcVirtAddr + srcFrame->stVFrame.u32Stride[0] * srcFrame->stVFrame.u32Height 
                + (y / 2) * srcFrame->stVFrame.u32Stride[1] + x;
    dstStart = pDstVirtAddr + plateFrame->stVFrame.u32Stride[0] * height;
    for (int i = 0; i < height / 2; i++)
    {
        memcpy(dstStart, srcStart, width);
        srcStart += srcFrame->stVFrame.u32Stride[1];
        dstStart += plateFrame->stVFrame.u32Stride[1];
    }

    HI_MPI_SYS_Munmap(pSrcVirtAddr, srcDataSize);
    HI_MPI_SYS_Munmap(pDstVirtAddr, dstDataSize);

    return 0;
}

HI_S32 GetJpgdataFromStream(VENC_STREAM_S *pstStream, HI_U8 **jpgData, HI_U32 *data_len)
{
    int i = 0;
    VENC_PACK_S *pstData = NULL;
    HI_U32 totalLen = 0u;
    HI_U8 *data_buf = NULL;
    HI_U32 offset = 0u;

    *data_len = 0;
    *jpgData = NULL;
    
    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        pstData = &pstStream->pstPack[i];
        totalLen += pstData->u32Len - pstData->u32Offset;
    }

    data_buf = (HI_U8 *)malloc(totalLen + 1);
    if (data_buf == NULL)
    {
        SAMPLE_PRT("Plate_get_jpgdata_from_stream malloc failed!\n");
        return -1;
    }
    
    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        pstData = &pstStream->pstPack[i];
        memcpy(data_buf + offset, pstData->pu8Addr + pstData->u32Offset, pstData->u32Len - pstData->u32Offset);
        offset += pstData->u32Len - pstData->u32Offset;
    }

    *data_len = totalLen;
    *jpgData = data_buf;
    
    return 0;
}


HI_S32 GetJpegDataFromVenc(VENC_CHN VencChn, HI_U8 **jpgData, HI_U32 *jpgDataLen)
{
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 s32VencFd;
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stStream;
    HI_S32 s32Ret;
    MPP_CHN_S stDestChn;
    MPP_CHN_S stSrcChn;
    VPSS_CHN_ATTR_S stVpssChnAttr;

    /******************************************
     step 3:  Start Recv Venc Pictures
    ******************************************/
    stDestChn.enModId  = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VencChn;
    s32Ret = HI_MPI_SYS_GetBindbyDest(&stDestChn, &stSrcChn);
    if (s32Ret == HI_SUCCESS && stSrcChn.enModId == HI_ID_VPSS)
    {
        s32Ret = HI_MPI_VPSS_GetChnAttr(stSrcChn.s32DevId, stSrcChn.s32ChnId, &stVpssChnAttr);
        if (s32Ret == HI_SUCCESS && stVpssChnAttr.enCompressMode != COMPRESS_MODE_NONE)
        {
            s32Ret = HI_MPI_VPSS_TriggerSnapFrame(stSrcChn.s32DevId, stSrcChn.s32ChnId, 1);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("call HI_MPI_VPSS_TriggerSnapFrame Grp = %d, ChanId = %d, SnapCnt = %d return failed(0x%x)!\n",
                    stSrcChn.s32DevId, stSrcChn.s32ChnId, 1, s32Ret);

                return HI_FAILURE;
            }
        }
    }

    /******************************************
     step 4:  recv picture
    ******************************************/
    s32VencFd = HI_MPI_VENC_GetFd(VencChn);
    if (s32VencFd < 0)
    {
        SAMPLE_PRT("HI_MPI_VENC_GetFd faild with%#x!\n", s32VencFd);
        return HI_FAILURE;
    }

    FD_ZERO(&read_fds);
    FD_SET(s32VencFd, &read_fds);
    TimeoutVal.tv_sec  = 10;
    TimeoutVal.tv_usec = 0;
    s32Ret = select(s32VencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
    if (s32Ret < 0)
    {
        SAMPLE_PRT("snap select failed!\n");
        return HI_FAILURE;
    }
    else if (0 == s32Ret)
    {
        SAMPLE_PRT("snap time out!\n");
        return HI_FAILURE;
    }
    else
    {
        if (FD_ISSET(s32VencFd, &read_fds))
        {
            s32Ret = HI_MPI_VENC_QueryStatus(VencChn, &stStat);
            if (s32Ret != HI_SUCCESS)
            {
                SAMPLE_PRT("HI_MPI_VENC_QueryStatus failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }
            /*******************************************************
            suggest to check both u32CurPacks and u32LeftStreamFrames at the same time,for example:
             if(0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
             {                SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                return HI_SUCCESS;
             }
             *******************************************************/
            if (0 == stStat.u32CurPacks)
            {
                SAMPLE_PRT("NOTE: Current  frame is NULL!\n");
                return HI_SUCCESS;
            }
            stStream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
            if (NULL == stStream.pstPack)
            {
                SAMPLE_PRT("malloc memory failed!\n");
                return HI_FAILURE;
            }
            stStream.u32PackCount = stStat.u32CurPacks;
            s32Ret = HI_MPI_VENC_GetStream(VencChn, &stStream, -1);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_GetStream failed with %#x!\n", s32Ret);

                free(stStream.pstPack);
                stStream.pstPack = NULL;
                return HI_FAILURE;
            }
            //SAMPLE_PRT("Get jpeg stream success, XXXXXXXXXXXXXXXXXX\n");
            GetJpgdataFromStream(&stStream, jpgData, jpgDataLen);
            s32Ret = HI_MPI_VENC_ReleaseStream(VencChn, &stStream);
            if (HI_SUCCESS != s32Ret)
            {
                SAMPLE_PRT("HI_MPI_VENC_ReleaseStream failed with %#x!\n", s32Ret);

                free(stStream.pstPack);
                stStream.pstPack = NULL;

                return HI_FAILURE;
            }

            free(stStream.pstPack);
            stStream.pstPack = NULL;
        }
    }
    
    return HI_SUCCESS;
}

HI_U8 *EncodeYuvDataToJpegData(VENC_CHN VencChn, VIDEO_FRAME_INFO_S *frame, HI_U8 **jpgData, HI_U32 *jpgDataLen)
{
    VENC_CHN_ATTR_S stAttr;
    HI_S32 ret = HI_MPI_VENC_GetChnAttr(VencChn, &stAttr);
    if (ret != 0)
    {
        SAMPLE_PRT("HI_MPI_VENC_GetChnAttr failed - %d!\n", ret);
        return NULL;
    }

    SAMPLE_PRT("EncodeYuvDataToJpegData frame, width: %d, height: %d\n", 
        frame->stVFrame.u32Width, frame->stVFrame.u32Height);
    stAttr.stVencAttr.u32PicWidth = frame->stVFrame.u32Width;
    stAttr.stVencAttr.u32PicHeight = frame->stVFrame.u32Height;
    ret = HI_MPI_VENC_SetChnAttr(VencChn, &stAttr);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VENC_SetChnAttr failed - 0x%08x!\n", ret);
        return NULL;
    }

    VENC_RECV_PIC_PARAM_S stRecvParam;
    stRecvParam.s32RecvPicNum = 1;
    HI_MPI_VENC_StartRecvFrame(VencChn, &stRecvParam);

    ret = HI_MPI_VENC_SendFrame(VencChn, frame ,-1);
    if (ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VENC_SendFrame failed - 0X%08x!\n", (HI_U32)ret);
    }
    GetJpegDataFromVenc(VencChn, jpgData, jpgDataLen);

    HI_MPI_VENC_StopRecvFrame(VencChn);

    return NULL;
}

HI_S32 CallPlateCarDetectAlg(VIDEO_FRAME_INFO_S *frame, PARKING_GUIDANCE_OUT_INFO *pstOutInfo)
{
    static time_t prevCallAlgTime = 0;
    const  int callAlgInterval = 5;
    time_t now = CurrentTimestamp();
    uint32_t callAlgTimeOut = CheckIsTimeout(now, prevCallAlgTime, callAlgInterval);
    uint32_t equipment = HI_FALSE;
    #ifdef EQUIPMENT_MODE
    equipment = HI_TRUE;
    SAMPLE_PRT("CallPlateCarDetectAlg Run on EQUIPMENT_MODE\n");
    #endif 

    HI_S32 ret = 0;
    
    if (frame->stVFrame.enPixelFormat != PIXEL_FORMAT_YVU_SEMIPLANAR_420)
    {
        SAMPLE_PRT("CallPlateCarDetectAlg unsupport pixel format - %d\n", frame->stVFrame.enPixelFormat);
        return -1;
    }
    if (frame->stVFrame.u32Stride[0] != frame->stVFrame.u32Width) {
        SAMPLE_PRT("CallPlateCarDetectAlg  stride and width is not equal, %u %u\n", 
            frame->stVFrame.u32Stride[0], frame->stVFrame.u32Width);
        return -1;
    }
    HI_U32 dataSize = (frame->stVFrame.u32Stride[0]) * (frame->stVFrame.u32Height) * 3 / 2;
    HI_U64 phy_addr = frame->stVFrame.u64PhyAddr[0];
    HI_U8 *pUserVirtAddr = (uint8_t*) HI_MPI_SYS_Mmap(phy_addr, dataSize);
    if (HI_NULL == pUserVirtAddr)
    {
        SAMPLE_PRT("CallPlateCarDetectAlg HI_MPI_SYS_Mmap failed\n");
        return -1;
    }

    struct timespec beforeAlg = {0};
    struct timespec afterAlg = {0};

    if (g_TestAlg) { clock_gettime(CLOCK_MONOTONIC, &beforeAlg); }
    ParkingGuidanceIOProcess(pUserVirtAddr, frame->stVFrame.u32Width, frame->stVFrame.u32Height);
    if (callAlgTimeOut && (!equipment)) {
        ParkingGuidanceIOGetInfo(pstOutInfo);
        prevCallAlgTime = now;
        ret = 0;
    } else {
        ret = 1;
    }

    if (g_TestAlg) {
        clock_gettime(CLOCK_MONOTONIC, &afterAlg);

        uint64_t msecBefore = beforeAlg.tv_sec * 1000 + beforeAlg.tv_nsec / 1000 / 1000;
        uint64_t msecAfter = afterAlg.tv_sec * 1000 + afterAlg.tv_nsec / 1000 / 1000;
        uint64_t runMsec = msecAfter - msecBefore;
        SAMPLE_PRT("Alg run time mseconds: %llu\n", runMsec);
    }
    
    HI_MPI_SYS_Munmap(pUserVirtAddr, dataSize);

    return ret;
}

int ProcessGetPicReq(struct ServerProtoHeader *req, HI_U8 *jpgData, HI_U32 jpgLen)
{
    struct GetPictureStru *picReq = (struct GetPictureStru *)req;
    struct ServerResponseHdr response = {0};
    int err = 0;
    int ret = OK;    
    char *base64 = NULL;
    cJSON *rootObj = NULL;
    cJSON *arrObj = NULL;
    cJSON *itmObj = NULL;
    char *jsonData = NULL;
        
    if (jpgLen == 0 || jpgData == NULL) {
        ret = ERROR;
        goto RET;
    }
    
    size_t buffLen = jpgLen * 2;
    base64 = (char *)malloc(buffLen);
    if (base64 == NULL) {
        ret = ERROR;
        goto RET;
    }
    memset(base64, 0, buffLen);

    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, ERROR);
    cJSON_AddStringToObject(rootObj, "result", "OK");
    cJSON_AddNumberToObject(rootObj, "com", 7);
    cJSON_AddNumberToObject(rootObj, "parknum", g_Allconfig.algParam.nPlaceNum);

    EXEC_NE((arrObj = cJSON_CreateArray()), NULL, ERROR);
    cJSON_AddItemToObject(rootObj, "stPlaceCoord", arrObj);

    for (int i = 0; i < g_Allconfig.algParam.nPlaceNum; i++) {
        EXEC_NE((itmObj = cJSON_CreateObject()), NULL, ERROR);
        cJSON_AddItemToArray(arrObj, itmObj);
        cJSON_AddStringToObject(itmObj, "carnum", g_Allconfig.placeCfgList[i].id);
        cJSON_AddNumberToObject(itmObj, "x1", g_Allconfig.placeCfgList[i].x1);
        cJSON_AddNumberToObject(itmObj, "y1", g_Allconfig.placeCfgList[i].y1);
        cJSON_AddNumberToObject(itmObj, "x2", g_Allconfig.placeCfgList[i].x2);
        cJSON_AddNumberToObject(itmObj, "y2", g_Allconfig.placeCfgList[i].y2);
        cJSON_AddNumberToObject(itmObj, "x3", g_Allconfig.placeCfgList[i].x3);
        cJSON_AddNumberToObject(itmObj, "y3", g_Allconfig.placeCfgList[i].y3);
        cJSON_AddNumberToObject(itmObj, "x4", g_Allconfig.placeCfgList[i].x4);
        cJSON_AddNumberToObject(itmObj, "y4", g_Allconfig.placeCfgList[i].y4);
    }

    base64_encode_http(jpgData, jpgLen, base64, buffLen);
    cJSON_AddStringToObject(rootObj, "pic", base64);
    jsonData = cJSON_Print(rootObj);
    EXEC_NE(jsonData, NULL, ERROR);
    //SAMPLE_PRT("jsonData:\n%s\n", jsonData);
    
    ret = OK;
RET:
    response.result = htonl(ret);
    response.len = htonl(0);
    if (jsonData != NULL) {
        response.len = htonl(strlen(jsonData));
    }
    WriteDataFromSocket(picReq->fd, &response, sizeof(response), &err);
    if (jsonData != NULL) {
        int off = WriteDataFromSocket(picReq->fd, jsonData, strlen(jsonData), &err);
        SAMPLE_PRT("jsonData Len: %d, off: %d\n", strlen(jsonData), off);
    }
    
    close(picReq->fd);
    free(req);

    if (base64 != NULL) {
        free(base64);
    }
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    if (jsonData != NULL) {
        free(jsonData);
    }

    return ret;
}

int ProcessSetAlgParamReq(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    
    cJSON *rootObj = NULL;
    rootObj = cJSON_Parse(algParamReq->jsonData);
    if (rootObj == NULL) {
        ret = ERROR;
        goto RET;
    }

    PARKING_GUIDANCE_CONFIG tmpParam = {0};
    int placeNum = 0;
    if ((ret = GetIntValFromJson(rootObj, "nPlaceNum", &placeNum)) != 0) {
        SAMPLE_PRT("Can't get nPlaceNum from post data!\n");
        ret = ERROR;
        goto RET;
    }
    tmpParam.nPlaceNum = placeNum;

    if ((ret = GetStringValFromJson(rootObj, "pDefaultProvince", 
        tmpParam.pDefaultProvince, sizeof(tmpParam.pDefaultProvince))) != 0) {
        SAMPLE_PRT("Can't get pDefaultProvince from post data!\n");
        ret = ERROR;
        goto RET;
    }

    PlaceInterfaceStru tmpPlaceCfgList[MAX_PLACE_NUM] = {0};
    memcpy(tmpPlaceCfgList, g_Allconfig.placeCfgList, sizeof(tmpPlaceCfgList));
    ret =  GetPlaceCoordFromJson(rootObj, tmpPlaceCfgList);
    if (ret != 0) {
        ret = ERROR;
        goto RET;
    }
    TranslatePlacePosToAlg(tmpPlaceCfgList, placeNum, &tmpParam);

    memcpy(&(g_Allconfig.placeCfgList), tmpPlaceCfgList, sizeof(g_Allconfig.placeCfgList));
    memcpy(&(g_Allconfig.algParam), &tmpParam, sizeof(g_Allconfig.algParam));
    ParkingGuidanceIOConfig(&(g_Allconfig.algParam));

    DumpConfigToJson(&g_Allconfig);

    SAMPLE_PRT("nPlaceNum: %d\n", g_Allconfig.algParam.nPlaceNum);
    SAMPLE_PRT("pDefaultProvince: %s\n", g_Allconfig.algParam.pDefaultProvince);
    for (int i = 0; i < g_Allconfig.algParam.nPlaceNum; i++) {
        SAMPLE_PRT("Place area %d: carnum: %s, x0: %d, y0: %d, x1: %d, y1: %d, x2: %d, y2: %d, x3: %d, y3: %d\n", 
            i, g_Allconfig.placeCfgList[i].id,
            g_Allconfig.algParam.stPlaceCoord[i + 0].x, g_Allconfig.algParam.stPlaceCoord[i + 0].y,
            g_Allconfig.algParam.stPlaceCoord[i + 1].x, g_Allconfig.algParam.stPlaceCoord[i + 1].y,
            g_Allconfig.algParam.stPlaceCoord[i + 2].x, g_Allconfig.algParam.stPlaceCoord[i + 2].y,
            g_Allconfig.algParam.stPlaceCoord[i + 3].x, g_Allconfig.algParam.stPlaceCoord[i + 3].y);
    }

    char *provinceGb2312 = ProvinceUtf8ToGb2312(g_Allconfig.algParam.pDefaultProvince);
    if (provinceGb2312 != NULL) {
        strcpy(g_Allconfig.algParam.pDefaultProvince, provinceGb2312);
        SAMPLE_PRT("pDefaultProvince gb2312: %s\n", g_Allconfig.algParam.pDefaultProvince);
    }
    ret = OK;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    struct ServerResponseHdr rsp = {0};
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    
    close(algParamReq->fd);
    free(req);
    return ret;
}

int ProcessSetHeartBeatParamReq(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    
    cJSON *rootObj = NULL;
    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, ERROR);

    int heartBeatInterval = 0;
    ret = GetIntValFromJson(rootObj, "interval", &heartBeatInterval);
    JUGE(ret == 0, ERROR);

    char postHeartBeatUrlBuf[MAX_URL_LEN] = {0};
    ret = GetStringValFromJson(rootObj, "url", postHeartBeatUrlBuf, sizeof(postHeartBeatUrlBuf));
    JUGE(ret == 0, ERROR);

    if ((heartBeatInterval == g_Allconfig.heartBeatInterval) &&
        (strncmp(postHeartBeatUrlBuf, g_Allconfig.postHeartBeatUrl, MAX_URL_LEN) == 0)) {
        SAMPLE_PRT("HeartBeat param is equal!\n");
        ret = OK;
        goto RET;
    }
    
    g_Allconfig.heartBeatInterval = heartBeatInterval;
    strncpy(g_Allconfig.postHeartBeatUrl, postHeartBeatUrlBuf, MAX_URL_LEN);
    g_Allconfig.postHeartBeatUrl[MAX_URL_LEN - 1] = '\0';
    ret = OK;

    SAMPLE_PRT("heartBeatInterval: %d, postHeartBeatUrl: %s\n", 
        g_Allconfig.heartBeatInterval, g_Allconfig.postHeartBeatUrl);
    DumpConfigToJson(&g_Allconfig);
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    struct ServerResponseHdr rsp = {0};
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    
    close(algParamReq->fd);
    free(req);
    return ret;
}

int ProcessSetRegularlyParamReq(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    
    cJSON *rootObj = NULL;
    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, ERROR);

    int postJpegInterval = 0;
    ret = GetIntValFromJson(rootObj, "urlParkTime", &postJpegInterval);
    JUGE(ret == 0, ERROR);

    char postJpegUrl[MAX_URL_LEN] = {0};
    ret = GetStringValFromJson(rootObj, "urlPark", postJpegUrl, sizeof(postJpegUrl));
    JUGE(ret == 0, ERROR);

    int heartBeatInterval = 0;
    ret = GetIntValFromJson(rootObj, "urlHearBitTimer", &heartBeatInterval);
    JUGE(ret == 0, ERROR);

    char heartBeatUrl[MAX_URL_LEN] = {0};
    ret = GetStringValFromJson(rootObj, "urlHearBit", heartBeatUrl, sizeof(heartBeatUrl));
    JUGE(ret == 0, ERROR);
    
    g_Allconfig.postJpegInterval = postJpegInterval;
    strncpy(g_Allconfig.postJpegUrl, postJpegUrl, MAX_URL_LEN);
    g_Allconfig.postJpegUrl[MAX_URL_LEN - 1] = '\0';

    
    g_Allconfig.heartBeatInterval = heartBeatInterval;
    strncpy(g_Allconfig.postHeartBeatUrl, heartBeatUrl, MAX_URL_LEN);
    g_Allconfig.postHeartBeatUrl[MAX_URL_LEN - 1] = '\0';
    ret = OK;

    SAMPLE_PRT("postJpegInterval: %d, postJpegUrl: %s, hearbeat: %d, %s\n", 
        g_Allconfig.postJpegInterval, g_Allconfig.postJpegUrl,
        heartBeatInterval, g_Allconfig.postHeartBeatUrl);
    DumpConfigToJson(&g_Allconfig);
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    struct ServerResponseHdr rsp = {0};
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    
    close(algParamReq->fd);
    free(req);
    return ret;
}

int ProcessGetRegularlyParamReq(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    cJSON *rootObj = NULL;
    char *rspdata = NULL;
    struct ServerResponseHdr rsp = {0};
    
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);
    
    cJSON_AddNumberToObject(rootObj, "com", 11);
    
    cJSON_AddStringToObject(rootObj, "urlPark", g_Allconfig.postJpegUrl);
    cJSON_AddNumberToObject(rootObj, "urlParkTime", g_Allconfig.postJpegInterval);

    cJSON_AddStringToObject(rootObj, "urlHearBit", g_Allconfig.postHeartBeatUrl);
    cJSON_AddNumberToObject(rootObj, "urlHearBitTimer", g_Allconfig.heartBeatInterval);
    rspdata = cJSON_Print(rootObj);
    if (rspdata == NULL) {
        ret = -1;
    }

    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    SAMPLE_PRT("ret: %d, rsp: %s\n", ret, rspdata);
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    if (rspdata != NULL) {
        rsp.len = htonl(strlen(rspdata));
    }
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    if (rspdata != NULL) {
        WriteDataFromSocket(algParamReq->fd, rspdata, strlen(rspdata), NULL);
        free(rspdata);
    }
    
    close(algParamReq->fd);
    free(req);
    return ret;
}


int ProcessRebootReq(struct ServerProtoHeader *req)
{
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    
    struct ServerResponseHdr rsp = {0};
    rsp.result = htonl(0);
    rsp.len = htonl(0);
    
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    close(algParamReq->fd);
    free(req);

    g_rebootSystem = HI_TRUE;
    
    return 0;

}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    pthread_mutex_lock(&(g_upgState.mutex));
    g_upgState.currSize += (size * nmemb);
    g_upgState.progress = (int)(((double)g_upgState.currSize / g_upgState.fileSize) * 100);
    pthread_mutex_unlock(&(g_upgState.mutex));

    return fwrite(ptr, size, nmemb, stream);
}



void *ProcessUpgThread(void *param)
{
    int ret = UPG_PROCESS;
    FILE *fp = NULL;
    char *url = (char *)param;
    JUGE(url != NULL, UPG_FAILED);
    
    pthread_mutex_lock(&(g_upgState.mutex));
    g_upgState.progress = 0;
    g_upgState.fileSize = 0;
    g_upgState.currSize = 0;
    g_upgState.state = UPG_PROCESS;
    pthread_mutex_unlock(&(g_upgState.mutex));

    fp = fopen(g_upgradeTmpFile, "w");
    JUGE(fp != NULL, UPG_FAILED);

    g_upgState.fileSize = GetRemoteFileSize(url);
    JUGE(g_upgState.fileSize > 0, UPG_FAILED);

    ret = GetRemoteFile(url, write_data, fp);
    JUGE(ret == 0, UPG_FAILED);

    SAMPLE_PRT("upgrade success, url: %s, filesize: %d\n", url, g_upgState.fileSize);
    ret = UPG_PROCESS;
RET:
    if (fp != NULL) {
        fclose(fp);
    }
    if (url != NULL) {
        free(url);
    }
    
    pthread_mutex_lock(&(g_upgState.mutex));
    g_upgState.state = ret;    
    if (g_upgState.progress == 100 && g_upgState.state == UPG_PROCESS) {
        g_upgState.state = UPG_SUCCESS;
    }
    pthread_mutex_unlock(&(g_upgState.mutex));
    printf("thread: %d %d\n", g_upgState.state, g_upgState.progress);

    return NULL;
}

int ProcessUpgradeTrigger(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    cJSON *rootObj = NULL;

    UPGRADE_STATE currState = g_upgState.state;
    if (currState == UPG_PROCESS) {
        ret = ERROR;
        goto RET;
    }
    
    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, ERROR);
    
    char upgUrl[MAX_URL_LEN] = {0};
    ret = GetStringValFromJson(rootObj, "url", upgUrl, sizeof(upgUrl));
    JUGE(ret == 0, ERROR);

    g_upgState.progress = 0;
    g_upgState.state = UPG_PROCESS;
    g_upgState.fileSize = 0;

    pthread_t tid;
    ret = pthread_create(&tid, NULL, ProcessUpgThread, (void *)strdup(upgUrl));
    if (ret == 0) {
        pthread_detach(tid);
    } else {
        g_upgState.progress = 0;
        g_upgState.state = UPG_FAILED;
        goto RET;
    }
    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    struct ServerResponseHdr rsp = {0};
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    close(algParamReq->fd);
    free(req);

    return ret;
}

char *TranslateUpgState(UPGRADE_STATE state)
{
    char *stateStrList[] = {
        "Not Start",
        "Progress",
        "Success",
        "Failed"
    };

    if (state < 0 || state >= (sizeof(stateStrList) / sizeof(stateStrList[0]))) {
        return "Not Start";
    }

    return stateStrList[state];
}

int ProcessUpgradeGetState(struct ServerProtoHeader *req)
{
    struct SetAlgParamStru *stateReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr response = {0};
    int ret = 0;
    cJSON *rootObj = NULL;
    int err = 0;
    char *responseJson = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);

    UPGRADE_STATE currSate = UPG_NOT_START;
    int progress = 0;

    pthread_mutex_lock(&(g_upgState.mutex));
    currSate = g_upgState.state;
    progress = g_upgState.progress;
    if (progress == 100 && currSate == UPG_PROCESS) {
        g_upgState.state = UPG_SUCCESS;
    }
    pthread_mutex_unlock(&(g_upgState.mutex));

    cJSON_AddStringToObject(rootObj, "result", "OK");
    cJSON_AddStringToObject(rootObj, "state", TranslateUpgState(currSate));
    cJSON_AddNumberToObject(rootObj, "Postion", progress);
    responseJson = cJSON_Print(rootObj);
    if (responseJson != NULL) {
        ret = 0;
    } else {
        ret = -1;
    }
RET:
    response.result = htonl(ret);
    response.len = htonl(0);
    if (responseJson != NULL) {
        response.len = htonl(strlen(responseJson));
    }
    WriteDataFromSocket(stateReq->fd, &response, sizeof(response), &err);
    if (responseJson != NULL) {
        WriteDataFromSocket(stateReq->fd, responseJson, strlen(responseJson), &err);
    }

    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    if (responseJson != NULL) {
        free(responseJson);
    }
    
    close(stateReq->fd);
    free(req);
    
    return ret;
}

int ProcessTriggerUploadPic(struct ServerProtoHeader *req)
{
    struct SetAlgParamStru *stateReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr response = {0};
    int err = 0;

    g_triggerUploadPic = HI_TRUE;

    response.result = htonl(0);
    response.len = htonl(0);

    WriteDataFromSocket(stateReq->fd, &response, sizeof(response), &err);
    close(stateReq->fd);
    free(req);
    
    return 0;
}

int ProcSetPlaceStatus(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr rsp = {0};
    cJSON *rootObj = NULL;
    char tmpbuf[MAX_ID_LEN] = {0};
    
    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, ERROR);

    EXEC_EQ(GetStringValFromJson(rootObj, "postion", tmpbuf, sizeof(tmpbuf)), 0, -1);
    strncpy(g_Allconfig.postion, tmpbuf, sizeof(g_Allconfig.postion));

    EXEC_EQ(GetStringValFromJson(rootObj, "carnum1", tmpbuf, sizeof(tmpbuf)), 0, -1);
    strncpy(g_Allconfig.placeCfgList[0].id, tmpbuf, sizeof(g_Allconfig.placeCfgList[0].id));

    EXEC_EQ(GetStringValFromJson(rootObj, "carnum2", tmpbuf, sizeof(tmpbuf)), 0, -1);
    strncpy(g_Allconfig.placeCfgList[1].id, tmpbuf, sizeof(g_Allconfig.placeCfgList[1].id));

    EXEC_EQ(GetStringValFromJson(rootObj, "carnum3", tmpbuf, sizeof(tmpbuf)), 0, -1);
    strncpy(g_Allconfig.placeCfgList[2].id, tmpbuf, sizeof(g_Allconfig.placeCfgList[2].id));

    EXEC_EQ(GetIntValFromJson(rootObj, "carnumEnable1", &(g_Allconfig.placeCfgList[0].enable)), 0, -1);
    EXEC_EQ(GetIntValFromJson(rootObj, "carnumEnable2", &(g_Allconfig.placeCfgList[1].enable)), 0, -1);
    EXEC_EQ(GetIntValFromJson(rootObj, "carnumEnable3", &(g_Allconfig.placeCfgList[2].enable)), 0, -1);

    int plateCfgState = 0;
    EXEC_EQ(GetIntValFromJson(rootObj, "plateCfgState1", &(plateCfgState)), 0, -1);
    JUGE((plateCfgState >= PLATE_CONFIG_AUTO && plateCfgState <= PLATE_CONFIG_FULL), -1);
    g_Allconfig.placeCfgList[0].plateCfgState = plateCfgState;
    
    EXEC_EQ(GetIntValFromJson(rootObj, "plateCfgState2", &(plateCfgState)), 0, -1);
    JUGE((plateCfgState >= PLATE_CONFIG_AUTO && plateCfgState <= PLATE_CONFIG_FULL), -1);
    g_Allconfig.placeCfgList[1].plateCfgState = plateCfgState;

    EXEC_EQ(GetIntValFromJson(rootObj, "plateCfgState3", &(plateCfgState)), 0, -1);
    JUGE((plateCfgState >= PLATE_CONFIG_AUTO && plateCfgState <= PLATE_CONFIG_FULL), -1);
    g_Allconfig.placeCfgList[2].plateCfgState = plateCfgState;

    DumpConfigToJson(&g_Allconfig);
    
    ret = 0;
RET:
    printf("postion: %s, carnum: %s, %s, %s, enb: %d,%d,%d\n", g_Allconfig.postion, 
        g_Allconfig.placeCfgList[0].id, g_Allconfig.placeCfgList[1].id, g_Allconfig.placeCfgList[2].id,
        g_Allconfig.placeCfgList[0].enable, g_Allconfig.placeCfgList[1].enable, g_Allconfig.placeCfgList[2].enable);
    
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    
    close(algParamReq->fd);
    free(req);
    
    return ret;
}

int ProcGetPlaceStatus(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr rsp = {0};
    cJSON *rootObj = NULL;
    char *rspdata = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);

    cJSON_AddNumberToObject(rootObj, "com", 9);
    cJSON_AddStringToObject(rootObj, "postion", g_Allconfig.postion);

    cJSON_AddStringToObject(rootObj, "carnum1", g_Allconfig.placeCfgList[0].id);
    cJSON_AddNumberToObject(rootObj, "carnumEnable1", g_Allconfig.placeCfgList[0].enable);
    cJSON_AddNumberToObject(rootObj, "plateCfgState1", g_Allconfig.placeCfgList[0].plateCfgState);

    cJSON_AddStringToObject(rootObj, "carnum2", g_Allconfig.placeCfgList[1].id);
    cJSON_AddNumberToObject(rootObj, "carnumEnable2", g_Allconfig.placeCfgList[1].enable);
    cJSON_AddNumberToObject(rootObj, "plateCfgState2", g_Allconfig.placeCfgList[1].plateCfgState);

    cJSON_AddStringToObject(rootObj, "carnum3", g_Allconfig.placeCfgList[2].id);
    cJSON_AddNumberToObject(rootObj, "carnumEnable3", g_Allconfig.placeCfgList[2].enable);
    cJSON_AddNumberToObject(rootObj, "plateCfgState3", g_Allconfig.placeCfgList[2].plateCfgState);
    rspdata = cJSON_Print(rootObj);

    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    SAMPLE_PRT("ret: %d, rsp: %s\n", ret, rspdata);
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    if (rspdata != NULL) {
        rsp.len = htonl(strlen(rspdata));
    }
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    if (rspdata != NULL) {
        WriteDataFromSocket(algParamReq->fd, rspdata, strlen(rspdata), NULL);
        free(rspdata);
    }
    
    close(algParamReq->fd);
    free(req);
    
    return ret;
}

int ProcessSetCtrIPAddr(struct ServerProtoHeader *req)
{
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr response = {0};
    int err = 0;
    int ret = 0;
    cJSON *rootObj = NULL;
    cJSON *itmObj = NULL;

    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, ERROR);
    
    itmObj = cJSON_GetObjectItem(rootObj, "ControlIP");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);
    strncpy(g_Allconfig.ControlIP, itmObj->valuestring, sizeof(g_Allconfig.ControlIP));

    itmObj = cJSON_GetObjectItem(rootObj, "ControledIP");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);
    strncpy(g_Allconfig.ControledIP, itmObj->valuestring, sizeof(g_Allconfig.ControledIP));

    DumpConfigToJson(&g_Allconfig);
    ret = 0;
RET:
    response.result = htonl(ret);
    response.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &response, sizeof(response), &err);
    close(algParamReq->fd);
    free(req);

    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    
    return 0;
}

int ProcessGetCtrIPAddr(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr rsp = {0};
    cJSON *rootObj = NULL;
    char *rspdata = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);

    cJSON_AddStringToObject(rootObj, "result", "OK");
    cJSON_AddNumberToObject(rootObj, "com", 18);
    cJSON_AddStringToObject(rootObj, "ControlIP", g_Allconfig.ControlIP);
    cJSON_AddStringToObject(rootObj, "ControledIP", g_Allconfig.ControledIP);

    rspdata = cJSON_Print(rootObj);

    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    SAMPLE_PRT("ret: %d, rsp: %s\n", ret, rspdata);
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    if (rspdata != NULL) {
        rsp.len = htonl(strlen(rspdata));
    }
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    if (rspdata != NULL) {
        WriteDataFromSocket(algParamReq->fd, rspdata, strlen(rspdata), NULL);
        free(rspdata);
    }
    
    close(algParamReq->fd);
    free(req);
    
    return ret;
}

int ProcessSetLightSlaveInfo(struct ServerProtoHeader *req)
{
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr response = {0};
    int err = 0;
    int ret = 0;
    cJSON *rootObj = NULL;
    cJSON *itmObj = NULL;

    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, ERROR);
    
    itmObj = cJSON_GetObjectItem(rootObj, "Mac");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);
    strncpy(g_Allconfig.LightSlaveMac, itmObj->valuestring, sizeof(g_Allconfig.LightSlaveMac));

    itmObj = cJSON_GetObjectItem(rootObj, "IP");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);
    strncpy(g_Allconfig.LightSlaveIP, itmObj->valuestring, sizeof(g_Allconfig.LightSlaveIP));

    DumpConfigToJson(&g_Allconfig);
    ret = 0;
RET:
    response.result = htonl(ret);
    response.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &response, sizeof(response), &err);
    close(algParamReq->fd);
    free(req);

    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    
    return 0;
}

int ProcessGetPrivateInfo(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr rsp = {0};
    cJSON *rootObj = NULL;
    char *rspdata = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);

    cJSON_AddStringToObject(rootObj, "result", "OK");
    cJSON_AddNumberToObject(rootObj, "com", 0);
    cJSON_AddNumberToObject(rootObj, "ParkingPlaceState", g_prevParkingPlaceState);

    uint16_t bitMapRst = MAX_PARKING_PLACE_NUM << 8;
    bitMapRst |= g_parkingPlaceStateBitmap;
    cJSON_AddNumberToObject(rootObj, "ParkingPlaceStateBitmap", bitMapRst);

    rspdata = cJSON_Print(rootObj);

    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    SAMPLE_PRT("ret: %d, rsp: %s\n", ret, rspdata);
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    if (rspdata != NULL) {
        rsp.len = htonl(strlen(rspdata));
    }
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    if (rspdata != NULL) {
        WriteDataFromSocket(algParamReq->fd, rspdata, strlen(rspdata), NULL);
        free(rspdata);
    }
    
    close(algParamReq->fd);
    free(req);
    
    return ret;
}

int ProcessSetLightState(struct ServerProtoHeader *req)
{
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr response = {0};
    int err = 0;
    int ret = 0;
    cJSON *rootObj = NULL;
    cJSON *itmObj = NULL;
    int color = 0;

    rootObj = cJSON_Parse(algParamReq->jsonData);
    JUGE(rootObj != NULL, AUTO_ERR_CODE);
    
    itmObj = cJSON_GetObjectItem(rootObj, "type");
    JUGE(itmObj != NULL, AUTO_ERR_CODE);
    JUGE(itmObj->type == cJSON_Number, AUTO_ERR_CODE);
    color = itmObj->valueint - 1;
    JUGE(color >= LIGHT_RED || color <= LIGHT_FLASHING, AUTO_ERR_CODE);

    SetLightState(color);
    ret = 0;
RET:
    response.result = htonl(ret);
    response.len = htonl(0);
    WriteDataFromSocket(algParamReq->fd, &response, sizeof(response), &err);
    close(algParamReq->fd);
    free(req);

    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    
    return 0;
}

int ProcessGetPlateInfo(struct ServerProtoHeader *req)
{
    int ret = 0;
    struct SetAlgParamStru *algParamReq = (struct SetAlgParamStru *)req;
    struct ServerResponseHdr rsp = {0};
    cJSON *rootObj = NULL;
    char *rspdata = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, -1);

    cJSON_AddStringToObject(rootObj, "result", "OK");
    cJSON_AddNumberToObject(rootObj, "com", 21);
    cJSON_AddStringToObject(rootObj, "park1Plate", curPlateInfo[0]);
    cJSON_AddStringToObject(rootObj, "park2Plate", curPlateInfo[1]);
    cJSON_AddStringToObject(rootObj, "park3Plate", curPlateInfo[2]);

    rspdata = cJSON_Print(rootObj);

    ret = 0;
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }

    SAMPLE_PRT("ret: %d, rsp: %s\n", ret, rspdata);
    rsp.result = htonl(ret);
    rsp.len = htonl(0);
    if (rspdata != NULL) {
        rsp.len = htonl(strlen(rspdata));
    }
    WriteDataFromSocket(algParamReq->fd, &rsp, sizeof(rsp), NULL);
    if (rspdata != NULL) {
        WriteDataFromSocket(algParamReq->fd, rspdata, strlen(rspdata), NULL);
        free(rspdata);
    }
    
    close(algParamReq->fd);
    free(req);
    
    return ret;
}


int ProcessHttpProcReq(HI_U8 *jpgData, HI_U32 jpgLen)
{
    struct ServerProtoHeader *req = (struct ServerProtoHeader *)GetWebServerRequest();
    if (req == NULL) {
        return -1;
    }

    int ret = -1;
    switch (req->cmd) {
    case GET_JPEG:
        ProcessGetPicReq(req, jpgData, jpgLen);
        break;
    case SET_ALG_PARAM:
        ProcessSetAlgParamReq(req);
        break;
    case SET_REGULARLY_PARAM:
        ProcessSetRegularlyParamReq(req);
        break;
    case GET_REGULARLY_PARAM:
        ProcessGetRegularlyParamReq(req);
        break;
    case SET_HEART_BEAT_PARAM:
        ProcessSetHeartBeatParamReq(req);
        break;
    case REBOOT:
        ProcessRebootReq(req);
        break;
    case TRIGGER_UPG_START:
        ProcessUpgradeTrigger(req);
        break;
    case GET_UPG_STATE:
        ProcessUpgradeGetState(req);
        break;
    case TRIGGER_PUSH_DATA:
        ProcessTriggerUploadPic(req);
        break;
    case SET_PLACE_STATUS:
        ProcSetPlaceStatus(req);
        break;
    case GET_PLACE_STATUS:
        ProcGetPlaceStatus(req);
        break;
    case GET_CTR_IPADDR:
        ProcessGetCtrIPAddr(req);
        break;
    case SET_CTR_IPADDR:
        ProcessSetCtrIPAddr(req);
        break;
    case SET_LIGHT_SLAVE_INFO:
        ProcessSetLightSlaveInfo(req);
        break;
    case GET_PRIVATE_INFO:
        ProcessGetPrivateInfo(req);
        break;
    case SET_LIGHT_STATE:
        ProcessSetLightState(req);
        break;
    case GET_PLATE_INFO:
        ProcessGetPlateInfo(req);
        break;
    default:
        break;
    }

    return ret;
}

int AddStringValToHttpHeader(struct curl_slist **list, const char *key, const char *val)
{
    char itmBuf[256];
    snprintf(itmBuf, sizeof(itmBuf), "%s:%s", key, val);
    struct curl_slist *tmp = curl_slist_append(*list, itmBuf);
    if (tmp == NULL) {
        return ERROR;
    }

    *list = tmp;

    return OK;
}

int AddIntValToHttpHeader(struct curl_slist **list, const char *key, int val)
{
    char itmBuf[256];
    snprintf(itmBuf, sizeof(itmBuf), "%s:%d", key, val);

    struct curl_slist *tmp = curl_slist_append(*list, itmBuf);
    if (tmp == NULL) {
        return ERROR;
    }

    *list = tmp;

    return OK;
}

int GetStatusByVehicleAndPlateStatus(int isVehicle, int isPlate)
{
    if (isVehicle && !isPlate) {
        return 1;
    }
    if (!isVehicle && isPlate) {
        return 2;
    }
    if (isVehicle && isPlate) {
        return 3;
    }

    return 0;
}

char* GeneratePostJpegData(PARKING_SINGLE_INFO *parkResult, int idx,
    HI_U8 *jpgData, HI_U32 jpgLen, HI_U8 *smallData, HI_U32 smallLen)
{
    char *ret = NULL;
    cJSON *rootObj = NULL;
    cJSON *headObj = NULL;
    cJSON *bodyObj = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, NULL);
    
    EXEC_NE((headObj = cJSON_CreateObject()), NULL, NULL);
    cJSON_AddItemToObject(rootObj, "head", headObj);
    
    cJSON_AddStringToObject(headObj, "Type", "PKResult");
    cJSON_AddStringToObject(headObj, "SoftVersion", g_SoftWareVersion);
    cJSON_AddStringToObject(headObj, "HardVersion", g_HardWareVersion);
    
    char tmpBuf1[64] = {0};
    char tmpBuf2[64] = {0};
    getIPAddrByName(g_NetDeviceName, tmpBuf1, sizeof(tmpBuf1), tmpBuf2, sizeof(tmpBuf2));
    cJSON_AddStringToObject(headObj, "HostIP", tmpBuf1);
    
    int status = GetStatusByVehicleAndPlateStatus(parkResult->bIsVehicle, parkResult->bIsPlate);
    snprintf(tmpBuf1, sizeof(tmpBuf1), "%d", status);
    cJSON_AddStringToObject(headObj, "Status", tmpBuf1);

    if (parkResult->bIsPlate) {
        cJSON_AddStringToObject(headObj, "PlateNumber", parkResult->pcPlateInfo);
    } else {
        cJSON_AddStringToObject(headObj, "PlateNumber", "");
    }

    snprintf(tmpBuf1, sizeof(tmpBuf1), "%u", parkResult->plateType);
    cJSON_AddStringToObject(headObj, "PlateType", tmpBuf1);

    snprintf(tmpBuf1, sizeof(tmpBuf1), "%u", parkResult->plateReliability);
    cJSON_AddStringToObject(headObj, "Confine", tmpBuf1);
    
    snprintf(tmpBuf1, sizeof(tmpBuf1), "%d", idx + 1);
    cJSON_AddStringToObject(headObj, "ImageIndex", tmpBuf1);
    
    GetTimeNowString(tmpBuf1, sizeof(tmpBuf1));
    cJSON_AddStringToObject(headObj, "RequestTime", tmpBuf1);
    
    EXEC_NE((bodyObj = cJSON_CreateObject()), NULL, NULL);
    cJSON_AddItemToObject(rootObj, "body", bodyObj);

    char *base64All = NULL;
    EXEC_NE((base64All = (char *)malloc(jpgLen * 2)), NULL, NULL);
    base64_encode_http(jpgData, jpgLen, base64All, jpgLen * 2);
    cJSON_AddStringToObject(bodyObj, "ImageAll", base64All);
    free(base64All);
    base64All = NULL;

    snprintf(tmpBuf1, sizeof(tmpBuf1), "%d", jpgLen);
    cJSON_AddStringToObject(bodyObj, "ImageAllSize", tmpBuf1);
    cJSON_AddStringToObject(bodyObj, "ImageDataLength", tmpBuf1);

    if (smallData != NULL) {
        char *base64Small = NULL;
        EXEC_NE((base64Small = (char *)malloc(smallLen * 2)), NULL, NULL);
        base64_encode_http(smallData, smallLen, base64Small, smallLen * 2);
        cJSON_AddStringToObject(bodyObj, "ImageSmall", base64Small);
        free(base64Small);
        base64Small = NULL;

        snprintf(tmpBuf1, sizeof(tmpBuf1), "%d", smallLen);
        cJSON_AddStringToObject(bodyObj, "ImageSmallSize", tmpBuf1);
    } else {
        cJSON_AddStringToObject(bodyObj, "ImageSmall", "");
        cJSON_AddStringToObject(bodyObj, "ImageSmallSize", "0");
    }

    ret = cJSON_Print(rootObj);
    //printf("jpeg data:\n%s\n", ret);
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    return ret;
}

char *GenerateHeartBeatPostData()
{
    char *ret = NULL;
    cJSON *rootObj = NULL;
    cJSON *headObj = NULL;
    EXEC_NE((rootObj = cJSON_CreateObject()), NULL, NULL);
    
    EXEC_NE((headObj = cJSON_CreateObject()), NULL, NULL);
    cJSON_AddItemToObject(rootObj, "head", headObj);

    cJSON_AddStringToObject(headObj, "SoftVersion", g_SoftWareVersion);
    cJSON_AddStringToObject(headObj, "HardVersion", g_HardWareVersion);
    char tmpBuf1[64] = {0};
    char tmpBuf2[64] = {0};
    getIPAddrByName(g_NetDeviceName, tmpBuf1, sizeof(tmpBuf1), tmpBuf2, sizeof(tmpBuf2));
    cJSON_AddStringToObject(headObj, "HostIP", tmpBuf1);

    GetTimeNowString(tmpBuf1, sizeof(tmpBuf1));
    cJSON_AddStringToObject(headObj, "RequestTime", tmpBuf1);

    ret = cJSON_Print(rootObj);
    printf("heart beat:\n%s\n", ret);
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    return ret;
}

int ProcessHeartBeat()
{
    int ret = 0;
    char *heart = NULL;
    PostInfoItem *itm = NULL;
    static time_t prevHeartbeatTime = 0;
    time_t now = CurrentTimestamp();
	uint32_t heartbeatTimeOut = CheckIsTimeout(now, prevHeartbeatTime, g_Allconfig.heartBeatInterval);

    if (strlen(g_Allconfig.postHeartBeatUrl) == 0) {
        return AUTO_ERR_CODE;
    }

    if (heartbeatTimeOut) {
        heart = GenerateHeartBeatPostData();
        JUGE(heart != NULL, AUTO_ERR_CODE);
        itm = (PostInfoItem *)malloc(sizeof(PostInfoItem));
        JUGE(heart != NULL, AUTO_ERR_CODE);

        itm->postUrl = g_Allconfig.postHeartBeatUrl;
        itm->postData = heart;
        ret = fifo_ring_push(g_othPostFifo, itm);
        JUGE(ret == 0, AUTO_ERR_CODE);

        heart = NULL;
        itm = NULL;
        prevHeartbeatTime = now;
    }
RET:
    CHECK_AND_FREE(heart);
    CHECK_AND_FREE(itm);
    return ret;
}

void GetCrediblePlateNum(char *currPlate, char *plateNumBuf, uint32_t idx)
{
    static CheckPlateCredibleItmList allPlaceCheckList[MAX_PLACE_NUM] = {0};

    CheckPlateCredibleItmList *curr = &(allPlaceCheckList[idx]);
    strncpy(curr->plateList[curr->insert], currPlate, PALTE_BUF_LEN);
    curr->plateList[curr->insert][PALTE_BUF_LEN - 1] = '\0';
    curr->insert = (curr->insert + 1) % MAX_CHECK_NUM;

    uint32_t plateCntList[MAX_CHECK_NUM] = {0};
    for (int i = 0; i < MAX_CHECK_NUM; i++) {
        plateCntList[i] = 1;
    }
    
    for (int i = 0; i < MAX_CHECK_NUM; i++) {
        if (plateCntList[i] == 0) {
            continue;
        }
        for (int j = i + 1; j < MAX_CHECK_NUM; j++) {
            if (strcmp(curr->plateList[i], curr->plateList[j]) == 0) {
                plateCntList[i] += 1;
                plateCntList[j]  = 0;
            }
        }
    }

    uint32_t maxIntIdx = 0;
    for (int i = 1; i < MAX_CHECK_NUM; i++) {
        if (plateCntList[i] > plateCntList[maxIntIdx]) {
            maxIntIdx = i;
        }
    }

    strncpy(plateNumBuf, curr->plateList[maxIntIdx], PALTE_BUF_LEN);
    return;
}



void ProcessAlgResult(PARKING_GUIDANCE_OUT_INFO *outInfo, VIDEO_FRAME_INFO_S *frame,
        VB_POOL u32PoolId, HI_U64 phyPlateAddr, VENC_CHN vencChn, char *postUrl, 
        HI_U8 *jpgData, HI_U32 jpgLen)
{
    HI_U8 *smallJpgData = NULL;
    HI_U32 smallJpgDataLen = 0;

    static int preIsVehicle[MAX_PLACE_NUM] = {0};
    static time_t prevUploadJpgTime = 0;
    
    static time_t prevUpdateLightStatus = 0;
    const  int updateLightInterval = 5;
    

    time_t now = CurrentTimestamp();
    uint32_t uploadJpgTimeOut = CheckIsTimeout(now, prevUploadJpgTime, g_Allconfig.postJpegInterval * 60);
    uint32_t updateLightTimeOut = CheckIsTimeout(now, prevUpdateLightStatus, updateLightInterval);
    
    
    char plateUtf8[16] = {0};
    PARKING_PLACE_STATE currParkingPlaceState = PARKING_PLACE_FULL;
    
    for (uint32_t i = 0; i < MAX_PARKING_PLACE_NUM; i++) {
        if (!(outInfo->stParkInfo[i].bIsAvailible) || !(g_Allconfig.placeCfgList[i].enable)) {
            //车位未使能标记为1
            g_parkingPlaceStateBitmap |= (1u << i); 
            continue;
        }
        
        char *provinceUtf8 = NULL;
        memset(plateUtf8, 0, sizeof(plateUtf8));
        if (outInfo->stParkInfo[i].bIsPlate) {
            provinceUtf8 = ProvinceGb2312ToUtf8(outInfo->stParkInfo[i].pcPlateInfo);
            if (provinceUtf8 != NULL) {
                strcat(plateUtf8, provinceUtf8);
                strcat(plateUtf8, outInfo->stParkInfo[i].pcPlateInfo + 2);
                strcpy(outInfo->stParkInfo[i].pcPlateInfo, plateUtf8);
            }
        }
        SAMPLE_PRT("idx: %d, bIsAvailible: %d, bIsPlate: %d, bIsVehicle: %d, pcPlateInfo: %s, x0: %d, y0: %d, x1: %d, y1: %d, type: %d, rel: %d\n", 
            i, outInfo->stParkInfo[i].bIsAvailible, outInfo->stParkInfo[i].bIsPlate, 
            outInfo->stParkInfo[i].bIsVehicle, outInfo->stParkInfo[i].pcPlateInfo, outInfo->stParkInfo[i].rtPlate.x0,
            outInfo->stParkInfo[i].rtPlate.y0, outInfo->stParkInfo[i].rtPlate.x1, outInfo->stParkInfo[i].rtPlate.y1,
            outInfo->stParkInfo[i].plateType, outInfo->stParkInfo[i].plateReliability);

        // 检查是否配置车位状态
        if (g_Allconfig.placeCfgList[i].plateCfgState == PLATE_CONFIG_EMPTY) {
            outInfo->stParkInfo[i].bIsVehicle = HI_FALSE;
            outInfo->stParkInfo[i].bIsPlate = HI_FALSE;
            SAMPLE_PRT("Set plate status empty!\n");
        }

        if (g_Allconfig.placeCfgList[i].plateCfgState == PLATE_CONFIG_FULL) {
            outInfo->stParkInfo[i].bIsVehicle = HI_TRUE;
            SAMPLE_PRT("Set plate status full!\n");
        }
        
        int isVehicel = HI_FALSE;
        char currPlate[10] = {0};
        int postJpgData = HI_FALSE;
        int PostPlateJpgData = HI_FALSE;
        if (outInfo->stParkInfo[i].bIsVehicle) {
            isVehicel = HI_TRUE;
            g_parkingPlaceStateBitmap |= (1u << i);
        } else {
            currParkingPlaceState = PARKING_PLACE_EMPTY;
            g_parkingPlaceStateBitmap &= ~(1u << i);
        }

        //消除抖动，消除偶然识别错误，导致上传错误的车牌
        if (outInfo->stParkInfo[i].bIsPlate) {
            isVehicel = HI_TRUE;
            GetCrediblePlateNum(outInfo->stParkInfo[i].pcPlateInfo, currPlate, i);
            if (strlen(currPlate) == 0) {
                outInfo->stParkInfo[i].bIsPlate = HI_FALSE;
            }
            strncpy(outInfo->stParkInfo[i].pcPlateInfo, currPlate, 10);
        } else {
            GetCrediblePlateNum("", currPlate, i);
        }

        if (isVehicel != preIsVehicle[i]) {
            postJpgData = HI_TRUE;
            preIsVehicle[i] = isVehicel;
        }
        if (memcmp(currPlate, curPlateInfo[i], sizeof(currPlate)) != 0) {
            PostPlateJpgData = HI_TRUE;
            postJpgData = HI_TRUE;
            memcpy(curPlateInfo[i], currPlate, sizeof(currPlate));
        }
        if (uploadJpgTimeOut || g_triggerUploadPic) {
            g_triggerUploadPic = HI_FALSE;
            postJpgData = HI_TRUE;
            if (outInfo->stParkInfo[i].bIsPlate) {
                PostPlateJpgData = HI_TRUE;
            }
        }

        smallJpgData = NULL;
        smallJpgDataLen = 0;
        if (PostPlateJpgData) {
            HI_U32 width = outInfo->stParkInfo[i].rtPlate.x1 - outInfo->stParkInfo[i].rtPlate.x0;
            HI_U32 height = outInfo->stParkInfo[i].rtPlate.y1 - outInfo->stParkInfo[i].rtPlate.y0;
            VIDEO_FRAME_INFO_S plateFrame;
            FillPlateFrame(frame, &plateFrame, u32PoolId, phyPlateAddr, outInfo->stParkInfo[i].rtPlate.x0,
                outInfo->stParkInfo[i].rtPlate.y0, width, height, i);
    
            EncodeYuvDataToJpegData(vencChn, &plateFrame, &smallJpgData, &smallJpgDataLen);
            SAMPLE_PRT("Small jpg data: %p, data len: %u\n", smallJpgData, smallJpgDataLen);
        }
        if (postJpgData && (jpgData != NULL)) {
            char *postData = GeneratePostJpegData(&(outInfo->stParkInfo[i]), i, 
                jpgData, jpgLen, smallJpgData, smallJpgDataLen);
            PostInfoItem *postItm = (PostInfoItem *)malloc(sizeof(PostInfoItem));
            if (postData != NULL && postItm != NULL) {
                SAMPLE_PRT("XXXX Start to upload picture to Server!\n");
                postItm->postUrl = postUrl;
                postItm->postData = postData;
                //post_json_to_server(postUrl, postData, NULL, 0);
                //free(postData);
                if (fifo_ring_push(g_jpgePostFifo, postItm) == 0) {
                    postItm = NULL;
                    postData = NULL;
                    prevUploadJpgTime = now;
                } else {
                    SAMPLE_PRT("YYYY Push jpeg data to fifo failed!\n");
                }
                CHECK_AND_FREE(postItm);
                CHECK_AND_FREE(postData);
            }
        }
        if (smallJpgData != NULL) {
            free(smallJpgData);
        }
    }

    if ((g_prevParkingPlaceState != currParkingPlaceState)  || updateLightTimeOut) {
        g_prevParkingPlaceState = currParkingPlaceState;
        if (fifo_ring_push(g_CtrLightFifo, (void *)currParkingPlaceState) != 0) {
            SAMPLE_PRT("YYYY Push ctrl light work to fifo failed!\n");
        }
        prevUpdateLightStatus = now;
    }
}

UPGRADE_STATE GetCurrentUpgState()
{
    UPGRADE_STATE currState = UPG_NOT_START;
    pthread_mutex_lock(&(g_upgState.mutex));
    currState = g_upgState.state;
    pthread_mutex_unlock(&(g_upgState.mutex));

    return currState;
}


/******************************************************************************
* function: Mjpeg + Jpeg,Channel resolution adaptable with sensor
******************************************************************************/
HI_S32 SAMPLE_VENC_MJPEG_JPEG(void)
{
    HI_S32 i;
    //char  ch;
    HI_S32 s32Ret;
    SIZE_S          stSize[2];
    PIC_SIZE_E      enSize[2]     = {SMALL_STREAM_SIZE, BIG_STREAM_SIZE};
    HI_S32          s32ChnNum     = 2;
    VENC_CHN        VencChn[2]    = {0,1};
    HI_U32          u32Profile[2] = {0,0};
    PAYLOAD_TYPE_E  enPayLoad[2]  = {PT_H264, PT_JPEG};
    VENC_GOP_MODE_E enGopMode     = VENC_GOPMODE_NORMALP;
    VENC_GOP_ATTR_S stGopAttr;
    SAMPLE_RC_E     enRcMode;
    HI_BOOL         bSupportDcf   = HI_TRUE;

    VI_DEV          ViDev         = 0;
    VI_PIPE         ViPipe        = 0;
    VI_CHN          ViChn         = 0;
    SAMPLE_VI_CONFIG_S stViConfig;

    VPSS_GRP        VpssGrp        = 0;
    VPSS_CHN        VpssChn[2]     = {0,1};
    HI_BOOL         abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {1,1,0};
    SAMPLE_VPSS_CHN_ATTR_S stParam;
    SAMPLE_VB_ATTR_S commVbAttr;

    VB_POOL_CONFIG_S stVbPoolCfg;
    //ParkingGuidanceParamStru config = {0};

    // 初始化看门狗
    init_wdt();
    
    g_upgState.progress = 0;
    g_upgState.state = UPG_NOT_START;
    pthread_mutex_init(&(g_upgState.mutex), NULL);

    memset(&g_Allconfig, 0, sizeof(g_Allconfig));
    int ret = ParseConfigJson(g_ConfigFile, &g_Allconfig);
    if (ret != 0) {
        SAMPLE_PRT("SAMPLE_VENC_MJPEG_JPEG parse config json - %d!\n", ret);
        g_Allconfig.algParam.nPlaceNum = MAX_PARKING_PLACE_NUM;
        strcpy(g_Allconfig.ControlIP, "127.0.0.1");
        DumpConfigToJson(&g_Allconfig);
        //return ret;
    }
    if (strlen(g_Allconfig.ControlIP) == 0) {
        strcpy(g_Allconfig.ControlIP, "127.0.0.1");
        DumpConfigToJson(&g_Allconfig);
    }
    
    SAMPLE_PRT("postJpegUrl: %s\n", g_Allconfig.postJpegUrl);
    SAMPLE_PRT("postJpegInterval: %d\n", g_Allconfig.postJpegInterval);

    SAMPLE_PRT("postHeartBeatUrl: %s\n", g_Allconfig.postHeartBeatUrl);
    SAMPLE_PRT("heartBeatInterval: %d\n", g_Allconfig.heartBeatInterval);

    SAMPLE_PRT("ControledIP: %s\n", g_Allconfig.ControledIP);
    SAMPLE_PRT("ControlIP: %s\n", g_Allconfig.ControlIP);

    SAMPLE_PRT("LightSlaveIP: %s\n", g_Allconfig.LightSlaveIP);
    SAMPLE_PRT("LightSlaveMac: %s\n", g_Allconfig.LightSlaveMac);

    SAMPLE_PRT("position: %s\n", g_Allconfig.postion);
    SAMPLE_PRT("nPlaceNum: %d\n", g_Allconfig.algParam.nPlaceNum);
    SAMPLE_PRT("pDefaultProvince: %s\n", g_Allconfig.algParam.pDefaultProvince);
    for (int i = 0; i < g_Allconfig.algParam.nPlaceNum; i++) {
        SAMPLE_PRT("Place area idx: %d, carnum: %s, enb: %d, cfgState: %d, x0: %d, y0: %d, x1: %d, y1: %d, x2: %d, y2: %d, x3: %d, y3: %d\n", i,
            g_Allconfig.placeCfgList[i].id,
            g_Allconfig.placeCfgList[i].enable,
            g_Allconfig.placeCfgList[i].plateCfgState,
            g_Allconfig.algParam.stPlaceCoord[i + 0].x, g_Allconfig.algParam.stPlaceCoord[i + 0].y,
            g_Allconfig.algParam.stPlaceCoord[i + 1].x, g_Allconfig.algParam.stPlaceCoord[i + 1].y,
            g_Allconfig.algParam.stPlaceCoord[i + 2].x, g_Allconfig.algParam.stPlaceCoord[i + 2].y,
            g_Allconfig.algParam.stPlaceCoord[i + 3].x, g_Allconfig.algParam.stPlaceCoord[i + 3].y);
    }

    char *provinceGb2312 = ProvinceUtf8ToGb2312(g_Allconfig.algParam.pDefaultProvince);
    if (provinceGb2312 != NULL) {
        strcpy(g_Allconfig.algParam.pDefaultProvince, provinceGb2312);
        SAMPLE_PRT("pDefaultProvince gb2312: %s\n", g_Allconfig.algParam.pDefaultProvince);
    }
    
    /******************************************
      step 0: Initialize related parameters
    ******************************************/
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_SYS_GetPicSize(enSize[i], &stSize[i]);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("SAMPLE_COMM_SYS_GetPicSize failed!\n");
            return s32Ret;
        }
    }

    SAMPLE_COMM_VI_GetSensorInfo(&stViConfig);

    if(SAMPLE_SNS_TYPE_BUTT == stViConfig.astViInfo[0].stSnsInfo.enSnsType)
    {
        SAMPLE_PRT("Not set SENSOR%d_TYPE !\n",0);
        return HI_FAILURE;
    }

    s32Ret = SAMPLE_VENC_CheckSensor(stViConfig.astViInfo[0].stSnsInfo.enSnsType,stSize[1]);
    if(s32Ret != HI_SUCCESS)
    {
        for (i = 1; i < s32ChnNum; i++)
        {
            s32Ret = SAMPLE_VENC_ModifyResolution(stViConfig.astViInfo[0].stSnsInfo.enSnsType,&enSize[i],&stSize[i]);
            if(s32Ret != HI_SUCCESS)
            {
                return HI_FAILURE;
            }
        }
    }

    SAMPLE_VENC_GetDefaultVpssAttr(stViConfig.astViInfo[0].stSnsInfo.enSnsType, abChnEnable, stSize, &stParam);
    stParam.stOutPutSize[0]  = stSize[1];
    stParam.stOutPutSize[1]  = stSize[0];
    stParam.enCompressMode[0] = COMPRESS_MODE_NONE;
    stParam.enCompressMode[1] = COMPRESS_MODE_NONE;

    /******************************************
      step 1: init sys alloc common vb
    ******************************************/
    memset(&commVbAttr, 0, sizeof(commVbAttr));
    commVbAttr.supplementConfig = VB_SUPPLEMENT_JPEG_MASK;
    SAMPLE_VENC_GetCommVbAttr(stViConfig.astViInfo[0].stSnsInfo.enSnsType, &stParam, bSupportDcf, &commVbAttr);

    s32Ret = SAMPLE_VENC_SYS_Init(&commVbAttr);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("Init SYS err for %#x!\n", s32Ret);
        return s32Ret;
    }
    
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize  = MAX_PLATE_WIDTH * MAX_PLATE_HEIGHT * 3 / 2;
    stVbPoolCfg.u32BlkCnt   = 1;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    VB_POOL u32PoolId = HI_MPI_VB_CreatePool(&stVbPoolCfg);
    if (VB_INVALID_POOLID == u32PoolId)
    {
        SAMPLE_PRT("HI_MPI_VB_CreatePool failed!\n");
        return HI_FAILURE;
    }
    VB_BLK plateBlk = HI_MPI_VB_GetBlock(u32PoolId, MAX_PLATE_WIDTH * MAX_PLATE_HEIGHT * 3 / 2, NULL);
    if (plateBlk == VB_INVALID_HANDLE) {
        SAMPLE_PRT("HI_MPI_VB_GetBlock failed!\n");
        return HI_FAILURE;
    }
    HI_U64 phyPlateAddr = HI_MPI_VB_Handle2PhysAddr(plateBlk);
    if (phyPlateAddr == 0) {
        SAMPLE_PRT("HI_MPI_VB_Handle2PhysAddr failed!\n");
        return HI_FAILURE;
    }
    
    /******************************************
      step 2: init and start vi
    ******************************************/
    stViConfig.s32WorkingViNum       = 1;
    stViConfig.astViInfo[0].stDevInfo.ViDev     = ViDev;
    stViConfig.astViInfo[0].stPipeInfo.aPipe[0] = ViPipe;
    stViConfig.astViInfo[0].stChnInfo.ViChn     = ViChn;
    stViConfig.astViInfo[0].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stViConfig.astViInfo[0].stChnInfo.enPixFormat    = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    s32Ret = SAMPLE_VENC_VI_Init(&stViConfig, stParam.ViVpssMode);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("Init VI err for %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    /******************************************
      step 3: init and start vpss
    ******************************************/
    s32Ret = SAMPLE_VENC_VPSS_Init(VpssGrp, &stParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Init VPSS err for %#x!\n", s32Ret);
        goto EXIT_VI_STOP;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(ViPipe, ViChn, VpssGrp);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("VI Bind VPSS err for %#x!\n", s32Ret);
        goto EXIT_VPSS_STOP;
    }
    SAMPLE_VENC_SetDCFInfo(ViPipe);

   /******************************************
     start stream venc
    ******************************************/

    enRcMode = SAMPLE_VENC_GetRcMode();

    s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode,&stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr for %#x!\n", s32Ret);
        goto EXIT_VI_VPSS_UNBIND;
    }


   /***encode Mjpege **/
    s32Ret = SAMPLE_COMM_VENC_Start(VencChn[0], enPayLoad[0],enSize[0], enRcMode,u32Profile[0],HI_FALSE,&stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT_VI_VPSS_UNBIND;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[1],VencChn[0]);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Get GopAttr failed for %#x!\n", s32Ret);
        goto EXIT_VENC_MJPEGE_STOP;
    }

    /***encode Jpege **/
    s32Ret = SAMPLE_COMM_VENC_SnapStart(VencChn[1], &stSize[1], bSupportDcf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Venc Start failed for %#x!\n", s32Ret);
        goto EXIT_VENC_MJPEGE_UnBind;
    }

    //s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(VpssGrp, VpssChn[0],VencChn[1]);
    //if (HI_SUCCESS != s32Ret)
    //{
    //    SAMPLE_PRT("Venc bind Vpss failed for %#x!\n", s32Ret);
    //    goto EXIT_VENC_JPEGE_STOP;
    //}
    
    pthread_t rtspThreadId;
    ringmalloc(400*1024);
    pthread_create(&rtspThreadId, NULL, rtsp_main, NULL);

    pthread_t postThreadId;
    pthread_create(&postThreadId, NULL, PostInfoToServerThread, NULL);
    pthread_detach(postThreadId);

    StartJpegTcpServer();

    /******************************************
     stream save process
    ******************************************/
    s32Ret = SAMPLE_COMM_VENC_StartGetStream(VencChn,1);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("Start Venc failed!\n");
        goto EXIT_VENC_JPEGE_UnBind;
    }

    /******************************************
     stream venc process -- get jpeg stream, then save it to file.
    ******************************************/
    printf("press 'q' to exit snap!\nperess any other key to capture one picture to file\n");
    i = 0;

    ParkingGuidanceIOInit();
    ParkingGuidanceIOConfig(&(g_Allconfig.algParam));
    
    VPSS_CHN_ATTR_S attr;
    s32Ret = HI_MPI_VPSS_GetChnAttr(VpssGrp, VpssChn[0], &attr);
    SAMPLE_PRT("HI_MPI_VPSS_GetChnAttr, ret: 0x%08x\n", s32Ret);
    attr.u32Depth = 3;
    s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn[0], &attr);
    SAMPLE_PRT("HI_MPI_VPSS_SetChnAttr, ret: 0x%08x\n", s32Ret);

    //配置默认颜色
    SetLightSateByGpio(g_colorState.currentColor);

    FILE *fp = NULL;
    if (g_ReplaceYuvData) {
        fp = fopen(g_ReplaceYuvFile, "r");
        if (fp == NULL) {
            SAMPLE_PRT("Open replace.yuv failed - %s\n", g_ReplaceYuvFile);
            return -1;
        }
    }

    while (!g_exitMainThread)
    {
        if (g_rebootSystem) {
            SAMPLE_PRT("Will reboot the system after 2s!\n");
            sleep(2);
            execl("/bin/sh", "sh", "-c", "/sbin/reboot", (char *) 0);
        }
        feed_wdt();

        if (GetCurrentUpgState() == UPG_SUCCESS) {
            exit(123);
        }
        
        VIDEO_FRAME_INFO_S stFrame;
        PARKING_GUIDANCE_OUT_INFO outInfo;
        s32Ret = HI_MPI_VPSS_GetChnFrame(VpssGrp,VpssChn[0],&stFrame,-1);
        if (s32Ret != HI_SUCCESS) {
            SAMPLE_PRT("HI_MPI_VPSS_GetChnFrame failed: 0x%08x\n", s32Ret);
            continue;
        }

        if (g_ReplaceYuvData && (GetCurrentUpgState() != UPG_PROCESS)) {
            s32Ret = PlateReplaceFrameFromFile(fp, &stFrame);
            if (s32Ret != 0) {
                HI_MPI_VPSS_ReleaseChnFrame(VpssGrp,VpssChn[0],&stFrame);
                continue;
            }
        }
        
        HI_U8 *jpgData = NULL;
        HI_U32 jpgLen = 0;
        EncodeYuvDataToJpegData(VencChn[1], &stFrame, &jpgData, &jpgLen);

        if (GetCurrentUpgState() != UPG_PROCESS) {
            memset(&outInfo, 0, sizeof(outInfo));
            HI_S32 callAlgRet = 0;
            if ((callAlgRet = CallPlateCarDetectAlg(&stFrame, &outInfo)) == -1) {
                SAMPLE_PRT("CallPlateCarDetectAlg failed!\n");
            }
            
            if ((jpgData != NULL) && (jpgLen != 0) && (callAlgRet == 0)) {            
                ProcessAlgResult(&outInfo, &stFrame, u32PoolId, phyPlateAddr, 
                    VencChn[1], g_Allconfig.postJpegUrl, jpgData, jpgLen);
            }
        }

        ProcessHttpProcReq(jpgData, jpgLen);
        ProcessHeartBeat();
        ProcessLightFlash();
        
        if (jpgData != NULL) {
            free(jpgData);
            jpgData = NULL;
        }
        
        printf("snap %d success!\n", i);
        i++;

        HI_MPI_VPSS_ReleaseChnFrame(VpssGrp,VpssChn[0],&stFrame);
        //sleep(1);

        printf("\npress 'q' to exit snap!\nperess any other key to capture one picture to file\n");
    }
    printf("exit this sample!\n");

    /******************************************
     exit process
    ******************************************/
    
    SAMPLE_COMM_VENC_StopGetStream();

    HI_MPI_VB_ReleaseBlock(plateBlk);
    HI_MPI_VB_DestroyPool(u32PoolId);

EXIT_VENC_JPEGE_UnBind:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp,VpssChn[0],VencChn[1]);
//EXIT_VENC_JPEGE_STOP:
    SAMPLE_COMM_VENC_Stop(VencChn[1]);
EXIT_VENC_MJPEGE_UnBind:
    SAMPLE_COMM_VPSS_UnBind_VENC(VpssGrp,VpssChn[1],VencChn[0]);
EXIT_VENC_MJPEGE_STOP:
    SAMPLE_COMM_VENC_Stop(VencChn[0]);
EXIT_VI_VPSS_UNBIND:
    SAMPLE_COMM_VI_UnBind_VPSS(ViPipe,ViChn,VpssGrp);
EXIT_VPSS_STOP:
    SAMPLE_COMM_VPSS_Stop(VpssGrp,abChnEnable);
EXIT_VI_STOP:
    SAMPLE_COMM_VI_StopVi(&stViConfig);
    SAMPLE_COMM_SYS_Exit();

    StopJpegTcpServer();

    exit_rtsp();
    pthread_join(rtspThreadId, NULL);
    
    return s32Ret;
}

/******************************************************************************
* function    : main()
* Description : video venc sample
******************************************************************************/
#ifdef __HuaweiLite__
    int app_main(int argc, char *argv[])
#else
    int main(int argc, char *argv[])
#endif
{
    HI_S32 s32Ret;

    if (argc >= 2) {
        g_ReplaceYuvData = HI_TRUE;
        g_ReplaceYuvFile = argv[1];
    }
    if (argc >= 3) {
        if (strstr(argv[2], "algruntest") != NULL) {
            g_TestAlg = HI_TRUE;
        }
    }
#ifndef __HuaweiLite__
    signal(SIGINT, SAMPLE_VENC_HandleSig);
    signal(SIGTERM, SAMPLE_VENC_HandleSig);
    signal(SIGPIPE, SIG_IGN);
#endif

    s32Ret = SAMPLE_VENC_MJPEG_JPEG();
    if (HI_SUCCESS == s32Ret)
    { printf("program exit normally!\n"); }
    else
    { printf("program exit abnormally!\n"); }

#ifdef __HuaweiLite__
    return s32Ret;
#else
    exit(s32Ret);
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
