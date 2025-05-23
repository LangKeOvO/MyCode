// test_System_V_Lock.c
// 该代码演示了如何使用System V信号量来实现进程间的互斥锁。
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>

// 添加：信号量联合体用于初始化信号量
union semun {
    int val;
};

// 添加：P操作（申请信号量）
void sem_wait(int semid) {
    struct sembuf sb = {0, -1, 0};
    semop(semid, &sb, 1);
}

// 添加：V操作（释放信号量）
void sem_signal(int semid) {
    struct sembuf sb = {0, 1, 0};
    semop(semid, &sb, 1);
}

// 每次只打印一个字符的 printf 替代函数，模拟输出撕裂
void noisy_printf(const char* msg) {
    for (int i = 0; msg[i] != '\0'; ++i) {
        putchar(msg[i]);
        fflush(stdout);
        usleep(1000);
    }
}

// 修改：增加参数semid,表示信号量
void child_task(const char* who, int semid) {
    for (int i = 0; i < 5; ++i) {
        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%s is printing line %d\n", who, i + 1);

        sem_wait(semid);      // P操作，加锁，进入临界区
        noisy_printf(buffer); // 临界区：打印输出
        sem_signal(semid);    // V操作，解锁，离开临界区

        usleep(100000);       // 模拟逻辑处理耗时（0.1秒）
    }
    exit(0); // 子进程退出
}

int main() {
    pid_t pid1, pid2;

    // 添加：创建一个System V信号量，初始值为1
    int semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        exit(1);
    }

    union semun sem_union;
    sem_union.val = 1; // 初始值为1
    if (semctl(semid, 0, SETVAL, sem_union) == -1) {
        perror("semctl init failed");
        exit(1);
    }

    // 创建第一个子进程
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork1 failed");
        exit(1);
    } else if (pid1 == 0) {
        // 子进程1
        child_task("Child1", semid); // 传入信号量ID
    }

    // 创建第二个子进程
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork2 failed");
        exit(1);
    } else if (pid2 == 0) {
        // 子进程2
        child_task("Child2", semid); // 传入信号量ID
    }

    // 父进程任务
    child_task("Parent", semid); // 传入信号量ID

    // 等待两个子进程结束
    wait(NULL);
    wait(NULL);

    // 清理信号量
    semctl(semid, 0, IPC_RMID);

    printf("\n=== 所有进程结束 ===\n");
    return 0;
}