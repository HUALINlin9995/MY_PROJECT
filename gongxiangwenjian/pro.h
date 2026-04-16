#ifndef PRO_H
#define PRO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h> 
#include <fcntl.h> 
#include <sys/types.h>
#include<sys/stat.h>
#include <sys/mman.h>

#define SERVER_IP "192.168.1.100"
#define PORT 5000
#define MOVIE_SAVE_PATH "/ceshi/movie/"
#define IMG_SAVE_PATH "/ceshi/img/"
#define AUDIO_SAVE_PATH "/ceshi/audio/"
#define MAX_FILE_SIZE 30 * 1024 * 1024
#define MAX_THREADS 3
#define BUF_SIZE 8192


int cmd_main(const char* command);
int wallpaper_main(const char* failname);
int movie_main(const char* full_path）;

//全局信号量
sem_t g_sem;


typedef struct {
    char url[512];
    char save_path[512];
} DownloadTask;

#endif