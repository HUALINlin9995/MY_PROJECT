#include "pro.h"

void* download_pthread(void* arg)
{
    PlayParam* param = (PlayParam*)arg;

    //从结构体里取出参数
    const char* full_path = param->full_path;
    int play_seconds = param->play_seconds;

    char* mplayer_argv[] = {
        "mplayer",
        "-really-quiet",
        "-noconsolecontrols",
        "-loop",
        "0",
        (char*)full_path,
        NULL
    };

    pid_t mplayer_pid = fork();
    if (mplayer_pid == 0) {
        execvp("mplayer", mplayer_argv);
        perror("execvp mplayer failed");
        _exit(1);
    }
    else if (mplayer_pid < 0) {
        perror("fork mplayer failed");
        free(param);
        return NULL;
    }

    printf("准备播放 %d 秒\n", play_seconds);
    sleep(play_seconds);
    sleep(1);
    wallpaper_main(WALLPAPER_NAME);
    sleep(0.2);
    tubiao_main();
    printf("%d秒到，准备关闭mplayer\n", play_seconds);
    kill(mplayer_pid, SIGKILL);
    waitpid(mplayer_pid, NULL, 0);
    free(param);
    return NULL;
}

int movie_main(const char* full_path, int play_seconds)
{
    PlayParam* param = (PlayParam*)malloc(sizeof(PlayParam));
    if (param == NULL) {
        perror("malloc PlayParam failed");
        return -1;
    }

    strncpy(param->full_path, full_path, sizeof(param->full_path) - 1); 
    param->full_path[sizeof(param->full_path) - 1] = '\0'; 
    param->play_seconds = play_seconds;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, download_pthread, (void*)param);
    if (ret != 0) {
        perror("pthread_create failed");
        free(param);
        return -1;
    }

    pthread_detach(tid);
    return 0;
}