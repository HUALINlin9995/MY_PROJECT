#include "pro.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>


char file_list[MAX_FILES][FILE_NAME_LEN];  // 存所有文件名
int file_count = 0;                       // 实际拿到的文件总数

int get_all_files(const char* dir_path)
{
    DIR* dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir failed");
        return -1;
    }

    struct dirent* entry;
    file_count = 0;  // 清空计数

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (file_count < MAX_FILES)
        {
            snprintf(file_list[file_count], FILE_NAME_LEN, "%s", entry->d_name);
            file_count++;
        }
    }

    closedir(dir);
    printf("共获取到 %d 个文件\n", file_count);
    return 0;
}

int tupian_main(void)
{
    if (fb_init() < 0) {
        return -1;
    }
    get_all_files(IMG_SAVE_PATH);
	printf("获取文件列表成功，准备显示...\n");
    if (file_count <= 0) {
        printf("没有找到图片文件\n");
        fb_release();
        return -1;
    }
    wallpaper_main(file_list[0]); // 显示第一个文件
    draw_bmp("/ceshi/tubiao/zuo.bmp", 0, 208);
    draw_bmp("/ceshi/tubiao/you.bmp", 736, 208);
    draw_bmp("/ceshi/tubiao/exit.bmp", 0, 416);
    draw_bmp("/ceshi/tubiao/shezhi.bmp", 368, 416);
	int i = 0;
    while (1)
    {
        int x, y;
		get_pos(&x, &y);
		printf("点击坐标：(%d, %d)\n", x, y);
        if (x > 942&&x<1024 && y > 260&&y<340)
        {
			if (i < file_count - 1)
			{
				i++;
			}
			else
			{
				i = 0; // 循环回第一个
			}
			wallpaper_main(file_count > 0 ? file_list[i] : ""); // 显示当前文件
            draw_bmp("/ceshi/tubiao/zuo.bmp", 0, 208);
            draw_bmp("/ceshi/tubiao/you.bmp", 736, 208);
            draw_bmp("/ceshi/tubiao/exit.bmp", 0, 416);
            draw_bmp("/ceshi/tubiao/shezhi.bmp", 368, 416);
        }
        if (x > 0 && x < 83 && y > 260 && y < 340)
        {
			if (i > 0)
			{
				i--;
			}
			else
			{
				i = file_count - 1; // 循环回最后一个
			}
            wallpaper_main(file_count > 0 ? file_list[i] : ""); // 显示当前文件
            draw_bmp("/ceshi/tubiao/zuo.bmp", 0, 208);
            draw_bmp("/ceshi/tubiao/you.bmp", 736, 208);
            draw_bmp("/ceshi/tubiao/exit.bmp", 0, 416);
            draw_bmp("/ceshi/tubiao/shezhi.bmp", 368, 416);
        }
        if (x > 0 && x < 83 && y > 520 && y < 601)
        {
			printf("点击了退出，执行应用程序...\n");
			break;
        }
        if (x > 472 && x < 553 && y > 520 && y < 601)
        {
			printf("点击了设置壁纸，执行应用程序...\n");
            WALLPAPER_NAME = file_list[i];
			printf("设置壁纸为：%s\n", WALLPAPER_NAME);
            draw_bmp("/ceshi/tubiao/cg.bmp", 336, 176);
            sleep(2);
            break;

        }
    }
    fb_release();
    return 0;
}