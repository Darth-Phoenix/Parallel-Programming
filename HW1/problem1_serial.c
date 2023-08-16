#include <stdio.h>     // printf()
#include <limits.h>    // UINT_MAX
#include <time.h>

int checkCircuit (int, int);

int main (int argc, char *argv[]) {
    int i;               /* loop variable (32 bits) */
    int id = 0;           /* process id */
    int count = 0;        /* number of solutions */
    clock_t start, end;

    start = clock();
    for (i = 0; i <= USHRT_MAX; i++) {
        count += checkCircuit (id, i);
    }
    end = clock();
    double total = (double)(end - start) / CLOCKS_PER_SEC;
    printf ("Process %d finished in %f secs.\n", id, total);
    fflush (stdout);

    printf("\nA total of %d solutions were found.\n\n", count);
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
