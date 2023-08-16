#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

int cmp(const void *a, const void *b)
{
	return *(int *)a > *(int *)b;
}


int main(int argc, char *argv[])
{
	if (argc !=2) {
		printf("Wrong arguments, try ./[a.out] [num_count]\n");
		return 0;
	}
	int n = atoi(argv[1]);

	srand(time(NULL));
	int *a = (int*)malloc(n * sizeof(int));
	for (int i = 0; i < n; i++) {
		a[i] = rand();
	}

	struct timeval start, end;
	gettimeofday(&start, NULL);
	qsort(a, n, sizeof(int), cmp);
	gettimeofday(&end, NULL);
	double total =  end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) * 1e-6;

	for (int i = 0; i < n; i++) {
		printf("%d ", a[i]);
	}

	printf("\n");
	printf("The execution time = %lf\n", total);
}