#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// 每次只打印一个字符的 printf 替代函数，模拟输出撕裂
void noisy_printf(const char* msg) {
    for (int i = 0; msg[i] != '\0'; ++i) {
        putchar(msg[i]);         // 一次输出一个字符
        fflush(stdout);          // 确保立即输出
        usleep(1000);            // 每个字符之间停1毫秒，制造并发竞争
    }
}

// 子进程或父进程运行的任务函数
void child_task(const char* who) {
    for (int i = 0; i < 5; ++i) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%s is printing line %d\n", who, i + 1);
        noisy_printf(buffer);  // 使用逐字符输出函数
        usleep(100000);        // 模拟逻辑处理耗时（0.1秒）
    }
    exit(0); // 子进程退出
}

int main() {
    pid_t pid1, pid2;

    // 创建第一个子进程
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork1 failed");
        exit(1);
    } else if (pid1 == 0) {
        // 子进程1
        child_task("Child1");
    }

    // 创建第二个子进程
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork2 failed");
        exit(1);
    } else if (pid2 == 0) {
        // 子进程2
        child_task("Child2");
    }

    // 父进程任务
    child_task("Parent");

    // 等待两个子进程结束
    wait(NULL);
    wait(NULL);

    printf("\n=== 所有进程结束 ===\n");
    return 0;
}
