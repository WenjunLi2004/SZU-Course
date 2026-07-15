#ifndef SYSTEMPROGRAM_MESSAGE_H
#define SYSTEMPROGRAM_MESSAGE_H
#define Register_FIFO "register_fifo" // 注册
#define Login_FIFO "login_fifo"       // 登录
#define Chat_FIFO "chat_fifo"         // 聊天
#define BuffSize 100
#define UserCapacity 10 // 可容纳用户量
typedef struct {
    char fifo[BuffSize]; //client's FIFO name
    char username[20];
    char password[20];
} User;
typedef struct {
    char fifo[BuffSize]; //client's FIFO name
    char targetUser[20]; // 向谁发送信息
    char message[BuffSize];
} Chat;
typedef struct{
   int ok;
   char message[BuffSize];
}Response;
#endif //SYSTEMPROGRAM_MESSAGE_H
