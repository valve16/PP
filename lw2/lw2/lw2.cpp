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

    // Привязка потока к конкретному ядру
    SetThreadAffinityMask(GetCurrentThread(), params->coreAffinity);

    for (int j = params->startRow; j < params->endRow; j++) 
    {
        for (int i = 0; i < widthIm; i++) 
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
        }
    }
    ExitThread(0);
}


int main(int argc, char* argv[]) 
{
    if (argc != 4) 
    {
        std::cerr << "Usage: " << argv[0] << " <image_path> <num_threads> <num_cores>" << std::endl;
        return -1;
    }
    const std::string filename = argv[1];
    int numThreads = std::atoi(argv[2]);
    int numCores = std::atoi(argv[3]);

    auto start = std::chrono::steady_clock::now();
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
        params[i].coreAffinity = (1 << (i % numCores)); // Привязка к конкретному ядру

        handles[i] = CreateThread(NULL, 0, ThreadProc, &params[i], 0, NULL);
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

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Threads: " << numThreads << " Time: " << duration << std::endl;
    return 0;
}