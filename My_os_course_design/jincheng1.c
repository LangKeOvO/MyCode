// jinchen1.c
// gcc jincheng1.c -o jincheng1
// ./jincheng1
// 该代码演示了如何使用fork创建两个子进程，并在父进程和子进程中交替输出字符。

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    pid_t pid1, pid2;

    pid1 = fork();
    if (pid1 < 0) {
        perror("fork1 failed");
        exit(1);
    } else if (pid1 == 0) {
        // 子进程1
        while (1) {
            //srand(getpid()); // 每个进程设置不同随机种子
            printf("b");
            fflush(stdout);
            //usleep(100000 + rand() % 300000);
        }
    }

    pid2 = fork();
    if (pid2 < 0) {
        perror("fork2 failed");
        exit(1);
    } else if (pid2 == 0) {
        // 子进程2
        while (1) {
            //srand(getpid()); // 每个进程设置不同随机种子
            printf("a");
            fflush(stdout);
            //usleep(100000 + rand() % 300000);
        }
    }

    // 父进程
    while (1) {
        //srand(getpid()); // 每个进程设置不同随机种子
        printf("c");
        fflush(stdout);
        //usleep(100000 + rand() % 300000);
    }

    exit(0);
}

//🧪 运行效果与分析
//输出混乱：字符 a、b、c 会随机交错输出，说明三个进程在并发执行。
//非确定性调度：多次运行结果不同，是典型的进程调度不可预测性体现。