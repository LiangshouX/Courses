/*
* @file OS.cpp
* @author 肖良寿 1120200563
* @time   2022/10/15
* @brief 生产者消费者问题
*        2个生产者进程
*           –  随机等待一段时间，往缓冲区添加数据，
*           –  若缓冲区已满，等待消费者取走数据后再添加
*           –  重复12次
*       3个消费者进程
*           –  随机等待一段时间，从缓冲区读取数据
*           –  若缓冲区为空，等待生产者添加数据后再读取
*           –  重复8次
* 
*/

#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#include <stdio.h>
#include <time.h>


HANDLE handleOfProcess[5];

struct buffer
{
    char buffer[10];
    int write;
    int read;
};

int rand_1()
{//随机产生处于1000~1099的随机数
    return rand() % 100 + 1000;
}

char rand_char()
{//随机产生大写的26位字母
    return rand() % 26 + 'A';
}

void StartClone(int nCloneID)
{
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);

    TCHAR szCmdLine[MAX_PATH];
    sprintf(szCmdLine, "\"%s\" %d", szFilename, nCloneID);

    STARTUPINFO si;
    ZeroMemory(reinterpret_cast<void*>(&si), sizeof(si));

    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;

    BOOL bCreateOK = CreateProcess(
        szFilename,
        szCmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_DEFAULT_ERROR_MODE,
        NULL,
        NULL,
        &si,
        &pi);
    if (bCreateOK)
        handleOfProcess[nCloneID] = pi.hProcess;
    else
    {
        printf("Error in create process!\n");
        exit(0);
    }
}

void Producer()//生产者
{
    HANDLE mutex = CreateMutex(NULL, FALSE, "MYMUTEX");
    HANDLE empty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "MYEMPTY");
    HANDLE full = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "MYFULL");

    HANDLE hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "myfilemap");
    LPVOID Data = MapViewOfFile(//文件映射
        hMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0);
    struct buffer* pint = reinterpret_cast<struct buffer*>(Data);

    for (int i = 0; i < 12; i++)
    {
        WaitForSingleObject(empty, INFINITE);   //P(empty)
        //sleep
        srand((unsigned)time(0));
        int tim = rand_1();
        Sleep(tim);

        WaitForSingleObject(mutex, INFINITE);  //P(mutex)

        //Add nextp to buffer
        pint->buffer[pint->write] = rand_char();
        pint->write = (pint->write + 1) % 10;


        ReleaseMutex(mutex);                    //V(mutex)
        ReleaseSemaphore(full, 1, NULL);        //V(full)

        SYSTEMTIME syst;
        time_t t = time(0);
        GetSystemTime(&syst);
        char tmpBuf[10];
        strftime(tmpBuf, 10, "%H:%M:%S", localtime(&t));
        printf("生产者%d向缓冲区写入数据:\t%c\t%c\t%c\t@%s.%d\n",
            (int)GetCurrentProcessId(), pint->buffer[0], pint->buffer[1],
            pint->buffer[2], tmpBuf, syst.wMilliseconds);
        fflush(stdout);
    }
    UnmapViewOfFile(Data);//解除映射
    Data = NULL;
    CloseHandle(mutex);
    CloseHandle(empty);
    CloseHandle(full);
}

void Consumer()//消费者
{
    HANDLE mutex = CreateMutex(NULL, FALSE, "MYMUTEX");
    HANDLE empty = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "MYEMPTY");
    HANDLE full = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, "MYFULL");

    HANDLE hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "myfilemap");
    LPVOID Data = MapViewOfFile(//文件映射
        hMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0);
    struct buffer* pint = reinterpret_cast<struct buffer*>(Data);

    for (int i = 0; i < 8; i++)
    {
        WaitForSingleObject(full, INFINITE);  //P(full)
        
        srand((unsigned)time(0));
        int tim = rand_1();
        Sleep(tim);

        WaitForSingleObject(mutex, INFINITE);  //P(mutex)

        pint->buffer[pint->read] = ' ';
        pint->read = (pint->read + 1) % 10;

        ReleaseMutex(mutex);                   //V(mutex)
        ReleaseSemaphore(empty, 1, NULL);      //V(empty)

        time_t t = time(0);
        char tmpBuf[10];
        SYSTEMTIME syst;
        GetSystemTime(&syst);
        strftime(tmpBuf, 10, "%H:%M:%S", localtime(&t));

        printf("消费者%d从缓冲区读取数据:\t%c\t%c\t%c\t@%s.%d\n",
            (int)GetCurrentProcessId(), pint->buffer[0], pint->buffer[1],
            pint->buffer[2], tmpBuf, syst.wMilliseconds);
        fflush(stdout);

    }
    UnmapViewOfFile(Data);//解除映射
    Data = NULL;
    CloseHandle(mutex);
    CloseHandle(empty);
    CloseHandle(full);
}

int main(int argc, char* argv[])
{
    int nCloneID = 20;
    if (argc > 1)
    {
        sscanf(argv[1], "%d", &nCloneID);
    }

    if (nCloneID < 2)//生产者进程
    {
        Producer();
    }
    else if (nCloneID < 5)//消费者进程
    {
        Consumer();
    }
    else//主进程
    {
        HANDLE hMap = CreateFileMapping(
            NULL,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(struct buffer),
            "myfilemap");

        if (hMap != INVALID_HANDLE_VALUE)
        {
            LPVOID Data = MapViewOfFile(//文件映射
                hMap,
                FILE_MAP_ALL_ACCESS,
                0,
                0,
                0);

            if (Data != NULL)
            {
                ZeroMemory(Data, sizeof(struct buffer));
            }
            struct buffer* pnData = reinterpret_cast<struct buffer*>(Data);
            pnData->read = 0;
            pnData->write = 0;
            memset(pnData->buffer, 0, sizeof(pnData->buffer));
            UnmapViewOfFile(Data);//解除映射
            Data = NULL;
        }

        HANDLE empty = CreateSemaphore(NULL, 10, 10, "MYEMPTY");
        HANDLE full = CreateSemaphore(NULL, 0, 10, "MYFULL");
        for (int i = 0; i < 5; i++)//创建子进程
            StartClone(i);
        WaitForMultipleObjects(5, handleOfProcess, TRUE, INFINITE);
        CloseHandle(empty);
        CloseHandle(full);
    }
}