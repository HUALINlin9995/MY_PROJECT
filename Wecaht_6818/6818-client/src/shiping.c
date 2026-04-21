#include "pro.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char file_list[MAX_FILES][FILE_NAME_LEN];
extern int file_count;

int shiping_main()
{
    get_all_files(MOVIE_SAVE_PATH);
    if (file_count <= 0) {
        printf("没有找到视频文件\n");
        return -1;
    }

    int i = 0;
    int x, y;
    char cmd[512];
    char full_path[FILE_NAME_LEN];

    fb_release();
    printf("已释放帧缓冲，准备播放视频\n");

    snprintf(full_path, sizeof(full_path), "%s/%s", MOVIE_SAVE_PATH, file_list[i]);
    printf("开始播放：%s（无限循环）\n", full_path);
    snprintf(cmd, sizeof(cmd), "TERM=vt100 mplayer -loop 0 -really-quiet -noconsolecontrols %s &", full_path);
    system(cmd);

    while (1)
    {
        // 等待点击
        get_pos(&x, &y);
        printf("点击坐标：%d, %d\n", x, y);

        // 退出：点击左下角区域
        if (x > 0 && x < 164 && y > 440 && y < 800)
        {
            system("killall mplayer");
            printf("退出视频播放\n");
            break;
        }

        // 切换下一个视频
        printf("切换下一个视频...\n");
        i++;
        if (i >= file_count)
            i = 0;

        system("killall mplayer");

        snprintf(full_path, sizeof(full_path), "%s/%s", MOVIE_SAVE_PATH, file_list[i]);
        printf("切换播放：%s（无限循环）\n", full_path);
        snprintf(cmd, sizeof(cmd), "TERM=vt100 mplayer -loop 0 -really-quiet -noconsolecontrols %s &", full_path);
        system(cmd);
    }

    fb_release(); 
    usleep(500000);
    if (fb_init() < 0) {
        printf("重新初始化帧缓冲失败！尝试再次初始化...\n");
        usleep(100000);
        if (fb_init() < 0) {
            printf("帧缓冲初始化失败！\n");
        }
        else {
            printf("帧缓冲重新初始化成功！\n");
        }
    }
    else {
        printf("帧缓冲重新初始化成功！\n");
    }

    return 0;
}