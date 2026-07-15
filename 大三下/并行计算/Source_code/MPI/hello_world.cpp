#include <stdio.h>
#include <string.h>  /* 为了使用strlen    */
#include <mpi.h>     /* 为了使用MPI函数等 */
const int MAX_STRING = 100;
int main(void) {
    char greeting[MAX_STRING];
    int comm_sz;   /* 进程的数量     */
    int my_rank;   /* 我的进程序列号 */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if (my_rank != 0) {
        sprintf(greeting, "Greetings from process %d of %d!", my_rank, comm_sz);
        greeting[strlen(greeting)]='s';
        MPI_Send(greeting, strlen(greeting), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }
    else {
        printf("Greetings from process %d of %d!\n", my_rank, comm_sz);
        for (int q = 1; q < comm_sz; q++) {
            MPI_Recv(greeting, MAX_STRING, MPI_CHAR, q, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("%s\n", greeting);
        }
    }
    MPI_Finalize();
    return 0;
}
