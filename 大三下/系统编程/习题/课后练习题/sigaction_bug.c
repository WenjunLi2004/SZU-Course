#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static void dump_sigaction_before_install(struct sigaction *act)
{
#ifdef DEBUG_SIGACTION
    unsigned int raw_flags = 0;
    unsigned char *p = (unsigned char *)act;
    size_t i;

    memcpy(&raw_flags, &act->sa_flags, sizeof(raw_flags));
    printf("debug: raw newact.sa_flags = 0x%08x (%u)\n",
           raw_flags, raw_flags);
    printf("debug: raw struct sigaction bytes:");
    for (i = 0; i < sizeof(*act); i++)
        printf(" %02x", p[i]);
    printf("\n");
#else
    (void)act;
#endif
}

void sigalrm_handler(int signum)
{
    printf("Alarming...\n");
}

void sigint_handler(int signum)
{
}

int sf_sleep_with_suspend(int nsec)
{
    sigset_t mask;
    sigset_t oldmask, newmask;

    signal(SIGINT, sigint_handler);
    printf("Enter sf_sleep_with_sigsuspend()...\n");

    struct sigaction newact, oldact;
    unsigned int remaining = 0;

    newact.sa_handler = sigalrm_handler;
#ifndef OMIT_SA_FLAGS_INIT
    newact.sa_flags = 0;
#endif

    dump_sigaction_before_install(&newact);

    sigaction(SIGALRM, &newact, &oldact);

    sigemptyset(&newmask);
    sigemptyset(&oldmask);
    sigaddset(&newmask, SIGALRM);

    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    alarm(nsec);

    mask = oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigdelset(&mask, SIGALRM);

    sigsuspend(&mask);

    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    remaining = alarm(0);

    sigaction(SIGALRM, &oldact, NULL);

    printf("Exit sf_sleep_with_sigsuspend()...\n");
    return remaining;
}

int main()
{
    printf("Start to sleep...\n");
    int remaining = sf_sleep_with_suspend(5);
    printf("Wake up with %d seconds remained...\n", remaining);
}
