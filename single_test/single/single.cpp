#include "iostream"
#include <windows.h>
#include <random>
using namespace std;

#define MAX_THREADS 64
#define SUBDATANUM 200000
#define DATANUM (SUBDATANUM *64)  

float rawFloatData[DATANUM];
//求最大值时数据处理
float a[DATANUM];

float SortResult[DATANUM];

//求和
float sum(const float data[], const int len) {
    double sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        sum += log(sqrt(data[i]));
    }
    sum = (float)sum;
    return sum;
}

//求最大值
float MyMax(const float data[], const int len) {

    for (size_t i = 0; i < DATANUM; i++) {
        a[i] = log(sqrt(data[i] ));
    }

    float max = 0.0f;
    for (size_t i = 0; i < DATANUM; i++) {
        if (a[i] > max) {
            max = a[i];
        }
    }
    return max;
}

//快速排序
void quickSort(int left, int right, float* arr)
{
    if (left >= right)
        return;
    int i, j;
    float base, temp;
    i = left, j = right;
    base = arr[left];
    while (i < j)
    {
        while (arr[j] >= base && i < j)
            j--;
        while (arr[i] <= base && i < j) 
            i++;
        if (i < j)
        {
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    arr[left] = arr[i];
    arr[i] = base;
    quickSort(left, i - 1, arr);
    quickSort(i + 1, right, arr);
}

//排序
void MySort(const float data[], const int len, float result[])
{
    for (int i = 0; i < len; i++)
    {
        result[i] = log(sqrt(data[i] ));
    }
    quickSort(0, len - 1, result);
}

//排序测试
bool checkSort(const float data[], const int len)
{
    bool flag = 1;
    for (int i = 0; i < len - 1; i++)
        flag = data[i + 1] >= data[i];
    return flag;
}

int main() {
   /* for (size_t i = 0; i < DATANUM; i++)//数据初始化
    {
        rawFloatData[i] = float((rand() + rand() + rand() + rand()));//rand返回short数据类型，增加随机性
    }*/
    // 使用 Mersenne Twister 引擎
    std::minstd_rand rng(std::random_device{}());

    // 使用均匀分布，范围是 [0, 100]
    std::uniform_real_distribution<float> dist(0.0f, 3000.0f);

    // 生成随机数
    
    for (size_t i = 0; i < DATANUM; i++)//数据初始化
    {
        float random_number = dist(rng);
        rawFloatData[i] = random_number;//rand返回short数据类型，增加随机性
    }
    double run_time;
    _LARGE_INTEGER time_start, time_over;	//开始时间 & 结束时间
    double dqFreq;		//计时器频率
    LARGE_INTEGER f;	//计时器频率
    QueryPerformanceFrequency(&f);
    dqFreq = (double)f.QuadPart;
    QueryPerformanceCounter(&time_start);	//计时开始

    //调用求和函数
    float SumResult = 0.0f;
    SumResult = sum(rawFloatData, DATANUM);

    //调用求最大值的函数
    float MaxResult = 0.0f;
    MaxResult = MyMax(rawFloatData, DATANUM);

    //调用排序算法
    MySort(rawFloatData, DATANUM, SortResult);

    QueryPerformanceCounter(&time_over);	//计时结束
    run_time =  (time_over.QuadPart - time_start.QuadPart) / dqFreq;
    printf("\nrun_time：%f\n", run_time);

    cout << endl <<"sum:" << SumResult;
    cout << endl << "max:" << MaxResult << endl;

    //排序测试
    bool checkFlag;
    checkFlag = checkSort(SortResult, DATANUM);
    printf("\ncheck sort:%d\n", checkFlag);
}