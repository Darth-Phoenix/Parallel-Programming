#include <mpi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include "bmp.h"

using namespace std;

//定義平滑運算的次數
#define NSmooth 1000

/*********************************************************/
/*變數宣告：                                             */
/*  bmpHeader    ： BMP檔的標頭                          */
/*  bmpInfo      ： BMP檔的資訊                          */
/*  **BMPSaveData： 儲存要被寫入的像素資料               */
/*  **BMPData    ： 暫時儲存要被寫入的像素資料           */
/*********************************************************/
BMPHEADER bmpHeader;
BMPINFO bmpInfo;
RGBTRIPLE **BMPSaveData = NULL;
RGBTRIPLE **BMPData = NULL;

/*********************************************************/
/*函數宣告：                                             */
/*  readBMP    ： 讀取圖檔，並把像素資料儲存在BMPSaveData*/
/*  saveBMP    ： 寫入圖檔，並把像素資料BMPSaveData寫入  */
/*  **alloc_memory： 動態分配一個Y * X矩陣               */
/*********************************************************/
int readBMP( char *fileName);        //read file
int saveBMP( char *fileName);        //save file
void swap(RGBTRIPLE *a, RGBTRIPLE *b);
void edge_reg(RGBTRIPLE **data, int height, int width, int id, int size);
RGBTRIPLE **alloc_memory( int Y, int X );        //allocate memory

