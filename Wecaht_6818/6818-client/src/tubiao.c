#include "pro.h"

//// 帧缓冲全局变量
//static int fb_fd = -1;
//static unsigned char* fb_mem = NULL;  // 帧缓冲内存映射地址
//static int screen_width = 0;          // 屏幕宽度 800
//static int screen_height = 0;         // 屏幕高度 480
//static int screen_bpp = 0;            // 像素位深 32位
//static int line_length = 0;           // 屏幕每行字节数

int fb_fd = -1;
unsigned char* fb_mem = NULL;
int screen_width = 0;
int screen_height = 0;
int screen_bpp = 0;
int line_length = 0;

// BMP文件头结构体
#pragma pack(1) // 取消结构体对齐，保证读取正确
typedef struct {
    unsigned short bfType;      // 文件类型
    unsigned int bfSize;        // 文件总大小
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;     // 像素数据的偏移量
} BMP_FILE_HEADER;

typedef struct {
    unsigned int biSize;          // 本结构体大小
    int biWidth;                   // 图片宽度
    int biHeight;                  // 图片高度
    unsigned short biPlanes;       // 必须为1
    unsigned short biBitCount;     // 位深
    unsigned int biCompression;    // 压缩方式，0=无压缩
    unsigned int biSizeImage;      // 像素数据大小
    int biXPelsPerMeter;           // 水平分辨率
    int biYPelsPerMeter;           // 垂直分辨率
    unsigned int biClrUsed;        // 使用的颜色数
    unsigned int biClrImportant;   // 重要颜色数
} BMP_INFO_HEADER;
#pragma pack()


int fb_init(void)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    // 打开帧缓冲设备
    fb_fd = open("/dev/fb0", O_RDWR);
    if (fb_fd < 0) {
        perror("open /dev/fb0 failed");
        return -1;
    }

    // 获取屏幕可变参数
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("ioctl FBIOGET_VSCREENINFO failed");
        close(fb_fd);
        return -1;
    }

    // 获取屏幕固定参数
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("ioctl FBIOGET_FSCREENINFO failed");
        close(fb_fd);
        return -1;
    }

    // 赋值屏幕参数
    screen_width = vinfo.xres;
    screen_height = vinfo.yres;
    screen_bpp = vinfo.bits_per_pixel;
    line_length = finfo.line_length;

    // 校验：GEC6818默认是32位色，不是的话报错
    if (screen_bpp != 32) {
        printf("屏幕位深不是32位，当前位深：%d\n", screen_bpp);
        close(fb_fd);
        return -1;
    }

    // 内存映射：把帧缓冲映射到用户空间，直接操作内存即可修改屏幕像素
    fb_mem = mmap(NULL, line_length * screen_height, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fb_mem == MAP_FAILED) {
        perror("mmap failed");
        close(fb_fd);
        return -1;
    }
    printf("帧缓冲初始化成功：%d×%d，%d位色\n", screen_width, screen_height, screen_bpp);
    return 0;
}

