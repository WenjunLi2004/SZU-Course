#include "mpi.h"
#include <iostream>
using namespace std;

int main(void)
{
    const int n = 1000;
    int rank,size;

    MPI_Init(NULL,NULL);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    int local_rows = (n + size - 1) / size;
    int padded_rows = local_rows * size;
    int padded_elements = padded_rows * n;
    int local_n = local_rows * n;


    double *a = new double[padded_elements];
    double *b = new double[n * n];
    double *c = new double[padded_elements];

    if (rank==0)
    {
        srand(0);
        for(int i=0;i<n;i++){
            for(int j=0;j<n;j++){
                a[i * n + j] = rand()*1.0 / RAND_MAX;
                b[i * n + j] = rand()*1.0 / RAND_MAX;
            }
        }
        for(int i = n; i < padded_rows; i++){
            for(int j = 0; j < n; j++){
                a[i * n + j] = 0.0;
            }
        }
    }
    double start = MPI_Wtime();

    double * local_a = new double[local_n];
    double * local_c = new double[local_n];

    MPI_Scatter(a,local_n,MPI_DOUBLE,local_a,local_n,MPI_DOUBLE,0,MPI_COMM_WORLD);

    MPI_Bcast(b,n*n,MPI_DOUBLE,0,MPI_COMM_WORLD);

    for (int i=0;i<local_n;i++)
        local_c[i]=0.0;

    for(int i=0;i<local_n / n;i++)
    {
        for (int k=0;k<n;k++)
        {
            double temp = local_a[i*n+k];
            for(int j=0;j<n;j++)
            {
                local_c[i * n + j] += temp * b[k * n + j];
            }
        }
    }

    MPI_Gather(local_c,local_n,MPI_DOUBLE,c,local_n,MPI_DOUBLE,0,MPI_COMM_WORLD);

    double finish = MPI_Wtime();
    if (rank==0)
    {
        cout<<"execution time = "<<finish - start <<endl;
        for(int i=0 ;i<n;i++)
        {
            for(int j=0;j<n;j++)
            {
                double s =0;
                for (int k=0;k<n;k++)
                    s += a[i*n+k]*b[k*n+j];
                if(s!= c[i*n+j])
                    cout<<"error"<<endl;
            }
        }
    }
    MPI_Finalize();
    delete[] a;
    delete[] b;
    delete[] c;
    return 0;
}