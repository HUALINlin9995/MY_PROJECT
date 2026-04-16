#include "pro.h"

static void json_escape(const char* src, char* dst, size_t dst_size) {
    size_t i = 0, j = 0;
    while (src[i] && j < dst_size - 1) {
        switch (src[i]) {
        case '"':  dst[j++] = '\\'; dst[j++] = '"'; break;
        case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
        case '\n': dst[j++] = '\\'; dst[j++] = 'n'; break;
        case '\r': dst[j++] = '\\'; dst[j++] = 'r'; break;
        case '\t': dst[j++] = '\\'; dst[j++] = 't'; break;
        default:   dst[j++] = src[i]; break;
        }
        i++;
    }
    dst[j] = '\0';
}

void http_send(const char* ip, int port, const char* data)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket创建失败");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("IP地址无效");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("连接服务器失败");
        close(sock);
        return;
    }

    char escaped_data[BUF_SIZE] = { 0 };
    json_escape(data, escaped_data, sizeof(escaped_data));

    char http[BUF_SIZE + 512] = { 0 };
    snprintf(http, sizeof(http),
        "POST /api/output_cmd/ HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "{\"data\":\"%s\"}",
        ip, port, strlen("{\"data\":\"\"}") + strlen(escaped_data), escaped_data);

    send(sock, http, strlen(http), 0);
    close(sock);
    printf("命令输出已发送到服务器\n");
}

int cmd_main(const char* command)
{
    char m[256];
    strncpy(m, command, sizeof(m) - 1);
    m[sizeof(m) - 1] = '\0';
    m[strcspn(m, "\n")] = '\0';

    if (strncmp(m, "mplayer", 7) == 0)
    {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork 失败");
            http_send(SERVER_IP, PORT, "mplayer 启动失败");
            return -1;
        }
        else if (pid == 0) {
            int fd = open("/dev/null", O_RDWR);
            if (fd >= 0) {
                dup2(fd, STDOUT_FILENO);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
            // 执行mplayer，脱离主进程
            execl("/bin/sh", "sh", "-c", m, (char*)NULL);
            _exit(127); // execl失败直接退出
        }
        http_send(SERVER_IP, PORT, "成功播放");
        return 0;
    }

    // 处理cd命令
    if (strncmp(m, "cd", 2) == 0) {
        char* path = m + 3;
        if (path[0] == '\0') {
            path = "/";
        }
        if (chdir(path) == -1) {
            perror("cd 失败");
            http_send(SERVER_IP, PORT, "cd 失败");
        }
        else {
            char msg[256];
            snprintf(msg, sizeof(msg), "成功切换目录到: %s\n", path);
            printf("%s", msg);
            http_send(SERVER_IP, PORT, msg);
        }
        return 0;
    }

    // 执行普通命令
    FILE* fp = popen(m, "r");
    if (fp == NULL) {
        perror("执行命令失败");
        http_send(SERVER_IP, PORT, "执行命令失败");
        return -1;
    }

    char result[BUF_SIZE] = { 0 };
    char buf[1024];
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        strncat(result, buf, sizeof(result) - strlen(result) - 1);
        printf("%s", buf);
    }

    pclose(fp);
    http_send(SERVER_IP, PORT, result);
    return 0;
}