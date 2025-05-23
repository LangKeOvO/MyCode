// test_POSIX_Lock.c
// 该代码演示了如何使用POSIX信号量来实现进程间的互斥锁。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h> // POSIX 信号量API
#include <sys/wait.h>
#include <sys/mman.h> // 内存映射，用于创建共享内存区域，方便多进程共享信号量

int main() {
    // 创建共享内存中的信号量
    sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    if (sem == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    // 初始化信号量，pshared = 1，表示跨进程共享
    sem_init(sem, 1, 1);

    pid_t pid = fork();
    if (pid == 0) {
        //子进程
        for (int i = 0; i < 5; ++i) {
            sem_wait(sem);
            printf("Child: %d\n", i);
            sem_post(sem);
            usleep(100000);
        }
        exit(0);
    } else {
        //父进程
        for (int i = 0; i < 5; ++i) {
            sem_wait(sem);
            printf("Parent: %d\n", i);
            sem_post(sem);
            usleep(100000);
        }
        wait(NULL); // 等待子进程结束

        // 清理资源
        sem_destroy(sem);
        munmap(sem, sizeof(sem_t));
    }
    exit(0);
}