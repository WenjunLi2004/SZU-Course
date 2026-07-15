#include<unistd.h>
#include<stdio.h>
#include <stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include"mutiplexing.h"

void showPage(User*user);

void registerClient(User*user){
    int register_fd = open(Register_FIFO, O_RDWR | O_NONBLOCK);
    write(register_fd, user, sizeof(User));
    int fd=open(user->fifo,O_RDWR | O_NONBLOCK);
    char message[BuffSize];
    while(1){
        int result= read(fd,message,BuffSize);
        if(result>0){
            printf("%s\n",message);
            break;
        }
    }
    showPage(user);
}
void chatClient(User*user){
    char what;
    printf("Chat Page:\npress r for receive | s for send | q for quit\n");
    while((what=getchar())=='\n'){}
    if (what == 'q') {
        exit(0);
    } else if(what =='s') { // 发送信息
        Chat chat;
        strcpy(chat.fifo,user->fifo);
        printf("send username: ");
        scanf("%s",chat.targetUser);
        getchar();
        printf("send data: ");
        scanf("%[^\n]",chat.message);
        int chat_fd=open(Chat_FIFO,O_RDWR | O_NONBLOCK);
        write(chat_fd,&chat,sizeof(Chat));
        // 等待服务器响应
        int fd=open(user->fifo,O_RDWR | O_NONBLOCK);
        char message[BuffSize];
        while(1){
            int result= read(fd,message,BuffSize);
            if(result>0){
                printf("%s\n",message);
                break;
            }
        }
    } else if(what=='r'){ //接收消息
        int fd=open(user->fifo,O_RDWR | O_NONBLOCK);
        char message[BuffSize];
        while(1){
            int result= read(fd,message,BuffSize);
            if(result>0){
                printf("%s\n",message);
                break;
            }
        }
    }else{
        printf("Wrong press key, please try again!\n");
    }
    chatClient(user);
}
void loginClient(User*user){
    int login_fd = open(Login_FIFO, O_RDWR | O_NONBLOCK);
    write(login_fd, user, sizeof(User));
    int fd=open(user->fifo,O_RDWR | O_NONBLOCK);
    Response response;
    while(1){
        int result= read(fd,&response,sizeof(Response));
        if(result>0){
            printf("%s\n",response.message);
            break;
        }
    }
    if(response.ok!=0){ // 登录失败
        showPage(user);
    }else{
        chatClient(user);
    }
}

void showPage(User*user){
    char what;
    printf("Chat Client:\npress r for register | l for login | q for quit\n");
    while((what=getchar())=='\n'){}
    if (what == 'q') {
        exit(0);
    } else if(what=='r'||what=='l') {
        printf("username: ");
        scanf("%s", user->username);
        printf("password: ");
        scanf("%s", user->password);
    }else{
        printf("Wrong press key, please try again!\n");
        showPage(user);
    }
    if (what == 'r') {
        registerClient(user);
    }else if(what=='l'){
        loginClient(user);
    }
}
int main() {
    User user;
    sprintf(user.fifo, "client_fifo/client_fifo%d", getpid());
    mkfifo(user.fifo, 0777);
    showPage(&user);
    exit(0);
}
