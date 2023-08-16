#include <stdio.h>     // printf()
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

int main (int argc, char *argv[]) {
    int my_rank, comm_sz;
    long long int rcv_num;
    long long int number_in_circle = 0;
    long long int number_of_tosses = 0;
    double startTime = 0.0, totalTime = 0.0;
    srand(time(NULL));

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    if (my_rank==0){
        printf("Number of tosses: ");
        scanf("%lld", &number_of_tosses);
    }

    int m=1;
    while (m < comm_sz){
        if (my_rank < m){
            int dest = my_rank + m;
            if (dest < comm_sz){
                MPI_Send(&number_of_tosses, 1, MPI_LONG_LONG, dest, 0, MPI_COMM_WORLD);
            }
        }
        else if (my_rank < 2*m){
            int src = my_rank - m;
            MPI_Recv(&number_of_tosses, 1, MPI_LONG_LONG, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        m*=2;
    }

    startTime = MPI_Wtime();

    for (long long int i = my_rank; i < number_of_tosses; i+=comm_sz){
        double x = (double)rand()/RAND_MAX*2 - 1;
        double y = (double)rand()/RAND_MAX*2 - 1;
        double distance_squared = x*x + y*y;
        if (distance_squared <= 1) number_in_circle++;
    }

    m = comm_sz;
    int p = m;
    while (p > 1 && my_rank < p){
        m = (m+1) / 2;
        if (my_rank < m){
            int src = my_rank + m;
            if (src < p){
                MPI_Recv(&rcv_num, 1, MPI_LONG_LONG, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                number_in_circle += rcv_num;
            }
        }
        else if (my_rank < p){
            int dest = my_rank - m;
            MPI_Send(&number_in_circle, 1, MPI_LONG_LONG, dest, 0, MPI_COMM_WORLD);
        }
        p = m;
    }

    totalTime = MPI_Wtime() - startTime;
    printf("Process %d finished in time %f secs.\n", my_rank, totalTime);

    fflush (stdout);

    if (my_rank == 0){
        double pi_estimate = 4 * (long double)number_in_circle / number_of_tosses;
        printf("pi is estimated %f.\n", pi_estimate);
    }

    MPI_Finalize();
    return 0;
}