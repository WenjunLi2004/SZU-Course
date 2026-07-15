/*
 * signals.c - Examples of Unix process and signal control
 * From code accompanying Computer Systems: A Programmer's Perspective, 2/E (CS:APP2e)
 * Randal E. Bryant and David R. O'Hallaron, Carnegie Mellon University
 *
 * Minor adaptation by Brighten Godfrey <pbg@illinois.edu>
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 
#include <signal.h>

#define N 5

/********* 
 * Signals
 *********/

/*
 * fork1 - Sending signals with the kill() function
 */
void fork1()
{
    pid_t pid[N];
    int i;
    int child_status;

    for (i = 0; i < N; i++)
	if ((pid[i] = fork()) == 0) {
	    /* Child: Infinite Loop */
	    while(1)
		;
	}
    for (i = 0; i < N; i++) {
	printf("Killing process %d\n", pid[i]);
	kill(pid[i], SIGINT);
    }

    for (i = 0; i < N; i++) {
	pid_t wpid = wait(&child_status);
	if (WIFEXITED(child_status))
	    printf("Child %d terminated with exit status %d\n",
		   wpid, WEXITSTATUS(child_status));
	else
	    printf("Child %d terminated abnormally\n", wpid);
    }
}

/*
 * int_handler - SIGINT handler
 */
void int_handler(int sig)
{
    printf("Process %d received signal %d\n", getpid(), sig);
    exit(0);
}

/*
 * fork2 - Simple signal handler example
 */
void fork2()
{
    pid_t pid[N];
    int i;
    int child_status;

    signal(SIGINT, int_handler);
    for (i = 0; i < N; i++)
	if ((pid[i] = fork()) == 0) {
	    /* Child: Infinite Loop */
	    while(1)
		;
	}

    for (i = 0; i < N; i++) {
	printf("Killing process %d\n", pid[i]);
	kill(pid[i], SIGINT);
    }

    for (i = 0; i < N; i++) {
	pid_t wpid = wait(&child_status);
	if (WIFEXITED(child_status))
	    printf("Child %d terminated with exit status %d\n",
		   wpid, WEXITSTATUS(child_status));
	else
	    printf("Child %d terminated abnormally\n", wpid);
    }
}


/*
 * child_handler - SIGCHLD handler that reaps one terminated child
 */
int ccount = 0;
void child_handler(int sig)
{
    int child_status;
    pid_t pid = wait(&child_status);
    ccount--;
    printf("Received SIGCHLD signal %d for process %d\n", sig, pid);
    fflush(stdout);
}

/*
 * fork3 - Signal funkiness: Pending signals are not queued
 */
void fork3()
{
    pid_t pid[N];
    int i;
    ccount = N;
    signal(SIGCHLD, child_handler);

    for (i = 0; i < N; i++) {
	if ((pid[i] = fork()) == 0) {
	    sleep(1);
	    exit(0);  /* Child: Exit */
	}
    }
    while (ccount > 0)
	pause();
}


/*
 * child_handler2 - SIGCHLD handler that reaps all terminated children
 */
void child_handler2(int sig)
{
    int child_status;
    pid_t pid;
    while ((pid = wait(&child_status)) > 0) {
	ccount--;
	printf("Received signal %d from process %d\n", sig, pid);
	fflush(stdout);
    }
}

/*
 * fork4 - Using a handler that reaps multiple children
 */
void fork4()
{
    pid_t pid[N];
    int i;
    ccount = N;

    signal(SIGCHLD, child_handler2);

    for (i = 0; i < N; i++)
	if ((pid[i] = fork()) == 0) {
	    sleep(1);
	    exit(0); /* Child: Exit */

	}
    while (ccount > 0) {
	pause();
    }
}



int main(int argc, char *argv[])
{
    int option = 0;
	option = atoi(argv[1]);
    switch(option) {
    case 1: fork1();
	break;
    case 2: fork2();
	break;
    case 3: fork3();
	break;
    case 4: fork4();
	break;
    default:
	printf("Unknown option %d, it should be 1,2,3,4\n", option);
	break;
    }
    return 0;
}
