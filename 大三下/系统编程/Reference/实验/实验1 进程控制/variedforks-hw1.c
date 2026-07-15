/*
 * variedforks.c - Examples of Unix process control
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

/*
 * fork0 - The simplest fork example
 * Call once, return twice
 * Creates child that is identical to parent
 * Returns 0 to child process
 * Returns child PID to parent process
 */
void fork0() 
{
    if (fork() == 0) {
	printf("Hello from child\n");
    }
    else {
	printf("Hello from parent\n");
    }
}

/* 
 * fork1 - Simple fork example 
 * Parent and child both run same code
 * Child starts with identical private state
 */
void fork1()
{
    int x = 1;
    pid_t pid = fork();

    if (pid == 0) {
	printf("Child has x = %d\n", ++x);
    } 
    else {
	printf("Parent has x = %d\n", --x);
    }
    printf("Bye from process %d with x = %d\n", getpid(), x);
}

/*
 * fork2 - Two consecutive forks
 * Both parent and child can continue forking
 * Ordering undetermined
 */
void fork2()
{
    printf("L0\n");
    fork();
    printf("L1\n");    
    fork();
    printf("Bye\n");
}

/*
 * borkfork - Three consective forks
 * Parent and child can continue forking
 */
#define bork fork
void borkfork()
{
    bork(); bork(); bork();
    printf("borked\n");
}


/*
 * fork3 - Three consective forks
 * Parent and child can continue forking
 */
void fork3()
{
    printf("L0\n");
    fork();
    printf("L1\n");    
    fork();
    printf("L2\n");    
    fork();
    printf("Bye\n");
}


/* 
 * fork4 - Nested forks in parents
 */
void fork4()
{
    printf("L0\n");
    if (fork() != 0) {
	printf("L1\n");    
	if (fork() != 0) {
	    printf("L2\n");
	    fork();
	}
    }
    printf("Bye\n");
}

/*
 * fork5 - Nested forks in children
 */
void fork5()
{
    printf("L0\n");
    if (fork() == 0) {
	printf("L1\n");    
	if (fork() == 0) {
	    printf("L2\n");
	    fork();
	}
    }
    printf("Bye\n");
}

void cleanup(void) {
    printf("Cleaning up\n");
}

/*
 * fork6 - Exit system call terminates process
 * call once, return never
 */
void fork6()
{
    atexit(cleanup);
    fork();
    exit(0);
}

/* 
 * fork7 - Demonstration of zombies.
 * Run in background and then perform ps 
 */
void fork7()
{
    if (fork() == 0) {
	/* Child */
	printf("Terminating Child, PID = %d\n", getpid());
	exit(0);
    } else {
	printf("Running Parent, PID = %d\n", getpid());
	while (1)
	    ; /* Infinite loop */
    }
}

/* 
 * fork8 - Demonstration of nonterminating child.  
 * Child still running even though parent terminated
 * Must kill explicitly
 */
void fork8()
{
    if (fork() == 0) {
	/* Child */
	printf("Running Child, PID = %d\n",
	       getpid());
	while (1)
	    ; /* Infinite loop */
    } else {
	printf("Terminating Parent, PID = %d\n",
	       getpid());
	exit(0);
    }
}

/*
 * fork9 - synchronizing with and reaping children (wait)
 */
void fork9()
{
    int child_status;

    if (fork() == 0) {
	printf("HC: hello from child\n");
    } else {
	printf("HP: hello from parent\n");
	wait(&child_status);
	printf("CT: child has terminated\n");
    }
    printf("Bye\n");
}

#define N 5
/* 
 * fork10 - Synchronizing with multiple children (wait)
 * Reaps children in arbitrary order
 * WIFEXITED and WEXITSTATUS to get info about terminated children
 */
void fork10()
{
    pid_t pid[N];
    int i, child_status;

    for (i = 0; i < N; i++)
	if ((pid[i] = fork()) == 0) {
	    exit(100+i); /* Child */
	}
    for (i = 0; i < N; i++) { /* Parent */
	pid_t wpid = wait(&child_status);
	if (WIFEXITED(child_status))
	    printf("Child %d terminated with exit status %d\n",
		   wpid, WEXITSTATUS(child_status));
	else
	    printf("Child %d terminate abnormally\n", wpid);
    }
}

/* 
 * fork11 - Using waitpid to reap specific children
 * Reaps children in reverse order
 */
void fork11()
{
    pid_t pid[N];
    int i;
    int child_status;

    for (i = 0; i < N; i++)
	if ((pid[i] = fork()) == 0)
	    exit(100+i); /* Child */
    for (i = N-1; i >= 0; i--) {
	pid_t wpid = waitpid(pid[i], &child_status, 0);
	if (WIFEXITED(child_status))
	    printf("Child %d terminated with exit status %d\n",
		   wpid, WEXITSTATUS(child_status));
	else
	    printf("Child %d terminate abnormally\n", wpid);
    }
}



/* 
 * fork12 - Demonstration of using /bin/kill program 
 */
void fork12() 
{
    if (fork() == 0) {
	printf("Child1: pid=%d pgrp=%d\n",
	       getpid(), getpgrp());
	if (fork() == 0)
	    printf("Child2: pid=%d pgrp=%d\n",
		   getpid(), getpgrp());
	while(1);
    }
} 

/* 
 * Demonstration of using ctrl-c and ctrl-z 
 */
void fork13() 
{
    if (fork() == 0) {
	printf("Child: pid=%d pgrp=%d\n",
	       getpid(), getpgrp());
    }
    else {
	printf("Parent: pid=%d pgrp=%d\n",
	       getpid(), getpgrp());
    }
    while(1);
} 

/* 
 * Simple demo of execve: run ls -l 
 */
int fork14()
{
    char * argv[3] = {"/bin/ls", "-l", NULL};
    char * env[1] = {NULL};
    int pid;

    if ((pid = fork()) == 0) { /* Child runs user job */
	if (execve(argv[0], argv, env) < 0) {
	    printf("%s: Command not found.\n", argv[0]);
	    exit(0);
	}
    }
}


int main(int argc, char *argv[])
{
    int option = 0;
    if (argc > 1) {
	if (!strcmp(argv[1], "borkfork")) {
	    borkfork();
	    exit(0);
	}
    }
	
	option = atoi(argv[1]);
    switch(option) {
    case 0: fork0();
	break;
    case 1: fork1();
	break;
    case 2: fork2();
	break;
    case 3: fork3();
	break;
    case 4: fork4();
	break;
    case 5: fork5();
	break;
    case 6: fork6();
	break;
    case 7: fork7();
	break;
    case 8: fork8();
	break;
    case 9: fork9();
	break;
    case 10: fork10();
	break;
    case 11: fork11();
	break;
    case 12: fork12();
	break;
    case 13: fork13();
	break;
    case 14: fork14();
	break;
    default:
	printf("Unknown option %d\n", option);
	break;
    }
    return 0;
}
