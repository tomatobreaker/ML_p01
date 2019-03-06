#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <base.h>
#include <setjmp.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

 
#define PACKET_SIZE     4096
#define ERROR           0
#define SUCCESS         1
 
 
void icmp_type_name (int id)  //定义返回类型，提示出我们看到的信息，其中，ICMP_ECHOREPLY是正确的返回，ICMP_ECHO是正确的发送
{
        switch (id) {
        case ICMP_ECHOREPLY:                 applog(NOTICE,"[Xminerd] Echo Reply\n"); break;
        case ICMP_DEST_UNREACH:          applog(NOTICE, "[Xminerd] Destination Unreachable\n");break;
        case ICMP_SOURCE_QUENCH:          applog(NOTICE, "[Xminerd] Source Quench\n");break;
        case ICMP_REDIRECT:                  applog(NOTICE, "[Xminerd] Redirect (change route)\n");break;
        case ICMP_ECHO:                          applog(NOTICE, "[Xminerd] Echo Request\n");break;
        case ICMP_TIME_EXCEEDED:          applog(NOTICE, "[Xminerd] Time Exceeded\n");break;
        case ICMP_PARAMETERPROB:          applog(NOTICE, "[Xminerd] Parameter Problem\n");break;
        case ICMP_TIMESTAMP:                   applog(NOTICE, "[Xminerd] Timestamp Request\n");break;
        case ICMP_TIMESTAMPREPLY:          applog(NOTICE, "[Xminerd] Timestamp Reply\n");break;
        case ICMP_INFO_REQUEST:          applog(NOTICE, "[Xminerd] Information Request\n");break;
        case ICMP_INFO_REPLY:                  applog(NOTICE, "[Xminerd] Information Reply\n");break;
        case ICMP_ADDRESS:                         applog(NOTICE, "[Xminerd] Address Mask Request\n");break;
        case ICMP_ADDRESSREPLY:         applog(NOTICE, "[Xminerd] Address Mask Reply\n");break;
        default:                                         applog(NOTICE, "[Xminerd] unknown ICMP type\n");break;
        }
}
 
// 效验算法
unsigned short cal_chksum(unsigned short *addr, int len)
{
    int nleft=len;
    int sum=0;
    unsigned short *w=addr;
    unsigned short answer=0;
        
    /*把ICMP报头二进制数据以2字节为单位累加起来*/
    while(nleft > 1)
    {
           sum += *w++;
        nleft -= 2;
    }
    /*若ICMP报头为奇数个字节，会剩下最后一字节
    。把最后一个字节视为一个2字节数据的高字节
    ，这个2字节数据的低字节为0，继续累加*/
    if( nleft == 1)
    {      
        *(unsigned char *)(&answer) = *(unsigned char *)w;
           sum += answer;
    }
   
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
   
    return answer;
}
 
/*两个timeval结构相减 计算时间差*/
void tv_sub(struct timeval *out,struct timeval *in)
{       if( (out->tv_usec-=in->tv_usec)<0)
        {       --out->tv_sec;
                out->tv_usec+=1000000;
        }
        out->tv_sec-=in->tv_sec;
}
 
/*设置ICMP报头,返回报文长度*/
int pack(int pack_no, char *sendpacket)
{       int i,packsize;
         int datalen = 56;//为啥是56??
          
        struct icmp *icmp;
        struct timeval *tval;
          
          icmp=(struct icmp*)sendpacket;    
        icmp->icmp_type=ICMP_ECHO;
        icmp->icmp_code=0;
        icmp->icmp_cksum=0;
        icmp->icmp_seq=pack_no;
          // 取得PID，作为Ping的Sequence ID
        icmp->icmp_id=getpid();
        packsize=8+datalen;
        tval= (struct timeval *)icmp->icmp_data;
        gettimeofday(tval,NULL);    /*记录发送时间*/
        icmp->icmp_cksum=cal_chksum((unsigned short *)icmp,packsize); /*校验算法*/
        return packsize;
 
 
}
 
/*剥去ICMP报头*/
int unpack(char *buf,int len,  struct sockaddr_in from, char *ips)
{       int i,iphdrlen;
        struct ip *ip;
        struct icmp *icmp;
        struct timeval *tvsend;
          struct timeval tvrecv;
        double time;
          char *from_ip;
 
          gettimeofday(&tvrecv,NULL);    /*记录接收时间*/
          ip=(struct ip *)buf;
          iphdrlen=ip->ip_hl<<2;    /*求ip报头长度,即ip报头的长度标志乘4*/
          icmp=(struct icmp *)(buf+iphdrlen);  /*越过ip报头,指向ICMP报头*/
          len-=iphdrlen;            /*ICMP报头及ICMP数据报的总长度*/
          if( len<8)                /*小于ICMP报头长度则不合理*/
          {       applog(NOTICE, "[Xminerd] ICMP packets\'s length is less than 8\n");
                  return -1;
          }
          // 判断是否是自己Ping的回复
          from_ip = (char *)inet_ntoa(from.sin_addr);
         if (strcmp(from_ip,ips) != 0)
         {
            applog(NOTICE, "[Xminerd] ip:%s, %s Ip wang\n",ips, from_ip);
            return -1;
         }
       
        /*确保所接收的是我所发的的ICMP的回应*/
        if( (icmp->icmp_type==ICMP_ECHOREPLY) && (icmp->icmp_id==getpid()) )
        {     
                tvsend=(struct timeval *)icmp->icmp_data;
                tv_sub(&tvrecv,tvsend);  /*接收和发送的时间差*/
                time=tvrecv.tv_sec*1000+tvrecv.tv_usec/1000;  /*以毫秒为单位计算rtt*/
                /*显示相关信息*/
#if 0
                applog(NOTICE, "[Xminerd] %d byte from %s: icmp_seq=%u ttl=%d rtt=%.3f ms and reply type : ",
                                len,
                                inet_ntoa(from.sin_addr),
                                icmp->icmp_seq,
                                ip->ip_ttl,
                                time
                      );
                icmp_type_name(icmp->icmp_type);
#endif
                return 0;
        }
        else  
        {
                return -1;
#if 0
                icmp_type_name(icmp->icmp_type);
#endif

        };
}


