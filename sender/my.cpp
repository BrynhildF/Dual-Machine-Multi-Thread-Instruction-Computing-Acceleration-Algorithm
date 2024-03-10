#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib,"ws2_32.lib")
#include <WinSock2.h>
#include <random>
#include <io.h>
#include <iostream>
#include <immintrin.h>
#include <windows.h>
#include <stdlib.h>

#include <vector>
#include <numeric>  
unsigned long calculateChecksum(const float* data, size_t size) {
    unsigned long checksum = 0;
    for (size_t i = 0; i < size; ++i) {
        // 将每个浮点数的位表示转换为无符号长整型，并累加
        unsigned long numAsInt = *reinterpret_cast<const unsigned long*>(&data[i]);
        checksum += numAsInt;
    }
    return checksum;
}





#define NUMperTHREAD 100000
#define DATANUM (NUMperTHREAD * 64)   


#define THREADS_NUM 64

#define THREADS_NUM_SORT 64
//#define NUMperTHREAD NUMperTHREAD
#define JOIN_NUM 2

typedef struct myParameter {
    int lpParameter;
    float* result;
}MyParameter;

//信号量
extern HANDLE hSemaphores[THREADS_NUM];
extern HANDLE hSemaphores_sort[THREADS_NUM_SORT];
extern HANDLE hSemaphores_join[JOIN_NUM];
//线程ID
extern int ThreadID[THREADS_NUM];
extern int ThreadID_join[THREADS_NUM];


extern float rawFloatData[DATANUM];
extern float floatResults[THREADS_NUM], floatMaxs[THREADS_NUM];//线程的中间结果（sum、max）
extern float floatSorts[DATANUM];//线程排序结果
extern float joinSorts[DATANUM];//归并

extern __m256 Multiple_max[THREADS_NUM][NUMperTHREAD / 8];//求最大值





DWORD WINAPI ThreadProc_sum(LPVOID lpParameter)
{
    int thread_ID = *(int*)lpParameter;
    int startIndex = thread_ID * NUMperTHREAD;

    size_t datanum_per_AVX256 = 8;    // 每个m256中有8个float
    size_t Multipledata_num = NUMperTHREAD / datanum_per_AVX256;   // 总共有多少multipledata.


    __m256 Multiple_sum = _mm256_setzero_ps();   // 求和变量
    __m256 Multiple_mid;
    __m256 Multiple_load;    // 加载.
    const float* p = rawFloatData + startIndex;    // 为线程确定数据点
    const float* q;    


    for (size_t i = 0; i < Multipledata_num; ++i)
    {
        Multiple_load = _mm256_load_ps(p);
        Multiple_mid = _mm256_log_ps(_mm256_sqrt_ps(Multiple_load));
        Multiple_sum = _mm256_add_ps(Multiple_sum, Multiple_mid);
        p += datanum_per_AVX256;
    }

    
    q = (const float*)&Multiple_sum;// 合并muti变量
    floatResults[thread_ID] = 0.0f;
    for (int i = 0; i < 8; ++i)
    {
        floatResults[thread_ID] += q[i];
    }

    ReleaseSemaphore(hSemaphores[thread_ID], 1, NULL);
    return 0;
}

float sumSpeedUp(const float data[], const int len) {
    
    
    float floatSum = 0.0f;
    
    HANDLE hThreads[THREADS_NUM];

    //创建线程
    for (int i = 0; i < THREADS_NUM; i++)
    {
        hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价
        ThreadID[i] = i;
        floatResults[i] = 0.0f;

        hThreads[i] = CreateThread(
            NULL,// default security attributes
            0,// use default stack size
            ThreadProc_sum,// thread function
            &ThreadID[i],// argument to thread function
            CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
            NULL);
    }
    //开启线程
    for (int i = 0; i < THREADS_NUM; i++)
    {
        if (hThreads[i]) {
            ResumeThread(hThreads[i]);
        }
    }
    WaitForMultipleObjects(THREADS_NUM, hSemaphores, TRUE, INFINITE);

    //收割
    floatSum = 0.0f;
    size_t datanum_per_AVX256 = 8;
    size_t Multipledata_num = THREADS_NUM / datanum_per_AVX256; 
    __m256 Multiple_sum = _mm256_setzero_ps();
    __m256 Multiple_load; 
    const float* p = floatResults;  
    const float* q;
    for (size_t i = 0; i < Multipledata_num; ++i)
    {
        Multiple_load = _mm256_load_ps(p);
        Multiple_sum = _mm256_add_ps(Multiple_sum, Multiple_load);
        p += datanum_per_AVX256;
    }
    q = (const float*)&Multiple_sum;

    for (int i = 0; i < 8; ++i)
    {
        floatSum+= q[i];
    }
    //关闭线程
    for (int i = 0; i < THREADS_NUM; i++)
    {
        if (hThreads[i]) {
            CloseHandle(hThreads[i]);
        }
    }
    return floatSum;
}

