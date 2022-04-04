#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/time.h>
#include "fcntl.h"

#include "rtspservice.h"
#include "rtsputils.h"
#include "rtputils.h"
#include "ringfifo.h"
extern int g_s32DoPlay;
//===============add===================

//=====================================

char *sock_ntop_host(const struct sockaddr *sa, socklen_t salen, char *str, size_t len)
{
    switch(sa->sa_family)
    {
        case AF_INET:
        {
            struct sockaddr_in  *sin = (struct sockaddr_in *) sa;

            if(inet_ntop(AF_INET, &sin->sin_addr, str, len) == NULL)
                return(NULL);
            return(str);
        }

        default:
            snprintf(str, len, "sock_ntop_host: unknown AF_xxx: %d, len %d",
                     sa->sa_family, salen);
            return(str);
    }
    return (NULL);
}

int tcp_accept(int fd)
{
    int f;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);

    memset(&addr,0,sizeof(addr));
    addrlen=sizeof(addr);

    /*�������ӣ�����һ���µ�socket,������������*/
    f = accept(fd, (struct sockaddr *)&addr, &addrlen);

    return f;
}

void tcp_close(int s)
{
    close(s);
}

int tcp_connect(unsigned short port, char *addr)
{
    int f;
    int on=1;
    int one = 1;/*used to set SO_KEEPALIVE*/

    struct sockaddr_in s;
    int v = 1;
    if((f = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
    {
        fprintf(stderr, "socket() error in tcp_connect.\n");
        return -1;
    }
    setsockopt(f, SOL_SOCKET, SO_REUSEADDR, (char *) &v, sizeof(int));
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = inet_addr(addr);//htonl(addr);
    s.sin_port = htons(port);
    // set to non-blocking
    if(ioctl(f, FIONBIO, &on) < 0)
    {
        fprintf(stderr,"ioctl() error in tcp_connect.\n");
        return -1;
    }
    if(connect(f,(struct sockaddr*)&s, sizeof(s)) < 0)
    {
        fprintf(stderr,"connect() error in tcp_connect.\n");
        return -1;
    }
    if(setsockopt(f, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one))<0)
    {
        fprintf(stderr,"setsockopt() SO_KEEPALIVE error in tcp_connect.\n");
        return -1;
    }
    return f;
}

int tcp_listen(unsigned short port)
{
    int f;
    int on=1;

    struct sockaddr_in s;
    int v = 1;

    /*�����׽���*/
    if((f = socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        fprintf(stderr, "socket() error in tcp_listen.\n");
        return -1;
    }

    /*����socket�Ŀ�ѡ����*/
    setsockopt(f, SOL_SOCKET, SO_REUSEADDR, (char *) &v, sizeof(int));

    s.sin_family = AF_INET;
    s.sin_addr.s_addr = htonl(INADDR_ANY);
    s.sin_port = htons(port);

    /*��socket*/
    if(bind(f, (struct sockaddr *)&s, sizeof(s)))
    {
        fprintf(stderr, "bind() error in tcp_listen");
        return -1;
    }

    //����Ϊ��������ʽ
    if(ioctl(f, FIONBIO, &on) < 0)
    {
        fprintf(stderr, "ioctl() error in tcp_listen.\n");
        return -1;
    }

    /*����*/
    if(listen(f, SOMAXCONN) < 0)
    {
        fprintf(stderr, "listen() error in tcp_listen.\n");
        return -1;
    }

    return f;
}

int tcp_read(int fd, void *buffer, int nbytes, struct sockaddr *Addr)
{
    int n;
    socklen_t Addrlen = sizeof(struct sockaddr);
    char addr_str[128];
    //fprintf(stderr, "readaddr:%s ", sock_ntop_host(Addr, Addrlen, addr_str, sizeof(addr_str)) );
    //fprintf(stderr, "readPort:%d\n",ntohs(((struct sockaddr_in *)Addr)->sin_port));
    n=recv(fd, buffer, nbytes, 0);
    //printf ("read count:%d\n",n);
    if(n>0)
    {
        //��ȡ�Է�IP��Ϣ
        if(getpeername(fd, Addr, &Addrlen) < 0)
        {
            fprintf(stderr,"error getperrname:%s %i\n", __FILE__, __LINE__);
        }
        else
        {
            //��ӡ��IP��port
            fprintf(stderr, "%s ", sock_ntop_host(Addr, Addrlen, addr_str, sizeof(addr_str)));
            fprintf(stderr, "Port:%d\n",ntohs(((struct sockaddr_in *)Addr)->sin_port));
        }
    }

    return n;
}

/*
int tcp_write(int fd, void *buffer, int nbytes)
{
    int n;
    n = write(fd, buffer, nbytes);

    return n;
}
*/

int tcp_write(int connectSocketId, char *dataBuf, int dataSize)
{
    int     actDataSize;

    //��������
    while(dataSize > 0)
    {
        actDataSize = send(connectSocketId, dataBuf, dataSize, 0);

        if(actDataSize<=0)
            break;

        dataBuf  += actDataSize;
        dataSize -= actDataSize;
    }

    if(dataSize > 0)
    {
        printf("Send Data error\n");
        return -1;
    }

    return 0;
}

/*      schedule ���     */
stScheList sched[MAX_CONNECTION];

int stop_schedule = 0;//�Ƿ��˳�schedule
int num_conn = 2;    /*���Ӹ���*/

int ScheduleInit()
{
    int i;
    pthread_t thread=0;

    /*��ʼ������*/
    for(i=0; i<MAX_CONNECTION; ++i)
    {
        sched[i].rtp_session=NULL;
        sched[i].play_action=NULL;
        sched[i].valid=0;
        sched[i].BeginFrame=0;
    }

    /*�����������߳�*/
    pthread_create(&thread,NULL,schedule_do,NULL);

    return 0;
}

void *schedule_do(void *arg)
{
    int i=0;
    struct timeval now;
    unsigned long long mnow;
    char *pDataBuf, *pFindNal;
    unsigned int ringbuffer;
    struct timespec ts = {0,33333};
    int s32FileId;
    unsigned int u32NaluToken;
    char *pNalStart=NULL;
    int s32NalSize;
    int s32FindNal = 0;
    int buflen=0,ringbuflen=0,ringbuftype;
    struct ringbuf ringinfo;
//=====================
#ifdef RTSP_DEBUG
    printf("The pthread %s start\n", __FUNCTION__);
#endif

    do
    {
        nanosleep(&ts, NULL);
//      trace_point();

        s32FindNal = 0;

        //����пͻ������ӣ���g_s32DoPlay������
      //  if(g_s32DoPlay>0)
        {
            ringbuflen = ringget(&ringinfo);
            if(ringbuflen ==0)
                continue ;
        }
        s32FindNal = 1;
        for(i=0; i<MAX_CONNECTION; ++i)
        {
            if(sched[i].valid)
            {
                if(!sched[i].rtp_session->pause)
                {
                    //����ʱ���
                    gettimeofday(&now,NULL);
                    mnow = (now.tv_sec*1000 + now.tv_usec/1000);//����
                    if((sched[i].rtp_session->hndRtp)&&(s32FindNal))
                    {
                        //printf("send i frame,length:%d,pointer:%x,timestamp:%lld\n",ringinfo.size,(int)(ringinfo.buffer),mnow);
                        buflen=ringbuflen;
                        if(ringinfo.frame_type ==FRAME_TYPE_I)
                            sched[i].BeginFrame=1;
                        //if(sched[i].BeginFrame== 1)
                            sched[i].play_action((unsigned int)(sched[i].rtp_session->hndRtp), ringinfo.buffer, ringinfo.size, mnow);
                    }
                }
            }

        }
        //============add================
        //===============================
    }
    while(!stop_schedule);

cleanup:

    //free(pDataBuf);
    //close(s32FileId);

#ifdef RTSP_DEBUG
    printf("The pthread %s end\n", __FUNCTION__);
#endif
    return ERR_NOERROR;
}

//��RTP�Ự��ӽ�schedule�У����󷵻�-1,��������schedule���к�
int schedule_add(RTP_session *rtp_session)
{
    int i;
    for(i=0; i<MAX_CONNECTION; ++i)
    {
        /*���ǻ�û�б����뵽���ȶ����еĻỰ*/
        if(!sched[i].valid)
        {
            sched[i].valid=1;
            sched[i].rtp_session=rtp_session;

            //���ò��Ŷ���
            sched[i].play_action=RtpSend;
            printf("**adding a schedule object action %s,%d**\n", __FILE__, __LINE__);

            return i;
        }
    }
    return ERR_GENERIC;
}

int schedule_start(int id,stPlayArgs *args)
{
    /*    struct timeval now;
        double mnow;
        gettimeofday(&now,NULL);
        mnow=(double)now.tv_sec*1000+(double)now.tv_usec/1000;
    */
    sched[id].rtp_session->pause=0;
    sched[id].rtp_session->started=1;

    //����״̬,���������ʾ�пͻ��˲����ļ�
    g_s32DoPlay++;

    return ERR_NOERROR;
}

void schedule_stop(int id)
{
//    RTCP_send_packet(sched[id].rtp_session,SR);
//    RTCP_send_packet(sched[id].rtp_session,BYE);
}

int schedule_remove(int id)
{
    sched[id].valid=0;
    sched[id].BeginFrame=0;
    return ERR_NOERROR;
}


//����Ҫ���͵���Ϣ����rtsp.out_buffer��
int bwrite(char *buffer, unsigned short len, RTSP_buffer * rtsp)
{
    /*����Ƿ��л������*/
    if((rtsp->out_size + len) > (int) sizeof(rtsp->out_buffer))
    {
        fprintf(stderr,"bwrite(): not enough free space in out message buffer.\n");
        return ERR_ALLOC;
    }
    /*�������*/
    memcpy(&(rtsp->out_buffer[rtsp->out_size]), buffer, len);
    rtsp->out_buffer[rtsp->out_size + len] = '\0';
    rtsp->out_size += len;

#ifdef RTSP_DEBUG
    printf("(Send to client:)\n%s\n",rtsp->out_buffer);
#endif
    return ERR_NOERROR;
}

int send_reply(int err, char *addon, RTSP_buffer * rtsp)
{
    unsigned int len;
    char *b;
    int res;

    if(addon != NULL)
    {
        len = 256 + strlen(addon);
    }
    else
    {
        len = 256;
    }

    /*����ռ�*/
    b = (char *) malloc(len);
    if(b == NULL)
    {
        fprintf(stderr,"send_reply(): memory allocation error.\n");
        return ERR_ALLOC;
    }
    memset(b, 0, sizeof(b));
    /*����Э���ʽ�������*/
    sprintf(b, "%s %d %s"RTSP_EL"CSeq: %d"RTSP_EL, RTSP_VER, err, get_stat(err), rtsp->rtsp_cseq);
    strcat(b, RTSP_EL);

    /*������д�뵽��������*/
    res = bwrite(b, (unsigned short) strlen(b), rtsp);
    //�ͷſռ�
    free(b);

    return res;
}


//�ɴ����뷵�ش�����Ϣ
const char *get_stat(int err)
{
    struct
    {
        const char *token;
        int code;
    } status[] =
    {
        {
            "Continue", 100
        }, {
            "OK", 200
        }, {
            "Created", 201
        }, {
            "Accepted", 202
        }, {
            "Non-Authoritative Information", 203
        }, {
            "No Content", 204
        }, {
            "Reset Content", 205
        }, {
            "Partial Content", 206
        }, {
            "Multiple Choices", 300
        }, {
            "Moved Permanently", 301
        }, {
            "Moved Temporarily", 302
        }, {
            "Bad Request", 400
        }, {
            "Unauthorized", 401
        }, {
            "Payment Required", 402
        }, {
            "Forbidden", 403
        }, {
            "Not Found", 404
        }, {
            "Method Not Allowed", 405
        }, {
            "Not Acceptable", 406
        }, {
            "Proxy Authentication Required", 407
        }, {
            "Request Time-out", 408
        }, {
            "Conflict", 409
        }, {
            "Gone", 410
        }, {
            "Length Required", 411
        }, {
            "Precondition Failed", 412
        }, {
            "Request Entity Too Large", 413
        }, {
            "Request-URI Too Large", 414
        }, {
            "Unsupported Media Type", 415
        }, {
            "Bad Extension", 420
        }, {
            "Invalid Parameter", 450
        }, {
            "Parameter Not Understood", 451
        }, {
            "Conference Not Found", 452
        }, {
            "Not Enough Bandwidth", 453
        }, {
            "Session Not Found", 454
        }, {
            "Method Not Valid In This State", 455
        }, {
            "Header Field Not Valid for Resource", 456
        }, {
            "Invalid Range", 457
        }, {
            "Parameter Is Read-Only", 458
        }, {
            "Unsupported transport", 461
        }, {
            "Internal Server Error", 500
        }, {
            "Not Implemented", 501
        }, {
            "Bad Gateway", 502
        }, {
            "Service Unavailable", 503
        }, {
            "Gateway Time-out", 504
        }, {
            "RTSP Version Not Supported", 505
        }, {
            "Option not supported", 551
        }, {
            "Extended Error:", 911
        }, {
            NULL, -1
        }
    };

    int i;
    for(i = 0; status[i].code != err && status[i].code != -1; ++i);

    return status[i].token;
}
