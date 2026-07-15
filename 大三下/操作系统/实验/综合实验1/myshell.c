#include <stdio.h>       // 提供 printf, fgets, perror 等标准输入输出函数
#include <stdlib.h>      // 提供 exit, malloc 等标准库函数
#include <string.h>      // 提供 strlen, strcspn 等字符串处理函数
#include <unistd.h>      // 提供 getcwd, fork, exec 等 POSIX 系统调用API
#include <sys/wait.h>    // 提供 waitpid 等等待子进程的函数
#include <fcntl.h>       // 为了使用 open 函数的 O_WRONLY 等宏
#define MAX_CMD_LEN 1024 // 宏定义：限制用户输入的命令最大长度为 1024 字节
#define MAX_ARGS 128     // 最大参数量
int main()
{
    char cmd_str[MAX_CMD_LEN]; // 定义一个字符数组，用来存放用户在键盘敲下的那一串字符串
    char cwd[1024];            // 定义一个字符数组，用来存放当前的工作路径(Current Working Directory)

    // Shell 的本质就是一个死循环：打印提示符 -> 读命令 -> 执行命令 -> 再次打印提示符
    while (1)
    {
        // 1. 获取当前工作路径
        // getcwd 函数会将当前路径填充到 cwd 数组中。如果获取成功，返回 cwd 的指针
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            // 打印高级提示符：用中括号包围路径，加上 $ 符号，不换行
            printf("[MyShell:%s]$ ", cwd);
        }
        else
        {
            // 如果获取路径失败（比如目录被删了），就退化成最低标准的固定提示符
            printf("[MyShell]$ ");
        }

        // fflush 强制把缓冲区里的内容输出到屏幕上。
        // 因为 printf 没有加 \n，系统可能会把提示符憋在内存里不显示，fflush 可以把它“挤”出来。
        fflush(stdout);

        // 2. 读取用户从键盘输入的命令
        // fgets 会从标准输入(stdin)读取一行文本，存入 cmd_str。
        // 如果用户按了 Ctrl+D (EOF)，fgets 会返回 NULL，此时我们让 shell 优雅退出
        if (fgets(cmd_str, MAX_CMD_LEN, stdin) == NULL)
        {
            printf("\nExiting MyShell...\n");
            break;
        }

        // 3. 清理用户输入时带入的换行符
        // 用户输入完毕敲回车时，会把 '\n' 也录入进 cmd_str 里。
        // strcspn 会查找 '\n' 在字符串中第一次出现的位置，我们把它替换成 '\0' (字符串结束符)
        cmd_str[strcspn(cmd_str, "\n")] = '\0';

        // 4. 判断用户是不是直接敲了一个空回车
        // 如果长度为0，说明没敲命令，直接 continue 进入下一次循环，重新打印提示符
        if (strlen(cmd_str) == 0)
        {
            continue;
        }

        // 测试打印：看看我们是否成功抓取到了命令（后续这一步会替换为命令解析与执行）
        // printf("【Debug】你刚刚输入的命令是: %s\n", cmd_str);

        // 复制一份原始字符串，用于提示信息
        char original_cmd[MAX_CMD_LEN];
        strcpy(original_cmd, cmd_str);

        // 分割 -> pipe_str[0]=‘|’
        char *pipe_str = strstr(cmd_str, "||");
        char *redir_str = strstr(cmd_str, ">"); // 查找重定向符号
        if (pipe_str != NULL)
        {
            // ================= 管道处理核心逻辑 =================
            printf("%s transferring data...\n", original_cmd);

            // 1. 分割两半
            pipe_str[0] = '\0';            // 将 "||" 的第一个 '|' 变成了字符串结束符 '\0'
            char *cmd1_str = cmd_str;      // 左半部分，比如 "ls "
            char *cmd2_str = pipe_str + 2; // 右半部分，跳过 "||"，比如 " more"

            // 2. 切分左右两个命令的参数
            char *args1[MAX_ARGS];
            int i = 0;
            char *token = strtok(cmd1_str, " ");
            while (token != NULL)
            {
                args1[i++] = token;
                token = strtok(NULL, " ");
            }
            args1[i] = NULL;

            char *args2[MAX_ARGS];
            int j = 0;
            token = strtok(cmd2_str, " ");
            while (token != NULL)
            {
                args2[j++] = token;
                token = strtok(NULL, " ");
            }
            args2[j] = NULL;

            // 打印命令的 starting 提示 (由父进程来打印)
            printf("%s starting...\n", args1[0]);
            printf("%s starting...\n", args2[0]);

            // 3. 创建无名管道
            int fds[2];
            if (pipe(fds) == -1)
            {
                perror("Pipe failed");
                continue;
            }

            // 4. fork 第一个子进程 (负责写入端)
            pid_t pid1 = fork();
            if (pid1 == 0)
            {
                // 将自己的标准输出(1)重定向到管道写端(fds[1])
                dup2(fds[1], STDOUT_FILENO);
                // 换完之后，原来的管道端口对它没用了，必须关闭
                close(fds[0]);
                close(fds[1]);

                if (execvp(args1[0], args1) == -1)
                {
                    perror("无效命令");
                    exit(1);
                }
            }
            // 5. fork 第二个子进程 (负责读取端)
            pid_t pid2 = fork();
            if (pid2 == 0)
            {
                // 将自己的标准输入(0)重定向到管道读端(fds[0])
                dup2(fds[0], STDIN_FILENO);
                // 同样，换完就关
                close(fds[0]);
                close(fds[1]);

                if (execvp(args2[0], args2) == -1)
                {
                    perror("无效命令");
                    exit(1);
                }
            }

            // 6. 父进程的职责
            // 如果父进程不关 fds[1]，子进程2(more)会一直等写数据，导致死锁！
            close(fds[0]);
            close(fds[1]);

            // 等待子进程
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);

            // 打印收尾提示
            printf("%s ending.\n", args1[0]);
            printf("%s ending.\n", args2[0]);
            printf("%s finish data.\n", original_cmd);
        }
        // "ls > a.txt"
        else if (redir_str != NULL)
        {
            redir_str[0] = '\0'; // 将 ">" 变成结束符，左边是命令，右边是文件
            char *cmd_part = cmd_str;
            char *file_part = redir_str + 1;
            // 提取文件名（去除可能的空格）
            char *filename = strtok(file_part, " ");

            // 提取命令参数
            char *args[MAX_ARGS];
            int i = 0;
            char *token = strtok(cmd_part, " ");
            while (token != NULL)
            {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            printf("%s starting...\n", args[0]);

            pid_t pid = fork();
            if (pid == 0)
            {
                // 打开文件：只写 | 若不存在则创建 | 若存在则清空，权限设为 0644
                // 返回一个文件描述符
                int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0)
                {
                    perror("文件打开失败");
                    exit(1);
                }

                // 将 1号（标准输出） 重定向到 文件描述符
                dup2(fd, STDOUT_FILENO);
                close(fd); // 关掉多余的 fd

                if (execvp(args[0], args) == -1)
                {
                    perror("无效命令");
                    exit(1);
                }
            }
            else
            {
                waitpid(pid, NULL, 0);
                printf("%s ending.\n", args[0]);
            }
        }
        else
        {
            // ================= 无管道的单命令逻辑 =================

            // 定义一个指针数组，用来存放切割后的命令和参数。
            // 比如输入 "ls -l"，切分后 args[0]="ls", args[1]="-l", args[2]=NULL
            char *args[MAX_ARGS];

            // 1. 字符串切割 (分词)
            int i = 0;
            // strtok 是 C 语言自带的切割函数，这里以空格 " " 为刀，切下第一块肉
            char *token = strtok(cmd_str, " ");
            while (token != NULL)
            {
                args[i] = token; // 把切下来的字符串存入 args 数组
                i++;
                // 继续对剩下的字符串切片。strtok 规定后续切片第一个参数必须传 NULL
                token = strtok(NULL, " ");
            }
            args[i] = NULL; // Linux 规定，给 exec 执行的参数数组，最后一个元素必须是 NULL，代表参数结束

            // 无论什么命令，一上来先打印 starting
            printf("%s starting...\n", args[0]);

            // 2. 处理内部命令
            if (strcmp(args[0], "exit") == 0)
            {
                printf("Bye!\n");
                printf("%s ending.\n", args[0]);
                break;
            }
            else if (strcmp(args[0], "help") == 0)
            {
                printf("--- 欢迎使用 MyShell ---\n");
                printf("支持内部命令: help, exit\n");
                printf("支持外部命令: ls, cp, pwd 以及当前目录下的可执行文件\n");
                printf("%s ending.\n", args[0]);
                continue;
            }

            // 3. 处理外部命令与无效命令
            else
            {
                pid_t pid = fork();

                if (pid < 0)
                {
                    perror("Fork failed");
                }
                else if (pid == 0)
                {
                    if (execvp(args[0], args) == -1)
                    {
                        // 如果是无效命令，execvp会失败并走到这里
                        perror("无效命令");
                        exit(1); // 子进程报错后必须立刻死掉
                    }
                }
                else
                {
                    // 父进程乖乖等待子进程结束
                    // 注意：如果子进程是因为“无效命令”而 exit(1) 死掉的，父进程也会在这里等它死透
                    waitpid(pid, NULL, 0);

                    // 父进程最终都会走到这里，打印 ending，完美契合实验要求！
                    printf("%s ending.\n", args[0]);
                }
            }
        }
    }

    return 0;
}