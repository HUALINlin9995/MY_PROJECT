#include "pro.h"

int ts_fd = -1;
char* WALLPAPER_NAME = "Nikki.bmp";

void init()
{
    // 打开触摸屏文件
    ts_fd = open("/dev/input/event0", O_RDWR);
    if (ts_fd < 0)
    {
        printf("open ts failed\n");
        return;
    }
    return;
}

void get_pos(int* x, int* y)
{
    struct input_event buf;
    while (1)
    {
        read(ts_fd, &buf, sizeof(buf));
        if (buf.type == EV_ABS)
        {
            if (buf.code == ABS_X)
            {
                *x = buf.value;
            }
            if (buf.code == ABS_Y)
            {
                *y = buf.value;
            }
        }
        if (buf.type == EV_KEY && buf.code == BTN_TOUCH && buf.value == 0)
        {
            break;
        }

    }


}

int xitong_main(void)
{
	printf("开始执行系统程序...\n");
	printf("加载壁纸...\n");
	wallpaper_main(WALLPAPER_NAME);
	sleep(0.2);
	printf("加载图标...\n");
	tubiao_main();
    int x, y;
    init();
    while (1)
    {
        get_pos(&x, &y);
        printf("(%d,%d)\n", x, y);
        if (x > 0 && x < 163 && y>84 && y < 295)
        {
			printf("点击了视频，执行应用程序...\n");
            fb_release();
			shiping_main();
            fb_release();
            sleep(1);
			wallpaper_main(WALLPAPER_NAME);
			sleep(0.2);
			tubiao_main();
            
        }
        if (x > 163 && x < 327 && y>84 && y < 295)
        {
			printf("点击了图片，执行应用程序...\n");
            tupian_main();
            wallpaper_main(WALLPAPER_NAME);
			printf("现在壁纸为：%s\n", WALLPAPER_NAME);
            sleep(0.2);
            tubiao_main();
        }
        if (x > 327 && x < 491 && y>84 && y < 295)
        {
			printf("点击了画板，执行应用程序...\n");
			huaban_main();
            wallpaper_main(WALLPAPER_NAME);
            sleep(0.2);
            tubiao_main();
        }
        if (x > 943 && x < 1024 && y>520 && y < 600)
        {
         
			printf("点击了关机，执行应用程序...\n");
            system("poweroff");
        }
        if (x > 943 && x < 1024 && y>440 && y < 520)
        {
            printf("点击了重启\n");
            system("reboot");
        }
        else 
        {
            wallpaper_main(WALLPAPER_NAME);
            sleep(0.2);
            tubiao_main();
        }
    }
    close(ts_fd);
    return 0;
}