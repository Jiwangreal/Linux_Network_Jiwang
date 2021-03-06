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
    int len;
    char buf[1024];
};

#define ERR_EXIT(m) \
        do  \
        {   \
            perror(m);  \
            exit(EXIT_FAILURE); \
        } while(0);

//参考man 2 read声明写出来的
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

void do_service(int connfd)
{
    // char recvbuf[1024];
    struct packet recvbuf;
    int n;
    while (1)
    {
        memset(&recvbuf, 0, sizeof(recvbuf));
        int ret = readn(connfd, &recvbuf.len, 4);
        if (ret == -1)
        {
            ERR_EXIT("read");
        }
        else if (ret < 4)
        {
            printf("client close\n");
            break;
        }

        n = ntohl(recvbuf.len);
        ret = readn(connfd, recvbuf.buf, n);
        if (ret == -1)
        {
            ERR_EXIT("read");
        }
        else if (ret < n)
        {
            printf("client close\n");
            break;
        }
        fputs(recvbuf.buf, stdout);
        writen(connfd, &recvbuf, 4+n);
    }

}

int main(int argc, char** argv) {
    // 1. 创建套接字
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ERR_EXIT("socket");
    }

    // 2. 分配套接字地址
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(6666);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // inet_aton("127.0.0.1", &servaddr.sin_addr);

    int on = 1;
    // 确保time_wait状态下同一端口仍可使用
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        ERR_EXIT("setsockopt");
    }

    // 3. 绑定套接字地址
    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    // 4. 等待连接请求状态
    if (listen(listenfd, SOMAXCONN) < 0) {
        ERR_EXIT("listen");
    }
    // 5. 允许连接
    struct sockaddr_in peeraddr;
    socklen_t peerlen = sizeof(peeraddr);


    // 6. 数据交换
    pid_t pid;
    while (1)
    {
        int connfd;
        if ((connfd = accept(listenfd, (struct sockaddr *) &peeraddr, &peerlen)) < 0) {
            ERR_EXIT("accept");
        }

        printf("id = %s, ", inet_ntoa(peeraddr.sin_addr));
        printf("port = %d\n", ntohs(peeraddr.sin_port));

        pid = fork();

        if (pid == -1)
        {
            ERR_EXIT("fork");
        }
        if (pid == 0)   // 子进程
        {
            close(listenfd);
            do_service(connfd);
            //printf("child exit\n");
            exit(EXIT_SUCCESS);
        }
        else
        {
            //printf("parent exit\n");
            close(connfd);
        }


    }
    // 7. 断开连接
    close(listenfd);



    return 0;
}
