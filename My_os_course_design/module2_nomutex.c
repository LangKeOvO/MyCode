//module2_nomutex.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// 子进程或父进程运行的任务函数
void child_task(const char* who) {
    // 输出5次， 表示进程正在运行
    for (int i = 0; i < 5; ++i) {
        // printf("%s is running. (iteration %d)\n", who, i + 1);
        // usleep(100000); //睡眠0.1秒， 模拟“处理耗时”

        printf("%s is ", who);
        usleep(1000);
        printf("running. (iteration %d)\n", i + 1);
        usleep(100000);
    }
    exit(0); // 退出子进程
}

int main()
{
    pid_t pid1, pid2;

    pid1 = fork();
    if (pid1 < 0) {
        perror("fork1 failed");
        exit(1);
    } else if (pid1 == 0) {
        // 子进程1
        child_task("Child1");
    }

    pid2 = fork();
    if(pid2 < 0) {
        perror("fork2 failed");
        exit(1);
    } else if (pid2 == 0) {
        // 子进程2
        child_task("Child2");
    }

    // 父进程
    child_task("Parent");

    // 等待子进程结束
    wait(NULL);
    wait(NULL);

    printf("=== 模块2 无互斥运行完毕 ===\n");
    return 0;
}