#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

#define SEM_PARENT "/sem_parent"
#define SEM_CHILD "/sem_child"

void child_sig_handler(int sig) {
    if (sig == SIGUSR1) {
        printf("子进程收到父进程的信号:SIGUSR1\n");
    }
}

void parent_sig_handler(int sig) {
    if (sig == SIGCHLD) {
        printf("父进程收到子进程结束信号:SIGCHLD\n");
    }
}

int main() {
    pid_t pid;
    sem_t *sem_parent, *sem_child;

    sem_parent = sem_open(SEM_PARENT, O_CREAT, 0644, 1); // 父进程初始可运行
    sem_child = sem_open(SEM_CHILD, O_CREAT, 0644, 0);   // 子进程初始阻塞

    if (sem_parent == SEM_FAILED || sem_child == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, parent_sig_handler);

    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        sem_unlink(SEM_PARENT);
        sem_unlink(SEM_CHILD);
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        signal(SIGUSR1, child_sig_handler);

        for (int i = 0; i < 5; i++) {
            sem_wait(sem_child);
            printf("子进程输出字符:%c\n", 'A' + i);
            fflush(stdout);  // 确保输出立即显示
            sem_post(sem_parent);
        }

        printf("子进程等待父进程信号...\n");
        fflush(stdout);
        pause();

        sem_close(sem_parent);
        sem_close(sem_child);
        exit(0);
    } else {
        for (int i = 0; i < 5; i++) {
            sem_wait(sem_parent);
            printf("父进程输出字符:%c\n", 'a' + i);
            fflush(stdout);
            sem_post(sem_child);
        }

        kill(pid, SIGUSR1);
        wait(NULL);

        sem_close(sem_parent);
        sem_close(sem_child);
        sem_unlink(SEM_PARENT);
        sem_unlink(SEM_CHILD);

        printf("父进程结束。\n");
    }

    return 0;
}
