#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "cJSON.h"
#include "tcpServer.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "shareHeader.h"

#define CONFIG_FILE     "/alg/config.json"
#define CONFIG_485      "/alg/485.json"

const char *g_serial485DevName = "/dev/ttyAMA1";

typedef enum {
	cgiParseSuccess,
	cgiParseMemory,
	cgiParseIO,
	cgiParseEnv,
	cgiParseBody
} cgiParseResultType;

typedef int (*CommProcessFunc)(cJSON *rootObj, const char *rawData, size_t dataLen);

static void cgiGetenv(char **s, char *var){
	*s = getenv(var);
	if (!(*s)) {
		*s = "";
	}
}

static int cgiStrEqNc(char *s1, char *s2) {
	while(1) {
		if (!(*s1)) {
			if (!(*s2)) {
				return 1;
			} else {
				return 0;
			}
		} else if (!(*s2)) {
			return 0;
		}
		if (isalpha(*s1)) {
			if (tolower(*s1) != tolower(*s2)) {
				return 0;
			}
		} else if ((*s1) != (*s2)) {
			return 0;
		}
		s1++;
		s2++;
	}
}

static int cgiStrBeginsNc(char *s1, char *s2) {
	while(1) {
		if (!(*s2)) {
			return 1;
		} else if (!(*s1)) {
			return 0;
		}
		if (isalpha(*s1)) {
			if (tolower(*s1) != tolower(*s2)) {
				return 0;
			}
		} else if ((*s1) != (*s2)) {
			return 0;
		}
		s1++;
		s2++;
	}
}

static int cgiGetHttpBody(char **pBody, size_t *bodyLen)
{
    char *cgiContentLengthString = NULL;
    cgiGetenv(&cgiContentLengthString, "CONTENT_LENGTH");
    if (strlen(cgiContentLengthString) == 0) {
        return cgiParseEnv;
    }

    int cgiContentLength = atoi(cgiContentLengthString);
    if (cgiContentLength == 0) {
        return cgiParseIO;
    }
    
    char *input = (char *) malloc(cgiContentLength + 1);
    if (input == NULL) {
        return cgiParseMemory;
    }
    memset(input, 0, cgiContentLength + 1);
	if (((int) fread(input, 1, cgiContentLength, stdin))  != cgiContentLength) 
	{
        return cgiParseIO;
	}	

    *pBody = input;
    *bodyLen = cgiContentLength;
    
    return cgiParseSuccess;
}

void GenerateResponse(char *result)
{
    printf("{\n");
    printf("\tresult: \"%s\"\n", result);
    printf("}");
}

int LoadSerialParamFromConfig(int *braud, int *databit, int *check)
{
    FILE *fp = NULL;
    cJSON *paramRootObj = NULL;
    cJSON *itmObj = NULL;
    int ret = 0;
    char buffer[256] = {0};
    
    fp = fopen(CONFIG_485, "r");
    JUGE(fp != NULL, ERROR);
    ret = fread(buffer, 1, sizeof(buffer) - 1, fp);
    JUGE(ret > 0, ERROR);

    paramRootObj = cJSON_Parse(buffer);
    JUGE(paramRootObj != NULL, ERROR);


    itmObj = cJSON_GetObjectItem(paramRootObj, "braud");
    JUGE(paramRootObj != NULL, ERROR);
    *braud = atoi(itmObj->valuestring) - 1;
    
    itmObj = cJSON_GetObjectItem(paramRootObj, "databit");
    JUGE(paramRootObj != NULL, ERROR);
    *databit = atoi(itmObj->valuestring) - 1;
    
    itmObj = cJSON_GetObjectItem(paramRootObj, "check");
    JUGE(paramRootObj != NULL, ERROR);
    *check = atoi(itmObj->valuestring) - 1;

    ret = OK;
RET:
    if (fp != NULL) { fclose(fp); }
    if (paramRootObj != NULL) { cJSON_Delete(paramRootObj); }
    
    return ret;
}

