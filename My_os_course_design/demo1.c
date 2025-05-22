#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    pid_t pid1, pid2;

    //创建第一个子进程
    pid1 = fork();

    if(pid1 < 0) {
        perror("fork1 failed");
        return 1;
    }

    if(pid1 == 0) {
        //第一个子进程
        printf("Child1 process: pid = %d, parent pid = %d\n", getpid(), getppid());
        return 0;
    }

    //创建第二个子进程
    pid2 = fork();

    if(pid2 < 0) {
        perror("fork2 failed");
        return 1;
    }

    if(pid2 == 0) {
        //第二个子进程
        printf("Child2 process: pid = %d, parent pid = %d\n", getpid(), getppid());
        return 0;
    }

    //父进程
    printf("parent process: pid = %d\n", getpid());

    //等待子进程结束(先不写wait)
    //wait(NULL);
    sleep(1);
}