DWORD WINAPI ThreadProc_max(LPVOID lpParameter) {
    int thread_ID = *(int*)lpParameter;
    int startIndex = thread_ID * NUMperTHREAD;
    const size_t datanum_per_AVX256 = 8;   
    const size_t Multipledata_num = NUMperTHREAD / datanum_per_AVX256; 

    // 数据处理
    __m256 Multiple_load;  
    const float* pp = rawFloatData + startIndex;

    for (size_t i = 0; i < Multipledata_num; ++i)
    {
        Multiple_load = _mm256_load_ps(pp);
        Multiple_max[thread_ID][i] = _mm256_log_ps(_mm256_sqrt_ps(Multiple_load));
        pp += datanum_per_AVX256;
    }

    // 求最大值
    __declspec(align(32)) float output[8];
    __m256 maxVal = Multiple_max[thread_ID][0];
    for (size_t i = 1; i < Multipledata_num; ++i)
    {
        maxVal = _mm256_max_ps(maxVal, Multiple_max[thread_ID][i]);
    }
    _mm256_store_ps(output, maxVal);
    floatMaxs[thread_ID] = output[0];
    for (int i = 1; i < 8; i++)
    {
        floatMaxs[thread_ID] = floatMaxs[thread_ID] > output[i] ? floatMaxs[thread_ID] : output[i];
    }

    ReleaseSemaphore(hSemaphores[thread_ID], 1, NULL);
    return 0;
}

float maxSpeedUp(const float data[], const int len) {
    float floatMax = 0.0f;
    HANDLE hThreads[THREADS_NUM];
    //创建线程
    for (int i = 0; i < THREADS_NUM; i++)
    {
        hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价
        ThreadID[i] = i;
        floatMaxs[i] = 0.0f;

        hThreads[i] = CreateThread(
            NULL,// default security attributes
            0,// use default stack size
            ThreadProc_max,// thread function
            &ThreadID[i],// argument to thread function
            CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
            NULL);
    }
    //开启线程
    for (int i = 0; i < THREADS_NUM; i++)
    {
        if (hThreads[i]) {
            ResumeThread(hThreads[i]);
        }
    }
    WaitForMultipleObjects(THREADS_NUM, hSemaphores, TRUE, INFINITE);

    const size_t datanum_per_AVX256 = 8;    
    size_t cntBlock2 = THREADS_NUM / datanum_per_AVX256;   
    __declspec(align(32)) float output[8];
    __m256 Multiple_load2;
    const float* pp = floatMaxs;
    __m256 maxVal = _mm256_load_ps(pp);
    pp += datanum_per_AVX256;

    // 求最大值
    for (size_t i = 1; i < cntBlock2; ++i)
    {
        Multiple_load2 = _mm256_load_ps(pp);
        maxVal = _mm256_max_ps(maxVal, Multiple_load2);
        pp += datanum_per_AVX256;
    }
    _mm256_store_ps(output, maxVal);
    floatMax = output[0];
    for (int i = 1; i < 8; i++)
    {
        floatMax = floatMax > output[i] ? floatMax : output[i];
    }

    //关闭线程
    for (int i = 0; i < THREADS_NUM; i++)
    {
        if (hThreads[i]) {
            CloseHandle(hThreads[i]);
        }
    }
    return floatMax;
}