int ConfigSerialParam()
{
    char *dataBitOpt[] = {"cs8", "cs7", "cs6"};
    char *braudOpt[] = {"speed 9600", "speed 19200", "speed 115200"};
    // 无校验 偶校验 奇校验
    char *checkOpt[] = {"-parenb -parodd", "parenb -parodd", "parenb parodd"};
    int ret = 0;
    char cmd[256] = {0};

    int braud = 0;
    int dataBit = 0;
    int check = 0;
    ret = LoadSerialParamFromConfig(&braud, &dataBit, &check);
    JUGE(ret == OK, ret);

    //common config
    snprintf(cmd, sizeof(cmd), "stty -F %s -opost -echo -icanon -iexten -isig -icrnl -inlcr -ixon -ixoff -ixany -cstopb > /dev/null 2>&1",
        g_serial485DevName);
    system(cmd);
    
    snprintf(cmd, sizeof(cmd), "stty -F %s %s %s %s > /dev/null 2>&1", g_serial485DevName, 
        braudOpt[braud], dataBitOpt[dataBit], checkOpt[check]);
    system(cmd);  
    
    ret = OK;
    
RET:
    return ret;
}

int PenetrateParam(enum ProtoCmd cmd, const char *rawData, size_t dataLen)
{
    int ret = 0;
    int fd = ConnectToServer(SERVER_ADDR, JPEG_SERVER_PORT);
    JUGE(fd >= 0, -1);
    
    size_t sendLen = dataLen + 1;
    struct ServerProtoHeader hdr;
    hdr.cmd = htonl(cmd);
    hdr.len = htonl(sendLen);
    
    ret = WriteDataFromSocket(fd, &hdr, sizeof(hdr), NULL);
    JUGE(ret == sizeof(hdr), -1);

    ret = WriteDataFromSocket(fd, rawData, sendLen, NULL);
    JUGE(ret == sendLen, -1);

    struct ServerResponseHdr rsp = {0};
    ret = ReadDataFromSocket(fd, &rsp, sizeof(rsp), NULL);
    JUGE(ret == sizeof(rsp), -1);
    JUGE(ntohl(rsp.result) == OK, ntohl(rsp.result));

    ret = 0;
RET:
    if (fd >= 0) {
        close(fd);
    }
    return ret;
}

int PenetrateParamAndGetRsp(enum ProtoCmd cmd, const char *rawData, size_t dataLen,
    char *rspBuf, size_t buffLen)
{
    int ret = 0;
    int fd = ConnectToServer(SERVER_ADDR, JPEG_SERVER_PORT);
    JUGE(fd >= 0, -1);
    
    size_t sendLen = dataLen + 1;
    struct ServerProtoHeader hdr;
    hdr.cmd = htonl(cmd);
    hdr.len = htonl(sendLen);
    
    ret = WriteDataFromSocket(fd, &hdr, sizeof(hdr), NULL);
    JUGE(ret == sizeof(hdr), -1);

    ret = WriteDataFromSocket(fd, rawData, sendLen, NULL);
    JUGE(ret == sendLen, -1);

    struct ServerResponseHdr rsp = {0};
    ret = ReadDataFromSocket(fd, &rsp, sizeof(rsp), NULL);
    JUGE(ret == sizeof(rsp), -1);
    JUGE(ntohl(rsp.result) == OK, -1);

    size_t rspLen = ntohl(rsp.len);
    JUGE(rsp.len > 0, -1);
    JUGE(buffLen >= rspLen, -1);
    ret = ReadDataFromSocket(fd, rspBuf, rspLen, NULL);
    JUGE(ret == rspLen, -1);
    ret = 0;
RET:
    if (fd >= 0) {
        close(fd);
    }
    return ret;
}

int ProcessRebootCmd(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    
    int ret = PenetrateParam(REBOOT, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }
}

int ProcessTriggerPushData(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = PenetrateParam(TRIGGER_PUSH_DATA, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return 0;
}

int ProcessSet485Param(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    int ret = 0;
    cJSON *itmObj = NULL;
    FILE *fp = NULL;

    itmObj = cJSON_GetObjectItem(rootObj, "braud");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);

    itmObj = cJSON_GetObjectItem(rootObj, "databit");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);

    itmObj = cJSON_GetObjectItem(rootObj, "check");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);

    fp = fopen(CONFIG_485, "w");
    JUGE(fp != NULL, -1);
    ret = fwrite(rawData, 1, dataLen, fp);
    JUGE(ret == dataLen, -1);
    
    ret = 0;
RET:
    if (fp != NULL) {
        fclose(fp);
    }

    ConfigSerialParam();

    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }
    
    return ret;
}

