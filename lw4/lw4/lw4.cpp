#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include "EasyBMP.h"
#include <memory>
#include <chrono>
#include <string>
#include <vector>
#include <fstream>

const int BLUR_SIZE = 50; // Количество пикселей по обе стороны от текущего
const int NUM_THREAD = 12;
const int NUM_CORES = 3;

struct TimeThreads
{
    int numThread = 0;
    long long timeThread = 0;
};


struct ThreadParams
{
    BMP* bmp;
    unsigned char* buffer;
    int startRow;
    int endRow;
    int threadNum;
    DWORD_PTR coreAffinity; // Маска для привязки к ядрам
    std::chrono::steady_clock::time_point startTime;
};


void WritePix2PicArray(unsigned char* buffer, BMP& bmp, int pixCol, int pixRow, RGBApixel pixData)
{
    // Запись данных пикселя в буфер
    buffer[pixCol * 3 + pixRow * bmp.TellWidth() * 3] = pixData.Red;
    buffer[pixCol * 3 + pixRow * bmp.TellWidth() * 3 + 1] = pixData.Green;
    buffer[pixCol * 3 + pixRow * bmp.TellWidth() * 3 + 2] = pixData.Blue;
}

RGBApixel GetPixFromPicArray(unsigned char* buffer, BMP& bmp, int pixCol, int pixRow)
{
    // Чтение данных пикселя из буфера
    RGBApixel _pixData;
    _pixData.Red = buffer[pixCol * 3 + pixRow * bmp.TellWidth() * 3];
    _pixData.Green = buffer[pixCol * 3 + pixRow * bmp.TellWidth() * 3 + 1];
    _pixData.Blue = buffer[pixCol * 3 + pixRow * bmp.TellWidth() * 3 + 2];

    return _pixData;
}

DWORD WINAPI ThreadProc(LPVOID lpParam) {
    ThreadParams* params = static_cast<ThreadParams*>(lpParam);
    BMP* bmp = params->bmp;
    unsigned char* buffer = params->buffer;
    int widthIm = bmp->TellWidth();
    int heightIm = bmp->TellHeight();
    std::string filename;
    switch (params->threadNum)
    {
    case 1: 
        filename = "output1.txt";
        break; 
    case 2: 
        filename = "output2.txt";
        break; 
    case 3: 
        filename = "output3.txt";
        break;
    }
    // Привязка потока к конкретному ядру
    SetThreadAffinityMask(GetCurrentThread(), params->coreAffinity);

    // Установка приоритета потока
    if (params->threadNum == 1) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
    else if (params->threadNum == 2) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    }
    else {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    }



    int totalProcessedPixels = 0;
    for (int j = params->startRow; j < params->endRow && totalProcessedPixels < 15; j++)
    {
        for (int i = 0; i < widthIm && totalProcessedPixels < 15; i++)
        {
            int sumR = 0, sumG = 0, sumB = 0;
            int count = 0;

            for (int x = -BLUR_SIZE; x <= BLUR_SIZE; x++)
            {
                int neighborX = i + x;
                int neighborY = j;

                if (neighborX >= 0 && neighborX < widthIm)
                {
                    RGBApixel neighborPix = bmp->GetPixel(neighborX, neighborY);
                    sumR += neighborPix.Red;
                    sumG += neighborPix.Green;
                    sumB += neighborPix.Blue;
                    count++;
                }
            }

            RGBApixel _pixData;
            _pixData.Red = sumR / count;
            _pixData.Green = sumG / count;
            _pixData.Blue = sumB / count;

            WritePix2PicArray(buffer, *bmp, i, j, _pixData);

            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - params->startTime).count();
            std::ofstream outFile(filename, std::ios::app);
            outFile << params->threadNum << "|" << duration << std::endl;
            outFile.close();

            totalProcessedPixels++;

        }
    }
    ExitThread(0);
}


int main(int argc, char* argv[])
{
    //if (argc != 4)
    //{
    //    std::cerr << "Usage: " << argv[0] << " <image_path> <num_threads> <num_cores>" << std::endl;
    //    return -1;
    //}
    //const std::string filename = argv[1];
    //int numThreads = std::atoi(argv[2]);
    //int numCores = std::atoi(argv[3]);  
    
    const std::string filename = "in1.bmp";
    int numThreads = 3;
    int numCores = 1;

    auto startTime = std::chrono::steady_clock::now();

    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    BMP _InputBmp;
    _InputBmp.ReadFromFile(filename.c_str());


    // Создание буфера для изображения
    int _bufferSize = _InputBmp.TellWidth() * _InputBmp.TellHeight() * 3;
    std::unique_ptr<unsigned char[]> _picBuffer(new unsigned char[_bufferSize]); // Использование уникального указателя

    int widthIm = _InputBmp.TellWidth();
    int heightIm = _InputBmp.TellHeight();

    int* threadNums = new int[numThreads];
    HANDLE* handles = new HANDLE[numThreads];
    std::vector<ThreadParams> params(numThreads);

    int rowsPerThread = heightIm / numThreads;

    // Запуск потоков
    for (int i = 0; i < numThreads; i++)
    {
        params[i].bmp = &_InputBmp;
        params[i].buffer = _picBuffer.get();
        params[i].startRow = i * rowsPerThread;
        params[i].endRow = (i == numThreads - 1) ? heightIm : (i + 1) * rowsPerThread;  // Последний поток обрабатывает остаток
        params[i].threadNum = i + 1;
        params[i].startTime = startTime;
        params[i].coreAffinity = (1 << (i % numCores)); // Привязка к конкретному ядру

        handles[i] = CreateThread(NULL, 0, ThreadProc, &params[i], CREATE_SUSPENDED, NULL);
    }

    for (int i = 0; i < numThreads; ++i)
    {
        ResumeThread(handles[i]);
    }

    // Ожидание завершения всех потоков
    WaitForMultipleObjects(numThreads, handles, TRUE, INFINITE);


    // Запись буфера обратно в BMP объект
    for (int j = 0; j < _InputBmp.TellHeight(); j++)
    {
        for (int i = 0; i < _InputBmp.TellWidth(); i++)
        {
            RGBApixel newPix = GetPixFromPicArray(_picBuffer.get(), _InputBmp, i, j);
            _InputBmp.SetPixel(i, j, newPix);  // Пример вызова метода SetPixel
        }
    }

    // Сохранение измененного изображения в файл
    _InputBmp.WriteToFile("out.bmp");

    //auto end = std::chrono::steady_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Threads: " << numThreads << " Time: " << std::endl;
    return 0;
}