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
#include <signal.h>
#include<fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <linux/input.h>



#define SERVER_IP "192.168.1.100"
#define PORT 5000
#define MOVIE_SAVE_PATH "/ceshi/movie/"
#define IMG_SAVE_PATH "/ceshi/img/"
#define AUDIO_SAVE_PATH "/ceshi/audio/"
#define MAX_FILE_SIZE 30 * 1024 * 1024
#define MAX_THREADS 3
#define BUF_SIZE 8192

#define MAX_FILES 200          // 最多存200个文件名
#define FILE_NAME_LEN 256      // 每个文件名最长256字符

extern char* WALLPAPER_NAME;

//客户端主程序
int socket_main();
//初始化函数
int deploy_main();
//系统——图标主程序
int tubiao_main();
//系统——壁纸主程序
int tupian_main(void);
//系统——视频主程序
int shiping_main();
//系统——获取坐标函数
void get_pos(int* x, int* y);
//系统——绘制图标函数
int draw_bmp(const char* bmp_path, int x0, int y0);
//客户端——执行远程命令函数
int cmd_main(const char* command);
//系统——画板主程序
int huaban_main();
//加载壁纸函数
int wallpaper_main(const char* failname);
//客户端——视频函数
int movie_main(const char* full_path,int play_seconds);
//系统——获取文件夹下所有文件函数
int get_all_files(const char* dir_path);
//系统主程序
int xitong_main(void);
//火焰检测主程序
int jc_main();
//检测——报警函数
int bj_main(int i);
//系统——画板图标函数
void tubiao_huaban();
//客户端——发送消息函数
void http_send(const char* ip, int port, const char* data);




// 帧缓冲全局变量
// pro.h 修正后
extern int fb_fd;
extern unsigned char* fb_mem;
extern int screen_width;
extern int screen_height;
extern int screen_bpp;
extern int line_length;


int fb_init(void);
void fb_release(void);
//全局信号量
sem_t g_sem;


typedef struct {
    char url[512];
    char save_path[512];
} DownloadTask;

typedef struct {
    char full_path[512]; // 视频路径
    int play_seconds;     // 播放时长
} PlayParam;

#endif