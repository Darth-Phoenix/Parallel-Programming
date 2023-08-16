#include <stdio.h>    
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

int cmp(const void *a, const void *b){
    return *(int*)a > *(int*)b;
}

int compute_partner(int phase, int id, int size){
    int partner;
    if (phase % 2 == 0){ //even phase
        if (id % 2 != 0) //odd rank
            partner = id - 1; 
        else //even rank
            partner = id + 1;
    }
    else { //even phase
        if (id % 2 != 0) //odd rank
            partner = id + 1; 
        else //even rank
            partner = id - 1;
    }

    if (partner == -1 || partner == size) //partner超出process範圍
        partner = MPI_PROC_NULL;
    
    return partner;
}

void merge(int *arr, int *arr1, int *arr2, int size){
    int i=0, j=0, k=0;
    while(i < size && j < size){ //當兩個陣列都還有剩餘元素
        //把小的數值先填進代表合併後的陣列
        if (arr1[i] < arr2[j]){
            arr[k++] = arr1[i++];
        }
        else {
            arr[k++] = arr2[j++];
        }
    }
    while (i < size){ //若第一個陣列還有元素且第二個陣列已經填完
        arr[k++] = arr1[i++];
    }
    while (j < size){ //若第二個陣列還有元素且第一個陣列已經填完
        arr[k++] = arr2[j++];
    }
    return;
}

int main(int argc,char *argv[]){
    int my_rank, comm_sz;
    int n, phase, partner;
    double startwtime = 0.0, endwtime = 0.0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    if (my_rank == 0){
        printf("Number of keys: ");
        scanf("%d", &n);
        printf("Each process gets %d keys respectively.\n", n / comm_sz);
    }

    //記錄開始時間 
    MPI_Barrier(MPI_COMM_WORLD);
    startwtime = MPI_Wtime();

    //傳送資訊
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //亂數種子
    srand(my_rank+1);

    //設定每個process陣列大小，產生一些陣列暫存器
    int local_size = n / comm_sz;
    int *local_array = (int*)malloc(sizeof(int) * local_size);
    int *buffer_array = (int*)malloc(sizeof(int) * local_size);
    int *merged_array = (int*)malloc(sizeof(int) * local_size * 2);
    int *global_array = NULL;

    //每個process產生(n/comm_sz)個數字的陣列
    for (int i = 0; i < local_size; i++){
        local_array[i] = rand();
    }

    //local sort
    qsort(local_array, local_size, sizeof(int), cmp);

    if (my_rank == 0){
        global_array = (int*)malloc(sizeof(int) * local_size * comm_sz);
    }

    //得到local sorted array
    MPI_Gather(local_array, local_size, MPI_INT, global_array, local_size, MPI_INT , 0, MPI_COMM_WORLD);

    //印出local sorted array
    if (my_rank == 0){
        printf("Local sorted lists:\n");
        for (int i = 0; i < comm_sz; i++){
            printf("Process %d: ", i);
            for (int j = i * local_size; j < (i+1) * local_size; j++){
                printf("%d ", global_array[j]);
            }
            printf("\n");
        }
    }

    //merge phase
    for (phase = 0; phase < comm_sz; phase++){
        //尋找要merge的partner
        partner = compute_partner(phase, my_rank, comm_sz);
        if (partner == MPI_PROC_NULL) continue;

        //把rank較大的process資料傳給rank小的prcoess計算，最後再把merge完比較大的keys取回
        if (my_rank > partner){ 
            //rank較大的process
            MPI_Send(local_array, local_size, MPI_INT, partner, 0, MPI_COMM_WORLD);
            MPI_Recv(local_array, local_size, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        else{
            //rank較小的process
            MPI_Recv(buffer_array, local_size, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            merge(merged_array, local_array, buffer_array, local_size);
            MPI_Send(&merged_array[local_size], local_size, MPI_INT, partner, 0, MPI_COMM_WORLD); //傳送較大的keys回去
            for (int i = 0; i < local_size; i++){ 
                local_array[i] = merged_array[i]; //複製較小的keys到自己的暫存器
            }
        }
    }

    //得到gloabl sorted array
    MPI_Gather(local_array, local_size, MPI_INT, global_array, local_size, MPI_INT , 0, MPI_COMM_WORLD);

    //得到結束時間
    MPI_Barrier(MPI_COMM_WORLD);
    endwtime = MPI_Wtime();

    //印出gloabl sorted array，並印出時間
    if (my_rank == 0){
        printf("Global sorted list:\n");
        for (int i = 0; i < local_size * comm_sz; i++){
            printf("%d ", global_array[i]);
        }
        printf("\n");
        printf("The execution time = %f\n", endwtime - startwtime); 
        free(global_array);
    }

    free(local_array);
    free(buffer_array);
    free(merged_array);

    MPI_Finalize();

    return 0;
}