static sigjmp_buf jmpbuf;

static void alarm_func()

{
        siglongjmp(jmpbuf, 1);
}

static struct hostent *gngethostbyname(char *HostName, int timeout)

{

        struct hostent *lpHostEnt;

        signal(SIGALRM, alarm_func);
        if(sigsetjmp(jmpbuf, 1) != 0)
        {
                alarm(0); /* 取消闹钟 */
                signal(SIGALRM, SIG_IGN);
                return NULL;

        }

        alarm(timeout); /* 设置超时时间 */

        lpHostEnt = gethostbyname(HostName);

        signal(SIGALRM, SIG_IGN);

        return lpHostEnt;
}

// Ping函数
int xping( char *ips, int timeout)
{
    struct timeval timeo;
    int sockfd;
    struct sockaddr_in addr;
    struct sockaddr_in from;
    struct timeval *tval;
    socklen_t fromlen ;
    int i,packsize;
    char sendpacket[PACKET_SIZE];
    char recvpacket[PACKET_SIZE];
    struct hostent * pHost = NULL;

    char r_ips[64];
    char str[INET_ADDRSTRLEN];

    in_addr_t inaddr;
   
    int n;
    int maxfds = 0;
    fd_set readfds;
   
    // 设定Ip信息
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;

    /* 将点分十进制ip地址转换为网络字节序 */
    if ((inaddr = inet_addr(ips)) == INADDR_NONE)
    {
            /* 转换失败，表明是主机名,需通过主机名获取ip */
            if ((pHost = gngethostbyname(ips, 2)) == NULL)
            {
                    applog(NOTICE, "Gethostbyname failed\n");
                    return ERROR;
            }
            memmove(&addr.sin_addr, pHost->h_addr_list[0], pHost->h_length);
            if(inet_ntop(pHost->h_addrtype, pHost->h_addr_list[0], str,sizeof(str)) != NULL)
                    sprintf(r_ips, "%s", str);
    }
    else
    {
            memmove(&addr.sin_addr, &inaddr, sizeof(struct in_addr));
            sprintf(r_ips, "%s", ips);
    }

    if (NULL != pHost)
            applog(NOTICE, "PING %s IP %s\n", pHost->h_name, r_ips);
    else
            applog(NOTICE, "PING %s\n", ips);
   
 
    // 取得socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
       applog(NOTICE, "[Xminerd] ip:%s,socket error\n",r_ips);
        return ERROR;
    }
   
    // 设定TimeOut时间
    timeo.tv_sec = timeout / 1000;
    timeo.tv_usec = timeout % 1000;
    /*在send(),recv()过程中有时由于网络状况等原因
    ，发收不能预期进行,而设置收发时限：
         int nNetTimeout=1000;//1秒
         //发送时限
         setsockopt(socket，SOL_S0CKET,SO_SNDTIMEO，(char *)&nNetTimeout,sizeof(int));
     */
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) == -1)
    {
            applog(NOTICE, "[Xminerd] ip:%s,setsockopt error\n",r_ips);
            close(sockfd);
            return ERROR;
    }

    //发送10个包数据
    for(i = 0 ; i< 10; i++)
    {
            // 设定Ping包
            memset(sendpacket, 0, sizeof(sendpacket));
            packsize=pack(i, sendpacket);
            // 发包
            n = sendto(sockfd, (char *)&sendpacket, packsize, 0, (struct sockaddr *)&addr, sizeof(addr));
            if (n < 1)
            {
                    applog(NOTICE, "[Xminerd] ip:%s,sendto error\n",r_ips);
                    close(sockfd);
                    return ERROR;
            }
    }
    i = 0;
    // 接受
    // 由于可能接受到其他Ping的应答消息，所以这里要用循环
    while(1)
    {
            // 设定TimeOut时间，这次才是真正起作用的
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            maxfds = sockfd + 1;
            n = select(maxfds, &readfds, NULL, NULL, &timeo);
            if (n <= 0)
            {
                    applog(NOTICE, "[Xminerd] ping %s,Time out error\n",r_ips);
                    close(sockfd);
                    return ERROR;
            }

            // 接受
            memset(recvpacket, 0, sizeof(recvpacket));
            fromlen = sizeof(from);
            n = recvfrom(sockfd, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, &fromlen);
            if (n < 1) {
                    break;
            }
            if(unpack(recvpacket, n, from, r_ips) ==0)
                    i++; 
            if(i >= 5)//这里假设10包全部能收到
                    break;
    }

    // 关闭socket
    close(sockfd);

    applog(NOTICE, "[Xminerd] ping %s,Success\n",ips);
    return SUCCESS;
}

#if 0 
int main(int argc, char** argv)
{
           if(argc<2)
        {       applog(NOTICE, "[Xminerd] usage:%s hostname/IP address\n",argv[0]);
                exit(1);
        }
         if(ping(argv[1], 1000) == ERROR)
                   applog(NOTICE, "[Xminerd] ping error \n");
         return 0;
}
#endif
 
