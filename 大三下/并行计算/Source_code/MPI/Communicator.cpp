#include "mpi.h"
#include <iostream>
#include <cmath>
// #include <windows.h>
using namespace std;
int main(void){
    int rank, comm_sz;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    int n = sqrt(comm_sz);
    MPI_Comm rowComm;
    MPI_Comm_split(MPI_COMM_WORLD, rank / n, rank % n, &rowComm);
    int rowRank, data = 0;
    MPI_Comm_rank(rowComm, &rowRank);
    if (rowRank == 0)
        data = rank * rank;
    MPI_Bcast(&data, 1, MPI_INT, 0, rowComm);
    // Sleep(rank * 100);
    cout << "P" << rank << ":\t" << data;
    fflush(stdout);
    MPI_Finalize();
    return 0;
}
