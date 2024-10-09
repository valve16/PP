#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include "EasyBMP.h"
#include <memory>


using namespace std;

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

int main()
{
    SetConsoleOutputCP(1251);
    BMP _InputBmp;
    _InputBmp.ReadFromFile("in2.bmp");

    // Вывод информации о изображении
    cout << "BitDepth " << _InputBmp.TellBitDepth() << endl
        << "NumberOfColor " << _InputBmp.TellNumberOfColors() << endl
        << "Width " << _InputBmp.TellWidth() << endl
        << "Height " << _InputBmp.TellHeight() << endl;
    int widthIm = _InputBmp.TellWidth();
    int heightIm = _InputBmp.TellHeight();
    // Создание буфера для изображения
    int _bufferSize = _InputBmp.TellWidth() * _InputBmp.TellHeight() * 3;
    cout << "bufferSize " << _bufferSize << endl;
    unique_ptr<unsigned char[]> _picBuffer(new unsigned char[_bufferSize]); // Использование уникального указателя

    cout << "WritePix2PicArray" << endl;

    // Запись пикселей изображения в буфер
    for (int j = 0; j < _InputBmp.TellHeight(); j++)
    {
        for (int i = 0; i < _InputBmp.TellWidth(); i++)
        {
            WritePix2PicArray(_picBuffer.get(), _InputBmp, i, j, _InputBmp.GetPixel(i, j));
        }
    }

    cout << "Change some pix data" << endl;

    // Изменение части пикселей в буфере (синие пиксели в заданном диапазоне)
    const int blurSize = 3; // Количество пикселей по обе стороны от текущего

    for (int j = 100; j < min(heightIm, _InputBmp.TellHeight()); j++) {  // Проверка диапазона
        for (int i = 10; i < min(widthIm, _InputBmp.TellWidth()); i++) { // Проверка диапазона
            int sumR = 0, sumG = 0, sumB = 0;
            int count = 0;

            // Проход по окну размытия в горизонтальном направлении
            for (int x = -blurSize; x <= blurSize; x++) {
                int neighborX = i + x;
                int neighborY = j;

                // Проверка границ
                if (neighborX >= 0 && neighborX < _InputBmp.TellWidth()) {
                    RGBApixel neighborPix = _InputBmp.GetPixel(neighborX, neighborY);
                    sumR += neighborPix.Red;
                    sumG += neighborPix.Green;
                    sumB += neighborPix.Blue;
                    count++;
                }
            }

            // Установка нового значения пикселя
            RGBApixel _pixData;
            _pixData.Red = sumR / count;
            _pixData.Green = sumG / count;
            _pixData.Blue = sumB / count;

            WritePix2PicArray(_picBuffer.get(), _InputBmp, i, j, _pixData);
        }
    }

    // Проверка изменения пикселя
    RGBApixel _pix = GetPixFromPicArray(_picBuffer.get(), _InputBmp, 15, 150);
    cout << (int)_pix.Red << " " << (int)_pix.Green << " " << (int)_pix.Blue << endl;

    cout << "WritePicArrayToBMP" << endl;

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
    _InputBmp.WriteToFile("test-out.bmp");

    cout << "Изменения успешно применены и файл сохранен." << endl;

    return 0;
}