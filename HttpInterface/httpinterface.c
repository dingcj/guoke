#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <curl/curl.h>

#include "httpinterface.h"
#include "base64.h"

#define HTTP_TIMEOUT (5)

char *generate_json_by_jpg(unsigned char *allJpgData, unsigned int allJpgLen,
        unsigned char *plateJpgData, unsigned int plateJpgLen)
{
    char *base64 = NULL;
    size_t offset = 0;
    int ret = 0;
    size_t buffLen = allJpgLen * 2 + plateJpgLen * 2;

    base64 = malloc(buffLen);
    if (base64 == NULL)
    {
        printf("post_jpg_to_server, malloc failed.\n");
        return NULL;
    }

    memset(base64, 0, buffLen);
    ret = snprintf(base64 + offset, buffLen - offset, "{\n\"ImageSmall\":\"");
    offset += ret;
    
    ret = base64_encode_http(plateJpgData, plateJpgLen, base64 + offset, buffLen - offset);
    offset += ret;

    ret = snprintf(base64 + offset, buffLen - offset, "\",\n");
    offset += ret;
    
    ret = snprintf(base64 + offset, buffLen - offset, "\"ImageSmallSize\":%u,\n", plateJpgLen);
    offset += ret;

    
    ret = snprintf(base64 + offset, buffLen - offset, "\"ImageAll\":\"");
    offset += ret;
    
    ret = base64_encode_http(allJpgData, allJpgLen, base64 + offset, buffLen - offset);
    offset += ret;

    ret = snprintf(base64 + offset, buffLen - offset, "\",\n");
    offset += ret;
    
    ret = snprintf(base64 + offset, buffLen - offset, "\"ImageAllSize\":%u\n}", allJpgLen);
    offset += ret;
    
    //printf("%s\n", base64);
    //free(base64);

    return base64;
}

size_t get_rsp_data_func(void *ptr, size_t size, size_t nmemb, PostRspCtx *ctx) {
    uint32_t left = ctx->buffLen - ctx->pos;
    uint32_t cpyLen = (left <= (size * nmemb)) ? left : (size * nmemb);

    memcpy(ctx->buff + ctx->pos, ptr, cpyLen);
    ctx->pos += cpyLen;

    return nmemb;
}

int post_json_to_server(const char * url, char *postData, char *buff, uint32_t buffLen)
{
    CURL *pCurl = NULL;
    int ret = 0;
    struct curl_slist *plist = NULL;
    CURLcode curlRet = CURLE_OK;
    long resCode = 0;
    PostRspCtx ctx;

    ctx.buff = buff;
    ctx.buffLen = buffLen;
    ctx.pos = 0u;

    curl_global_init(CURL_GLOBAL_ALL);
    pCurl = curl_easy_init();
    if (pCurl == NULL)
    {
        printf("curl_easy_init failed!\n");
        ret = -1;
        goto CURL_CLEANUP;
    }
    curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, HTTP_TIMEOUT);
    curl_easy_setopt(pCurl, CURLOPT_URL, url);
    if (buff != NULL && buffLen != 0) {
        curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, get_rsp_data_func);
        curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &ctx);
    }

    plist = curl_slist_append(NULL, "Content-Type:application/json;charset=UTF-8");
    if (plist == NULL)
    {
        printf("curl_slist_append failed!\n");
        ret = -1;
        goto EASY_CLEAN;
    }
    
    curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);
    curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, postData);

    curlRet = curl_easy_perform(pCurl);
    if (curlRet != CURLE_OK)
    {
        printf("curl_easy_perform failed!\n");
        ret = -1;
    }

    curlRet = curl_easy_getinfo(pCurl, CURLINFO_RESPONSE_CODE, &resCode);
    if ((curlRet != CURLE_OK) || (resCode != 200)) {
        printf("curl_easy_getinfo failed %d %ld!\n", curlRet, resCode);
        ret = -1;
    }
    
    curl_slist_free_all(plist);
