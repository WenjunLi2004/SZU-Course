#ifndef _CLIENTINFO_H
#define _CLIENTINFO_H
typedef struct {
    char myfifo[100]; //client's FIFO name
    int leftarg; // left argument of calculation
    int rightarg; //right argument of calculation
    char op; // operation: + - "*" /
} CLIENTINFO, *CLIENTINFOPTR;

#endif