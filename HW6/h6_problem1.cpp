#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <limits.h>
#include <queue>
#include <omp.h>

using namespace std;
int NC = 80; //計算次數
int M = 8000; //螞蟻數量
float Alpha = 1.4f; 
float Beta = 0.8f; 
float Q = 50.0f;
float evap = 0.25f; //evaporation rate

int main(int argc, char** argv) {
    int size; //城市數量
    int my_rank, comm_sz;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if (argc != 2) {
        if (my_rank == 0) cout << "Wrong arguments! Try ./a.out [filename]" << endl; 
        exit(1);
    }
    char *filename = argv[1];

    int a, cnt=0;
    queue<int> input;
    FILE *fp;
    fp = freopen(filename, "r", stdin);
    while (cin >> a) { //先讀入所有數字
        cnt++;
        input.push(a);
    }
    fclose(fp);
    size = sqrt(cnt); //計算城市數量
    vector<vector<int>> dis(size, vector<int>(size));
    vector<vector<float>> P(size, vector<float>(size));
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) { //城市間距離與機率初始化
            dis[i][j] = input.front();
            input.pop();
            if (i != j) P[i][j] = 10000.0 / dis[i][j];
        }
    }
    
    int *Tglobal = (int*)malloc((size+1) * sizeof(int));
    int Lglobal = INT_MAX;
    srand(my_rank+1); //用每個電腦id當作亂樹種子

    //用4個thread執行模擬蟻窩
    #pragma omp parallel num_threads(4) default(none)\
 shared(Tglobal, Lglobal, size, dis, NC, M, Alpha, Beta, evap, Q, P)
    {      
        int Lbest = INT_MAX;
        vector<int> L(M);
        vector<vector<float>> pheromone(size, vector<float>(size, 1));
        int *Tbest = (int*)malloc((size+1) * sizeof(int));
        int **T = (int**)malloc(M * sizeof(int*));
        for (int i = 0; i < M; i++) {
            T[i] = (int*)malloc((size+1) * sizeof(int));
        }

        for (int t = 0; t < NC; t++) { 
            for (int k = 0; k < M; k++) {
                vector<bool> Sp(size, true); //代表某隻螞蟻有沒有造訪過某個城市
                int city = rand() % size; //隨機初始城市
                T[k][0] = city; //紀錄路徑
                L[k] = 0; //紀錄走過距離
                Sp[city] = false;
                for (int i = 1; i < size; i++) {
                    float total = 0; 
                    int unvisited = 0;
                    for (int j = 0; j < size; j++){
                        if (Sp[j]) { //把沒走過城市都塞到輪盤
                            unvisited++;
                            total += P[city][j];                        
                        }
                    }

                    if ((int)total == 0) { //如果機率總和太小 就隨機取一個
                        int num = rand() % unvisited;
                        for (int j = 0; j < size; j++) {
                            if (Sp[j]) {
                                num--;
                                if (num <= 0) {
                                    city = j;
                                    break;
                                }
                            }
                        }                    
                    }
                    else {                    
                        int num = rand() % (int)total;
                        for (int j = 0; j < size; j++) {
                            if (Sp[j]) {
                                num -= P[city][j];
                                if (num <= 0) {
                                    city = j;
                                    break;
                                }
                            }
                        }
                    }

                    //紀錄走到的下一個城市
                    Sp[city] = false;
                    L[k] += dis[T[k][i-1]][city];
                    T[k][i] = city;
                }
                //回到最一開始的城市
                T[k][size] = T[k][0];
                L[k] += dis[T[k][size-1]][T[k][size]];

                //從這次計算選出每隻螞蟻中最短路徑
                if (L[k] < Lbest) {
                    Lbest = L[k];
                    memcpy(Tbest, T[k], sizeof(int) * (size+1));
                }
            }

            //更新全域最短路徑
            if (Lbest < Lglobal) {
                #pragma omp critical
                {
                    if (Lbest < Lglobal) { //再次檢查 以免race condition
                        Lglobal = Lbest;
                        memcpy(Tglobal, Tbest, sizeof(int) * (size+1));
                    }
                }
            }

            #pragma omp barrier
        
            memcpy(Tbest, Tglobal, sizeof(int) * (size+1));

            //更新費洛蒙濃度
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    pheromone[i][j] *= (1 - evap);
                }
            }

            for (int k = 0; k < M; k++) {
                for (int i = 0; i < size; i++) {
                    pheromone[T[k][i]][T[k][i+1]] += (Q / L[k]);
                }
            }    

            #pragma omp barrier
            
            //更新每個edge選擇的機率
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    if (i != j) P[i][j] = (pow(pheromone[i][j], Alpha) * pow(1.0f / dis[i][j], Beta)) * 10000;
                }
            }

            #pragma omp barrier
        }
    }
    struct {
        int cost;
        int rank;
    } loc_data, global_data;

    loc_data.cost = Lglobal;
    loc_data.rank = my_rank;
    
    MPI_Allreduce(&loc_data, &global_data, 1, MPI_2INT, MPI_MINLOC, MPI_COMM_WORLD);

    if (global_data.rank != 0){
        if (my_rank == 0)
            MPI_Recv(Tglobal, size + 1, MPI_INT, global_data.rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        else if (my_rank == global_data.rank)
            MPI_Send(Tglobal, size + 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    Lglobal = global_data.cost;

    if (my_rank == 0) {
        cout<<"Simulated minimal tour length: " << Lglobal <<endl;
        cout<<"Estimated best tour: " << endl;
        for (int i = 0; i < size; i++){
            cout << Tglobal[i] << " -> ";
        }
        cout << Tglobal[size] << endl;
        free(Tglobal);
    }
    
    MPI_Finalize();
}