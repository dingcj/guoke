#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "tcpServer.h"

#define FIFO_MAX_LEN (10)

FifoRing *g_clientFifo = NULL;

static volatile int g_exitServerThread = 0;
pthread_t threadId;

int ConnectToServer(const char *ipaddr, uint16_t port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        PRINT("socket: %s\n", strerror(errno));
        return -1;
    }
    
    struct sockaddr_in serverSocket;
    memset(&serverSocket, 0, sizeof(serverSocket));
    serverSocket.sin_family = AF_INET;
    serverSocket.sin_addr.s_addr = inet_addr(ipaddr);
    serverSocket.sin_port = htons(port);

    if (connect(fd, (struct sockaddr *)&serverSocket, sizeof(serverSocket)) != 0) {
        PRINT("connect: %s\n", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int ReadDataFromSocket(int sock, void *buf, size_t len, int *err)
{
    int offset = 0;
    while (offset < len) {
        int ret = recv(sock, buf + offset, len - offset, 0);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                if (err != NULL) {
                    *err = 1;
                }
                break;
            }
        }
        
        if (ret == 0) {
            break;
        }
        offset += ret;
    }

    return offset;
}

int WriteDataFromSocket(int sock, const void *buf, size_t len, int *err)
{
    int offset = 0;
    //*err = 0;
    while (offset < len) {
        int ret = send(sock, buf + offset, len - offset, 0);
        if (ret <= 0) {
            if (errno == EINTR) {
                continue;
            } else {
                if (err != NULL) {
                    *err = 1;
                }
                break;
            }
        }
        offset += ret;
    }

    return offset;
}


void *GetWebServerRequest()
{
    void *tmp = NULL;
    int ret = fifo_ring_pop(g_clientFifo, &tmp);
    if (ret != 0) {
        return NULL;
    }

    return tmp;
}

int ServerProcessGetPicReq(int fd, struct ServerProtoHeader *hdr)
{
    struct GetPictureStru *req = NULL;
    req = (struct GetPictureStru *)malloc(sizeof(struct GetPictureStru));
    if (req == NULL) {
        close(fd);
        return -1;
    }
    req->hdr.cmd = GET_JPEG;
    req->fd = fd;
    int fifoRet = fifo_ring_push(g_clientFifo, (void *)req);
    if (fifoRet != 0) {
        close(fd);
        free(req);
        return -1;
    }

    return 0;
}

int ServerPenetrateParam(int fd, struct ServerProtoHeader *hdr)
{
    int dataLen = ntohl(hdr->len);
    int ret = 0;
    struct SetAlgParamStru *req = NULL;
    
    req = (struct SetAlgParamStru *)malloc(sizeof(struct SetAlgParamStru) + dataLen);
    JUGE(req != NULL, -1);
    req->hdr.cmd = ntohl(hdr->cmd);
    req->hdr.len = dataLen;
    req->fd = fd;
    
    ret = ReadDataFromSocket(fd, req->jsonData, dataLen, NULL);
    JUGE(ret == dataLen, -1);
    
    int fifoRet = fifo_ring_push(g_clientFifo, (void *)req);
    JUGE(fifoRet == 0, -1);

    ret = 0;
RET:
    if (ret != 0) {
        if (fd >= 0) {
            close(fd);
        }
        if (req != NULL) {
            free(req);
        }
    }
    return ret;
}



void* JpegTcpServerFunc(void *param)
{
    int fd = 0;
    int ret = 0;
    int fifoRet = 0;

    g_clientFifo = fifo_ring_create(FIFO_MAX_LEN);
    if (g_clientFifo == NULL) {
        PRINT("fifo_ring_create failed!\n");
        return (void *)(-1);
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        PRINT("socket: %s\n", strerror(errno));
        return (void *)(-1);
    }
    
    int opt = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt))) {
        PRINT("setsockopt: %s\n", strerror(errno));
        ret = -1;
        goto RET;
    }

    struct sockaddr_in serverSocket;
    memset(&serverSocket, 0, sizeof(serverSocket));
    serverSocket.sin_family = AF_INET;
    serverSocket.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    serverSocket.sin_port = htons(JPEG_SERVER_PORT);
    if(bind(fd, (struct sockaddr*)&serverSocket, sizeof(serverSocket)) < 0)
    {
        PRINT("bind error: %s\n", strerror(errno));
        ret = -2;
        goto RET;
    }

    if(listen(fd, 5) < 0 ){
        PRINT("listen error: %s\n", strerror(errno));
        ret = -3;
        goto RET;
    }
    
    while(!g_exitServerThread) {
        struct sockaddr_in clientSocket;
        socklen_t addrlen = sizeof(clientSocket);
        int clientSockFd = accept(fd,(struct sockaddr*)&clientSocket, &addrlen);
        if(clientSockFd < 0)
        {
            PRINT("accept error: %s\n", strerror(errno));
            if (errno == EINTR) {
                continue;
            } else {
                ret = -4;
                goto RET;
            }
        }

        struct ServerProtoHeader hdr;
        int err = 0;
        ret = ReadDataFromSocket(clientSockFd, &hdr, sizeof(hdr), &err);
        if (ret != sizeof(hdr)) {
            close(clientSockFd);
            continue;
        }

        switch (ntohl(hdr.cmd)) {
        case GET_JPEG:
            ServerProcessGetPicReq(clientSockFd, &hdr);
            break;
        case SET_ALG_PARAM:
        case SET_REGULARLY_PARAM:
        case SET_HEART_BEAT_PARAM:
        case REBOOT:
        case TRIGGER_UPG_START:
        case GET_UPG_STATE:
        case TRIGGER_PUSH_DATA:
        case SET_PLACE_STATUS:
        case GET_PLACE_STATUS:
        case GET_REGULARLY_PARAM:
        case SET_CTR_IPADDR:
        case GET_CTR_IPADDR:
        case SET_LIGHT_SLAVE_INFO:
        case GET_PRIVATE_INFO:
        case SET_LIGHT_STATE:
        case GET_PLATE_INFO:
            ServerPenetrateParam(clientSockFd, &hdr);
            break;
        case EXIT_THREAD:
            ret = 0;
            close(clientSockFd);
            PRINT("Server thread exit normal!\n");
            goto RET;
            break;
        default:
            close(clientSockFd);
            break;
        }
    }
    
