#include "pro.h"


//获取到服务器发来的msg
void get_msg(const char* json, char* msg) {
    msg[0] = '\0';
    char* start = strstr(json, "\"msg\": \"");
    if (start == NULL) { msg[0] = 0; return; }
    start += 8; // 跳过 "msg":"
    char* end = strstr(start, "\"");
    if (!end)return;
    int len = end - start;
    if (len > 0 && len < 63)
    {
        strncpy(msg, start, len);
        msg[len] = 0;
    }
    else
        msg[0] = 0; // 如果长度不合法，返回空字符串
}

//获取服务器发来的url
void get_url(const char* json, char* url) {
    url[0] = '\0';
    char* start = strstr(json, "\"url\": \"");
    if (start == NULL) { url[0] = 0; return; }
    start += 8; // 跳过 "url":"
    char* end = strstr(start, "\"");
    if (!end)return;
    int len = end - start;
    if (len > 0 && len < 511) {
        strncpy(url, start, len);
        url[len] = '\0';
    }
}

//获取服务器发来的时间参数
void get_time(const char* json, char* time) {
    time[0] = '\0';
    char* start = strstr(json, "\"time\": ");
    if (start == NULL) {
        time[0] = '\0';
        return;
    }
    start += 8;
    char* end = start;
    while (*end >= '0' && *end <= '9') {
        end++; 
    }
    int len = end - start;
    if (len > 0 && len < 32) { 
        strncpy(time, start, len);
        time[len] = '\0';
    }
    else
        time[0] = '\0';
}

//获取服务器发来的名字列表
int get_name_array(const char* json, char names[][256], int max_count)
{
    int count = 0;
    char* start = strstr(json, "\"name\": [");
    if (!start) return 0;
    start = strchr(start, '[');
    char* end_bracket = strchr(start, ']');
    if (!start) return 0;
    while (count < max_count)
    {
        start = strchr(start, '"');
        if (!start || start >= end_bracket) break;

        char* end = strchr(start + 1, '"');
        if (!end || end >= end_bracket) break;

        int len = end - start - 1;
        if (len > 0 && len < 255)
        {
            strncpy(names[count], start + 1, len);
            names[count][len] = '\0';
            count++;
        }
        start = end + 1;
    }
    return count;
}

void download_file(const char* url, const char* save_path)
{
    if (!url || !save_path) return;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket创建失败: %s\n", strerror(errno));
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("连接服务器失败: %s\n", strerror(errno));
        close(sock);
        return;
    }

    char path[512] = { 0 };
    char* path_start = strstr(url, "://");
    if (path_start) {
        path_start += 3;
        path_start = strchr(path_start, '/');
        if (path_start) {
            strncpy(path, path_start + 1, sizeof(path) - 1);
            char* quote = strchr(path, '"');
            if (quote) *quote = '\0';
        }
    }
    if (path[0] == '\0') {
        printf("URL解析失败: %s\n", url);
        close(sock);
        return;
    }

    char req[1024];
    snprintf(req, sizeof(req),
        "GET /%s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Connection: close\r\n\r\n",
        path, SERVER_IP, PORT);

    if (send(sock, req, strlen(req), 0) < 0) {
        printf("发送请求失败: %s\n", strerror(errno));
        close(sock);
        return;
    }

    char* buffer = (char*)malloc(MAX_FILE_SIZE);
    if (!buffer) {
        printf("malloc失败: %s\n", strerror(errno));
        close(sock);
        return;
    }
    memset(buffer, 0, MAX_FILE_SIZE);

    int total = 0;
    ssize_t n;
    while ((n = recv(sock, buffer + total, MAX_FILE_SIZE - total - 1, 0)) > 0) {
        total += n;
    }
    if (n < 0) {
        printf("接收数据失败: %s\n", strerror(errno));
        free(buffer);
        close(sock);
        return;
    }

    char* body = strstr(buffer, "\r\n\r\n");
    if (!body) {
        printf("未找到HTTP头分隔符\n");
        free(buffer);
        close(sock);
        return;
    }
    body += 4;
    int body_len = total - (body - buffer);

    FILE* fp = fopen(save_path, "wb");
    if (!fp) {
        printf("文件打开失败: %s, %s\n", save_path, strerror(errno));
        free(buffer);
        close(sock);
        return;
    }
    fwrite(body, 1, body_len, fp);
    fclose(fp);

    free(buffer);
    close(sock);
    printf("下载完成！保存到：%s\n", save_path);
}

