#include "pro.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <math.h>

unsigned int pen_color = 0xFFFF0000;
unsigned int bg_color = 0xFFFFFFFF;
int last_x = -1, last_y = -1;
int touch_fd = -1;

// 坐标转换
void convert_touch_to_screen(int tx, int ty, int* sx, int* sy) {
    *sx = (tx * screen_width) / 1024;
    *sy = (ty * screen_height) / 600;
    if (*sx < 0) *sx = 0;
    if (*sx >= screen_width) *sx = screen_width - 1;
    if (*sy < 0) *sy = 0;
    if (*sy >= screen_height) *sy = screen_height - 1;
}

// 画点函数
void draw_point(int x, int y, unsigned int color) {
    if (fb_mem == NULL || fb_mem == MAP_FAILED) return;

    int radius = 10;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int nx = x + dx;
                int ny = y + dy;
                if (nx < 0 || nx >= screen_width || ny < 0 || ny >= screen_height)
                    continue;
                unsigned int* pixel = (unsigned int*)(fb_mem + ny * line_length + nx * 4);
                *pixel = color;
            }
        }
    }
}

// 清屏函数
void clear_screen() {
    if (fb_mem == NULL || fb_mem == MAP_FAILED) {
        printf("错误：fb_mem未初始化！\n");
        return;
    }
    for (int y = 0; y < screen_height; y++) {
        unsigned int* line = (unsigned int*)(fb_mem + y * line_length);
        for (int x = 0; x < screen_width; x++) {
            line[x] = bg_color;
            if (x < 64 || y < 100) {
                line[x] = 0xFF000000;
            }
        }
    }
    msync(fb_mem, screen_width * screen_height * 4, MS_SYNC);
    printf("清屏完成！\n");
}

int huaban_hansu() {
    clear_screen();
    tubiao_huaban();
    return 0;
}

int huaban_main() {
    printf("进入画板程序...\n");
    if (fb_init() < 0) {
        printf("帧缓冲初始化失败！\n");
        return -1;
    }
    huaban_hansu();
    struct input_event buf;
    int tx = 0, ty = 0;
    int sx = 0, sy = 0;
    int flag = 0;
    touch_fd = open("/dev/input/event0", O_RDONLY);
    if (touch_fd < 0) {
        printf("打开触摸设备失败！\n");
        fb_release();
        return -1;
    }

    while (1) {
        read(touch_fd, &buf, sizeof(buf));

        if (buf.type == EV_ABS) {
            if (buf.code == ABS_X) tx = buf.value;
            if (buf.code == ABS_Y) ty = buf.value;
        }

        if (buf.type == EV_KEY && buf.code == BTN_TOUCH) {
            flag = buf.value;
        }

        if (flag == 1) {
            convert_touch_to_screen(tx, ty, &sx, &sy);
            if (sx > 0 && sx < 64 && sy > 352 && sy < 416) {
                printf("点击了清除按钮\n");
                clear_screen();
                tubiao_huaban();
                last_x = last_y = -1;
                continue;

            }
            if (sx > 0 && sx < 64 && sy > 288 && sy < 352) {
                printf("点击了红色画笔\n");
                pen_color = 0xFFFF0000;
                last_x = last_y = -1;
                continue;
            }
            if (sx > 0 && sx < 64 && sy > 224 && sy < 288) {

                printf("点击了蓝色画笔\n");
                pen_color = 0xFF0000FF;
                last_x = last_y = -1;
                continue;
            }
            if (sx > 0 && sx < 64 && sy > 160 && sy < 224) {

                printf("点击了黑色画笔\n");
                pen_color = 0xFF000000;
                last_x = last_y = -1;
                continue;
            }
            if (sx > 0 && sx < 64 && sy > 416 && sy < 480) {
                printf("点击了退出，执行应用程序...\n");
                break;
            }

            if (sx >= 64 && sy >= 64) {
                printf("触摸坐标：原始(%d,%d) → 屏幕(%d,%d)\n", tx, ty, sx, sy);
                draw_point(sx, sy, pen_color);
                msync(fb_mem, screen_width * screen_height * 4, MS_SYNC);
            }
            else {
                last_x = last_y = -1;
            }
        }
        else {
            last_x = last_y = -1;
        }
    }

    close(touch_fd);
    fb_release();
    return 0;
}