void quickSort(int left, int right, float* arr)
{
    if (left >= right)
        return;
    int i, j;
    float base, temp;
    i = left, j = right;
    base = arr[left];  //取最左边的数为基准数
    while (i < j)
    {
        while (arr[j] >= base && i < j) //j持续左移直至找到小于基准数的数
            j--;
        while (arr[i] <= base && i < j) //i持续右移直到找到大于基准数的数
            i++;
        if (i < j) //如果找到了就交换 
        {
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    //基准数归位
    arr[left] = arr[i];
    arr[i] = base;
    quickSort(left, i - 1, arr);//递归左边
    quickSort(i + 1, right, arr);//递归右边
}

void joinsort(float a[], float b[], float c[], int lenth)
{
    int i = 0;
    int j = 0;
    while (i < lenth && j < lenth)
    {
        if (a[i] < b[j])
        {
            c[i + j] = a[i];
            i++;
            continue;
        }
        c[i + j] = b[j];
        j++;
    }
    while (i < lenth)
    {
        c[i + j] = a[i];
        i++;
    }
    while (j < lenth)
    {
        c[i + j] = b[j];
        j++;
    }
}






DWORD WINAPI ThreadProc_sort(LPVOID pM) {


    MyParameter* pt = (MyParameter*)pM;
    int thread_ID = pt->lpParameter;
    int startIndex = thread_ID * NUMperTHREAD;
    const size_t datanum_per_AVX256 = 8; 
    const size_t Multipledata_num = NUMperTHREAD / datanum_per_AVX256;  

    // 数据处理
    __m256 Multiple_sortdata[Multipledata_num];   
    __m256 Multiple_load;   
    const float* pp = rawFloatData + startIndex;

    for (size_t i = 0; i < Multipledata_num; ++i)
    {
        Multiple_load = _mm256_load_ps(pp);    
        Multiple_sortdata[i] = _mm256_log_ps(_mm256_sqrt_ps(Multiple_load));
        pp += datanum_per_AVX256;
    }

    float* q = (float*)Multiple_sortdata;
    quickSort(0, NUMperTHREAD - 1, q);

   
    __m256* aa = (__m256*)(pt->result);
    __m256* bb = (__m256*)q;
    for (size_t i = 0; i < NUMperTHREAD / 8; i++) {
        *aa = *bb;
        aa++;
        bb++;
    }

    ReleaseSemaphore(hSemaphores_sort[thread_ID], 1, NULL);
    return 0;
}

DWORD WINAPI ThreadProc_join(LPVOID lpParameter) {
    int thread_ID = *(int*)lpParameter;

    float* pin1;  //被合并数组
    float* pin2;  //合并后数组
    int num = 32;
    bool flag = 0;
    int length; //被合并数组长度
    for (size_t j = 1; j < 32; j = j * 2) {
        if (flag == 0) {
            pin1 = floatSorts + thread_ID * DATANUM / 2;
            pin2 = joinSorts + thread_ID * DATANUM / 2;
        }
        else {
            pin1 = joinSorts + thread_ID * DATANUM / 2;
            pin2 = floatSorts + thread_ID * DATANUM / 2;
        }
        length = NUMperTHREAD * j;
        for (size_t i = 0; i < num; i = i + 2) {
            joinsort((pin1 + i * length), (pin1 + (i + 1) * length), pin2 + i * length, length);
        }
        num = num / 2;
        flag = !flag;
    }
    ReleaseSemaphore(hSemaphores_join[thread_ID], 1, NULL);
    return 0;
}

float sortSpeedUp(const float data[], const int len, float result[])
{
    HANDLE hThreads[THREADS_NUM_SORT];
    MyParameter parameter[THREADS_NUM_SORT];
    //创建线程
    for (int i = 0; i < THREADS_NUM_SORT; i++)
    {
        hSemaphores_sort[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价
        parameter[i].lpParameter = i;
        parameter[i].result = result + i * NUMperTHREAD;  //排序结果指针

        hThreads[i] = CreateThread(
            NULL,// default security attributes
            0,// use default stack size
            ThreadProc_sort,// thread function
            (LPVOID) & (parameter[i]),// argument to thread function
            CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
            NULL);
    }
    //开启线程
    for (int i = 0; i < THREADS_NUM_SORT; i++)
    {
        ResumeThread(hThreads[i]);
    }
    WaitForMultipleObjects(THREADS_NUM_SORT, hSemaphores_sort, TRUE, INFINITE);
    //关闭线程
    for (int i = 0; i < THREADS_NUM_SORT; i++)
    {
        if (hThreads[i]) {
            CloseHandle(hThreads[i]);
        }
    }

    //归并
    HANDLE hThreads_join[JOIN_NUM];
    //创建线程
    for (int i = 0; i < JOIN_NUM; i++)
    {
        hSemaphores_join[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价
        ThreadID_join[i] = i;

        hThreads_join[i] = CreateThread(
            NULL,// default security attributes
            0,// use default stack size
            ThreadProc_join,// thread function
            &ThreadID_join[i],// argument to thread function
            CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
            NULL);
    }
    //开启线程
    for (int i = 0; i < JOIN_NUM; i++)
    {
        if (hThreads_join[i]) {
            ResumeThread(hThreads_join[i]);
        }
    }
    WaitForMultipleObjects(JOIN_NUM, hSemaphores_join, TRUE, INFINITE);
    //关闭线程
    for (int i = 0; i < JOIN_NUM; i++)
    {
        if (hThreads_join[i]) {
            CloseHandle(hThreads_join[i]);
        }
    }
    //总归并
    joinsort(joinSorts, joinSorts + DATANUM / 2, floatSorts, DATANUM / 2);
    return 0.0f;
}


bool checkSort(const float data[], const int len)
{
    bool flag = 1;
    for (int i = 0; i < len - 1; i++)
    {
        flag = data[i + 1] >= data[i];
        if (flag == 0)
            break;
    }
    return flag;
}