//线程执行函数
void* download_thread(void* arg) {
    DownloadTask* task = (DownloadTask*)arg;
    download_file(task->url, task->save_path);
    free(task);
    sem_post(&g_sem);
    return NULL;
}

int submit_download_task(const char* url, const char* save_path)
{
    // 信号量限制最大3线程
    sem_wait(&g_sem);
    // 分配任务
    DownloadTask* task = (DownloadTask*)malloc(sizeof(DownloadTask));
    if (!task) {
        sem_post(&g_sem);
        return -1;
    }
    strcpy(task->url, url);
    strcpy(task->save_path, save_path);
    // 创建线程
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, download_thread, task);
    if (ret != 0) {
        free(task);
        sem_post(&g_sem);
        return -1;
    }
    // 分离线程
    pthread_detach(tid);
    return 0;
}

//删除数据
void delete_files(const char* base_path, char names[][256], int count)
{
    char file_path[512];
    for (int i = 0; i < count; i++)
    {
        // 拼接完整路径
        snprintf(file_path, sizeof(file_path), "%s%s", base_path, names[i]);
        // 删除文件
        if (remove(file_path) == 0)
        {
            printf("删除成功：%s\n", file_path);
        }
        else
        {
            printf("删除失败（不存在或无权限）：%s\n", file_path);
        }
    }
}



