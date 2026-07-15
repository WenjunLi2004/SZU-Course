#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>
#include <stdarg.h>

void rsyslog(int priority, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsyslog(priority, format, ap);
    va_end(ap);
}

int init_daemon(void)
{
    int pid;
    int i;

    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    pid = fork();
    if (pid > 0) {
        exit(0);
    } else if (pid < 0) {
        return -1;
    }

    setsid();

    pid = fork();
    if (pid > 0) {
        exit(0);
    } else if (pid < 0) {
        return -1;
    }

    for (i = 0; i < NOFILE; close(i++));

    chdir("/");
    umask(0);
    signal(SIGCHLD, SIG_IGN);

    return 0;
}

int main()
{
    time_t now;

    init_daemon();
    openlog("daemon", LOG_PID, LOG_USER);
    rsyslog(LOG_USER | LOG_INFO, "测试守护进程!");

    while (1) {
        sleep(8);
        time(&now);
        rsyslog(LOG_USER | LOG_INFO, "系统时间:\t%s", ctime(&now));
    }
}
