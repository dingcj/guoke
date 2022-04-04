#ifndef __PLATE_HTTPINTERFACE_H__
#define __PLATE_HTTPINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <curl/curl.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

int post_json_to_server(const char * url, char *postData, char *buff, uint32_t buffLen);
char *ProvinceUtf8ToGb2312(char *utf8);
char *ProvinceGb2312ToUtf8(char *gb2312);
uint32_t CheckIsTimeout(time_t now, time_t prev, time_t interval);
time_t CurrentTimestamp();
int GetRemoteFileSize(const char * URL);
int GetRemoteFile(const char *url, void *writeFunc, void *ctx);

typedef struct PostRspCtx {
    char        *buff;
    uint32_t    buffLen;
    uint32_t    pos;
} PostRspCtx;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* */

