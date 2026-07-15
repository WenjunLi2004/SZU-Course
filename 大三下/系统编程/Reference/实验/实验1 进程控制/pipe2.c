#include<unistd.h>
#include<stdio.h>
int main(){
    char input[256];
    scanf("%[^\n]",input);
    printf("%s  ",input);
    printf("Pipe2: PID %d and PGID %d\n",getpid(),getpgrp());
    return 0;
}