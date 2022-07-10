#ifndef __SHARE_HEADER_H__
#define __SHARE_HEADER_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

const char *g_SoftWareVersion = "33";
const char *g_HardWareVersion = "33";
const char *g_GitCommitId = "1feee2d2e3e981dfda7cfc59898ecf909589683d";

char *g_ConfigFile = "/alg/config.json";
char *g_NetworkCfg = "/alg/network.json";

#define NET_DEV "eth0"

#define IP_CFG_FILE "/alg/ip_cfg.txt"
#define NETMASK_CFG_FILE "/alg/netmask_cfg.txt"
#define GATEWAY_CFG_FILE "/alg/gateway_cfg.txt"


#define MAX_PARKING_PLACE_NUM (3)

enum CommVal {
    COMM_REBOOT = 1,
    COMM_TRIGGER_PUSH,
    COMM_SET_485,
    COMM_GET_485,
    COMM_TIME,
    COMM_LIGHT_COLOR,
    COMM_GET_PIC,
    COMM_SET_PARK,
    COMM_SET_PAKK_STATUS,
    COMM_GET_PAKK_STATUS,
    COMM_SET_TIMER_PARAM,
    COMM_GET_TIMER_PARAM,
    COMM_SEND_485,
    COMM_SEND_485_GET_RSP,
    COMM_UPGRADE,
    COMM_UPGRADE_PROGRESS,
    COMM_SET_CTR_PARAM,
    COMM_GET_CTR_PARAM,
    COMM_SET_NET_PARAM,
    COMM_GET_NET_PARAM
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif

