#include <cstdlib>
#include <iostream>
#include <cstring>
#include <map>
#include <fstream>
#include <string>
#include <vector>
#include <omp.h>
#include <queue>
#include <sys/time.h>
#include <sstream>
using namespace std;

#define BUF_SIZE 1024

map<string, int> keywords;
vector<string> files;
queue<string> shared_queue;

void Producer(char* filename)
{
    //從給定filename一行一行讀取並存到shared_queue
    fstream fp;
    char buf[BUF_SIZE];
    fp.open(filename, ios::in);
    while (fp.getline(buf, sizeof(buf))) {
#pragma omp critical
        shared_queue.push(buf);
    }
}

void Consumer()
{
    string line="";
#pragma omp critical
    {
        //讀取shared_queue的front並pop
        if (!shared_queue.empty()) {
            line = shared_queue.front();
            shared_queue.pop();
        }
    }
    //避免其他consumer先進入crtical section而造成consumer進入critical section時shared_queue已經是空的
    if (line == "") return;
    string token;
    stringstream check(line);

    //tokenize一個line，並用map來紀錄目標字串的出現次數
    while(getline(check, token, ' ')) {
        auto u = keywords.find(token);
        if (u != keywords.end()) {
#pragma omp critical
            u->second++;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cout << "Wrong arguments, try ./[a.out] [producer_count] [consumer_count]\n" << endl;
        return 0;
    }

    int producer_cnt = atoi(argv[1]);
    int consumer_cnt = atoi(argv[2]);
    

    char buf[BUF_SIZE];
    int finished = 0;
    int id;
    string s;
    fstream fp;

    //讀取在keyword.txt裡面的keywords
    fp.open("keyword.txt", ios::in);
    while (fp >> s) {
        keywords[s] = 0;
    }
    fp.close(); 
    
    //開始計時
    struct timeval start, end;
    gettimeofday(&start, NULL);

    //將producer和consumer平行運算
#pragma omp parallel num_threads(producer_cnt + consumer_cnt) default(none) \
    shared(shared_queue, finished, producer_cnt) private(id, s, buf)
    {
        id = omp_get_thread_num();
        //producer
        if (id < producer_cnt) {
            //將producer以cyclic的方式讀取files/0~15.txt
            for (int i = id; i < 16; i += producer_cnt) {
                sprintf(buf, "files/%d.txt", i);
                Producer(buf);
            }
#pragma omp atomic
            finished++;
        }
        //consumer
        else {
            //當還沒有把所有檔案處理完就一直卡在while迴圈
            while (finished < producer_cnt || shared_queue.size()) {
                //如果shared_queue沒有東西就繼續wait
                if (shared_queue.empty())
                    continue;
                Consumer();
            }
        }
    }

    //結束計時
    gettimeofday(&end, NULL);
    double total = end.tv_sec - start.tv_sec + (end.tv_usec - start.tv_usec) * 1e-6;

    //讀取keywords出現次數
    for (auto u : keywords) {
        cout << u.first << ": " << u.second << endl;
    }
    cout << "Execution time = " << total << endl;
}