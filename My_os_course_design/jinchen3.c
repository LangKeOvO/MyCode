// jinchen3.c
// gcc jinchen3.c -o jinchen3
// ./jinchen3
// 该代码演示了如何使用 fork 创建一个子进程，并在父进程和子进程中使用信号进行通信。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

pid_t child1 = 0, child2 = 0;

// 子进程的信号处理函数
void child_handler(int sig) {
    if (sig == SIGUSR1) {
        printf("child process%d is killed by parent!\n", getpid() == child1 ? 1 : 2);
        fflush(stdout);
        exit(0);
    }
}

// 父进程的中断处理函数
void parent_handler(int sig) {
    if (sig == SIGINT || sig == SIGQUIT) {
        printf("Parent received keyboard interrupt signal\n");

        if (child1 > 0) kill(child1, SIGUSR1);
        if (child2 > 0) kill(child2, SIGUSR1);
    }
}

int main() {
    // 注册父进程的中断信号处理函数（如 Ctrl+C）
    signal(SIGINT, parent_handler);
    signal(SIGQUIT, parent_handler);

    // 创建子进程1
    child1 = fork();
    if (child1 < 0) {
        perror("fork1 failed");
        exit(1);
    } else if (child1 == 0) {
        // 子进程1注册信号处理函数并等待
        signal(SIGUSR1, child_handler);
        while (1) pause();
    }

    // 创建子进程2
    child2 = fork();
    if (child2 < 0) {
        perror("fork2 failed");
        exit(1);
    } else if (child2 == 0) {
        // 子进程2注册信号处理函数并等待
        signal(SIGUSR1, child_handler);
        while (1) pause();
    }

    // 父进程等待两个子进程退出
    wait(NULL);
    wait(NULL);

    printf("parent process is killed!\n");
    return 0;
}