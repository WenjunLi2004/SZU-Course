#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mutiplexing.h"
#include <sys/select.h>

int userNumber = 0; // 当前用户量
User users[UserCapacity];

void registerHandler(User*user) {
    char message[BuffSize];
    int ok = 0;
    for (int i = 0; i < userNumber; i++) {
        if (strcmp(user->username, users[i].username) == 0) {
            sprintf(message, "The username has already exited!");
            ok = -1;
            break;
        }
    }
    if (ok == 0) {
        strcpy(users[userNumber].username, user->username);
        strcpy(users[userNumber].password, user->password);
        sprintf(message, "Register succeed!");
        printf("User %s register\n",user->username);
        userNumber++;
    }
    int fd = open(user->fifo, O_RDWR | O_NONBLOCK);
    write(fd, message, BuffSize);
}

void loginHandler(User*user) {
    Response response;
    response.ok = -1;
    for (int i = 0; i < userNumber; i++) {
        if (strcmp(user->username, users[i].username) == 0) {
            if (strcmp(user->password, users[i].password) == 0) {
                strcpy(users[i].fifo, user->fifo);
                sprintf(response.message, "Login succeed!");
                printf("User %s login\n",user->username);
                response.ok = 0;
                break;
            } else {
                sprintf(response.message, "Wrong password!");
                response.ok=-2;
                break;
            }
        }
    }
    if (response.ok == -1) {
        sprintf(response.message, "Wrong username!");
    }
    int fd = open(user->fifo, O_RDWR | O_NONBLOCK);
    write(fd, &response, sizeof(Response));
}

void chatHandler(Chat*chat) {
    char message[BuffSize];
    int ok = -1;
    for (int i = 0; i < userNumber; i++) {
        if (strcmp(chat->targetUser, users[i].username) == 0) {
            int fd = open(users[i].fifo, O_RDWR | O_NONBLOCK);
            write(fd, chat->message, BuffSize);
            sprintf(message, "Send succeed!");
            ok = 0;
            break;
        }
    }
    if (ok == -1) {
        sprintf(message, "User %s does not exit!", chat->targetUser);
    }
    int fd = open(chat->fifo, O_RDWR | O_NONBLOCK);
    write(fd, message, BuffSize);
}

int main() {
    fd_set fds, read_fds; // 文件描述符集合
    int max_fd;
    int register_fd, login_fd, chat_fd;
    // 创建或打开FIFO文件
    mkfifo(Register_FIFO, 0777);
    mkfifo(Login_FIFO, 0777);
    mkfifo(Chat_FIFO, 0777);
    // 打开FIFO文件 O_RDWR 可读写 O_NONBLOCK 非阻塞
    register_fd = open(Register_FIFO, O_RDWR | O_NONBLOCK);
    login_fd = open(Login_FIFO, O_RDWR | O_NONBLOCK);
    chat_fd = open(Chat_FIFO, O_RDWR | O_NONBLOCK);
    // 指定要检查的文件描述符
    FD_ZERO(&fds);
    FD_SET(register_fd, &fds);
    FD_SET(login_fd, &fds);
    FD_SET(chat_fd, &fds);
    // 获取最大文件描述符
    max_fd = register_fd > login_fd ? register_fd : login_fd;
    max_fd = max_fd > chat_fd ? max_fd : chat_fd;
    printf("MutiplexingServer Listening\n");
    while (1) {
        read_fds = fds;
        User registerData;
        User loginData;
        Chat chatData;
        // 使用select监听文件描述符
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // 检查哪些文件描述符已经准备好
        if (FD_ISSET(register_fd, &read_fds)) {
            // 处理注册
            read(register_fd, &registerData, sizeof(User));
            registerHandler(&registerData);
        }
        if (FD_ISSET(login_fd, &read_fds)) {
            // 处理登录
            read(login_fd, &loginData, sizeof(User));
            loginHandler(&loginData);
        }
        if (FD_ISSET(chat_fd, &read_fds)) {
            // 处理聊天
            read(chat_fd, &chatData, sizeof(Chat));
            chatHandler(&chatData);
        }
    }
}
