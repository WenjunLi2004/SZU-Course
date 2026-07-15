# 实验二一步步操作清单

> 本清单按更正后的题目顺序编写：第 1 题是 GDB 调试原程序，第 2 题是修正并运行 FIFO 计算客户端/服务器。  
> `raw/` 目录只作为老师给出的原程序，不要直接修改；根目录下的 `calcserver.c`、`client.c`、`clientinfo.h` 是修正后的提交版本。  
> 服务器目录按本次报告使用：`/home/szu/liwenjun2023150001/chatapplication`。

## 0. 登录服务器并准备目录

```bash
mkdir -p /home/szu/liwenjun2023150001/chatapplication
cd /home/szu/liwenjun2023150001/chatapplication
mkdir -p raw data/server_fifo data/client_fifo
```

把老师给出的原始 `calcserver.c`、`client.c`、`clientinfo.h` 放到 `raw/`，把修正后的三个同名文件放到 `chatapplication/` 根目录。

## 1. 第一题：GDB 调试 raw 原程序

### 1.1 先尝试编译原程序

```bash
cd /home/szu/liwenjun2023150001/chatapplication
gcc -g -Wall -Wextra -o raw_calcserver calcserver.c
gcc -g -Wall -Wextra -o raw_client client.c
```

如果提示 `mkfifo` 未声明，说明原程序缺少：

```c
#include <sys/stat.h>
```

为了继续做 GDB，可临时复制一份调试用文件，不改动 `raw/` 原件：

```bash
cp calcserver.c gdb_calcserver.c
cp client.c gdb_client.c
```

在两个调试副本的头文件区域加入：

```c
#include <sys/stat.h>
```

然后重新编译：

```bash
gcc -g -Wall -Wextra -o gdb_calcserver gdb_calcserver.c
gcc -g -Wall -Wextra -o gdb_client gdb_client.c
```

截图 1：保留原程序编译警告或调试副本编译过程。

### 1.2 用 GDB 查看服务器公共 FIFO 路径

```bash
gdb ./gdb_calcserver
(gdb) break main
(gdb) run
(gdb) next
(gdb) print FIFO_NAME
(gdb) quit
```

应观察到：

```text
$1 = "/tmp/server_fifo"
```

结论：原程序公共 FIFO 路径写在 `/tmp/server_fifo`，不符合实验要求的：

```text
/home/szu/liwenjun2023150001/chatapplication/data/server_fifo/chat_server_fifo
```

截图 2：GDB 中 `print FIFO_NAME` 的结果。

### 1.3 用 GDB 查看客户端私有 FIFO 路径

先开一个终端运行原服务器调试副本：

```bash
./gdb_calcserver
```

再开另一个终端调试客户端：

```bash
gdb ./gdb_client
(gdb) break 55
(gdb) run 12 + 30
(gdb) next
(gdb) print mypipename
(gdb) quit
```

不同系统行号可能略有差异，断点放在 `sprintf(mypipename, "/tmp/client%d_fifo", getpid());` 附近即可。

应观察到类似：

```text
$1 = "/tmp/client12345_fifo"
```

结论：原程序私有 FIFO 也放在 `/tmp`，应改为实验目录下的：

```text
/home/szu/liwenjun2023150001/chatapplication/data/client_fifo/chat_client<PID>_fifo
```

截图 3：GDB 中 `print mypipename` 的结果。

### 1.4 记录原程序问题

报告中写这几条即可：

- 原程序缺少 `sys/stat.h`，严格编译时 `mkfifo` 可能报未声明。
- 公共 FIFO 使用 `/tmp/server_fifo`，不符合指导书路径要求。
- 私有 FIFO 使用 `/tmp/client<PID>_fifo`，不符合指导书路径要求。
- `signal(SIGKILL, handler)` 无效，因为 `SIGKILL` 不能被捕获、阻塞或忽略。
- 客户端 `mkfifo` 失败时打印未初始化的 `buffer`，应打印 `mypipename`。
- 服务器除法没有判断除数为 0。
- 服务器只读打开公共 FIFO，长期运行时容易遇到无写端 EOF 问题。

## 2. 第二题：修正并运行 FIFO 计算程序

### 2.1 编译修正后的程序

```bash
cd /home/szu/liwenjun2023150001/chatapplication
gcc -g -Wall -Wextra -o calcserver calcserver.c
gcc -g -Wall -Wextra -o client client.c
```

