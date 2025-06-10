// integrated_demo.c
// 多进程综合实验：创建、控制、互斥与软中断通信演示
// 编译：gcc integrated_demo.c -o integrated_demo -lrt
// 运行：sudo ./integrated_demo

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include <sched.h>

#define ITERATIONS 2000

// -----------------------------
// 辅助函数声明
// -----------------------------
void wait_children(int n);
void noisy_printf(const char* msg);
void child_sentence(const char* sentence);
void locked_sentence(const char* sentence, sem_t* sem);
void rand_sleep(int min_ms, int max_ms);

// -----------------------------
// 模块一-1：基本调度混乱输出（含 rand_sleep）
// -----------------------------
void module1_1() {
    printf("\n=== 模块一-1：基本调度混乱输出（含 rand_sleep） ===\n");

    pid_t p1 = fork();
    if (p1 < 0) { perror("fork p1"); exit(1); }
    if (p1 == 0) {
        srand(time(NULL) ^ getpid());
        for (int i = 0; i < ITERATIONS; i++) {
            putchar('b'); fflush(stdout);
            //rand_sleep(10, 50);
        }
        exit(0);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork p2"); exit(1); }
    if (p2 == 0) {
        srand(time(NULL) ^ getpid());
        for (int i = 0; i < ITERATIONS; i++) {
            putchar('c'); fflush(stdout);
            //rand_sleep(10, 50);
        }
        exit(0);
    }

    srand(time(NULL) ^ getpid());
    for (int i = 0; i < ITERATIONS; i++) {
        putchar('a'); fflush(stdout);
        //rand_sleep(10, 50);
    }

    wait_children(2);
    printf("\n=== 模块一-1 结束 ===\n");
}

// -----------------------------
// 模块一-2：优先级控制输出（不含 rand_sleep）
// -----------------------------
void module1_2() {
    printf("\n=== 模块一-2：优先级控制输出（不含 rand_sleep） ===\n");

    pid_t p1 = fork();
    if (p1 < 0) { perror("fork p1"); exit(1); }
    if (p1 == 0) {
        struct sched_param param = { .sched_priority = 1 };
        if (sched_setscheduler(0, SCHED_RR, &param) == -1) {
            perror("sched_setscheduler p1");
            exit(1);
        }
        for (int i = 0; i < ITERATIONS; i++) {
            putchar('B'); fflush(stdout);
            // 可加微小 sleep，避免过于激进
            //usleep(10);
        }
        exit(0);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork p2"); exit(1); }
    if (p2 == 0) {
        struct sched_param param = { .sched_priority = 99 };
        if (sched_setscheduler(0, SCHED_RR, &param) == -1) {
            perror("sched_setscheduler p2");
            exit(1);
        }
        for (int i = 0; i < ITERATIONS; i++) {
            putchar('C'); fflush(stdout);
            // 高优先级，不加 sleep，尽可能抢占
        }
        exit(0);
    }

    for (int i = 0; i < ITERATIONS; i++) {
        putchar('A'); fflush(stdout);
        //usleep(10);
    }

    wait_children(2);
    printf("\n=== 模块一-2 结束 ===\n");
}

// -----------------------------
// 模块二-1：输出撕裂（无锁）
// -----------------------------
void module2_1() {
    printf("\n=== 模块二-1：输出撕裂（无锁） ===\n");

    pid_t p1 = fork();
    if (p1 < 0) { perror("fork p1"); exit(1); }
    if (p1 == 0) {
        child_sentence("Process B speaking...\n");
        exit(0);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork p2"); exit(1); }
    if (p2 == 0) {
        child_sentence("Process C speaking...\n");
        exit(0);
    }

    child_sentence("Parent A speaking...\n");
    wait_children(2);
    printf("=== 模块二-1 结束 ===\n");
}

// -----------------------------
// 模块二-2：输出互斥（加锁）
// -----------------------------
void module2_2() {
    printf("\n=== 模块二-2：输出互斥（加锁） ===\n");

    sem_t* sem = mmap(NULL, sizeof(sem_t),
                      PROT_READ|PROT_WRITE,
                      MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (sem == MAP_FAILED) { perror("mmap"); exit(1); }
    sem_init(sem, 1, 1);

    pid_t p1 = fork();
    if (p1 < 0) { perror("fork p1"); exit(1); }
    if (p1 == 0) {
        locked_sentence("Process B speaking...\n", sem);
        exit(0);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror("fork p2"); exit(1); }
    if (p2 == 0) {
        locked_sentence("Process C speaking...\n", sem);
        exit(0);
    }

    locked_sentence("Parent A speaking...\n", sem);
    wait_children(2);

    sem_destroy(sem);
    munmap(sem, sizeof(sem_t));
    printf("=== 模块二-2 结束 ===\n");
}

// -----------------------------
// 模块三：软中断通信
// -----------------------------
pid_t child1 = 0, child2 = 0;

void child_handler(int signo) {
    printf("child process is killed by parent!\n");
    fflush(stdout);
    exit(0);
}

void parent_handler(int signo) {
    printf("parent caught signal, killing children...\n");
    if (child1 > 0) kill(child1, SIGUSR1);
    if (child2 > 0) kill(child2, SIGUSR1);
}

void module3() {
    printf("\n=== 模块三：软中断通信演示 ===\n");
    signal(SIGQUIT, parent_handler);

    child1 = fork();
    if (child1 < 0) { perror("fork child1"); exit(1); }
    if (child1 == 0) {
        signal(SIGQUIT, SIG_IGN);
        signal(SIGUSR1, child_handler);
        while (1) pause();
    }

    child2 = fork();
    if (child2 < 0) { perror("fork child2"); exit(1); }
    if (child2 == 0) {
        signal(SIGQUIT, SIG_IGN);
        signal(SIGUSR1, child_handler);
        while (1) pause();
    }

    printf("请按 Ctrl+\\ 发出 SIGQUIT...\n");
    wait_children(2);
    printf("parent process is killed!\n");
}

// -----------------------------
// 主菜单
// -----------------------------
int main() {
    int choice;
    while (1) {
        printf("\n菜单选择：\n");
        printf("1 - 模块一-1：基本调度混乱输出（含 rand_sleep）\n");
        printf("2 - 模块一-2：优先级控制输出（不含 rand_sleep）\n");
        printf("3 - 模块二-1：输出撕裂（无锁）\n");
        printf("4 - 模块二-2：输出互斥（加锁）\n");
        printf("5 - 模块三：软中断通信\n");
        printf("0 - 退出\n选择：");
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: module1_1(); break;
            case 2: module1_2(); break;
            case 3: module2_1(); break;
            case 4: module2_2(); break;
            case 5: module3();  break;
            case 0: printf("退出程序。\n"); return 0;
            default: printf("无效选项。\n");
        }
    }
    return 0;
}

// -----------------------------
// 辅助函数定义
// -----------------------------
void wait_children(int n) {
    for (int i = 0; i < n; i++) wait(NULL);
}

void noisy_printf(const char* msg) {
    for (int i = 0; msg[i]; ++i) {
        putchar(msg[i]); fflush(stdout);
        usleep(1000);
    }
}

void child_sentence(const char* sentence) {
    srand(time(NULL) ^ getpid());
    for (int i = 0; i < 5; ++i) {
        noisy_printf(sentence);
        rand_sleep(100, 300);
    }
}

void locked_sentence(const char* sentence, sem_t* sem) {
    srand(time(NULL) ^ getpid());
    for (int i = 0; i < 5; ++i) {
        sem_wait(sem);
        noisy_printf(sentence);
        sem_post(sem);
        rand_sleep(100, 300);
    }
}

void rand_sleep(int min_ms, int max_ms) {
    if (max_ms < min_ms) return;
    int ms = min_ms + rand() % (max_ms - min_ms + 1);
    usleep(ms * 1000);
}