int ProcessGet485Param(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    int ret = 0;
    char buffer[256];
    cJSON *paramRootObj = NULL;
    cJSON *itmObj = NULL;
    char *rsp = NULL;
    int err = -1;
    
    FILE *fp = fopen(CONFIG_485, "r");
    JUGE(fp != NULL, err--);
    ret = fread(buffer, 1, sizeof(buffer) - 1, fp);
    JUGE(ret > 0, err--);

    paramRootObj = cJSON_Parse(buffer);
    JUGE(paramRootObj != NULL, err--);

    itmObj = cJSON_GetObjectItem(paramRootObj, "braud");
    JUGE(itmObj != NULL, err--);
    JUGE(itmObj->type == cJSON_String, err--);

    itmObj = cJSON_GetObjectItem(paramRootObj, "databit");
    JUGE(itmObj != NULL, err--);
    JUGE(itmObj->type == cJSON_String, err--);

    itmObj = cJSON_GetObjectItem(paramRootObj, "check");
    JUGE(itmObj != NULL, err--);
    JUGE(itmObj->type == cJSON_String, err--);

    itmObj = cJSON_GetObjectItem(paramRootObj, "com");
    JUGE(itmObj != NULL, err--);
    JUGE(itmObj->type == cJSON_Number, err--);
    JUGE(itmObj->valueint == COMM_SET_485, err--);

    itmObj->valueint = COMM_GET_485;
    itmObj->valuedouble = (double)COMM_GET_485;
    rsp = cJSON_Print(paramRootObj);
    JUGE(rsp != NULL, err--);
    
    ret = 0;
RET:
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    if (ret == 0) {
        printf("%s", rsp);
    } else {
        GenerateResponse("false");
    }

    if (fp != NULL) {
        fclose(fp);
    }
    if (paramRootObj != NULL) {
        cJSON_Delete(paramRootObj);
    }
    if (rsp != NULL) {
        free(rsp);
    }
    
    return ret;
}


int ProcessSetTime(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    int ret = 0;
    
    cJSON *itmObj = cJSON_GetObjectItem(rootObj, "datetime");
    if ((itmObj == NULL) || (itmObj->type != cJSON_String)) {
        ret = -1;
        goto RET;
    }
    if (strlen(itmObj->valuestring) != 14) {
        ret = -1;
        goto RET;
    }

    char cmd[64] = "date -s \"";

    uint32_t pos = 0;
    strncat(cmd, itmObj->valuestring + pos, 4);
    pos += 4;
    strncat(cmd, "-", 1);

    strncat(cmd, itmObj->valuestring + pos, 2);
    pos += 2;
    strncat(cmd, "-", 1);

    strncat(cmd, itmObj->valuestring + pos, 2);
    pos += 2;
    strncat(cmd, " ", 1);

    strncat(cmd, itmObj->valuestring + pos, 2);
    pos += 2;
    strncat(cmd, ":", 1);

    strncat(cmd, itmObj->valuestring + pos, 2);
    pos += 2;
    strncat(cmd, ":", 1);

    strncat(cmd, itmObj->valuestring + pos, 2);
    strncat(cmd, "\"", 1);

    ret = system(cmd);
RET:
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }
    return ret;
}

int ProcessSetLightColor(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    int ret = 0;
    char buf[64];
    
    cJSON *itmObj = cJSON_GetObjectItem(rootObj, "color");
    JUGE(itmObj != NULL, AUTO_ERR_CODE);
    JUGE(itmObj->type == cJSON_Number, AUTO_ERR_CODE);

    int color = itmObj->valueint;

    if (color != LIGHT_SET_SLAVE_INFO && color != LIGHT_MANDATORY) {
        ret = -7;
        goto RET;
    }

    if (color == LIGHT_MANDATORY) {
        ret = PenetrateParam(SET_LIGHT_STATE, rawData, dataLen);
        JUGE(ret == 0, ret);

    } else {
        ret = PenetrateParam(SET_LIGHT_SLAVE_INFO, rawData, dataLen);
        JUGE(ret == 0, AUTO_ERR_CODE);
    }
RET:
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }
    
    return ret;
}

