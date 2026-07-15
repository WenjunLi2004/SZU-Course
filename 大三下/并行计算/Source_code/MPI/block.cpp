#include "mpi.h"
#include <iostream>
#include <unistd.h>

using namespace std;

const int n = 1000000; // 当 n 变得极大时，阻塞与非阻塞的差异会极其明显

int main(void) {
    int* a = new int[n];
    int rank;
    MPI_Status status;
    MPI_Request request; // 预留给非阻塞通信的句柄

    MPI_Init(NULL, NULL);
    double t = MPI_Wtime();
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // 核心 0：准备数据并发送
        for (int i = 0; i < n; i++) {
            a[i] = i;
        }

        // // 【阻塞发送】
        // MPI_Send(a, n, MPI_INT, 1, 0, MPI_COMM_WORLD);

        // 1. 【非阻塞发送】：把 a 交给底层去发，瞬间返回！必须绑定 request 句柄
        MPI_Isend(a, n, MPI_INT, 1, 0, MPI_COMM_WORLD, &request);

        cout << "Isend 瞬间脱身，此时耗时仅为: " << MPI_Wtime() - t << " 秒！" << endl;
        // 2. 【等待通信完成】：在修改 a 之前，必须确保底层已经把数据读完并发走了
        MPI_Wait(&request, &status);

        // 发送完后立刻将数组清零
        for (int i = 0; i < n; i++) {
            a[i] = 0;
        }
        cout << "time of send is " << MPI_Wtime() - t << endl;

    } else if (rank == 1) {
        // 核心 1：先睡 2 秒，模拟正在忙其他事情
        sleep(2);

        // // 【阻塞接收】
        // MPI_Recv(a, n, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        // 1. 【非阻塞接收】
        MPI_Irecv(a, n, MPI_INT, 0, 0, MPI_COMM_WORLD, &request);

        // 2. 同样必须等待接收彻底完成，才能去读数据！
        MPI_Wait(&request, &status);

        cout << "time of recv is " << MPI_Wtime() - t << endl;

        // 验证数据正确性
        for (int i = 0; i < n; i++) {
            if (a[i] != i) {
                cout << "error!" << endl;
                break;
            }
        }
    }

    delete[] a;
    MPI_Finalize();
    return 0;
}