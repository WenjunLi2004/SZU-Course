# openEuler Docker 环境下守护进程与 `rsyslog` 实验

为了高效利用系统资源，课程改用 Docker 容器的 openEuler 系统作为学生实验环境，由于实验环境不是完整的 `systemd` ，因此 `rsyslog` 的启动方式与普通虚拟机或物理机有所不同。现给出指导步骤。

## 项目简介

本项目基于 `aarch64` 架构的 `openEuler 24.03 (LTS-SP2)` Docker 容器，完成如下实验任务：

- 将给定程序修改为可在当前系统中编译运行的守护进程程序
- 配置 `rsyslog`，将用户级日志写入 `/var/log/test.log`
- 编译并运行程序
- 使用 `ps -ef` 查看进程状态
- 结合进程状态说明该程序为何属于守护进程
- 给出系统日志结果片段

## 实验准备

1. 安装 `rsyslog`

执行以下命令安装 `rsyslog`：
```
sudo dnf install -y rsyslog
```

安装完成后，可执行以下命令检查是否安装成功：
```
which rsyslogd
```
```
rpm -q rsyslog
```
2. Docker 容器环境说明
在本实验环境中，执行以下命令：
```bash
systemctl status rsyslog
```
通常会报错：`System has not been booted with systemd as init system`。这是因为当前实验平台是 Docker 容器，`PID 1` 不是 `systemd`。因此，本实验不能使用 `systemctl` 管理 `rsyslog`，而应采用手动启动 `rsyslogd` 的方式。

## 配置 `rsyslog`
1. 修改配置文件 `/etc/rsyslog.conf`
```
sudo vi /etc/rsyslog.conf
```
在文件末尾加入以下内容：
```
user.*    /var/log/test.log
```
该规则表示所有 `user` 类别日志全部写入 `/var/log/test.log`

2. 修正本地日志接收方式

在本实验环境中，`/etc/rsyslog.conf` 中原本存在如下配置：

```
module(load="imuxsock"
       SysSock.Use="off")
       
module(load="imjournal"
       StateFile="/run/log/imjournal.state")
```
这表示关闭本地日志 `socket` 接收，并尝试通过 `imjournal` 从 `journald` 中读取本地日志。

但是当前实验平台是 Docker 容器，没有完整的 `systemd/journald` 环境，因此会导致：
- `logger` 发出的本地日志无法被 `rsyslogd` 接收
- 程序写出的本地日志也无法进入日志文件

因此，需要将其修改为：
```
module(load="imuxsock")

module(load="imjournal"
       StateFile="/run/log/imjournal.state")
```
也就是删除`SysSock.Use="off"`，从而重新启用本地日志 socket 接收功能。

## 启动 `rsyslog`
由于 Docker 容器中不能使用 `systemctl`，本实验采用手动启动方式。

1. 启动命令
```
sudo rsyslogd -i /run/rsyslogd.pid
```
2. 检查是否启动成功
```
ps -ef | grep rsyslogd
```
示例输出：
```
root        1952       1  0 13:10 ?        00:00:00 rsyslogd -i /run/rsyslogd.pid
```
3. 若需重启 rsyslogd
```
sudo pkill rsyslogd
sudo rm -f /run/rsyslogd.pid
sudo rm -f /var/run/rsyslogd.pid
sudo rsyslogd -i /run/rsyslogd.pid
```
## 验证 `rsyslog` 配置
在运行守护进程程序之前，可先使用 `logger` 测试日志链路是否正常。

1. 测试命令
```
logger -p user.info "hello test"
tail -n 20 /var/log/test.log
```
2. 示例输出
```
Apr 20 13:10:18 euler-container-30047 szu: hello test
```
3. 结果说明
若能看到该输出，说明：
- `rsyslogd` 已正常运行
- `user.* /var/log/test.log` 规则已生效
- 本地日志 `socket` 接收已恢复正常

## 编译与运行
1. 编译程序
```
gcc daemon.c -o daemon -Wall
```
2. 运行程序
```
./daemon
```
程序执行后命令会立即返回，这是正常现象。因为父进程退出后，子进程已经作为后台守护进程继续运行。

## 进程状态查看
1. 查看进程
```
ps -ef | grep daemon
```
示例输出：
```
szu         2151       1  0 13:12 ?        00:00:00 ./daemon
```
2. 查看详细状态
```
ps -o pid,ppid,tty,stat,cmd -C daemon
```
示例输出：
```
PID    PPID TT       STAT CMD
2151      1 ?        S    ./daemon
```
从上述结果可看出：
- `PPID=1`：说明原父进程已经退出，当前进程被 `PID 1` 接管
- `TTY=?`：说明该进程没有控制终端，已经脱离终端运行
- `STAT=S`：说明该进程当前处于可中断睡眠状态，这是因为程序循环中执行了 sleep(8)
由于该进程在后台运行、脱离终端、父进程退出后被系统接管，因此属于守护进程。

3. 日志结果查看
执行：
```
tail -n 20 /var/log/test.log
```
示例输出：
```
Apr 20 13:12:12 euler-container-30047 daemon[2151]: 测试守护进程!
Apr 20 13:12:20 euler-container-30047 daemon[2151]: 系统时间: Mon Apr 20 13:12:20 2026
Apr 20 13:12:28 euler-container-30047 daemon[2151]: 系统时间: Mon Apr 20 13:12:28 2026
```
从日志结果可知：
- 程序启动后成功写入`测试守护进程!`日志
- 程序每隔 8 秒写入一次系统时间
- 守护进程已在后台持续运行
- `rsyslogd` 已成功接收程序日志并写入 `/var/log/test.log`

