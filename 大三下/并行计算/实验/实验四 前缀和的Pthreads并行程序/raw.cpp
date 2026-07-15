#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <cmath>

using namespace std;

int p = 8;
int n = 2e8;
double *a;
double *b;
double *c;
double *offset;
pthread_mutex_t mutex;
pthread_cond_t cond;
int count = 0;
int phase = 0;
void Barrier()
{
    pthread_mutex_lock(&mutex);
    int my_phase = phase;
    count++;
    if (count == p)
    {
        count = 0;
        phase++;
        pthread_cond_broadcast(&cond);
    }
    else
        while (my_phase == phase)
            pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
}
void *Prefix_sum(void *r)
{
    long rank = (long)r;
    int local_n = n / p;
    int local_start = rank * local_n;
    int local_end = (rank + 1) * local_n;
    
    b[local_start] = a[local_start];
    for (int i = local_start + 1; i < local_end; i++)
        b[i] = b[i - 1] + a[i];

    Barrier();

    if (rank == 0)
    {
        offset[0] = 0;
        for (int i = 1; i < p; i++)
            offset[i] = offset[i - 1] + b[i * local_n - 1];
    }

    Barrier();

    if (rank != 0)
    {
        double my_offset = offset[rank];
        for (int i = local_start; i < local_end; i++)
            b[i] += my_offset;
    }

    return NULL;
}
static uint64_t seed = 1;
inline double fast_rand()
{
    seed = seed * 6364136223846793005ULL + 1;
    return (double)(seed >> 32) / (1ULL << 32);
}
int main(int argc, char *argv[])
{
    a = new double[n];
    b = new double[n];
    c = new double[n];
    offset = new double[p];
    for (int i = 0; i < n; i++)
        a[i] = fast_rand() * 2 - 1;
    long thread;
    pthread_t *thread_handles;
    thread_handles = (pthread_t *)malloc(p * sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    struct timespec start, finish;
    timespec_get(&start, TIME_UTC);

    for (thread = 0; thread < p; thread++)
        pthread_create(&thread_handles[thread], NULL, Prefix_sum, (void *)thread);
    for (thread = 0; thread < p; thread++)
        pthread_join(thread_handles[thread], NULL);

    timespec_get(&finish, TIME_UTC);
    cout << "parallel execution time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
    timespec_get(&start, TIME_UTC);
    c[0] = a[0];
    for (int i = 1; i < n; i++)
        c[i] = c[i - 1] + a[i];
    timespec_get(&finish, TIME_UTC);
    cout << "serial execution time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
    double error = 0;
    for (int i = 0; i < n; i++)
        if (abs(b[i] - c[i]) > error)
            error = abs(b[i] - c[i]);
    cout << "max error = " << error << endl;
    delete[] a;
    delete[] b;
    delete[] c;
    delete[] offset;
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    free(thread_handles);
    return 0;
}
