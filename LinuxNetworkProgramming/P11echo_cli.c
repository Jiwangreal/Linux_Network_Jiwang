//
// Created by wangji on 19-8-6.
//

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

using namespace std;

struct packet
{
    int len;
    char buf[1024];
};

#define ERR_EXIT(m) \
        do  \
        {   \
            perror(m);  \
            exit(EXIT_FAILURE); \
        } while(0);

ssize_t readn(int fd, void *buf, size_t count)
{
    size_t nleft = count;   // 剩余字节数
    ssize_t nread;
    char *bufp = (char*) buf;

    while (nleft > 0)
    {
        nread = read(fd, bufp, nleft);
        if (nread < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return  -1;
        } else if (nread == 0)
        {
            return count - nleft;
        }

        bufp += nread;
        nleft -= nread;
    }
    return count;
}

ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nleft = count;
    ssize_t nwritten;
    char* bufp = (char*)buf;

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        else if (nwritten == 0)
        {
            continue;
        }
        bufp += nwritten;
        nleft -= nwritten;
    }
    return count;
}


ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
    while (1)
    {
        int ret = recv(sockfd, buf, len, MSG_PEEK); // 查看传入消息
        if (ret == -1 && errno == EINTR)
        {
            continue;
        }
        return ret;
    }
}

ssize_t readline(int sockfd, void *buf, size_t maxline)
{
    int ret;
    int nread;
    char *bufp = (char*)buf;    // 当前指针位置
    int nleft = maxline;
    while (1)
    {
        ret = recv_peek(sockfd, buf, nleft);
        if (ret < 0)
        {
            return ret;
        }
        else if (ret == 0)
        {
            return ret;
        }
        nread = ret;
        int i;
        for (i = 0; i < nread; i++)
        {
            if (bufp[i] == '\n')
            {
                ret = readn(sockfd, bufp, i+1);
                if (ret != i+1)
                {
                    exit(EXIT_FAILURE);
                }
                return ret;
            }
        }
        if (nread > nleft)
        {
            exit(EXIT_FAILURE);
        }
        nleft -= nread;
        ret = readn(sockfd, bufp, nread);
        if (ret != nread)
        {
            exit(EXIT_FAILURE);
        }
        bufp += nread;
    }
    return -1;
}

//测试收到SIGPIPE信号
void handle_sigpipe(int sig)
{
    printf("recv a sig = %d\n", sig);
}

void echo_cli(int sock)
{
    //测试收到SIGPIPE信号
    signale(SIGPIPE, handle_sigpipe);

    //一般忽略SIGPIPE信号即可
    signale(SIGPIPE, SIG_IGN);

    char recvbuf[1024]= [0];
    char sendbuf[1024]= [0];
    // struct packet recvbuf;
    // struct packet sendbuf;
    memset(recvbuf, 0, sizeof recvbuf);
    memset(sendbuf, 0, sizeof sendbuf);
    int n = 0;
    while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)   // 键盘输入获取,默认带\n
    {
        //writen(sockfd, sendbuf, strlen(sendbuf)); // 写入服务器

         /*模拟SIGPIPE
        若服务端已经关闭了，客户端收到了FIN，客户端调用第一个writen，会导致服务端发送一个RST段
        过来，再次调用第二个writen，会导致SIGPIPE信号的产生，此信号会终止当前进程，所以不会走
        下面的readline
        */
        writen(sockfd, sendbuf, 1);//首先发送一个字节
        writen(sockfd, sendbuf+1, strlen(sendbuf) - 1);//接着发送剩余字节


        int ret = readline(sockfd, recvbuf, sizeof(recvbuf));    // 服务器读取
        if (ret == -1)
        {
            ERR_EXIT("readline");
        }
        if (ret == 0)
        {
            printf("server close\n");
            break;
        }

        fputs(recvbuf, stdout); // 服务器返回数据输出

        // 清空
        memset(recvbuf, 0, sizeof(recvbuf));
        memset(sendbuf, 0, sizeof(sendbuf));
    }
}

int main(int argc, char** argv) {
    // 1. 创建套接字
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ERR_EXIT("socket");
    }

    // 2. 分配套接字地址
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    // servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // inet_aton("127.0.0.1", &servaddr.sin_addr);

    // 3. 请求链接
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof servaddr) < 0) {
        ERR_EXIT("connect");
    }

    struct sockaddr_in localaddr;//本地地址
    socklen_t addrlen = sizeof(localaddr);//要有初始值，和accept是一样的

    //已连接的套接口sockfd，既有本地地址，又有对等方的地址
    if (getsockname(sockfd, (struct sockaddr*)&localaddr, &addrlen) < 0)
    {
        ERR_EXIT("getsockname");
    }
    printf("id = %s, ", inet_ntoa(localaddr.sin_addr));
    printf("port = %d\n", ntohs(localaddr.sin_port));

    // 4. 数据交换
    echo_cli(sockfd);

    // 5. 断开连接
    close(sockfd);


    return 0;
}