//處理邊界暫存器
void edge_reg(RGBTRIPLE **data, int height, int width, int id, int size){
    int upper = (id + size - 1) % size;
    int lower = (id + 1) % size;
    if (size == 1){
        for (int i = 0; i < width; i++){
            data[0][i] = data[height][i];
            data[height + 1][i] = data[1][i];
        }
        return;
    }

    //將下界傳給下個process上界
    MPI_Send(data[height], width * 3, MPI_CHAR, lower, 0, MPI_COMM_WORLD);
    MPI_Recv(data[0], width * 3, MPI_CHAR, upper, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //將下上界傳給上個process上界
    MPI_Send(data[1], width * 3, MPI_CHAR, upper, 0, MPI_COMM_WORLD);
    MPI_Recv(data[height + 1], width * 3, MPI_CHAR, lower, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return;
}

int main(int argc,char *argv[])
{
/*********************************************************/
/*變數宣告：                                             */
/*  *infileName  ： 讀取檔名                             */
/*  *outfileName ： 寫入檔名                             */
/*  startwtime   ： 記錄開始時間                         */
/*  endwtime     ： 記錄結束時間                         */
/*********************************************************/
    int my_rank, comm_sz;
    char *infileName = "input.bmp";
    char *outfileName = "output.bmp";
    double startwtime = 0.0, endwtime = 0.0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    int *send_shape = (int*)malloc(sizeof(int) * (2*comm_sz));
    int *recv_shape = (int*)malloc(sizeof(int) * 2);
    int *send_cnt = (int*)malloc(sizeof(int) * comm_sz);
    int *displs = (int*)malloc(sizeof(int) * comm_sz);
    for (int i = 0; i < comm_sz; i++){
        send_cnt[i] = 2;
        displs[i] = 2*i;
    }

    //讀取檔案, 準備傳送圖片height, width資訊
    if (my_rank == 0){
        if ( readBMP( infileName) )
            cout << "Read file successfully!!" << endl;
        else
            cout << "Read file fails!!" << endl;

        for (int i = 0; i < 2*comm_sz; i++){
            if (i % 2 == 0){
                send_shape[i] = bmpInfo.biHeight;
            }
            else {
                send_shape[i] = bmpInfo.biWidth;
            }
        }
    }

    //記錄開始時間 
    MPI_Barrier(MPI_COMM_WORLD);
    startwtime = MPI_Wtime();

    //傳送圖片height, width資訊
    MPI_Scatterv(send_shape, send_cnt, displs, MPI_INT, recv_shape, 2, MPI_INT, 0, MPI_COMM_WORLD);

    //切割圖片, 計算每個process計算區間
    int low = recv_shape[0] * my_rank / comm_sz;
    int high = recv_shape[0] * (my_rank+1) / comm_sz;

    int local_height = high - low;
    int width = recv_shape[1];

    //傳送像素資料給其他process
    if (my_rank == 0){        
        for (int i = 0; i < comm_sz; i++){
            send_cnt[i] = (recv_shape[0] * (i+1) / comm_sz - recv_shape[0] * i / comm_sz) * width * 3;
            displs[i] =  (i==0)? 0 : displs[i-1] + send_cnt[i-1];
        }
    }
    else {
        BMPSaveData = alloc_memory(local_height + 2, width);
    }
    
    //分割圖片
    BMPData = alloc_memory(local_height + 2, width);
    MPI_Scatterv(*BMPSaveData, send_cnt, displs, MPI_CHAR, BMPData[1], local_height * width * 3, MPI_CHAR, 0, MPI_COMM_WORLD);

    //進行多次的平滑運算
    for(int count = 0; count < NSmooth ; count ++){     
		//設定邊界暫存器
        edge_reg(BMPData, local_height, width, my_rank, comm_sz);
		//進行平滑運算
		for(int i = 1; i <= local_height ; i++){
			for(int j = 0; j < width; j++){
				/*********************************************************/
				/*設定上下左右像素的位置                                 */
				/*********************************************************/
				int Top =  i-1;
                int Down = i+1;
				int Left = j>0 ? j-1 : width-1;
				int Right = j<width-1 ? j+1 : 0;
				/*********************************************************/
				/*與上下左右像素做平均，並四捨五入                       */
				/*********************************************************/
				BMPSaveData[i][j].rgbBlue =  (double) (BMPData[i][j].rgbBlue+BMPData[Top][j].rgbBlue+BMPData[Top][Left].rgbBlue+BMPData[Top][Right].rgbBlue+BMPData[Down][j].rgbBlue+BMPData[Down][Left].rgbBlue+BMPData[Down][Right].rgbBlue+BMPData[i][Left].rgbBlue+BMPData[i][Right].rgbBlue)/9+0.5;
				BMPSaveData[i][j].rgbGreen =  (double) (BMPData[i][j].rgbGreen+BMPData[Top][j].rgbGreen+BMPData[Top][Left].rgbGreen+BMPData[Top][Right].rgbGreen+BMPData[Down][j].rgbGreen+BMPData[Down][Left].rgbGreen+BMPData[Down][Right].rgbGreen+BMPData[i][Left].rgbGreen+BMPData[i][Right].rgbGreen)/9+0.5;
				BMPSaveData[i][j].rgbRed =  (double) (BMPData[i][j].rgbRed+BMPData[Top][j].rgbRed+BMPData[Top][Left].rgbRed+BMPData[Top][Right].rgbRed+BMPData[Down][j].rgbRed+BMPData[Down][Left].rgbRed+BMPData[Down][Right].rgbRed+BMPData[i][Left].rgbRed+BMPData[i][Right].rgbRed)/9+0.5;
			}
		}
        swap(BMPSaveData, BMPData);
    }

    //合併圖片
    MPI_Gatherv(BMPData[1], local_height * width * 3, MPI_CHAR, *BMPSaveData, send_cnt, displs, MPI_CHAR, 0, MPI_COMM_WORLD);

    //得到結束時間
    MPI_Barrier(MPI_COMM_WORLD);
    endwtime = MPI_Wtime();

    //寫入檔案，並印出執行時間
    if (my_rank == 0){    
        cout << "The execution time = "<< endwtime-startwtime <<endl ; 
        if ( saveBMP( outfileName ) )
            cout << "Save file successfully!!" << endl;
        else
            cout << "Save file fails!!" << endl;
    }

    free(BMPData[0]);
    free(BMPSaveData[0]);
    free(BMPData);
    free(BMPSaveData);
    MPI_Finalize();

    return 0;
}

/*********************************************************/
/* 讀取圖檔                                              */
/*********************************************************/
int readBMP(char *fileName)
{
    //建立輸入檔案物件
    ifstream bmpFile( fileName, ios::in | ios::binary );

    //檔案無法開啟
    if ( !bmpFile ){
        cout << "It can't open file!!" << endl;
        return 0;
    }

    //讀取BMP圖檔的標頭資料
    bmpFile.read( ( char* ) &bmpHeader, sizeof( BMPHEADER ) );

    //判決是否為BMP圖檔
    if( bmpHeader.bfType != 0x4d42 ){
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //讀取BMP的資訊
    bmpFile.read( ( char* ) &bmpInfo, sizeof( BMPINFO ) );

    //判斷位元深度是否為24 bits
    if ( bmpInfo.biBitCount != 24 ){
        cout << "The file is not 24 bits!!" << endl;
        return 0;
    }

    //修正圖片的寬度為4的倍數
    while( bmpInfo.biWidth % 4 != 0 )
        bmpInfo.biWidth++;

    //動態分配記憶體
    BMPSaveData = alloc_memory(bmpInfo.biHeight + 2, bmpInfo.biWidth);

    //讀取像素資料
    //for(int i = 0; i < bmpInfo.biHeight; i++)
    //	bmpFile.read( (char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE));
    bmpFile.read( (char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight);

    //關閉檔案
    bmpFile.close();

    return 1;

}
/*********************************************************/
/* 儲存圖檔                                              */
/*********************************************************/
int saveBMP( char *fileName)
{
    //判決是否為BMP圖檔
    if( bmpHeader.bfType != 0x4d42 ){
        cout << "This file is not .BMP!!" << endl ;
        return 0;
    }

    //建立輸出檔案物件
    ofstream newFile( fileName,  ios:: out | ios::binary );

    //檔案無法建立
    if ( !newFile ){
        cout << "The File can't create!!" << endl;
        return 0;
    }

    //寫入BMP圖檔的標頭資料
    newFile.write( ( char* )&bmpHeader, sizeof( BMPHEADER ) );

    //寫入BMP的資訊
    newFile.write( ( char* )&bmpInfo, sizeof( BMPINFO ) );

    //寫入像素資料
    //for( int i = 0; i < bmpInfo.biHeight; i++ )
    //        newFile.write( ( char* )BMPSaveData[i], bmpInfo.biWidth*sizeof(RGBTRIPLE) );
    newFile.write( ( char* )BMPSaveData[0], bmpInfo.biWidth*sizeof(RGBTRIPLE)*bmpInfo.biHeight );

    //寫入檔案
    newFile.close();

    return 1;

}


/*********************************************************/
/* 分配記憶體：回傳為Y*X的矩陣                           */
/*********************************************************/
RGBTRIPLE **alloc_memory(int Y, int X )
{
	//建立長度為Y的指標陣列
    RGBTRIPLE **temp = new RGBTRIPLE *[ Y ];
    RGBTRIPLE *temp2 = new RGBTRIPLE [ Y * X ];
    memset( temp, 0, sizeof( RGBTRIPLE ) * Y);
    memset( temp2, 0, sizeof( RGBTRIPLE ) * Y * X );

	//對每個指標陣列裡的指標宣告一個長度為X的陣列
    for( int i = 0; i < Y; i++){
        temp[ i ] = &temp2[i*X];
    }

    return temp;
}

/*********************************************************/
/* 交換二個指標                                          */
/*********************************************************/
void swap(RGBTRIPLE *a, RGBTRIPLE *b)
{
	RGBTRIPLE *temp;
	temp = a;
	a = b;
	b = temp;
}

