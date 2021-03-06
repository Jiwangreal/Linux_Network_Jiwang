//
// Created by wangji on 19-7-21.
//

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

struct packet
{
    int len;//包头：存放包体实际的数据长度
    char buf[1024];//包体
};

#define ERR_EXIT(m) \
        do  \
        {   \
            perror(m);  \
            exit(EXIT_FAILURE); \
        } while(0);

/参考man 2 read声明写出来的
//ssize_t是无符号整数
ssize_t readn(int fd, void *buf, size_t count)
{
    size_t nleft = count;   // 剩余字节数
    ssize_t nread;//已接收字节数
    char *bufp = (char*) buf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR)
                continue;
            return  -1;
        } 
        else if (nread == 0)//表示读取到了EOF，表示对方关闭
            return count - nleft;//表示剩余的字节数

        bufp += nread;//读到的nread，要将bufp指针偏移
        nleft -= nread;
    }
    return count;
}

//参考man 2 write声明写出来的
ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nleft = count;//剩余要发送的字节数
    ssize_t nwritten;
    char* bufp = (char*)buf;

    while (nleft > 0)
    {
        //write一般不会阻塞，缓冲区数据大于发送的数据，就能够成功将数据拷贝到缓冲区中
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
        bufp += nwritten;//已发送字节数
        nleft -= nwritten;//剩余字节数
    }
    return count;
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

    // 4. 数据交换
//    char recvbuf[1024];
//    char sendbuf[1024];
    struct packet recvbuf;
    struct packet sendbuf;
    memset(&recvbuf, 0, sizeof(recvbuf));
    memset(&sendbuf, 0, sizeof(sendbuf));
    int n = 0;
    while (fgets(sendbuf.buf, sizeof(sendbuf.buf), stdin) != NULL)   // 键盘输入获取
    {
        n = strlen(sendbuf.buf);//n是包体的长度
        sendbuf.len = htonl(n); // 主机字节序转换为网络字节序
        writen(sockfd, &sendbuf, 4+n); // 头部4字节+包体

        int ret = readn(sockfd, &recvbuf.len, 4); //先接收头部
        if (ret == -1)
        {
            ERR_EXIT("read");
        }
        else if (ret < 4)
        {
            printf("server close\n");
            break;
        }

        n = ntohl(recvbuf.len);//转换程主机字节序
        ret = readn(sockfd, &recvbuf.buf, n);//接收包体
        if (ret == -1)
        {
            ERR_EXIT("read");
        }
        else if (ret < n)
        {
            printf("server close\n");
            break;
        }

        fputs(recvbuf.buf, stdout); //将接收到的数据输出

        // 清空
        memset(&recvbuf, 0, sizeof recvbuf);
        memset(&sendbuf, 0, sizeof sendbuf);
    }

    // 5. 断开连接
    close(sockfd);


    return 0;
}