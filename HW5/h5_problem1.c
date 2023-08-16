#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

void Count_sort(int a[], int n, int thread_count)
{
	int count, id;
	int disp, cnt;
	int *temp = (int *)malloc(n * sizeof(int));
	
	//count_sort平行化
#pragma omp parallel for num_threads(thread_count) default(none) \
	shared(a, n, temp) private(count) 
	for (int i = 0; i < n; i++) {
		count = 0;
		for (int j = 0; j < n; j++) {
			if (a[j] < a[i])
				count++;
			else if (a[j] == a[i] && j < i)
				count++;
		}
		temp[count] = a[i];
	}

	//memcpy平行化
#pragma omp parallel num_threads(thread_count) default(none) \
	shared(a, n, thread_count, temp) private(id, disp, cnt)
	{
		id = omp_get_thread_num();
		//根據id切割資料複製範圍
		disp = n * id / thread_count;
		cnt = n * (id + 1) / thread_count - disp;
		memcpy(a + disp, temp + disp, cnt * sizeof(int));
	}
	free(temp);
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Wrong arguments, try ./[a.out] [num_count] [thread_count]\n");
		return 0;
	}
	int n = atoi(argv[1]);
	int thread_count = atoi(argv[2]);

	//產生亂數陣列
	srand(time(NULL));
	int *a = (int*)malloc(n * sizeof(int));
	for (int i = 0; i < n; i++) {
		a[i] = rand();
	}

	//計時
	struct timeval start, end;
	gettimeofday(&start, NULL);
	Count_sort(a, n, thread_count);
	gettimeofday(&end, NULL);
	double total =  end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) * 1e-6;

	//印出排序後陣列
	for (int i = 0; i < n; i++) {
		printf("%d ", a[i]);
	}

	printf("\n");
	printf("Execution time = %lf\n", total);
}