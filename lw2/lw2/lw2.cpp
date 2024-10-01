#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <string>

const int NUM_THR = 1;

#pragma pack(push, 1) // Пакетирование структур для правильного чтения

struct BMPFileHeader {
    uint16_t bfType;      // Тип файла (0x4D42 для "BM")
    uint32_t bfSize;      // Размер файла в байтах
    uint16_t bfReserved1; // Зарезервировано
    uint16_t bfReserved2; // Зарезервировано
    uint32_t bfOffBits;   // Смещение до данных
};

struct BMPInfoHeader {
    uint32_t biSize;          // Размер заголовка
    int32_t biWidth;          // Ширина изображения
    int32_t biHeight;         // Высота изображения
    uint16_t biPlanes;        // Количество плоскостей
    uint16_t biBitCount;      // Количество бит на пиксель
    uint32_t biCompression;    // Тип сжатия
    uint32_t biSizeImage;     // Размер изображения
    int32_t biXPelsPerMeter;   // Горизонтальное разрешение
    int32_t biYPelsPerMeter;   // Вертикальное разрешение
    uint32_t biClrUsed;       // Количество цветов
    uint32_t biClrImportant;   // Важные цвета
};

#pragma pack(pop) // Вернуть упаковку структур к стандартной

bool readBMP(const std::string& filename, std::vector<uint8_t>& imageData, int& width, int& height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open BMP file: " << filename << std::endl;
        return false;
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    width = infoHeader.biWidth;
    height = infoHeader.biHeight;
    std::cout << "File Type: " << fileHeader.bfType << std::endl;
    std::cout << "Bit Count: " << infoHeader.biBitCount << std::endl;
    std::cout << "Width: " << infoHeader.biWidth << ", Height: " << infoHeader.biHeight << std::endl;
    // Проверка формата BMP
    if (fileHeader.bfType != 0x4D42) {
        std::cerr << "Unsupported BMP format." << std::endl;
        return false;
    }

    // Расчет размера данных и чтение пикселей
    imageData.resize(height * width * 3);
    file.seekg(fileHeader.bfOffBits);
    file.read(reinterpret_cast<char*>(imageData.data()), imageData.size());

    return true;
}

bool saveBMP(const std::string& filename, const std::vector<uint8_t>& imageData, int width, int height) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open output BMP file: " << filename << std::endl;
        return false;
    }

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    fileHeader.bfType = 0x4D42; // "BM"
    fileHeader.bfSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + imageData.size();
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

    infoHeader.biSize = sizeof(BMPInfoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = imageData.size();
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;

    file.write(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.write(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
    file.write(reinterpret_cast<const char*>(imageData.data()), imageData.size());

    return true;
}
void gaussianBlur(std::vector<uint8_t>& imageData, int width, int height, int startRow, int endRow) {
    const float kernel[3][3] = {
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f},
        {2 / 16.0f, 4 / 16.0f, 2 / 16.0f},
        {1 / 16.0f, 2 / 16.0f, 1 / 16.0f}
    };

    std::vector<uint8_t> outputData = imageData;

    for (int y = startRow; y < endRow; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            float r = 0, g = 0, b = 0;

            // Применение ядра размытия
            for (int ky = -1; ky <= 1; ++ky) {
                for (int kx = -1; kx <= 1; ++kx) {
                    int neighborX = x + kx;
                    int neighborY = y + ky;

                    // Проверка границ
                    if (neighborX >= 0 && neighborX < width && neighborY >= 0 && neighborY < height)
                    {
                        int pixelIndex = (neighborY * width + neighborX) * 3;
                        r += outputData[pixelIndex + 2] * kernel[ky + 1][kx + 1]; // R
                        g += outputData[pixelIndex + 1] * kernel[ky + 1][kx + 1]; // G
                        b += outputData[pixelIndex] * kernel[ky + 1][kx + 1]; // B
                    }
                }
            }

            int outputIndex = (y * width + x) * 3;
            imageData[outputIndex + 2] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
            imageData[outputIndex + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
            imageData[outputIndex] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
        }
    }
}
void blurImage(std::vector<uint8_t>& imageData, int width, int height, int numThreads) {
    std::vector<std::thread> threads;
    int rowsPerThread = height / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? height : startRow + rowsPerThread;
        threads.emplace_back(gaussianBlur, std::ref(imageData), width, height, startRow, endRow);
    }

    for (auto& thread : threads) {
        thread.join();
    }
}
int main(int argc, char* argv[]) {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    /*if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <input.bmp> <output.bmp> <numThreads>" << std::endl;
        return 1;
    }*/

    //std::string inputFile = argv[1];
    //std::string outputFile = argv[2];
    //int numThreads = std::stoi(argv[3]);
    std::string inputFile = "in2.bmp";
    std::string outputFile = "out.bmp";
    int numThreads = NUM_THR;

    int width, height;
    std::vector<uint8_t> imageData;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Чтение изображения
    if (!readBMP(inputFile, imageData, width, height)) {
        return 1;
    }

    // Применение размытия
    blurImage(imageData, width, height, numThreads);

    // Сохранение изображения
    if (!saveBMP(outputFile, imageData, width, height)) {
        return 1;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTime - startTime;
    std::cout << duration.count() << " мс\n";
}