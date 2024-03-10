//��66�޸�Ϊ�������ݣ�������receiver�ĵ��ԣ�
//���޷����У��뽫��Ŀ�����У�VC++Ŀ¼->����Ŀ¼�У���ӵ�ǰĿ¼��������my.cpp��Ŀ¼��
#include "my.cpp"
using namespace std;

//�ź���
HANDLE hSemaphores[THREADS_NUM];
HANDLE hSemaphores_sort[THREADS_NUM_SORT];
HANDLE hSemaphores_join[JOIN_NUM];
//�߳�ID
int ThreadID[THREADS_NUM];
int ThreadID_join[THREADS_NUM];


float rawFloatData[DATANUM];
float floatResults[THREADS_NUM], floatMaxs[THREADS_NUM];//���W̎��ֵ
float floatSorts[DATANUM];//�߳�������
float joinSorts[DATANUM];//�鲢

__m256 Multiple_max[THREADS_NUM][NUMperTHREAD / 8];//�����ֵ


int main() {
 

    //���������
    //std::default_random_engine e;
    // ʹ�� Mersenne Twister ����
    std::minstd_rand e(std::random_device{}());
    std::uniform_real_distribution<float> u(0, 0x7fff); // ����ұ�����
    e.seed(time(0));


    for (size_t i = 0; i < DATANUM; i++)//���ݳ�ʼ��
    {
        rawFloatData[i] = float(u(e));//���������
        //  rawFloatData[i] = float(1+i);
    }






    double run_time, dqFreq;
    _LARGE_INTEGER time_start, time_over;	//��ʼʱ�� ����ʱ��
    LARGE_INTEGER f;	//��ʱ��Ƶ��
    QueryPerformanceFrequency(&f);
    dqFreq = (double)f.QuadPart;
    

    //����TCP����
    
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
    addr.sin_port = htons(8081); //Port = 8081  �������Ķ˿�
    addr.sin_family = AF_INET; //IPv4 Socket



    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL); //TCP ����Ҫ�������� SOCK_STREAM 
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

    


    int begin;//��ʼ��־
    while (1)
    {
        recv(Connection, (char*)&begin, 3 * sizeof(int), NULL);//�ȴ�����ʼָ��
        if (begin == 666)
        {
            printf("test begin\n");
            QueryPerformanceCounter(&time_start);	//��ʱ��ʼ
            //����speedup��ͺ���
            float SumResult = 0.0f;
            SumResult = sumSpeedUp(rawFloatData, DATANUM);
            printf("sum: %f\n", SumResult);

            //����speedup�����ֵ����
            float MaxResult = 0.0f;
            MaxResult = maxSpeedUp(rawFloatData, DATANUM);
            printf("max: %f\n", MaxResult);

            //����speedup������
            float SortResult = 0.0f;
            SortResult = sortSpeedUp(rawFloatData, DATANUM, floatSorts);

           


            unsigned long checksum = calculateChecksum(floatSorts, DATANUM); // ����У���

            //���ͽ��
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

    QueryPerformanceCounter(&time_over);	//��ʱ����
    run_time =  (time_over.QuadPart - time_start.QuadPart) / dqFreq;
    printf("run_time��%f\n", run_time);

    system("pause");
    return 0;
}