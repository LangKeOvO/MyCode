// jincheng3.c
// gcc jincheng3.c -o jincheng3
// ./jincheng3
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

pid_t child1 = 0, child2 = 0;

// 子进程1 的 SIGUSR1 处理函数
void child1_handler(int signum) {
    if (signum == SIGUSR1) {
        printf("child process1 is killed by parent!\n");
        fflush(stdout); // 确保输出被刷新
        exit(0); // 终止进程
    }
}

// 子进程2 的 SIGUSR1 处理函数
void child2_handler(int signum) {
    if (signum == SIGUSR1) {
        printf("child process2 is killed by parent!\n");
        fflush(stdout); // 确保输出被刷新
        exit(0); // 终止进程
    }
}

// 父进程的键盘中断（软中断）处理函数
void parent_handler(int signum) {
    if (signum == SIGINT || signum == SIGQUIT) {
        printf("Parent received keyboard interrupt signal\n");
        fflush(stdout);
        if (child1 > 0) kill(child1, SIGUSR1);
        if (child2 > 0) kill(child2, SIGUSR1);
    }
}

int main() {
    // 创建第一个子进程
    child1 = fork();
    if (child1 < 0) {
        perror("fork1 failed");
        exit(1);
    }
    if (child1 == 0) {
        // 子进程1：忽略 SIGINT/SIGQUIT，只处理 SIGUSR1
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGUSR1, child1_handler);
        setbuf(stdout, NULL);
        while (1) pause();
    }

    // 父进程才会执行到这里，再创建第二个子进程
    child2 = fork();
    if (child2 < 0) {
        perror("fork2 failed");
        exit(1);
    }
    if (child2 == 0) {
        // 子进程2：忽略 SIGINT/SIGQUIT，只处理 SIGUSR1
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGUSR1, child2_handler);
        setbuf(stdout, NULL);
        while (1) pause();
    }

    // 父进程：注册对 Ctrl+C(SIGINT) 和 Ctrl+\(SIGQUIT) 的处理
    signal(SIGINT, parent_handler);
    signal(SIGQUIT, parent_handler);

    // 父进程等待两个子进程退出
    wait(NULL); // 等待并回收 child1 或 child2 中的一个
    wait(NULL); // 再次等待并回收剩下的那个

    printf("parent process is killed!\n");
    return 0;
}