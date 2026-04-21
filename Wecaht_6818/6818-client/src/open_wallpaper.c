#include "pro.h"


int wallpaper_main(const char* filename)
{
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", IMG_SAVE_PATH, filename);
    int bpm_fd = open(full_path, O_RDWR);
    int lcd_fd = open("/dev/fb0", O_RDWR);
    if (lcd_fd < 0 || bpm_fd < 0)
    {
        printf("open lcd or bmp failed\n");
        return -1;
    }
    int* mp = mmap(NULL, 800 * 480 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, lcd_fd, 0);
    if (mp == NULL)
    {
        printf("mmap lcd failed\n");
        return -1;
    }
    // 读取图片数据
    char bpm_buf[800 * 480 * 3];
    lseek(bpm_fd, 54, SEEK_SET);
    read(bpm_fd, bpm_buf, sizeof(bpm_buf));
    int i = 0;
    for (int y = 479; y >= 0; y--)
    {
        for (int x = 0; x < 800; x++)
            *(mp + 800 * y + x) = bpm_buf[i++] | bpm_buf[i++] << 8 | bpm_buf[i++] << 16;
    }

    close(lcd_fd);
    close(bpm_fd);

    return 0;
}