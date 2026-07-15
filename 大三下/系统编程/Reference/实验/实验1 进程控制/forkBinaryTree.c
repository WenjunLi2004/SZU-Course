#include<unistd.h>
#include<stdio.h>
#include <stdlib.h>
#include <wait.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s processes \n", argv[0]);
        return 1;
    }
    int depth = atoi(argv[1]);
    int number = 1;
    int child;
    pid_t pid1, pid2;
    for (int i = 0; i < depth; i++) {
        printf("I am process no %5d  with PID %5d and PPID %d\n", number, getpid(), getppid());
        switch (pid1 = fork()) {
            case 0:
                number = 2 * number;
                break;
            case -1:
                perror("fork");
                exit(1);
            default:
                switch (pid2 = fork()) {
                    case 0:
                        number = 2 * number + 1;
                        break;
                    case -1:
                        perror("fork");
                        exit(1);
                    default:
                        waitpid(pid1,&child,WUNTRACED);
                        waitpid(pid2,&child,WUNTRACED);
                        exit(0);
                }
        }
    }
}