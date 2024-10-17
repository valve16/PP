#include <iostream>
#include <windows.h>
#include <mmsystem.h>  
#include <fstream>    

#pragma comment(lib, "winmm.lib") 

DWORD startTime;
HANDLE hMutex;

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    int threadNum = *static_cast<int*>(lpParam);  // Получение номера потока
    const int operationCount = 15;

    for (int i = 0; i < operationCount; i++)
    {
        WaitForSingleObject(hMutex, INFINITE);
        Sleep(100);
        DWORD currentTime = timeGetTime();
        DWORD elapsedTime = currentTime - startTime;
        std::ofstream outFile("output.txt", std::ios::app);
        outFile << threadNum << "|" << elapsedTime << std::endl;
        outFile.close();
        ReleaseMutex(hMutex);
    }

    ExitThread(0);  // Завершение потока
}

int main(int argc, char* argv[])
{
    SetConsoleCP(1251);      
    SetConsoleOutputCP(1251); 

    const int N = 2;          
    int threadNums[N];
    HANDLE handles[N];
    //std::cin.get();

    startTime = timeGetTime();

    hMutex = CreateMutex(NULL, FALSE, NULL);

    for (int i = 0; i < N; ++i)
    {
        threadNums[i] = i + 1;
        handles[i] = CreateThread(NULL, 0, &ThreadProc, &threadNums[i], CREATE_SUSPENDED, NULL);
    }

    for (int i = 0; i < N; ++i)
    {
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(N, handles, TRUE, INFINITE);

    // Освобождение ресурсов
    for (int i = 0; i < N; ++i)
    {
        CloseHandle(handles[i]);
    }

    CloseHandle(hMutex);

    std::cout << "Все потоки завершили работу." << std::endl;

    return 0;
}
