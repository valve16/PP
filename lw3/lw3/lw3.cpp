﻿#include <iostream>
#include <windows.h>
#include <mmsystem.h>  
#include <fstream>    

#pragma comment(lib, "winmm.lib") 

DWORD startTime;

DWORD WINAPI ThreadProc(CONST LPVOID lpParam)
{
    int threadNum = *static_cast<int*>(lpParam);  // Получение номера потока
    const int operationCount = 15;

    std::string filename = (threadNum == 1) ? "output5.txt" : "output6.txt";
    for (int i = 0; i < operationCount; i++)
    {
        for (int j = 0; j < 10000; ++j)
        {
            for (int k = 0; k < 10000; ++k)
            {
            }
        }
        std::ofstream outFile(filename, std::ios::app);
        DWORD currentTime = timeGetTime();
        DWORD elapsedTime = currentTime - startTime;
        outFile << threadNum << "|" << elapsedTime << std::endl;
        outFile.close();
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
    std::cin.get();

    startTime = timeGetTime();


    for (int i = 0; i < N; ++i)
    {
        threadNums[i] = i + 1;
        handles[i] = CreateThread(NULL, 0, &ThreadProc, &threadNums[i], CREATE_SUSPENDED, NULL);
        if (i == 0)
        { 
            SetThreadPriority(handles[i], THREAD_PRIORITY_HIGHEST); // Низкий приоритет для первого потока +2 для первого потока
        }
        //else
        //{
        //    SetThreadPriority(handles[i], THREAD_PRIORITY_TIME_CRITICAL); // Высокий приоритет для второго потока +15
        //}
    }

    for (int i = 0; i < N; ++i)
    {
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(N, handles, TRUE, INFINITE);

    for (int i = 0; i < N; ++i)
    {
        CloseHandle(handles[i]);
    }

    std::cout << "Все потоки завершили работу." << std::endl;

    return 0;
}
