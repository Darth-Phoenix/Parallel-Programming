#include <stdio.h>     // printf()
#include <stdlib.h>
#include <time.h>

int main (int argc, char *argv[]) {
    int my_rank=0, comm_sz;
    long long int number_in_circle = 0;
    long long int number_of_tosses = 0;
    srand(time(NULL));

    printf("Number of tosses: ");
    scanf("%lld", &number_of_tosses);

    clock_t start, end;
    start = clock();

    for (long long int i = 0; i < number_of_tosses; i++){
        double x = (double)rand()/RAND_MAX*2 - 1;
        double y = (double)rand()/RAND_MAX*2 - 1;
        double distance_squared = x*x + y*y;
        if (distance_squared <= 1) number_in_circle++;
    }

    end = clock();
    double total = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Process %d finished in time %f secs.\n", my_rank, total);

    fflush (stdout);

    double pi_estimate = 4 * (long double)number_in_circle / number_of_tosses;
    printf("pi is estimated %f.\n", pi_estimate);
    return 0;
}