int draw_bmp(const char* bmp_path, int x0, int y0)
{
    BMP_FILE_HEADER file_header;
    BMP_INFO_HEADER info_header;
    FILE* fp = NULL;
    unsigned char* bmp_data = NULL;
    int img_width, img_height;
    int line_byte; // BMP每行字节数

    // 打开BMP文件
    fp = fopen(bmp_path, "rb");
    if (fp == NULL) {
        perror("fopen bmp failed");
        return -1;
    }

    // 读取文件头
    if (fread(&file_header, sizeof(BMP_FILE_HEADER), 1, fp) != 1) {
        printf("读取BMP文件头失败\n");
        fclose(fp);
        return -1;
    }

    if (file_header.bfType != 0x4D42) {
        printf("不是标准BMP文件\n");
        fclose(fp);
        return -1;
    }

    // 读取信息头
    if (fread(&info_header, sizeof(BMP_INFO_HEADER), 1, fp) != 1) {
        printf("读取BMP信息头失败\n");
        fclose(fp);
        return -1;
    }

    if (info_header.biBitCount != 24 || info_header.biCompression != 0) {
        printf("只支持24位无压缩BMP，当前位深：%d\n", info_header.biBitCount);
        fclose(fp);
        return -1;
    }

    img_width = info_header.biWidth;
    img_height = info_header.biHeight;
    // BMP每行字节数必须是4的倍数，不足补0
    line_byte = (img_width * 3 + 3) / 4 * 4;

    // 校验坐标是否超出屏幕
    if (x0 + img_width > screen_width || y0 + img_height > screen_height) {
        printf("图标超出屏幕范围\n");
        fclose(fp);
        return -1;
    }

    // 分配内存存储BMP像素数据
    bmp_data = (unsigned char*)malloc(line_byte * img_height);
    if (bmp_data == NULL) {
        perror("malloc failed");
        fclose(fp);
        return -1;
    }

    // 跳转到像素数据的位置
    fseek(fp, file_header.bfOffBits, SEEK_SET);
    // 读取所有像素数据
    if (fread(bmp_data, line_byte * img_height, 1, fp) != 1) {
        printf("读取BMP像素数据失败\n");
        free(bmp_data);
        fclose(fp);
        return -1;
    }

    for (int y = 0; y < img_height; y++) {
        unsigned char* bmp_line = bmp_data + (img_height - 1 - y) * line_byte;
        unsigned char* fb_line = fb_mem + (y0 + y) * line_length;

        for (int x = 0; x < img_width; x++) {
            // BMP像素格式：B G R（24位）
            unsigned char b = bmp_line[x * 3 + 0];
            unsigned char g = bmp_line[x * 3 + 1];
            unsigned char r = bmp_line[x * 3 + 2];
            // 帧缓冲格式：B G R A（32位，A=0xFF不透明）
            int fb_offset = (x0 + x) * 4;
            fb_line[fb_offset + 0] = b;
            fb_line[fb_offset + 1] = g;
            fb_line[fb_offset + 2] = r;
            fb_line[fb_offset + 3] = 0xFF; // 透明度，不透明
        }
    }

    printf("绘制成功：%s 到坐标(%d,%d)\n", bmp_path, x0, y0);

    free(bmp_data);
    fclose(fp);
    return 0;
}

void fb_release(void)
{
    if (fb_mem != NULL) {
        munmap(fb_mem, line_length * screen_height);
        fb_mem = NULL;
    }
    if (fb_fd >= 0) {
        close(fb_fd);
        fb_fd = -1;
    }
}

void tubiao_huaban()
{
    draw_bmp("/ceshi/tubiao/exit.bmp", 0, 416);
    printf("exit.bmp绘制完成\n");
    draw_bmp("/ceshi/tubiao/qingchu.bmp", 0, 352);
    printf("qingchu.bmp绘制完成\n");
    draw_bmp("/ceshi/tubiao/red.bmp", 0, 288);
    printf("red.bmp绘制完成\n");
    draw_bmp("/ceshi/tubiao/blue.bmp", 0, 224);
    printf("blue.bmp绘制完成\n");
    draw_bmp("/ceshi/tubiao/while.bmp", 0, 160);
    printf("所有图标绘制完成，进入触摸循环...\n");
}

int tubiao_main(void)
{
    if (fb_init() < 0) {
        return -1;
    }

    draw_bmp("/ceshi/tubiao/shiping.bmp", 0, 50);
    draw_bmp("/ceshi/tubiao/tupian.bmp", 128, 50);
    draw_bmp("/ceshi/tubiao/huatu.bmp", 256, 50);
    draw_bmp("/ceshi/tubiao/guanji.bmp", 736, 416);
    draw_bmp("/ceshi/tubiao/chongqi.bmp", 736, 352);
    /*fb_release();*/
    return 0;
}