EASY_CLEAN:
    curl_easy_cleanup(pCurl);
CURL_CLEANUP:
    curl_global_cleanup();
    return ret;
}

static char ProvinceUtf8Tbl[][16] = {
    //"云京冀吉宁川新晋桂沪津浙渝湘琼甘皖粤苏蒙藏豫贵赣辽鄂闽陕青鲁黑"
    {0xe4, 0xba, 0x91, 0x00},
    {0xe4, 0xba, 0xac, 0x00},
    {0xe5, 0x86, 0x80, 0x00},
    {0xe5, 0x90, 0x89, 0x00},
    {0xe5, 0xae, 0x81, 0x00},
    {0xe5, 0xb7, 0x9d, 0x00},
    {0xe6, 0x96, 0xb0, 0x00},
    {0xe6, 0x99, 0x8b, 0x00},
    {0xe6, 0xa1, 0x82, 0x00},
    {0xe6, 0xb2, 0xaa, 0x00},
    {0xe6, 0xb4, 0xa5, 0x00},
    {0xe6, 0xb5, 0x99, 0x00},
    {0xe6, 0xb8, 0x9d, 0x00},
    {0xe6, 0xb9, 0x98, 0x00},
    {0xe7, 0x90, 0xbc, 0x00},
    {0xe7, 0x94, 0x98, 0x00},
    {0xe7, 0x9a, 0x96, 0x00},
    {0xe7, 0xb2, 0xa4, 0x00},
    {0xe8, 0x8b, 0x8f, 0x00},
    {0xe8, 0x92, 0x99, 0x00},
    {0xe8, 0x97, 0x8f, 0x00},
    {0xe8, 0xb1, 0xab, 0x00},
    {0xe8, 0xb4, 0xb5, 0x00},
    {0xe8, 0xb5, 0xa3, 0x00},
    {0xe8, 0xbe, 0xbd, 0x00},
    {0xe9, 0x84, 0x82, 0x00},
    {0xe9, 0x97, 0xbd, 0x00},
    {0xe9, 0x99, 0x95, 0x00},
    {0xe9, 0x9d, 0x92, 0x00},
    {0xe9, 0xb2, 0x81, 0x00},
    {0xe9, 0xbb, 0x91, 0x00},

    //"北成广海济京军空兰南沈WJ"
    {0xe5, 0x8c, 0x97, 0x00},
    {0xe6, 0x88, 0x90, 0x00},
    {0xe5, 0xb9, 0xbf, 0x00},
    {0xe6, 0xb5, 0xb7, 0x00},
    {0xe6, 0xb5, 0x8e, 0x00},
    {0xe4, 0xba, 0xac, 0x00},
    {0xe5, 0x86, 0x9b, 0x00},
    {0xe7, 0xa9, 0xba, 0x00},
    {0xe5, 0x85, 0xb0, 0x00},
    {0xe5, 0x8d, 0x97, 0x00},
    {0xe6, 0xb2, 0x88, 0x00},
    {0x57, 0x4a, 0x00, 0x00},

    //"学警领港澳台挂试超使"
    {0xe5, 0xad, 0xa6, 0x00},
    {0xe8, 0xad, 0xa6, 0x00},
    {0xe9, 0xa2, 0x86, 0x00},
    {0xe6, 0xb8, 0xaf, 0x00},
    {0xe6, 0xbe, 0xb3, 0x00},
    {0xe5, 0x8f, 0xb0, 0x00},
    {0xe6, 0x8c, 0x82, 0x00},
    {0xe8, 0xaf, 0x95, 0x00},
    {0xe8, 0xb6, 0x85, 0x00},
    {0xe4, 0xbd, 0xbf, 0x00},

    //"消边通森电金林特农武"
    {0xe6, 0xb6, 0x88, 0x00},
    {0xe8, 0xbe, 0xb9, 0x00},
    {0xe9, 0x80, 0x9a, 0x00},
    {0xe6, 0xa3, 0xae, 0x00},
    {0xe7, 0x94, 0xb5, 0x00},
    {0xe9, 0x87, 0x91, 0x00},
    {0xe6, 0x9e, 0x97, 0x00},
    {0xe7, 0x89, 0xb9, 0x00},
    {0xe5, 0x86, 0x9c, 0x00},
    {0xe6, 0xad, 0xa6, 0x00},

    //??
    {0x3f, 0x3f, 0x00, 0x00},
};

