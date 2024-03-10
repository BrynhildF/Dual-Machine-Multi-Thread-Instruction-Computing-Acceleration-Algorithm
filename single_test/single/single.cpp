#include "iostream"
#include <windows.h>
#include <random>
using namespace std;

#define MAX_THREADS 64
#define SUBDATANUM 200000
#define DATANUM (SUBDATANUM *64)  

float rawFloatData[DATANUM];
//�����ֵʱ���ݴ���
float a[DATANUM];

float SortResult[DATANUM];

//���
float sum(const float data[], const int len) {
    double sum = 0.0f;
    for (size_t i = 0; i < len; i++) {
        sum += log(sqrt(data[i]));
    }
    sum = (float)sum;
    return sum;
}

//�����ֵ
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

//��������
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

//����
void MySort(const float data[], const int len, float result[])
{
    for (int i = 0; i < len; i++)
    {
        result[i] = log(sqrt(data[i] ));
    }
    quickSort(0, len - 1, result);
}

//�������
bool checkSort(const float data[], const int len)
{
    bool flag = 1;
    for (int i = 0; i < len - 1; i++)
        flag = data[i + 1] >= data[i];
    return flag;
}

int main() {
   /* for (size_t i = 0; i < DATANUM; i++)//���ݳ�ʼ��
    {
        rawFloatData[i] = float((rand() + rand() + rand() + rand()));//rand����short�������ͣ����������
    }*/
    // ʹ�� Mersenne Twister ����
    std::minstd_rand rng(std::random_device{}());

    // ʹ�þ��ȷֲ�����Χ�� [0, 100]
    std::uniform_real_distribution<float> dist(0.0f, 3000.0f);

    // ���������
    
    for (size_t i = 0; i < DATANUM; i++)//���ݳ�ʼ��
    {
        float random_number = dist(rng);
        rawFloatData[i] = random_number;//rand����short�������ͣ����������
    }
    double run_time;
    _LARGE_INTEGER time_start, time_over;	//��ʼʱ�� & ����ʱ��
    double dqFreq;		//��ʱ��Ƶ��
    LARGE_INTEGER f;	//��ʱ��Ƶ��
    QueryPerformanceFrequency(&f);
    dqFreq = (double)f.QuadPart;
    QueryPerformanceCounter(&time_start);	//��ʱ��ʼ

    //������ͺ���
    float SumResult = 0.0f;
    SumResult = sum(rawFloatData, DATANUM);

    //���������ֵ�ĺ���
    float MaxResult = 0.0f;
    MaxResult = MyMax(rawFloatData, DATANUM);

    //���������㷨
    MySort(rawFloatData, DATANUM, SortResult);

    QueryPerformanceCounter(&time_over);	//��ʱ����
    run_time =  (time_over.QuadPart - time_start.QuadPart) / dqFreq;
    printf("\nrun_time��%f\n", run_time);

    cout << endl <<"sum:" << SumResult;
    cout << endl << "max:" << MaxResult << endl;

    //�������
    bool checkFlag;
    checkFlag = checkSort(SortResult, DATANUM);
    printf("\ncheck sort:%d\n", checkFlag);
}