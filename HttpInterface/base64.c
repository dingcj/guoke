#include <stdio.h>
#include <string.h>
 

// 全局常量定义
const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char padding_char = '=';
 

/*编码代码
 * const unsigned char * sourcedata， 源数组
 * char * base64 ，码字保存
 */
int base64_encode_http(const unsigned char * sourcedata, size_t source_len, char * base64, size_t buf_len)
{
    int i=0, j=0;
    unsigned char trans_index=0;
    //const int datalength = strlen((const char*)sourcedata);
    for (; i < source_len && j < buf_len - 1; i += 3){
        trans_index = ((sourcedata[i] >> 2) & 0x3f);
        base64[j++] = base64char[(int)trans_index];
        trans_index = ((sourcedata[i] << 4) & 0x30);
        if (i + 1 < source_len){
            trans_index |= ((sourcedata[i + 1] >> 4) & 0x0f);
            base64[j++] = base64char[(int)trans_index];
        }else{
            base64[j++] = base64char[(int)trans_index];
 
            base64[j++] = padding_char;
 
            base64[j++] = padding_char;
 
            break;
        }
        trans_index = ((sourcedata[i + 1] << 2) & 0x3c);
        if (i + 2 < source_len){
            trans_index |= ((sourcedata[i + 2] >> 6) & 0x03);
            base64[j++] = base64char[(int)trans_index];
 
            trans_index = sourcedata[i + 2] & 0x3f;
            base64[j++] = base64char[(int)trans_index];
        }
        else{
            base64[j++] = base64char[(int)trans_index];
 
            base64[j++] = padding_char;
 
            break;
        }
    }
 
    base64[j] = '\0'; 
 
    return j;
}

int num_strchr(const char *str, char c) // 
{
    const char *pindex = strchr(str, c);
    if (NULL == pindex){
        return -1;
    }
    return pindex - str;
}

int base64_decode(const char * base64, unsigned char * dedata)
{
    int i = 0, j=0;
    int trans[4] = {0,0,0,0};
    for (;base64[i]!='\0';i+=4){
        trans[0] = num_strchr(base64char, base64[i]);
        trans[1] = num_strchr(base64char, base64[i+1]);
        // 1/3
        dedata[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1]>>4) & 0x03);
 
        if (base64[i+2] == '='){
            continue;
        }
        else{
            trans[2] = num_strchr(base64char, base64[i + 2]);
        }
        // 2/3
        dedata[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);
 
        if (base64[i + 3] == '='){
            continue;
        }
        else{
            trans[3] = num_strchr(base64char, base64[i + 3]);
        }
 
        // 3/3
        dedata[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
    }
 
    dedata[j] = '\0';
 
    return 0;
}

