//将66修改为接受数据（即运行receiver的电脑）
//若无法运行，请将项目属性中：VC++目录->包含目录中，添加当前目录（即包含my.cpp的目录）
#include "my.cpp"
using namespace std;

//信号量
HANDLE hSemaphores[THREADS_NUM];
HANDLE hSemaphores_sort[THREADS_NUM_SORT];
HANDLE hSemaphores_join[JOIN_NUM];
//线程ID
int ThreadID[THREADS_NUM];
int ThreadID_join[THREADS_NUM];


float rawFloatData[DATANUM];
float floatResults[THREADS_NUM], floatMaxs[THREADS_NUM];//數學處理值
float floatSorts[DATANUM];//线程排序结果
float joinSorts[DATANUM];//归并

__m256 Multiple_max[THREADS_NUM][NUMperTHREAD / 8];//求最大值


int main() {
 

    //随机数种子
    //std::default_random_engine e;
    // 使用 Mersenne Twister 引擎
    std::minstd_rand e(std::random_device{}());
    std::uniform_real_distribution<float> u(0, 0x7fff); // 左闭右闭区间
    e.seed(time(0));


    for (size_t i = 0; i < DATANUM; i++)//数据初始化
    {
        rawFloatData[i] = float(u(e));//增加随机性
        //  rawFloatData[i] = float(1+i);
    }






    double run_time, dqFreq;
    _LARGE_INTEGER time_start, time_over;	//开始时间 结束时间
    LARGE_INTEGER f;	//计时器频率
    QueryPerformanceFrequency(&f);
    dqFreq = (double)f.QuadPart;
    

    //建立TCP连接
    
    WSAData wsaData;
    WORD DllVersion = MAKEWORD(2, 1);
    if (WSAStartup(DllVersion, &wsaData) != 0)
    {
        MessageBoxA(NULL, "Winsock startup error", "Error", MB_OK | MB_ICONERROR);
        exit(1);
    }



    SOCKADDR_IN addr; //Adres przypisany do socketu Connection
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("100.80.168.54"); //Addres = localhost
    addr.sin_port = htons(8081); //Port = 8081  服务器的端口
    addr.sin_family = AF_INET; //IPv4 Socket



    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL); //TCP 必须要建立连接 SOCK_STREAM 
    if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) //Connection
    {
        MessageBoxA(NULL, "Bad Connection", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    


    int optVal = DATANUM * 4 * 8;
    int optLen = sizeof(optVal);
    setsockopt(Connection, SOL_SOCKET, SO_SNDBUF, (char*)&optVal, optLen);



    int optVal2 = 0;
    getsockopt(Connection, SOL_SOCKET, SO_SNDBUF, (char*)&optVal2, &optLen);
    printf("send buf is %d\n", optVal2); 

    


    int begin;//开始标志
    while (1)
    {
        recv(Connection, (char*)&begin, 3 * sizeof(int), NULL);//等待任务开始指令
        if (begin == 666)
        {
            printf("test begin\n");
            QueryPerformanceCounter(&time_start);	//计时开始
            //调用speedup求和函数
            float SumResult = 0.0f;
            SumResult = sumSpeedUp(rawFloatData, DATANUM);
            printf("sum: %f\n", SumResult);

            //调用speedup求最大值函数
            float MaxResult = 0.0f;
            MaxResult = maxSpeedUp(rawFloatData, DATANUM);
            printf("max: %f\n", MaxResult);

            //调用speedup求排序
            float SortResult = 0.0f;
            SortResult = sortSpeedUp(rawFloatData, DATANUM, floatSorts);

           


            unsigned long checksum = calculateChecksum(floatSorts, DATANUM); // 计算校验和

            //发送结果
            int sended;
            sended = send(Connection, (char*)&checksum, sizeof(unsigned long), NULL);


            sended = send(Connection, (char*)&floatSorts, DATANUM * sizeof(float), NULL);
            sended = send(Connection, (char*)&SumResult, sizeof(float), NULL);
            sended = send(Connection, (char*)&MaxResult,  sizeof(float), NULL);

            //printf("sended:%d", sended);
            break;

        }
    }
    closesocket(Connection);
    WSACleanup();

    QueryPerformanceCounter(&time_over);	//计时结束
    run_time =  (time_over.QuadPart - time_start.QuadPart) / dqFreq;
    printf("run_time：%f\n", run_time);

    system("pause");
    return 0;
}