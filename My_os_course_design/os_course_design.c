/* os_course_design.c
 *
 * 32 进程间同步与互斥 —— 三大模块示例
 *
 * 编译:
 *   gcc os_course_design.c -o os_design -lrt
 *
 * 运行:
 *   ./os_design 1   # 运行模块1:进程创建与字符输出
 *   ./os_design 2   # 运行模块2:句子输出 + 文件锁互斥
 *   ./os_design 3   # 运行模块3:软中断信号通信
 */

#define _GNU_SOURCE       // 为 nice()、O_CLOEXEC 等打开扩展
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

/* ================== 模块1 ==================
 * fork 两次创建父 + 两个子进程，让它们分别输出字符
 * 父输出 'a'，子1 输出 'b'，子2 输出 'c'，循环几次，观察调度
 * 同时用 nice() 改变子进程优先级，观察效果
 */
void module1() {
    pid_t p1, p2;

    printf("=== 模块1:进程创建与字符输出 ===\n");
    // 创建第一个子进程
    p1 = fork();
    if (p1 < 0) { perror("fork p1"); exit(1); }
    if (p1 == 0) {
        // 子进程1:提高 nice 值，让它优先级更低
        nice(10);
        for (int i = 0; i < 10; i++) {
            write(1, "b", 1);
            usleep(100000);
        }
        exit(0);
    }

    // 父进程继续再 fork 第二个子进程
    p2 = fork();
    if (p2 < 0) { perror("fork p2"); exit(1); }
    if (p2 == 0) {
        // 子进程2:保持默认优先级
        for (int i = 0; i < 10; i++) {
            write(1, "c", 1);
            usleep(100000);
        }
        exit(0);
    }

    // 父进程输出 a
    for (int i = 0; i < 10; i++) {
        write(1, "a", 1);
        usleep(100000);
    }

    // 等待子进程结束
    wait(NULL);
    wait(NULL);
    printf("\n=== 模块1 运行完毕 ===\n");
}

/* ================== 模块2 ==================
 * 在模块1基础上，改为输出一句话，并用文件锁做互斥:
 *   父进程输出 "Parent is running.\n"
 *   子1 输出 "Child1 is running.\n"
 *   子2 输出 "Child2 is running.\n"
 * 每次打印前都 lock，加完锁后再 unlock
 */
void module2() {
    pid_t p1, p2;
    int fd;
    struct flock lock;

    printf("=== 模块2:句子输出 + 文件锁互斥 ===\n");
    // 打开一个共享文件，用于加锁演示
    fd = open("lock.dat", O_CREAT|O_RDWR, 0666);
    if (fd < 0) { perror("open lock.dat"); exit(1); }

    // 准备 flock 结构
    memset(&lock, 0, sizeof(lock));
    lock.l_whence = SEEK_SET;
    lock.l_start  = 0;
    lock.l_len    = 0;      // 0 表示从当前位置到文件尾

    // 第一个子进程
    p1 = fork();
    if (p1 < 0) { perror("fork p1"); exit(1); }
    if (p1 == 0) {
        for (int i = 0; i < 5; i++) {
            lock.l_type = F_WRLCK;       // 加写锁
            fcntl(fd, F_SETLKW, &lock);

            printf("Child1 is running. (iteration %d)\n", i+1);

            lock.l_type = F_UNLCK;       // 解锁
            fcntl(fd, F_SETLK, &lock);

            sleep(1);
        }
        close(fd);
        exit(0);
    }

    // 第二个子进程
    p2 = fork();
    if (p2 < 0) { perror("fork p2"); exit(1); }
    if (p2 == 0) {
        for (int i = 0; i < 5; i++) {
            lock.l_type = F_WRLCK;
            fcntl(fd, F_SETLKW, &lock);

            printf("Child2 is running. (iteration %d)\n", i+1);

            lock.l_type = F_UNLCK;
            fcntl(fd, F_SETLK, &lock);

            sleep(1);
        }
        close(fd);
        exit(0);
    }

    // 父进程
    for (int i = 0; i < 5; i++) {
        lock.l_type = F_WRLCK;
        fcntl(fd, F_SETLKW, &lock);

        printf("Parent is running. (iteration %d)\n", i+1);

        lock.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lock);

        sleep(1);
    }

    // 等待
    wait(NULL);
    wait(NULL);
    close(fd);
    printf("=== 模块2 运行完毕 ===\n");
}

/* ================== 模块3 ==================
 * 软中断通信:
 *  父进程 捕捉 SIGINT（Ctrl+\\ 或 Del）
 *  父捕捉到后，向两个子进程发 SIGUSR1
 *  子进程 捕捉 SIGUSR1 后打印消息并退出
 *  父进程 wait 后打印结束信息
 */
pid_t cpid1 = -1, cpid2 = -1;
void on_parent_int(int sig) {
    // 父进程捕捉到中断后，向两个子进程发信号
    if (cpid1 > 0) kill(cpid1, SIGUSR1);
    if (cpid2 > 0) kill(cpid2, SIGUSR1);
}
void on_child_usr1(int sig) {
    // 子进程捕捉到父发的 SIGUSR1 后打印并退出
    printf("Child (pid=%d) is killed by parent!\n", getpid());
    exit(0);
}

void module3() {
    printf("=== 模块3:软中断通信 ===\n");
    // 安装父进程 SIGINT 捕捉（Del键在终端里可以用 Ctrl+\\ 发送 SIGQUIT，大部分终端 Ctrl+C 发送 SIGINT）
    signal(SIGINT, on_parent_int);

    // 创建子进程1
    cpid1 = fork();
    if (cpid1 < 0) { perror("fork1"); exit(1); }
    if (cpid1 == 0) {
        // 子1:捕捉 SIGUSR1
        signal(SIGUSR1, on_child_usr1);
        // 无限等待信号
        while(1) pause();
    }

    // 创建子进程2
    cpid2 = fork();
    if (cpid2 < 0) { perror("fork2"); exit(1); }
    if (cpid2 == 0) {
        signal(SIGUSR1, on_child_usr1);
        while(1) pause();
    }

    // 父进程等待子进程结束（会在收到 SIGINT 后被触发）
    wait(NULL);
    wait(NULL);
    printf("Parent process is killed!\n");
    printf("=== 模块3 运行完毕 ===\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "用法:%s [1|2|3]\n", argv[0]);
        return 1;
    }
    int mode = atoi(argv[1]);
    switch (mode) {
        case 1: module1(); break;
        case 2: module2(); break;
        case 3: module3(); break;
        default:
            fprintf(stderr, "无效的参数，请输入 1、2 或 3\n");
            return 1;
    }
    return 0;
}
