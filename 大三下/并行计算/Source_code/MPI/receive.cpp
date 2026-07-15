#include "mpi.h"
#include <iostream>
using namespace std;
int main(void) {
    int rank, size;
    MPI_Status status;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0)
        for (int i = 0; i < size - 1; i++)
        {
            int a[20];
            MPI_Recv(a, 20, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            int count;
            MPI_Get_count(&status, MPI_INT, &count);
            cout << "source = " << status.MPI_SOURCE << endl;
            cout << "tag = " << status.MPI_TAG << endl;
            cout << "count = " << count << endl;
            for (int i = 0; i < count; i++)
                cout << a[i] << " ";
            cout << endl << endl;
        }
    else {
        int a[20];
        for (int i = 0; i < 20; i++)
            a[i] = i;
        MPI_Send(a, 10 + rank, MPI_INT, 0, rank, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    return 0;
}
