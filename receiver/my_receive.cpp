//��59�޸�Ϊ�������ݣ�������receiver�ĵ��ԣ�
//���޷����У��뽫��Ŀ�����У�VC++Ŀ¼->����Ŀ¼�У���ӵ�ǰĿ¼��������my.cpp��Ŀ¼��


#include"my.hpp"

using namespace std;

//�ź���
HANDLE hSemaphores[THREADS_NUM];
HANDLE hSemaphores_sort[THREADS_NUM_SORT];
HANDLE hSemaphores_join[JOIN_NUM];
//�߳�ID
int ThreadID[THREADS_NUM];
int ThreadID_join[THREADS_NUM];


float rawFloatData[DATANUM];
//�̵߳��м�����sum��max��
float floatResults[THREADS_NUM], floatMaxs[THREADS_NUM];
//�߳�������
float floatSorts[DATANUM];
//�鲢����
float joinSorts[DATANUM];
//���ս��
float receiveSorts[DATANUM * 2];
//���ս��
float finalSorts[DATANUM * 2];
//�����ֵ
__m256 Multiple_max[THREADS_NUM][NUMperTHREAD / 8];

float receivedmax;
float receivedsum;

int main() {

    //���������
    std::default_random_engine e;
    std::uniform_real_distribution<float> u(0, 0x7fff); // ����ұ�����
    e.seed(time(0));

    for (size_t i = 0; i < DATANUM; i++)//���ݳ�ʼ��
    {
        rawFloatData[i] = float(u(e));//���������
        //  rawFloatData[i] = float(1+i);
    }

    //TCP����
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
    addr.sin_port = htons(8081); //server Port ָ��server�Լ��Ķ˿ں�
    addr.sin_family = AF_INET; //IPv4 Socket

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL); //sListen ���socket���������� ֻ�����Ƿ��������� �Ѵ�������ָ�ɸ���Ķ˿�
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    SOCKET newConnection; 
    newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);

    //�޸Ļ�������С
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
        _LARGE_INTEGER time_start, time_over;	//��ʼʱ�� ����ʱ��
        LARGE_INTEGER f;	//��ʱ��Ƶ��
        QueryPerformanceFrequency(&f);
        dqFreq = (double)f.QuadPart;
        QueryPerformanceCounter(&time_start);	//��ʱ��ʼ

        int send_start_flag = 666;
        send(newConnection, (char*)&send_start_flag, sizeof(int), NULL);
        printf("start signal sended\n");

        //����speedup��ͺ���
        float SumResult = 0.0f;
        SumResult = sumSpeedUp(rawFloatData, DATANUM);
        // cout << endl << SumResult;

         //����speedup�����ֵ����
        float MaxResult = 0.0f;
        MaxResult = maxSpeedUp(rawFloatData, DATANUM);
        // cout << endl << MaxResult;

         //����speedup������
        float SortResult = 0.0f;
        SortResult = sortSpeedUp(rawFloatData, DATANUM, floatSorts);

        //��������
        int receivesuccess = 0;
        int received = 0;
        int i = 0;
        char* receivePin = (char*)receiveSorts;
        while (1) {

            receivesuccess = recv(newConnection, &receivePin[received], DATANUM * sizeof(float), NULL);
            printf("��%d�ν��գ�������������%d\n", i + 1, receivesuccess);
            i = i + 1;

            if (receivesuccess == -1)//��ӡ������Ϣ
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
        //�ٹ鲢
        //receivedChecksum = calculateChecksum(receiveSorts, DATANUM);
        
        memcpy(&receivedChecksum, &receivePin[0], sizeof(unsigned long));
        
        unsigned long Checksum = 0;
        Checksum = calculateChecksum(&receiveSorts[2], DATANUM);

        joinsort(receiveSorts, floatSorts, finalSorts, DATANUM);
        if (receiveSorts[DATANUM + 3] > MaxResult)
            MaxResult = receiveSorts[DATANUM + 3];
        SumResult = SumResult + receiveSorts[DATANUM + 2];

        
        printf("���У��ֵ��%lu\n", Checksum);

        QueryPerformanceCounter(&time_over);	//��ʱ����
        run_time =  (time_over.QuadPart - time_start.QuadPart) / dqFreq;
        printf("\nrun_time��%f\n", run_time);

        //�������
        bool checkflag;
        checkflag = checkSort(floatSorts, DATANUM);
        printf("�������%d\n", checkflag);

        //������
        printf("sum: %f\n", SumResult);
        printf("max: %f\n", MaxResult);

    }
    closesocket(newConnection);
    WSACleanup();
    system("pause");
    return 0;








}




















