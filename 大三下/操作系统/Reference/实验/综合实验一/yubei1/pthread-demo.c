#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>

void thread(void)
{
    printf("This is a pthread.\n");
    sleep(10);  // 线程休眠10秒
}

int main(void)
{
    pthread_t id;     // 线程标识符
    int i, ret;

    // 创建新线程
    ret = pthread_create(&id, NULL, (void*)thread, NULL);

    if (ret != 0) {
        printf("Create pthread error!\n");
        exit(1);     // 创建线程失败，退出程序
    }

    printf("This is the main process.\n");
    pthread_join(id, NULL);  // 等待线程结束
    return 0;
}