int socket_main()
{
    sem_init(&g_sem, 0, MAX_THREADS);
    while (1) {
        char msg[64] = { 0 };
        char url[512] = { 0 };
		char time[64] = { 0 };
        char names[10][256] = { 0 };
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            printf("连接服务器失败\n");
            close(sockfd);
            usleep(500000);
            continue;
        }
        char http_request[2048];
        snprintf(http_request, sizeof(http_request),
            "GET /Temporary_message HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",
            SERVER_IP);

        send(sockfd, http_request, strlen(http_request), 0);
        shutdown(sockfd, SHUT_WR);

        char buffer[4096] = { 0 };
        ssize_t total = 0;
        while (total < sizeof(buffer) - 1) {
            ssize_t n = read(sockfd, buffer + total, sizeof(buffer) - 1 - total);
            if (n <= 0) break;
            total += n;
        }
        buffer[total] = '\0';

        char* body_start = strstr(buffer, "\r\n\r\n");
        if (body_start != NULL) {
            body_start += 4;
        }
        else {
            // 没找到分隔符，说明响应异常，直接判定为无消息
            close(sockfd);
            sleep(5);
            continue;
        }
        int i = 0;
        if (strstr(body_start, "\"msg\": \"\"") != NULL) {
            i = 1;
        }
        else {
            get_msg(body_start, msg);
            get_url(body_start, url);
            printf("--------------------------------\n");
            printf("收到数据：%s\n", body_start);
            if (strstr(body_start, "add_img") != NULL)
            {
                printf("收到指令:%s\n", msg);
                printf("收到URL:%s\n", url);

                char filename[256] = { 0 };
                char* p = strrchr(url, '/');
                if (p != NULL)
                    strcpy(filename, p + 1);

                char save_path[512];
                sprintf(save_path, "%s%s", IMG_SAVE_PATH, filename);

                submit_download_task(url, save_path);
                printf("图片已加入下载队列\n");
            }
            if (strstr(body_start, "add_movie") != NULL)
            {

                printf("收到指令:%s\n", msg);
                printf("收到URL:%s\n", url);

                char filename[256] = { 0 };
                char* p = strrchr(url, '/');
                if (p != NULL)
                    strcpy(filename, p + 1);

                char save_path[512];
                sprintf(save_path, "%s/%s", MOVIE_SAVE_PATH, filename);

                submit_download_task(url, save_path);
                printf("视频已加入下载队列\n");

            }
            if (strstr(body_start, "add_audio") != NULL)
            {

                printf("收到指令:%s\n", msg);
                printf("收到URL:%s\n", url);

                char filename[256] = { 0 };
                char* p = strrchr(url, '/');
                if (p != NULL)
                    strcpy(filename, p + 1);

                char save_path[512];
                sprintf(save_path, "%s/%s", AUDIO_SAVE_PATH, filename);

                submit_download_task(url, save_path);
                printf("音频已加入下载队列\n");

            }
            if (strstr(body_start, "del_img") != NULL)
            {
                int name_count = get_name_array(body_start, names, 10);
                printf("收到指令:%s\n", msg);
                printf("收到名字列表:\n");
                for (int i = 0; i < name_count; i++) {
                    printf("  - %s\n", names[i]);
                }
                delete_files(IMG_SAVE_PATH, names, name_count);
                printf("收到删除图片消息\n");
            }
            if (strstr(body_start, "del_movie") != NULL)
            {
                int name_count = get_name_array(body_start, names, 10);
                printf("收到指令:%s\n", msg);
                printf("收到名字列表:\n");
                for (int i = 0; i < name_count; i++) {
                    printf("  - %s\n", names[i]);
                }
                delete_files(MOVIE_SAVE_PATH, names, name_count);
                printf("收到删除视频消息\n");
            }
            if (strstr(body_start, "del_audio") != NULL)
            {
                int name_count = get_name_array(body_start, names, 10);
                printf("收到指令:%s\n", msg);
                printf("收到名字列表:\n");
                for (int i = 0; i < name_count; i++) {
                    printf("  - %s\n", names[i]);
                }
                delete_files(AUDIO_SAVE_PATH, names, name_count);
                printf("收到删除音频消息\n");
            }
            printf("--------------------------------\n");
            if (strstr(body_start, "\"msg\": \"cmd\"") != NULL)
            {
                printf("收到指令:%s\n", url);
                cmd_main(url);
            }
            if (strstr(body_start, "\"msg\": \"set_wallpaper\"") != NULL)
            {
                printf("收到设置壁纸指令:%s\n", url);
                WALLPAPER_NAME = url;
                wallpaper_main(WALLPAPER_NAME);
                tubiao_main();

            }
            if (strstr(body_start, "\"msg\": \"set_movie\"") != NULL)
            {
                char full_path[512];
                char cmd[256];
                snprintf(full_path, sizeof(full_path), "%s%s", MOVIE_SAVE_PATH, url);
                printf("收到设置视频指令:%s\n", url);
                if (strstr(body_start, "\"mode\": \"once\"") != NULL)
                {
                    sprintf(cmd, "mplayer %s", full_path);
                    printf("执行命令:%s\n", cmd);
                    system(cmd);
                }
                else if (strstr(body_start, "\"mode\": \"hours\"") != NULL)
                {
					get_time(body_start, time);
                    int minute = atoi(time);
                    int play_second = minute * 60;
                    if (play_second <= 0) {
                        printf("时间解析异常，使用默认30秒播放时长\n");
                        play_second = 30;
                    }
                    printf("播放视频:%s,时间:%d\n", url,play_second);
                    movie_main(full_path,play_second);
                }

            }
            if (strstr(body_start, "\"msg\": \"set_audio\"") != NULL)
            {
                char full_path[512];
                char cmd[256];
                snprintf(full_path, sizeof(full_path), "%s%s", AUDIO_SAVE_PATH, url);
                sprintf(cmd, "madplay %s", full_path);
                printf("收到设置音频指令:%s\n", cmd);
                system(cmd);

            }
        }


        close(sockfd);
        sleep(3);
    }
    return 0;
}