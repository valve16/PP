#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::chrono;

#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t fileType{ 0x4D42 }; // Тип файла (должен быть 'BM')
    uint32_t fileSize{ 0 };       // Размер файла
    uint16_t reserved1{ 0 };      // Зарезервировано
    uint16_t reserved2{ 0 };      // Зарезервировано
    uint32_t offsetData{ 54 };     // Смещение до начала данных изображения
};

struct BMPInfoHeader {
    uint32_t size{ 40 };           // Размер этого заголовка (40 байт)
    int32_t width{ 0 };            // Ширина изображения
    int32_t height{ 0 };           // Высота изображения
    uint16_t planes{ 1 };          // Количество цветовых плоскостей (всегда 1)
    uint16_t bitCount{ 24 };       // Количество бит на пиксель (например, 24 для RGB)
    uint32_t compression{ 0 };     // Тип сжатия (0 = нет сжатия)
    uint32_t sizeImage{ 0 };       // Размер изображения в байтах (может быть 0 для несжатых изображений)
    int32_t xPixelsPerMeter{ 0 };  // Горизонтальное разрешение
    int32_t yPixelsPerMeter{ 0 };  // Вертикальное разрешение
    uint32_t colorsUsed{ 0 };      // Количество цветов в палитре (0 = все цвета)
    uint32_t colorsImportant{ 0 }; // Количество важных цветов (0 = все важны)
};
#pragma pack(pop)

struct RGB {
    uint8_t blue{ 0 };
    uint8_t green{ 0 };
    uint8_t red{ 0 };
};

void blurSegment(vector<vector<RGB>>& image, int startX, int endX, int width, int height) {
    vector<vector<RGB>> temp = image;

    for (int x = startX; x < endX; ++x) {
        for (int y = 0; y < height; ++y) {
            int r = 0, g = 0, b = 0, count = 0;

            for (int dx = -5; dx <= 5; ++dx) {
                for (int dy = -5; dy <= 5; ++dy) {
                    int nx = x + dx;
                    int ny = y + dy;

                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        r += image[ny][nx].red;
                        g += image[ny][nx].green;
                        b += image[ny][nx].blue;
                        ++count;
                    }
                }
            }

            if (count > 0) { // Проверка на деление на ноль
                temp[y][x].red = r / count;
                temp[y][x].green = g / count;
                temp[y][x].blue = b / count;
            }
        }
    }

    for (int x = startX; x < endX; ++x) {
        for (int y = 0; y < height; ++y) {
            image[y][x] = temp[y][x];
        }
    }
}

bool loadBMP(const char* filename, BMPFileHeader& fileHeader, BMPInfoHeader& infoHeader, RGB*& data) {
    ifstream file(filename, ios::binary);
    if (!file) return false;

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (fileHeader.fileType != 0x4D42) {
        file.close();
        return false;
    }

    int width = infoHeader.width;
    int height = infoHeader.height;

    data = new RGB[width * height];
    file.seekg(fileHeader.offsetData, ios::beg);

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            file.read(reinterpret_cast<char*>(&data[y * width + x]), sizeof(RGB));
        }
    }

    file.close();
    return true;
}

bool saveBMP(const char* filename, const BMPFileHeader& fileHeader, const BMPInfoHeader& infoHeader, RGB* data) {
    ofstream file(filename, ios::binary);
    if (!file) return false;

    BMPFileHeader updatedFileHeader = fileHeader;
    updatedFileHeader.fileSize = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + (infoHeader.width * infoHeader.height * sizeof(RGB));

    file.write(reinterpret_cast<const char*>(&updatedFileHeader), sizeof(updatedFileHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    int width = infoHeader.width;
    int height = infoHeader.height;

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            file.write(reinterpret_cast<const char*>(&data[y * width + x]), sizeof(RGB));
        }
    }

    file.close();
    return true;
}

