#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
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

#include "cJSON.h"
#include "tcpServer.h"
#include "shareHeader.h"

#define SEARCH_REQ (0xfff0)
#define SEARCH_RESPONSE (0xfff1)

#define MAX_BUFF_LEN 2048

#define MAX_PACKET_LEN 1400

#define OPERATE_CMD "Operate"
#define SEARCH_CMD "Search"
#define SET_CMD "Set"

#define RESULT "Result"
#define RESPONSE "Response"

#define CHECK_CREATE_JSON_OBJ(callfunc, itm) \
    do { \
        (itm) = (callfunc); \
        if ((itm) == NULL) { \
            goto RET; \
        } \
    } while(0)

int isValidIpv4(const char *ip_str) {
    struct sockaddr_in sa; 
    int result = inet_pton(AF_INET, ip_str, &(sa.sin_addr));
    if (result == 0) {
        return result;
    }   
    return 1;
}

cJSON * LoadJsonConfigFile(const char *cfgfile)
{
    FILE *fp = NULL;
    long len = 0;
    char *data = NULL;
    cJSON *rootObj = NULL;
    
    fp = fopen(cfgfile,"rb");
    if (fp == NULL) {
        printf("Open config file failed!\n");
        goto RET;
    }
    
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    data = (char*)malloc(len + 1);
    if (data == NULL) {
        printf("Malloc memory error!\n");
        goto RET;
    }
    
    fread(data, 1, len, fp);
    rootObj = cJSON_Parse(data);
    if (rootObj == NULL) {
        printf("cJSON_Parse error!\n");
        goto RET;
    }
    
RET:
    if (fp != NULL) { fclose(fp); }
    if (data != NULL) { free(data); }
    
    return rootObj;
}

int GetParkingPlaceNumFromCfg()
{
    int ret = MAX_PARKING_PLACE_NUM;
    
    cJSON *cfgJson = LoadJsonConfigFile(g_ConfigFile);
    JUGE(cfgJson != NULL, MAX_PARKING_PLACE_NUM);

    cJSON *parkObj = cJSON_GetObjectItem(cfgJson, "PARKING_GUIDANCE_CONFIG");
    JUGE(parkObj != NULL, MAX_PARKING_PLACE_NUM);

    cJSON *placeNumObj = cJSON_GetObjectItem(parkObj, "nPlaceNum");
    JUGE(placeNumObj != NULL, MAX_PARKING_PLACE_NUM);

    printf("Get park num: %d\n", placeNumObj->valueint);
    
    ret = placeNumObj->valueint;
RET:
    if (cfgJson != NULL) { cJSON_Delete(cfgJson); }
    
    return ret;
}

int ProcessSearch(char *responseBuf, size_t bufLen)
{
    char netaddr[32] = {0};
    char netmask[32] = {0};
    char *out = NULL;
    int ret = getIPAddrByName(NET_DEV, netaddr, sizeof(netaddr), netmask, sizeof(netmask));
    printf("netaddr: %s\n", netaddr);
    printf("netmask: %s\n", netmask);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return -1;
    }

    cJSON *itm = NULL;
    CHECK_CREATE_JSON_OBJ(cJSON_CreateString(netaddr), itm);
    cJSON_AddItemToObject(root, "ipadr", itm);

    //CHECK_CREATE_JSON_OBJ(cJSON_CreateString(netmask), itm);
    //cJSON_AddItemToObject(root, "Netmask", itm);
    
    //////////////////////////////////////////////////////////
    CHECK_CREATE_JSON_OBJ(cJSON_CreateString(g_SoftWareVersion), itm);
    cJSON_AddItemToObject(root, "sVer", itm);

    CHECK_CREATE_JSON_OBJ(cJSON_CreateString(g_HardWareVersion), itm);
    cJSON_AddItemToObject(root, "hVer", itm);

    CHECK_CREATE_JSON_OBJ(cJSON_CreateNumber(GetParkingPlaceNumFromCfg()), itm);
    cJSON_AddItemToObject(root, "parknum", itm);

    cJSON *cfgJson = LoadJsonConfigFile(g_ConfigFile);
    if (cfgJson != NULL) {
        cJSON *positionObj = cJSON_GetObjectItem(cfgJson, "postion");
        if (positionObj != NULL) {
            CHECK_CREATE_JSON_OBJ(cJSON_CreateString(positionObj->valuestring), itm);
            cJSON_AddItemToObject(root, "spostion", itm);
        }
        cJSON_Delete(cfgJson);
    }


    out = cJSON_Print(root);
    snprintf(responseBuf, bufLen, "%s", out);