int ProcessGetPicture(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    
    int ret = 0;
    int fd = ConnectToServer(SERVER_ADDR, JPEG_SERVER_PORT);
    if (fd < 0) {
        ret = -1;
        goto RET;
    }

    struct ServerProtoHeader hdr;
    hdr.cmd = htonl(GET_JPEG);
    hdr.len = 0;

    int err = 0;
    ret = WriteDataFromSocket(fd, &hdr, sizeof(hdr), &err);
    if (ret != sizeof(hdr)) {
        ret = -1;
        goto RET;
    }

    struct ServerResponseHdr response = {0};
    ret = ReadDataFromSocket(fd, &response, sizeof(response), &err);
    if (ret != sizeof(response)) {
        ret = -1;
        goto RET;
    }
    if (ntohl(response.len) == 0 || ntohl(response.result) != OK) {
        ret = -1;
        goto RET;
    }

    //printf("XXX: %d %d\n", ntohl(response.len), ntohl(response.result));
    char buf[512] = {0};
    while (1) {
        err = 0;
        ret = ReadDataFromSocket(fd, buf, sizeof(buf) - 1, &err);
        if (ret > 0) {
            fwrite(buf, 1, ret, stdout);
        } else {
            break;
        }
    }
    
    ret = 0;

RET:
    if (ret != 0) {
        GenerateResponse("false");
    }
    if (fd >= 0) {
        close(fd);
    }
    return ret;
}