RET:
    close(fd);
    fifo_ring_destroy(g_clientFifo);
    return (void *)(ret);
}

int StartJpegTcpServer()
{
    int ret = pthread_create(&threadId, NULL, JpegTcpServerFunc, NULL);
    if (ret != 0) {
        PRINT("pthread_create error: %s\n", strerror(errno));
        return ret;
    }

    return 0;
}

int StopJpegTcpServer()
{
    int fd = ConnectToServer(SERVER_ADDR, JPEG_SERVER_PORT);
    if (fd < 0) {
        return -1;
    }

    struct ServerProtoHeader hdr;
    hdr.cmd = htonl(EXIT_THREAD);

    send(fd, &hdr, sizeof(hdr), 0);
    close(fd);

    g_exitServerThread = 1;

    pthread_join(threadId, NULL);

    return 0;
}


FifoRing* fifo_ring_create(size_t length)
{
    FifoRing* thiz = NULL;
    thiz = (FifoRing*)malloc(sizeof(FifoRing) + length * sizeof(void*));
    if(thiz != NULL)
    {
        thiz->r_cursor = 0;
        thiz->w_cursor = 0;
        thiz->length = length;
    }
    return thiz;
}

void fifo_ring_destroy(FifoRing* thiz)
{
    if(thiz != NULL)
    {
        free(thiz);
    }
    
    return;
}

int fifo_ring_push(FifoRing* thiz, void* data)
{
    int w_cursor = 0;
    int ret = -1;
    
    if (thiz == NULL) {
        return -1;
    }
    
    w_cursor = (thiz->w_cursor + 1) % thiz->length;
    if(w_cursor != thiz->r_cursor)
    {
        thiz->data[thiz->w_cursor] = data;
        thiz->w_cursor = w_cursor;
        ret = 0;
    }
    return ret;
}

int fifo_ring_pop(FifoRing* thiz, void** data)
{
    int ret = -1;
    
    if (thiz == NULL) {
        return -1;
    }
    if(thiz->r_cursor != thiz->w_cursor)
    {
        *data = thiz->data[thiz->r_cursor];
        thiz->r_cursor = (thiz->r_cursor + 1) % thiz->length;
        ret = 0;
    }
    
    return ret;
}

int getMacAddr(const char *ifname, uint8_t *mac)
{
    int sock = 0;
    int ret = 0;
    struct ifreq ifr = {0};
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        return -1;
    }

    strcpy(ifr.ifr_name, ifname);
    ret = ioctl(sock, SIOCGIFHWADDR, &ifr);
    if (ret < 0) {
        goto RET;
    }
    memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
RET:
    close(sock);
    return ret;
}

int getIPAddrByName(const char *ifname, 
    char *netaddr, size_t netBufLen, char *netmask, size_t maskBufLen)
{
    int rc = 0;
    struct sockaddr_in *addr = NULL;

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(struct ifreq));

    /* 0. create a socket */
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd == -1) {
        return -1;
    }

    /* 1. set type of address to retrieve : IPv4 */
    ifr.ifr_addr.sa_family = AF_INET;
    /* 2. copy interface name to ifreq structure */
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    /* 3. get the IP address */
    if ((rc = ioctl(fd, SIOCGIFADDR, &ifr)) != 0) {
        goto done;
    }

    //char ipv4[16] = { 0 };
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    strncpy(netaddr, inet_ntoa(addr->sin_addr), netBufLen);

    /* 4. get the mask */
    if ((rc = ioctl(fd, SIOCGIFNETMASK, &ifr)) != 0) {
        goto done;
    }

    //char mask[16] = { 0 };
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    strncpy(netmask, inet_ntoa(addr->sin_addr), maskBufLen);

    /* 6. close the socket */
done:
    close(fd);
    return rc;
}


void GetTimeNowString(char *buf, size_t bufLen)
{
    time_t now = 0;
    struct tm localTime = {0};
    time(&now);
    localtime_r(&now, &localTime);

    snprintf(buf, bufLen, "%04d-%02d-%02d %02d:%02d:%02d", 
        localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
        localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

    return;
}

