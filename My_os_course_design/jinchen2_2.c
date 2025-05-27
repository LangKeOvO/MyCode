// jincheng2_2.c
// gcc jinchen2_2.c -o jincheng2_2
// ./jinchen2_2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

// 信号量指针（共享）
sem_t* sem = NULL;

// 输出整句（仍用 noisy_printf 逐字符输出以证明没穿插）
void noisy_printf(const char* msg) {
    for (int i = 0; msg[i] != '\0'; ++i) {
        putchar(msg[i]);
        fflush(stdout);
        usleep(1000);
    }
}

// 输出任务：先加锁 -> 输出 -> 解锁
void child_task(const char* who) {
    for (int i = 0; i < 5; ++i) {
        sem_wait(sem);  // 加锁

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%s is printing line %d\n", who, i + 1);
        noisy_printf(buffer);

        sem_post(sem);  // 解锁
        usleep(100000);
    }
    exit(0);
}

int main() {
    // 创建匿名共享内存中的信号量（适用于多进程）
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // 初始化信号量为 1，实现互斥
    if (sem_init(sem, 1, 1) < 0) {
        perror("sem_init failed");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork1 failed");
        exit(1);
    } else if (pid1 == 0) {
        child_task("Child1");
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork2 failed");
        exit(1);
    } else if (pid2 == 0) {
        child_task("Child2");
    }

    // 父进程也执行任务
    child_task("Parent");

    wait(NULL);
    wait(NULL);

    // 销毁信号量 + 解除映射
    sem_destroy(sem);
    munmap(sem, sizeof(sem_t));

    printf("\n=== 所有进程结束（互斥保护成功） ===\n");
    return 0;
}