int ProcessSetParkParam(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    
    int ret = PenetrateParam(SET_ALG_PARAM, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return 0;
}

int ProcessSetPlaceStatus(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    
    int ret = PenetrateParam(SET_PLACE_STATUS, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return 0;
}

int ProcessGetPlaceStatus(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    char rspBuff[1024] = {0};
    int ret = PenetrateParamAndGetRsp(GET_PLACE_STATUS, rawData, dataLen, rspBuff, sizeof(rspBuff));
    if (ret == 0) {
        printf("%s", rspBuff);
    } else {
        GenerateResponse("false");
    }

    return ret;
}

int ProcessSetPushRegularlyParam(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = PenetrateParam(SET_REGULARLY_PARAM, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return ret;
}

int ProcessGetPushRegularlyParam(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    char rspBuff[1024] = {0};
    int ret = PenetrateParamAndGetRsp(GET_REGULARLY_PARAM, rawData, dataLen, rspBuff, sizeof(rspBuff));
    if (ret == 0) {
        printf("%s", rspBuff);
    } else {
        GenerateResponse("false");
    }

    return ret;
}


int ProcessSetHeartBeatParam(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = PenetrateParam(SET_HEART_BEAT_PARAM, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return ret;
}

int WriteDataToSerial(const char *data, uint32_t dataLen)
{
    int ret = 0;

    int fd = open(g_serial485DevName, O_RDWR | O_NOCTTY | O_NDELAY);
    JUGE(fd >= 0, ERROR);

    uint32_t offset = 0;
    while (offset < dataLen) {
        int writeRet = write(fd, data + offset,  dataLen - offset);
        if (writeRet <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            } else {
                ret = ERROR;
                goto RET;
            }
        }
        
        offset += writeRet;
    }

    ret = OK;
RET:
    if (fd >= 0) { close(fd); }
    return ret;
}


int GetRspFromSerial(uint8_t *buf, uint32_t bufLen)
{
    return 1;
}

int ProcessSend485Data(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    int ret = ERROR;
    cJSON *itmObj = NULL;
    uint8_t *sendData = NULL;
    uint32_t sendDataLen = 0;
    
    itmObj = cJSON_GetObjectItem(rootObj, "data");
    JUGE(itmObj != NULL, ERROR);
    JUGE(itmObj->type == cJSON_String, ERROR);

    ret = ConfigSerialParam();
    JUGE(ret == OK, ret);

    size_t rawStrLen = strlen(itmObj->valuestring);
    JUGE(rawStrLen % 2 == 0, ERROR);

    sendData = (uint8_t *)malloc(rawStrLen / 2);
    JUGE(sendData != NULL, ERROR);
    memset(sendData, 0, rawStrLen / 2);
    sendDataLen = 0;
    
    for (int i = 0; i < rawStrLen; i += 2) {
        char rawStrBuf[3] = {0};
        strncpy(rawStrBuf, itmObj->valuestring + i, 2);
        rawStrBuf[2] = '\0';

        char *end = NULL;
        uint8_t data = strtol(rawStrBuf, &end, 16);
        JUGE(*end == '\0', ERROR);

        sendData[sendDataLen] = data;
        sendDataLen++;
        //ret = WriteDataToSerial(&data, 1);
        //JUGE(ret == OK, ERROR);
    }
    ret = WriteDataToSerial(sendData, sendDataLen);
    JUGE(ret == OK, ERROR);
    
    ret = OK;
RET:
    if (sendData != NULL) {
        free(sendData);
    }
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }
    return ret;
}

int ProcessSend485DataGetRsp(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    int ret = 0;
    cJSON *itmObj = NULL;
    cJSON *rspObj = NULL;
    char buf[1024] = "00112233445566778899";
    
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
    
    itmObj = cJSON_GetObjectItem(rootObj, "data");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_String, -1);
    //SendDataToSerial(itmObj->valuestring);
    
    itmObj = cJSON_GetObjectItem(rootObj, "Bit");
    JUGE(itmObj != NULL, -1);
    JUGE(itmObj->type == cJSON_Number, -1);
    JUGE(itmObj->valueint == 2, -1);

    int rspRet = GetRspFromSerial(buf, sizeof(buf));
    ret = 0;
 RET:
     if (ret == 0) {
         printf("{\n");
         printf("\t\"result\":\"OK\",\n");
         printf("\t\"com\":%d,\n", COMM_SEND_485_GET_RSP);
         if (rspRet > 0) {        
             printf("\t\"Bit\":2,\n");
             printf("\t\"data\":\"%s\"", buf);
         } else {
             printf("\t\"Bit\":3\n");
         }
         printf("}");
     } else {
         GenerateResponse("false");
     }
    return ret;
}


int ProcessUpgrade(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = PenetrateParam(TRIGGER_UPG_START, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return ret;
}

int ProcessGetUpgradeState(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    char rspBuff[128] = {0};
    int ret = PenetrateParamAndGetRsp(GET_UPG_STATE, rawData, dataLen, rspBuff, sizeof(rspBuff));
    if (ret == 0) {
        printf("%s", rspBuff);
    } else {
        GenerateResponse("false");
    }

    return ret;

}

int ProcessSetCtrIpaddr(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = PenetrateParam(SET_CTR_IPADDR, rawData, dataLen);
    if (ret == 0) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return ret;
}

int ProcessGetCtrIpaddr(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    char rspBuff[256] = {0};
    int ret = PenetrateParamAndGetRsp(GET_CTR_IPADDR, rawData, dataLen, rspBuff, sizeof(rspBuff));
    if (ret == 0) {
        printf("%s", rspBuff);
    } else {
        GenerateResponse("false");
    }

    return ret;

}

int ProcessSetIpaddr(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = OK;
    char cmd[256] = {0};

    cJSON *gateObj = cJSON_GetObjectItem(rootObj, "ipgate");
    JUGE(gateObj != NULL, ERROR);

    cJSON *maskObj = cJSON_GetObjectItem(rootObj, "ipmask");
    JUGE(maskObj != NULL, ERROR);

    snprintf(cmd, sizeof(cmd), "echo -n \"%s\" > %s", gateObj->valuestring, GATEWAY_CFG_FILE);
    system(cmd);

    system("/sbin/route del default");
    snprintf(cmd, sizeof(cmd), "/sbin/route add default gw %s 2>&1 > /dev/null", gateObj->valuestring);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "echo -n \"%s\" > %s", maskObj->valuestring, NETMASK_CFG_FILE);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "/sbin/ifconfig %s netmask %s 2>&1 > /dev/null", NET_DEV, gateObj->valuestring);
    system(cmd);

    ret = OK;
    
RET:
    if (ret == OK) {
        GenerateResponse("OK");
    } else {
        GenerateResponse("false");
    }

    return 0;
}

int ProcessGetIpaddr(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    int ret = OK;
    char gateBuff[64] = {0};
    char maskBuff[64] = {0};

    FILE *gatefp = fopen(GATEWAY_CFG_FILE, "r");
    JUGE(gatefp != NULL, ERROR);

    FILE *maskfp = fopen(NETMASK_CFG_FILE, "r");
    JUGE(maskfp != NULL, ERROR);

    int readRet = fread(gateBuff, 1, sizeof(gateBuff), gatefp);
    JUGE(readRet > 0, ERROR);

    readRet = fread(maskBuff, 1, sizeof(maskBuff), maskfp);
    JUGE(readRet > 0, ERROR);

    ret = OK;
RET:
    if (gatefp != NULL) { fclose(gatefp); }
    if (maskfp != NULL) { fclose(maskfp); }
    
    if (ret == OK) {
        printf("{\"ipgate\":\"%s\", \"ipmask\":\"%s\"}", gateBuff, maskBuff);
    } else {
        GenerateResponse("false");
    }

    return 0;
}

