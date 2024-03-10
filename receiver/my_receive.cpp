//将59修改为接受数据（即运行receiver的电脑）
//若无法运行，请将项目属性中：VC++目录->包含目录中，添加当前目录（即包含my.cpp的目录）


#include"my.hpp"

using namespace std;

//信号量
HANDLE hSemaphores[THREADS_NUM];
HANDLE hSemaphores_sort[THREADS_NUM_SORT];
HANDLE hSemaphores_join[JOIN_NUM];
//线程ID
int ThreadID[THREADS_NUM];
int ThreadID_join[THREADS_NUM];


float rawFloatData[DATANUM];
//线程的中间结果（sum、max）
float floatResults[THREADS_NUM], floatMaxs[THREADS_NUM];
//线程排序结果
float floatSorts[DATANUM];
//归并辅助
float joinSorts[DATANUM];
//接收结果
float receiveSorts[DATANUM * 2];
//最终结果
float finalSorts[DATANUM * 2];
//求最大值
__m256 Multiple_max[THREADS_NUM][NUMperTHREAD / 8];

float receivedmax;
float receivedsum;

int main() {

    //随机数种子
    std::default_random_engine e;
    std::uniform_real_distribution<float> u(0, 0x7fff); // 左闭右闭区间
    e.seed(time(0));

    for (size_t i = 0; i < DATANUM; i++)//数据初始化
    {
        rawFloatData[i] = float(u(e));//增加随机性
        //  rawFloatData[i] = float(1+i);
    }

    //TCP连接
    WSAData wsaData;
    WORD DllVersion = MAKEWORD(2, 1);
    if (WSAStartup(DllVersion, &wsaData) != 0)
    {
        MessageBoxA(NULL, "WinSock startup error", "Error", MB_OK | MB_ICONERROR);
        return 0;
    }

    SOCKADDR_IN addr;
    int addrlen = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("100.80.168.54"); //target PC
    addr.sin_port = htons(8081); //server Port 指明server自己的端口号
    addr.sin_family = AF_INET; //IPv4 Socket

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL); //sListen 这个socket不传输数据 只监听是否有人连接 把传输任务指派给别的端口
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    SOCKET newConnection; 
    newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);

    //修改缓冲区大小
    int  optVal = DATANUM * 4 * 8;
    int optLen = sizeof(optVal);
    setsockopt(newConnection, SOL_SOCKET, SO_RCVBUF, (char*)&optVal, optLen);
    int optval2 = 0;
    getsockopt(newConnection, SOL_SOCKET, SO_RCVBUF, (char*)&optval2, &optLen);
    printf("recv buf is %d\n", optval2);

    if (newConnection == 0)
    {
        std::cout << "bad connection." << std::endl;
    }
    else {

        double run_time, dqFreq;
        _LARGE_INTEGER time_start, time_over;	//开始时间 结束时间
        LARGE_INTEGER f;	//计时器频率
        QueryPerformanceFrequency(&f);
        dqFreq = (double)f.QuadPart;
        QueryPerformanceCounter(&time_start);	//计时开始

        int send_start_flag = 666;
        send(newConnection, (char*)&send_start_flag, sizeof(int), NULL);
        printf("start signal sended\n");

        //调用speedup求和函数
        float SumResult = 0.0f;
        SumResult = sumSpeedUp(rawFloatData, DATANUM);
        // cout << endl << SumResult;

         //调用speedup求最大值函数
        float MaxResult = 0.0f;
        MaxResult = maxSpeedUp(rawFloatData, DATANUM);
        // cout << endl << MaxResult;

         //调用speedup求排序
        float SortResult = 0.0f;
        SortResult = sortSpeedUp(rawFloatData, DATANUM, floatSorts);

        //接收数据
        int receivesuccess = 0;
        int received = 0;
        int i = 0;
        char* receivePin = (char*)receiveSorts;
        while (1) {

            receivesuccess = recv(newConnection, &receivePin[received], DATANUM * sizeof(float), NULL);
            printf("第%d次接收：接收数据量：%d\n", i + 1, receivesuccess);
            i = i + 1;

            if (receivesuccess == -1)//打印错误信息
            {
                int erron = WSAGetLastError();
                printf("erron=%d\n", erron);
            }
            else
            {
                received = received + receivesuccess;
            }
            if (received >= (4 * DATANUM + 2))
            {
                //receivesuccess = recv(newConnection, (char*)&receivedsum, sizeof(float), NULL);
                //receivesuccess = recv(newConnection, (char*)&receivedmax, sizeof(float), NULL);
                break;
            }
        }
        unsigned long receivedChecksum = 0;
        //再归并
        //receivedChecksum = calculateChecksum(receiveSorts, DATANUM);
        
        memcpy(&receivedChecksum, &receivePin[0], sizeof(unsigned long));
        
        unsigned long Checksum = 0;
        Checksum = calculateChecksum(&receiveSorts[2], DATANUM);

        joinsort(receiveSorts, floatSorts, finalSorts, DATANUM);
        if (receiveSorts[DATANUM + 3] > MaxResult)
            MaxResult = receiveSorts[DATANUM + 3];
        SumResult = SumResult + receiveSorts[DATANUM + 2];

        
        printf("检查校验值：%lu\n", Checksum);

        QueryPerformanceCounter(&time_over);	//计时结束
        run_time =  (time_over.QuadPart - time_start.QuadPart) / dqFreq;
        printf("\nrun_time：%f\n", run_time);

        //检查排序
        bool checkflag;
        checkflag = checkSort(floatSorts, DATANUM);
        printf("检查排序：%d\n", checkflag);

        //输出结果
        printf("sum: %f\n", SumResult);
        printf("max: %f\n", MaxResult);

    }
    closesocket(newConnection);
    WSACleanup();
    system("pause");
    return 0;








}




















