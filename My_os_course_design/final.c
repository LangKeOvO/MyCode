// integrated_demo.c
// 多进程综合实验：创建、控制、互斥与软中断通信演示
// 编译：gcc final.c -o final -lrt
// 运行：sudo ./final

#include <stdio.h>         // 标准输入输出函数，如 printf、scanf
#include <stdlib.h>        // exit、malloc 等
#include <unistd.h>        // fork、usleep 等
#include <sys/types.h>     // pid_t 类型
#include <sys/wait.h>      // wait 函数
#include <sys/resource.h>  // nice 设置优先级
#include <semaphore.h>     // POSIX 信号量 sem_t
#include <sys/mman.h>      // mmap/mumap 用于共享内存
#include <signal.h>        // signal/kill 等信号处理函数
#include <string.h>        // strlen 函数

#define ITERATIONS 1000     // 每个进程输出字符次数
#define SENTENCE_COUNT 5    // 每句打印多少次

// 子进程 PID 全局变量（用于模块3）
static pid_t child1 = 0, child2 = 0;

// 等待子进程 n 个
void wait_children(int n) {
    while (n-- > 0) wait(NULL);
}

// ============================ 模块 2.1/2.2 公共函数 ============================
// 字符级输出（不加锁），用于观察字符撕裂现象
void module2_1_print(const char* msg) {
    size_t len = strlen(msg);
    for (int i = 0; i < SENTENCE_COUNT; i++) {
        for (size_t j = 0; j < len; j++) {
            putchar(msg[j]); fflush(stdout);
            usleep(5000); // 微小延时，放大调度撕裂
        }
    }
}

// 加锁输出（使用信号量），避免字符交叉撕裂
void module2_2_print(const char* msg, sem_t* sem) {
    size_t len = strlen(msg);
    for (int i = 0; i < SENTENCE_COUNT; i++) {
        sem_wait(sem); // 请求锁
        for (size_t j = 0; j < len; j++) {
            putchar(msg[j]); fflush(stdout);
            usleep(5000);
        }
        sem_post(sem); // 释放锁
    }
}

// ============================ 模块 1.1：基本并发输出 ============================
// 三个进程并发输出 a、b、c 字符，各输出 ITERATIONS 次
void module1_1() {
    printf("\n=== 模块 1.1：并发输出单字符 ===\n");

    pid_t p1 = fork();   // 创建子进程1
    if (p1 == 0) {
        for (int i = 0; i < ITERATIONS; i++) { putchar('b'); fflush(stdout); }
        exit(0);
    }

    pid_t p2 = fork();   // 创建子进程2
    if (p2 == 0) {
        for (int i = 0; i < ITERATIONS; i++) { putchar('c'); fflush(stdout); }
        exit(0);
    }

    for (int i = 0; i < ITERATIONS; i++) { putchar('a'); fflush(stdout); }

    wait_children(2);
    printf("\n=== 模块 1.1 完成 ===\n");
    // 注：b 通常先执行因其先被 fork，a 会穿插在 b/c 之间，最终继续打印。
}

// ============================ 模块 1.2：控制调度优先级 ============================
// 使用 nice() 提高/降低子进程优先级，观察字符输出次序变化
void module1_2() {
    printf("\n=== 模块 1.2：优先级控制输出 ===\n");

    pid_t p1 = fork();   // 子进程1优先级降低
    if (p1 == 0) {
        nice(19); // 增加 nice 值，降低优先级
        for (int i = 0; i < ITERATIONS; i++) { putchar('B'); fflush(stdout); }
        exit(0);
    }

    pid_t p2 = fork();   // 子进程2优先级提升
    if (p2 == 0) {
        nice(-19); // 减小 nice 值，提高优先级（需要 sudo）
        for (int i = 0; i < ITERATIONS; i++) { putchar('C'); fflush(stdout); }
        exit(0);
    }

    for (int i = 0; i < ITERATIONS; i++) { putchar('A'); fflush(stdout); }

    wait_children(2);
    printf("\n=== 模块 1.2 完成 ===\n");
}

// ============================ 模块 2.1：无锁输出句子 ============================
// 观察不同进程输出句子时出现的字符穿插/撕裂现象
void module2_1() {
    printf("\n=== 模块 2.1：无锁输出句子（可见撕裂） ===\n");

    pid_t p1 = fork();
    if (p1 == 0) { module2_1_print("Child B speaking...\n"); exit(0); }

    pid_t p2 = fork();
    if (p2 == 0) { module2_1_print("Child C speaking...\n"); exit(0); }

    module2_1_print("Parent A speaking...\n");

    wait_children(2);
    printf("=== 模块 2.1 完成 ===\n");
}

// ============================ 模块 2.2：加锁输出句子 ============================
// 使用 POSIX 信号量，确保一个进程完整输出句子
void module2_2() {
    printf("\n=== 模块 2.2：加锁输出句子 ===\n");

    // 创建匿名共享信号量
    sem_t* sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE,
                      MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    sem_init(sem, 1, 1);  // 初始化为 1，表示“可用”

    pid_t p1 = fork();
    if (p1 == 0) { module2_2_print("Child B speaking...\n", sem); exit(0); }

    pid_t p2 = fork();
    if (p2 == 0) { module2_2_print("Child C speaking...\n", sem); exit(0); }

    module2_2_print("Parent A speaking...\n", sem);

    wait_children(2);
    sem_destroy(sem);
    munmap(sem, sizeof(sem_t));
    printf("=== 模块 2.2 完成 ===\n");
}

// ============================ 模块 3：软中断通信 ============================
// 父进程响应 Ctrl+\ 信号，向子进程发送 SIGUSR1
// 子进程接收到 SIGUSR1 后退出
void child_sigusr1(int sig) {
    printf("child process is killed by parent!\n");
    fflush(stdout);
    exit(0);
}

void parent_sigquit(int sig) {
    if (child1) kill(child1, SIGUSR1);
    if (child2) kill(child2, SIGUSR1);
}

void module3() {
    printf("\n=== 模块 3：软中断通信 ===\n");

    signal(SIGQUIT, parent_sigquit);  // 捕捉 Ctrl+\ 触发的 SIGQUIT

    child1 = fork();
    if (child1 == 0) {
        signal(SIGQUIT, SIG_IGN);
        signal(SIGUSR1, child_sigusr1);
        pause(); // 等待信号
    }

    child2 = fork();
    if (child2 == 0) {
        signal(SIGQUIT, SIG_IGN);
        signal(SIGUSR1, child_sigusr1);
        pause();
    }

    printf("请按 Ctrl+\\ (SIGQUIT) 来触发通信\n");
    fflush(stdout);
    wait_children(2);
    printf("parent process in killed!\n");
}

// ============================ 主程序入口 ============================
int main() {
    int choice;
    while (1) {
        printf("\n菜单：1.模块1.1 2.模块1.2 3.模块2.1 4.模块2.2 5.模块3 0.退出\n选择：");
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: module1_1(); break;
            case 2: module1_2(); break;
            case 3: module2_1(); break;
            case 4: module2_2(); break;
            case 5: module3(); break;
            case 0: printf("退出程序\n"); return 0;
            default: printf("无效选项\n");
        }
    }
    return 0;
}
