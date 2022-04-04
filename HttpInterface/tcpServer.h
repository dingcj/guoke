#ifndef __UNIX_SOCKET_H__
#define __UNIX_SOCKET_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#define JPEG_SERVER_PORT (8087)
#define SERVER_ADDR ("127.0.0.1")

#define _DEBUG_

#ifdef _DEBUG_
#define PRINT(...) printf(__VA_ARGS__)
#else
#define PRINT(...) 
#endif

#define EXEC_EQ(func, expect, retval)    \
    do                             \
    {                              \
        if ((func) != expect) {    \
            ret = retval;             \
            goto RET;              \
        }                          \
    } while (0)

#define EXEC_NE(func, err, retval)       \
    do                             \
    {                              \
        if ((func) == err) {       \
            ret = retval;           \
            goto RET;              \
        }                          \
    } while (0)

#define JUGE(expression, retval)           \
    do                             \
    {                              \
        if (!(expression)) {       \
            ret = retval;            \
            goto RET;              \
        }                          \
    } while (0)

#define CHECK_AND_FREE(expression)           \
    do                             \
    {                              \
        if ((expression) != NULL) {       \
            free((expression));            \
        }                          \
    } while (0)


#define AUTO_ERR_CODE (__LINE__)

typedef struct _FifoRing
{
    int r_cursor;
    int w_cursor;
    size_t length;
    void* data[0];
} FifoRing;

#pragma pack(1)
struct ServerProtoHeader {
    uint32_t cmd;
    uint32_t len;
};

struct ServerResponseHdr {
    uint32_t result;
    uint32_t len;
};
#pragma pack()

struct SetAlgParamStru {
    struct ServerProtoHeader hdr;
    int fd;
    char jsonData[0];
};

struct GetPictureStru {
    struct ServerProtoHeader hdr;
    int fd;
};


enum ProtoCmd
{
    GET_JPEG = 0,
    SET_ALG_PARAM,
    SET_REGULARLY_PARAM,
    SET_HEART_BEAT_PARAM,
    REBOOT,
    TRIGGER_UPG_START,
    GET_UPG_STATE,
    TRIGGER_PUSH_DATA,
    SET_PLACE_STATUS,
    GET_PLACE_STATUS,
    GET_REGULARLY_PARAM,
    SET_CTR_IPADDR,
    GET_CTR_IPADDR,
    SET_LIGHT_SLAVE_INFO,
    GET_PRIVATE_INFO,
    SET_LIGHT_STATE,
    GET_PLATE_INFO,
    EXIT_THREAD
};

enum ProtoResult
{
    OK = 0,
    ERROR,
};

enum LightStatus
{
    LIGHT_RED = 0,
    LIGHT_GREEN,
    LIGHT_BLUE,
    LIGHT_WHITE,
    LIGHT_YELLOW,
    LIGHT_PURPLE,
    LIGHT_CYANBLUE,
    LIGHT_OFF,          //放最后一，确定静态状态的数量。
    LIGHT_FLASHING,
    LIGHT_STATUS_NUM
};

enum LightCtrl
{
    LIGHT_SET_SLAVE_INFO = 50,
    LIGHT_MANDATORY = 60,
};

enum LightBrdCmd
{
    TO_BRD_PARK_BITMAP = 0X35,
};

enum CmdDir
{
    TO_LIGHT_BRD = 0x55,
    TO_DETECTOR = 0xaa,
};

FifoRing* fifo_ring_create(size_t length);
void fifo_ring_destroy(FifoRing* thiz);
int fifo_ring_push(FifoRing* thiz, void* data);
int fifo_ring_pop(FifoRing* thiz, void** data);
void *GetWebServerRequest();

int StartJpegTcpServer();
int StopJpegTcpServer();

int ConnectToServer(const char *ipaddr, uint16_t port);
int ReadDataFromSocket(int sock, void *buf, size_t len, int *err);
int WriteDataFromSocket(int sock, const void *buf, size_t len, int *err);

int getMacAddr(const char *ifname, uint8_t *mac);
int getIPAddrByName(const char *ifname, 
    char *netaddr, size_t netBufLen, char *netmask, size_t maskBufLen);
void GetTimeNowString(char *buf, size_t bufLen);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* */