int main(int argc, char* argv[]) {
    const char* inputFile = "in1.bmp";
    const char* outputFile = "out.bmp";
    const int numThreads = 12; // Количество потоков, установите нужное значение

    auto start = steady_clock::now();

    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;
    RGB* data = nullptr;

    if (!loadBMP(inputFile, fileHeader, infoHeader, data)) {
        cout << "Error loading BMP file." << endl;
        return 1;
    }

    int width = infoHeader.width;
    int height = infoHeader.height;

    vector<vector<RGB>> image(height, vector<RGB>(width));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image[y][x] = data[y * width + x];
        }
    }

    delete[] data;

    vector<thread> threads;
    int segmentWidth = width / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int startX = i * segmentWidth;
        int endX = (i == numThreads - 1) ? width : (i + 1) * segmentWidth;
        threads.emplace_back(blurSegment, ref(image), startX, endX, width, height);
    }

    for (auto& t : threads) {
        t.join();
    }

   

    data = new RGB[width * height];
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            data[y * width + x] = image[y][x];
        }
    }

    if (!saveBMP(outputFile, fileHeader, infoHeader, data)) {
        cout << "Error saving BMP file." << endl;
        delete[] data;
        return 1;
    }
    auto end = steady_clock::now();
    auto duration = duration_cast<milliseconds>(end - start).count();
    cout << "Execution time: " << duration << " ms" << endl;
    delete[] data;
    return 0;
}



