#include "pro.h"
#include <pthread.h>

void* deploy_thread(void* arg) {
    deploy_main(); 
    return NULL;
}

void* socket_thread(void* arg) {
    socket_main();
    return NULL;
}

void* xitong_thread(void* arg) {
    xitong_main();
    return NULL;
}

void* jiance_thread(void* arg) {
    jc_main();
    return NULL;
}

int main(void)
{
    // 定义线程ID
    pthread_t tid_deploy, tid_socket,tid_xitong,tid_jiance;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // 启动deploy线程
    if (pthread_create(&tid_deploy, &attr, deploy_thread, NULL) != 0) {
        printf("创建deploy线程失败！\n");
        return -1;
    }

    // 启动socket线程
    if (pthread_create(&tid_socket, &attr, socket_thread, NULL) != 0) {
        printf("创建socket线程失败！\n");
        return -1;
    }

    //启动线程系统
    if (pthread_create(&tid_xitong, &attr, xitong_thread, NULL) != 0) {
        printf("创建系统线程失败！\n");
        return -1;
    }

	//启动线程检测火焰
    if (pthread_create(&tid_jiance, &attr, jiance_thread, NULL) != 0) {
        printf("创建系统线程失败！\n");
        return -1;
    }

    pthread_attr_destroy(&attr);

    // 主线程不退出
    while (1) {
        sleep(1);
    }
    return 0;
}