预期：无 error，生成 `calcserver` 和 `client`。

截图 4：编译成功。

### 2.2 运行服务器

终端 1：

```bash
./calcserver
```

预期：

```text
Server is ready on /home/szu/liwenjun2023150001/chatapplication/data/server_fifo/chat_server_fifo
```

截图 5：服务器启动成功。

### 2.3 运行客户端测试

终端 2：

```bash
./client 12 + 30
./client 50 - 8
./client 7 '*' 6
./client 81 / 9
./client 81 / 0
```

预期：

```text
Received from server: The result is 42
Client <PID> is terminating

Received from server: The result is 42
Client <PID> is terminating

Received from server: The result is 42
Client <PID> is terminating

Received from server: The result is 9
Client <PID> is terminating

Received from server: Error: division by zero
Client <PID> is terminating
```

服务器终端可看到类似：

```text
Client arrived: /home/szu/liwenjun2023150001/chatapplication/data/client_fifo/chat_client12345_fifo, expression: 12 + 30
```

截图 6：客户端测试结果。截图 7：服务器收到请求。

### 2.4 查看 FIFO 文件属性

```bash
ls -l /home/szu/liwenjun2023150001/chatapplication/data/server_fifo/chat_server_fifo
stat /home/szu/liwenjun2023150001/chatapplication/data/server_fifo/chat_server_fifo
```

`ls -l` 第一列应以 `p` 开头，例如：

```text
prw-r--r-- 1 liwenjun2023150001 ... chat_server_fifo
```

字段解释：

- `p`：文件类型是 FIFO/命名管道。
- 权限位：表示该 FIFO 节点的访问权限。
- 链接数：通常为 1。
- owner/group：创建 FIFO 的用户和用户组。
- size：通常为 0，因为 FIFO 不把消息作为普通文件内容保存在磁盘。
- 时间字段：最近修改或状态更新时间。
- 文件名：公共 FIFO 路径。

截图 8：`ls -l` 和 `stat` 结果。

## 3. 第三题：signals-hw2.c 信号实验

### 3.1 编译

```bash
gcc -g -Wall -Wextra -o signals-hw2 signals-hw2.c
```

### 3.2 运行并截图

```bash
./signals-hw2 borkfork
```

预期：连续 3 次 `fork()` 后共有 8 个进程执行 `printf`，输出 8 行 `borked`。

```bash
./signals-hw2 1
```

预期：父进程创建 5 个死循环子进程，然后逐个发送 `SIGINT`；子进程按默认动作异常终止。

```bash
./signals-hw2 2
```

预期：父子进程继承 `SIGINT` 处理函数；子进程收到信号后打印信号号并 `exit(0)` 正常退出。

```bash
./signals-hw2 3
```

预期：由于标准信号不排队，`SIGCHLD` 可能合并；处理函数每次只 `wait` 一个子进程，程序可能只打印少于 5 行后停在 `pause()`，可按 `Ctrl+C` 结束。

```bash
./signals-hw2 4
```

预期：处理函数循环 `wait`，一次信号处理可回收多个子进程，最终打印 5 行并正常退出。

截图 9：五组信号实验结果。

## 4. 第四题：多用户聊天系统

### 4.1 编译

```bash
gcc -g -Wall -Wextra -o chatserver chatserver.c
gcc -g -Wall -Wextra -o chatclient chatclient.c
```

### 4.2 运行服务器

终端 1：

```bash
./chatserver
```

预期：

```text
chatserver is ready
register fifo: /home/szu/liwenjun2023150001/chatapplication/data/server_fifo/FIFO_1
login fifo:    /home/szu/liwenjun2023150001/chatapplication/data/server_fifo/FIFO_2
message fifo:  /home/szu/liwenjun2023150001/chatapplication/data/server_fifo/FIFO_3
```

### 4.3 注册登录两个用户

终端 2：

```bash
./chatclient alice 123456
/register
/login
```

终端 3：

```bash
./chatclient bob 456789
/register
/login
```

预期都能看到：

```text
[server] OK: register ok
[server] OK: login ok
```

### 4.4 发送消息

Alice 终端输入：

```text
/send bob hello bob, this is alice
```

Alice 预期：

```text
[server] OK: message sent to bob
```

Bob 预期：

```text
[alice] hello bob, this is alice
```

截图 10：聊天系统注册、登录、收发消息结果。