RET:
    cJSON_Delete(root);
    if (out != NULL) {
        free(out);
    }
}

int ProcessSet(char *responseBuf, size_t bufLen, cJSON *rootObj)
{
    cJSON *ipItm = cJSON_GetObjectItem(rootObj, "ip");
    if (ipItm != NULL) {
        if (!isValidIpv4(ipItm->valuestring)) {
            printf("IP check: %s is invalid IP\n", ipItm->valuestring);
            return -1;
        }
        
        printf("Set Ip addr: %s\n", ipItm->valuestring);

        char cmd[256] = {0};
        snprintf(cmd, sizeof(cmd), "ifconfig %s %s", NET_DEV, ipItm->valuestring);
        printf("%s\n", cmd);
        system(cmd);

        snprintf(cmd, sizeof(cmd), "echo -n \"%s\" > %s", ipItm->valuestring, IP_CFG_FILE);
        printf("%s\n", cmd);
        system(cmd);
    }

    return 0;
}


int ProcessRequest(cJSON *rootObj, char *responseBuf, size_t buflen)
{
    int ret = -1;
    cJSON *opItm = cJSON_GetObjectItem(rootObj, OPERATE_CMD);
    
    if (opItm == NULL) {
        printf("Can't find operate itm\n");
        return -1;
    }
    printf("Operate: %s\n", opItm->valuestring);
    
    if (strcmp(SEARCH_CMD, opItm->valuestring) == 0) {
        ret = ProcessSearch(responseBuf, buflen);
    }
    
    if (strcmp(SET_CMD, opItm->valuestring) == 0) {
        ret = ProcessSet(responseBuf, buflen, rootObj);
    }

    return ret;
}

int main(int argc, char *argv[])
{
	int sock = 0, n = 0;
	char buffer[MAX_BUFF_LEN] = {0};
    char mac[ETH_ALEN] = {0};
	struct ethhdr *eth = NULL;
	uint16_t searchReqType = htons(SEARCH_REQ);
	if (0 > (sock = socket(PF_PACKET, SOCK_PACKET, searchReqType))) {
		perror("socket");
		exit(1);
	}

    memset(mac, 0xff, sizeof(mac));
    getMacAddr(NET_DEV, mac);
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	while (1) {
		n = recvfrom(sock, buffer, MAX_BUFF_LEN, 0, NULL, NULL);
		eth = (struct ethhdr*)buffer;
        printf("Recv data len: %d\n", n);
        printf("DST MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_dest[0], eth->h_dest[1], 
            eth->h_dest[2], eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        printf("SRC MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_source[0], eth->h_source[1], 
            eth->h_source[2], eth->h_source[3], eth->h_source[4], eth->h_source[5]);

        size_t dataLen = n - sizeof(struct ethhdr);
        char *data = buffer + sizeof(struct ethhdr);
        printf("Request Data Len: %u\n", dataLen);
        printf("Request Data:\n%s\n", data);

        cJSON *rootObj = NULL;
        rootObj = cJSON_Parse(data);
        if (rootObj == NULL) {
            continue;
        }

        memcpy(eth->h_dest, eth->h_source, sizeof(eth->h_dest));
        memcpy(eth->h_source, mac, sizeof(eth->h_source));
        ProcessRequest(rootObj, data, MAX_PACKET_LEN - sizeof(struct ethhdr));
        eth->h_proto = htons(SEARCH_RESPONSE);
        printf("Response Data:\n%s\n", data);

        struct sockaddr addr;
        strcpy(addr.sa_data, NET_DEV);
        int ret = sendto(sock, buffer, MAX_PACKET_LEN, 0, &addr, sizeof(addr));
        if (ret != 0) {
            perror("Sendto");
        }
        printf("Response send: ret = %d\n", ret);
        
        cJSON_Delete(rootObj);
	}

	return 0;
}

