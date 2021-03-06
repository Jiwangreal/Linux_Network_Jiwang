//
// Created by wangji on 19-8-14.
//

// p35 poxis共享内存

#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <sys/mman.h>

using namespace std;

#define ERR_EXIT(m) \
        do  \
        {   \
            perror(m);  \
            exit(EXIT_FAILURE); \
        } while (0);

struct student
{
    char name[32];
    int age;
}STU;

int main(int argc, char** argv)
{
    int shmid;
    shmid = shm_open("/xyz", O_RDONLY, 0666);
    //shmid = shm_open("/xyz", O_RDWR, S_IRUSR);
    if (shmid == -1)
    {
        ERR_EXIT("shmid");
    }
    printf("shmopen success\n");

    STU* stu;
    // if (ftruncate(shmid, sizeof(stu)) == -1)
    // {
    //     ERR_EXIT("ftruncate");
    // }
    // printf("truncate succ\n");

    struct stat buf;
    if (fstat(shmid, &buf) == -1)
    {
        ERR_EXIT("fstat");
    }

    printf("mode: %o, size: %ld\n", buf.st_mode & 07777, buf.st_size);

    stu = (student*)mmap(NULL, buf.st_size, PROT_READ, MAP_SHARED, shmid, 0);//yPROT_READ表示以读的方式进行映射

    //映射成功，将指针所指向的内存信息打印出来
    printf("name: %s, age: %d\n", stu->name, stu->age);

    close(shmid);

    return 0;
}