//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <thread>
//#include <chrono>
//#include <windows.h>
//
//using namespace std;
//using namespace std::chrono;
//const int NUM_THR = 12;
//
//#pragma pack(push, 1)
//struct BMPFileHeader {
//    uint16_t bfType;      // Тип файла, должен быть 'BM'
//    uint32_t bfSize;      // Размер файла
//    uint16_t bfReserved1; // Зарезервировано
//    uint16_t bfReserved2; // Зарезервировано
//    uint32_t bfOffBits;   // Смещение данных изображения от начала файла
//};
//
//struct BMPInfoHeader {
//    uint32_t biSize;         // Размер структуры BITMAPINFOHEADER
//    int32_t biWidth;         // Ширина изображения
//    int32_t biHeight;        // Высота изображения
//    uint16_t biPlanes;       // Количество плоскостей (должно быть 1)
//    uint16_t biBitCount;     // Количество бит на пиксель (24 для RGB)
//    uint32_t biCompression;  // Тип сжатия (0 = несжатый)
//    uint32_t biSizeImage;    // Размер изображения
//    int32_t biXPelsPerMeter; // Горизонтальное разрешение
//    int32_t biYPelsPerMeter; // Вертикальное разрешение
//    uint32_t biClrUsed;      // Количество используемых цветов
//    uint32_t biClrImportant; // Количество важных цветов
//};
//#pragma pack(pop)
//
//struct Pixel {
//    uint8_t blue;
//    uint8_t green;
//    uint8_t red;
//};
//
//// Функция для размытия (простой боксовый фильтр)
//void blurSegment(vector<vector<Pixel>>& image, int startX, int endX, int width, int height) {
//    vector<vector<Pixel>> temp = image; // Временная копия изображения
//
//    for (int x = startX; x < endX; ++x) {
//        for (int y = 0; y < height; ++y) {
//            int r = 0, g = 0, b = 0, count = 0;
//
//            // Бокс-фильтр, считаем среднее значение для 3x3 окрестности
//            for (int dx = -2; dx <= 2; ++dx) {
//                for (int dy = -2; dy <= 2; ++dy) {
//                    int nx = x + dx;
//                    int ny = y + dy;
//
//                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
//                        r += image[ny][nx].red;
//                        g += image[ny][nx].green;
//                        b += image[ny][nx].blue;
//                        ++count;
//                    }
//                }
//            }
//
//            temp[y][x].red = r / count;
//            temp[y][x].green = g / count;
//            temp[y][x].blue = b / count;
//        }
//    }
//
//    // Обновляем оригинальное изображение
//    for (int x = startX; x < endX; ++x) {
//        for (int y = 0; y < height; ++y) {
//            image[y][x] = temp[y][x];
//        }
//    }
//}
//
//// Функция для чтения BMP-файла
//bool readBMP(const char* filename, BMPFileHeader& fileHeader, BMPInfoHeader& infoHeader, vector<vector<Pixel>>& image) {
//    ifstream file(filename, ios::binary);
//    if (!file) {
//        cout << "Ошибка: не удается открыть файл." << endl;
//        return false;
//    }
//
//    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
//    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));
//
//    if (fileHeader.bfType != 0x4D42) { // Проверяем, что это BMP файл
//        cout << "Ошибка: это не BMP файл." << endl;
//        return false;
//    }
//
//    file.seekg(fileHeader.bfOffBits, ios::beg);
//
//    int width = infoHeader.biWidth;
//    int height = infoHeader.biHeight;
//
//    image.resize(height, vector<Pixel>(width));
//
//    int padding = (4 - (width * sizeof(Pixel)) % 4) % 4;// Padding до 4 байт
//
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            Pixel pixel;
//            file.read(reinterpret_cast<char*>(&pixel), sizeof(pixel));
//            image[height - y - 1][x] = pixel; // Изображение перевернуто по вертикали
//        }
//        file.ignore(padding);
//    }
//
//    file.close();
//    return true;
//}
//
//// Функция для записи BMP-файла
//void writeBMP(const char* filename, const BMPFileHeader& fileHeader, const BMPInfoHeader& infoHeader, const vector<vector<Pixel>>& image) {
//    ofstream file(filename, ios::binary);
//    if (!file) {
//        cout << "Ошибка: не удается открыть файл для записи." << endl;
//        return;
//    }
//
//    file.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
//    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
//
//    int width = infoHeader.biWidth;
//    int height = infoHeader.biHeight;
//    int padding = (4 - (width * sizeof(Pixel)) % 4) % 4;
//
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            file.write(reinterpret_cast<const char*>(&image[height - y - 1][x]), sizeof(Pixel));
//        }
//        file.write("\0\0\0", padding); // Пишем padding
//    }
//
//    file.close();
//}
//
//// Главная функция
//int main(int argc, char* argv[]) {
//    //if (argc < 5) {
//    //    cout << "Usage: <input.bmp> <output.bmp> <threads> <cores>" << endl;
//    //    return 1;
//    //}
//
//  /*  const char* inputFile = argv[1];
//    const char* outputFile = argv[2];
//    int numThreads = atoi(argv[3]);
//    int numCores = atoi(argv[4]);*/
//
//
//    const char* inputFile = "in1.bmp";
//    const char* outputFile = "out.bmp";
//    int numThreads = NUM_THR;
//    //int numCores = atoi(argv[4]);
//
//    BMPFileHeader fileHeader;
//    BMPInfoHeader infoHeader;
//    vector<vector<Pixel>> image;
//
//    if (!readBMP(inputFile, fileHeader, infoHeader, image)) {
//        return 1;
//    }
//
//    int width = infoHeader.biWidth;
//    int height = infoHeader.biHeight;
//
//    auto start = steady_clock::now();
//
//    // Вертикальное деление по потокам
//    vector<thread> threads;
//    int segmentWidth = width / numThreads;
//    for (int i = 0; i < numThreads; ++i) {
//        int startX = i * segmentWidth;
//        int endX = (i == numThreads - 1) ? width : (i + 1) * segmentWidth;
//        threads.emplace_back(blurSegment, ref(image), startX, endX, width, height);
//    }
//
//    for (auto& t : threads) {
//        t.join();
//    }
//
//    auto end = steady_clock::now();
//    auto duration = duration_cast<milliseconds>(end - start).count();
//    cout << "Execution time: " << duration << " ms" << endl;
//
//    writeBMP(outputFile, fileHeader, infoHeader, image);
//
//    return 0;
//}