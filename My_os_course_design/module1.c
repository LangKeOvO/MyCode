#include <stdio.h>
#include <stdlib.h>     // 标准库(exit)
#include <unistd.h>     // Unix标准函数(fork, usleep)
#include <sys/types.h> // Unix标准类型(pid_t)
#include <sys/wait.h>  // Unix标准函数(wait)

int main() {
    pid_t pid1, pid2; // 用来存储子进程的PID

    // 创建第一个子进程
    pid1 = fork();
    if (pid1 < 0) {
        // 创建失败
        perror("fork1 failed");
        exit(1);
    } else if (pid1 == 0) {
        // 子进程1执行的代码
        // 循环打印字符'b' 30次
        for (int i = 0; i < 30; ++i) {
            putchar('b');
            fflush(stdout); // 刷新输出缓冲区，确保字符立即输出
            usleep(100000); // 睡眠0.1秒，防止输出太快，方便观察
        }
        exit (0); // 结束子进程1
    }

    // 创建第二个子进程
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork2 failed");
        exit (1);
    } else if (pid2 == 0) {
        // 子进程2执行的代码
        // 循环打印字符'c' 30次
        for (int i = 0; i < 30; ++i) {
            putchar('c');
            fflush(stdout);
            usleep(100000);
        }
        exit (0); // 结束子进程2
    }

    // 父进程执行的代码
    // 循环打印字符'a' 30次
    for (int i = 0; i < 30; ++i) {
        putchar('a');
        fflush(stdout);
        usleep(100000);
    }

    // 等待子进程结束,防止父进程提前结束
    wait(NULL);
    wait(NULL);

    putchar('\n');
    printf("=== 模块1 运行完毕 ===\n");

    return 0;
}