static char *ProvinceGb2312Tbl[] = {
    "云", "京", "冀", "吉", "宁", "川", "新", "晋", "桂", "沪", "津",
    "浙", "渝", "湘", "琼", "甘", "皖", "粤", "苏", "蒙", "藏", "豫", 
    "贵", "赣", "辽", "鄂", "闽", "陕", "青", "鲁", "黑", "北", "成", 
    "广", "海", "济", "京", "军", "空", "兰", "南", "沈", "WJ", "学", 
    "警", "领", "港", "澳", "台", "挂", "试", "超", "使", "消", "边", 
    "通", "森", "电", "金", "林", "特", "农", "武", "??"
};

char *ProvinceUtf8ToGb2312(char *utf8)
{
    int utf8Num = sizeof(ProvinceUtf8Tbl) / sizeof(ProvinceUtf8Tbl[0]);
    int gb2312Num = sizeof(ProvinceGb2312Tbl) / sizeof(ProvinceGb2312Tbl[0]);

    if (gb2312Num != utf8Num) {
        return NULL;
    }

    for (int i = 0; i < gb2312Num; i++) {
        if (strcmp(utf8, ProvinceUtf8Tbl[i]) == 0) {
            return ProvinceGb2312Tbl[i];
        }
    }

    return NULL;
}

char *ProvinceGb2312ToUtf8(char *gb2312)
{
    int utf8Num = sizeof(ProvinceUtf8Tbl) / sizeof(ProvinceUtf8Tbl[0]);
    int gb2312Num = sizeof(ProvinceGb2312Tbl) / sizeof(ProvinceGb2312Tbl[0]);

    if (gb2312Num != utf8Num) {
        return NULL;
    }

    for (int i = 0; i < gb2312Num; i++) {
        if (strncmp(gb2312, ProvinceGb2312Tbl[i], 2) == 0) {
            return ProvinceUtf8Tbl[i];
        }
    }

    return NULL;
}

time_t CurrentTimestamp()
{
    struct timespec tp = {0};
    clock_gettime(CLOCK_MONOTONIC, &tp);

    return tp.tv_sec;
}

uint32_t CheckIsTimeout(time_t now, time_t prev, time_t interval)
{
    if ((prev > now) || ((now - prev) > interval)) {
        return 1;
    }

    return 0;
}

int GetRemoteFileSize(const char * URL)
{
 
    CURL * curl_fd = curl_easy_init();
    CURLcode code = -1;
    long response_code = 0;
    double size = 0.0;
    
    curl_easy_setopt(curl_fd, CURLOPT_NOBODY, 1);
    curl_easy_setopt(curl_fd, CURLOPT_TIMEOUT, HTTP_TIMEOUT);
    curl_easy_setopt(curl_fd, CURLOPT_URL, URL);
    code = curl_easy_perform(curl_fd);

    if (code == CURLE_OK) {
        curl_easy_getinfo(curl_fd, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 200) {
            curl_easy_getinfo(curl_fd, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
        }
    }

    curl_easy_cleanup(curl_fd);

    return (int)size;
}

int GetRemoteFile(const char *url, void *writeFunc, void *ctx)
{
    CURL *curl;
    CURLcode res;
    int ret = 0;
   
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code != 200) {
                ret = -1;
            }
        } else {
            ret = -1;
        }
        
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    
    return 0;
}



