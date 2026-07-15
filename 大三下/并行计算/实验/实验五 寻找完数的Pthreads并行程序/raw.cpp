#include <iostream>
#include <pthread.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
using namespace std;
const int MAX = 100;
int p = 8;
int n = 1000000;
int a[MAX];
int num = 0;
pthread_mutex_t mutex;

void *Perfect_number(void *r)
{
	long rank = (long)r;
	int total = n - 1;
	int start = 2 + rank * total / p;
	int end = 1 + (rank + 1) * total / p;
	int len = end - start + 1;

	if (len <= 0)
		return NULL;

	int *sum = new int[len]();
	int local_a[MAX];
	int local_num = 0;

	for (int d = 1; d <= end / 2; d++)
	{
		int first = ((start + d - 1) / d) * d;

		if (first < 2 * d)
			first = 2 * d;

		for (int x = first; x <= end; x += d)
			sum[x - start] += d;
	}

	for (int x = start; x <= end; x++)
		if (sum[x - start] == x && local_num < MAX)
			local_a[local_num++] = x;

	pthread_mutex_lock(&mutex);
	for (int i = 0; i < local_num && num < MAX; i++)
		a[num++] = local_a[i];
	pthread_mutex_unlock(&mutex);

	delete[] sum;
	return NULL;
}
int main(int argc, char *argv[])
{

	pthread_t *thread_handles = (pthread_t *)malloc(p * sizeof(pthread_t));
	pthread_mutex_init(&mutex, NULL);

	struct timespec start, finish;
	timespec_get(&start, TIME_UTC);

	for (long thread = 0; thread < p; thread++)
		pthread_create(&thread_handles[thread], NULL, Perfect_number, (void *)thread);
	for (long thread = 0; thread < p; thread++)
		pthread_join(thread_handles[thread], NULL);

	timespec_get(&finish, TIME_UTC);
	sort(a, a + num);

	cout << "parallel execution time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
	for (int i = 0; i < num; i++)
		cout << a[i] << " ";
	cout << endl;

	pthread_mutex_destroy(&mutex);
	free(thread_handles);
	return 0;
}
