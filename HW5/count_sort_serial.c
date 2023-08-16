#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

void Count_sort(int a[], int n) { 
    int i, j, count;
    int* temp = malloc(n*sizeof(int));
    for (i = 0; i < n; i++) { 
        count = 0; 
        for (j = 0; j < n; j++) 
            if (a[j] < a[i]) count++; 
        else if (a[j] == a[i] && j < i)
            count++;
        temp[count] = a[i]; 
    } 
    memcpy(a, temp, n*sizeof(int)); 
    free(temp); 
} 

int main(int argc, char *argv[])
{
	if (argc != 2) {
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
	Count_sort(a, n);
	gettimeofday(&end, NULL);
	double total =  end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) * 1e-6;

	for (int i = 0; i < n; i++) {
		printf("%d ", a[i]);
	}

	printf("\n");
	printf("The execution time = %lf\n", total);
}