int ProcessGetPrivateInfo(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    char rspBuff[256] = {0};
    int ret = PenetrateParamAndGetRsp(GET_PRIVATE_INFO, rawData, dataLen, rspBuff, sizeof(rspBuff));
    if (ret == 0) {
        printf("%s", rspBuff);
    } else {
        GenerateResponse("false");
    }

    return ret;

}

int ProcessGetPlateInfo(cJSON *rootObj, const char *rawData, size_t dataLen)
{
    printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    char rspBuff[256] = {0};
    int ret = PenetrateParamAndGetRsp(GET_PLATE_INFO, rawData, dataLen, rspBuff, sizeof(rspBuff));
    if (ret == 0) {
        printf("%s", rspBuff);
    } else {
        GenerateResponse("false");
    }

    return ret;

}



int ParsePostCtrlJson(const char *data, size_t dataLen)
{
    int ret =  0;
    cJSON *rootObj = NULL;
    
    rootObj = cJSON_Parse(data);
    if (rootObj == NULL) {
        printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
        GenerateResponse("false");
        ret = cgiParseBody;
        goto RET;
    }

    cJSON *comItm = cJSON_GetObjectItem(rootObj, "com");
    if (comItm == NULL) {
        printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
        GenerateResponse("false");
        ret = cgiParseBody;
        goto RET;
    }
    int funcIdx = comItm->valueint;

    CommProcessFunc processFuncTbl[] = {
        ProcessGetPrivateInfo, 
        ProcessRebootCmd, 
        ProcessTriggerPushData,
        ProcessSet485Param,
        ProcessGet485Param,
        ProcessSetTime,
        ProcessSetLightColor,
        ProcessGetPicture,
        ProcessSetParkParam,
        ProcessSetPlaceStatus,
        ProcessGetPlaceStatus,
        ProcessGetPushRegularlyParam,
        ProcessSetPushRegularlyParam,
        ProcessSend485Data,
        ProcessSend485DataGetRsp,
        ProcessUpgrade,
        ProcessGetUpgradeState,
        ProcessSetCtrIpaddr,
        ProcessGetCtrIpaddr,
        ProcessSetIpaddr,
        ProcessGetIpaddr,
        ProcessGetPlateInfo
    };

    int funcNum = sizeof(processFuncTbl) / sizeof(processFuncTbl[0]);
    if (funcIdx < 0 || funcIdx >= funcNum || processFuncTbl[funcIdx] == NULL) {
        PRINT("funcIdx: %d, funcNum: %d\n", funcIdx, funcNum);
        ret = cgiParseBody;
        goto RET;
    }
    CommProcessFunc processFunc = processFuncTbl[funcIdx];
    ret = processFunc(rootObj, data, dataLen);
RET:
    if (rootObj != NULL) {
        cJSON_Delete(rootObj);
    }
    return ret;
}

int main(int argc, char *argv[])
{
    char *body = NULL;
    size_t bodyLen = 0;
    char *contenType = NULL;
    char *cgiRequestMethod = NULL;

    cgiGetenv(&contenType, "CONTENT_TYPE");
    cgiGetenv(&cgiRequestMethod, "REQUEST_METHOD");
    //printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );

    if (contenType == NULL || !(cgiStrBeginsNc(contenType, "application/json"))) {
        printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
        GenerateResponse("false");
        return 0;
    }

    if (cgiRequestMethod == NULL || !(cgiStrBeginsNc(cgiRequestMethod, "post"))) {
        printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
        GenerateResponse("false");
        return 0;
    }

    int errCode = cgiGetHttpBody(&body, &bodyLen);
    if (errCode != cgiParseSuccess) {
        printf( "Content-Type: application/json; charset=utf-8\r\n\r\n" );
        GenerateResponse("false");
        if (body != NULL) {
            free(body);
        }
        return 0;
    }

    //body[bodyLen - 1] = '\0';
    ParsePostCtrlJson(body, bodyLen);
    
    if (body != NULL) {
        free(body);
    }
    return 0;
}

