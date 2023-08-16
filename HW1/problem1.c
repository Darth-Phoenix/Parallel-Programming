#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include <mpi.h>

int checkCircuit (int, int);

int main (int argc, char *argv[]) {
    int my_rank, comm_sz;
    int count=0, rcv_num=0;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    double startTime = 0.0, totalTime = 0.0;
    startTime = MPI_Wtime();

    for (int i = my_rank; i <= USHRT_MAX; i+=comm_sz) {
        count += checkCircuit (my_rank, i);
    }

    int m = comm_sz;
    int p = m;
    while (p > 1 && my_rank < p){
        m = (m+1)/2;
        if (my_rank < m){
            int src = my_rank + m;
            if (src < p){
                MPI_Recv(&rcv_num, 1, MPI_INT, src, src, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                count += rcv_num;
            }
        }
        else if (my_rank < p){
            int dest = my_rank - m;
            MPI_Send(&count, 1, MPI_INT, dest, my_rank, MPI_COMM_WORLD);
        }
        p = m;
    }

    totalTime = MPI_Wtime() - startTime;
    printf("Process %d finished in time %f secs.\n", my_rank, totalTime);

    fflush (stdout);

    if (my_rank == 0){
        printf("\nA total of %d solutions were found.\n\n", count);
    }

    MPI_Finalize();
    return 0;
}

#define EXTRACT_BIT(n,i) ( (n & (1<<i) ) ? 1 : 0)
#define SIZE 16

int checkCircuit (int id, int bits) {
    int v[SIZE];        /* Each element is a bit of bits */
    int i;

    for (i = 0; i < SIZE; i++)
        v[i] = EXTRACT_BIT(bits,i);

    if ( (v[0] || v[1]) && (!v[1] || !v[3]) && (v[2] || v[3])
        && (!v[3] || !v[4]) && (v[4] || !v[5])
        && (v[5] || !v[6]) && (v[5] || v[6])
        && (v[6] || !v[15]) && (v[7] || !v[8])
        && (!v[7] || !v[13]) && (v[8] || v[9])
        && (v[8] || !v[9]) && (!v[9] || !v[10])
        && (v[9] || v[11]) && (v[10] || v[11])
        && (v[12] || v[13]) && (v[13] || !v[14])
        && (v[14] || v[15]) )
    {
        printf ("%d) %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d \n", id,
            v[15],v[14],v[13],v[12],
            v[11],v[10],v[9],v[8],v[7],v[6],v[5],v[4],v[3],v[2],v[1],v[0]);
        fflush (stdout);
        return 1;
    }
    else
    {
        return 0;
    }
}