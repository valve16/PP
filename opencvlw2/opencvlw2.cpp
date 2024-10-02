#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <vector>
#include <fstream>

using namespace std;
using namespace cv;

void blurSegment(const Mat& input, Mat& output, int startX, int endX) {
    Mat segment = input(Rect(startX, 0, endX - startX, input.rows));
    GaussianBlur(segment, output(Rect(startX, 0, endX - startX, input.rows)), Size(11, 11), 0);
}

int main(int argc, char* argv[]) {
    const char* inputFile = "D:/downloads/in1.bmp";
    const char* outputFile = "out.bmp";
    const int numThreads = 4; // Установите нужное количество потоков

    // Загружаем изображение

    Mat image = imread(inputFile, IMREAD_COLOR);
    if (image.empty()) {
        cout << "Error loading BMP file." << endl;
        return 1;
    }

    int width = image.cols;
    int height = image.rows;
    Mat output = image.clone();

    // Создаем векторы потоков
    vector<thread> threads;
    int segmentWidth = width / numThreads;

    // Запускаем потоки для размытия изображения
    for (int i = 0; i < numThreads; ++i) {
        int startX = i * segmentWidth;
        int endX = (i == numThreads - 1) ? width : (i + 1) * segmentWidth;
        threads.emplace_back(blurSegment, ref(image), ref(output), startX, endX);
    }

    // Ожидаем завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    // Сохраняем размазанное изображение
    if (!imwrite(outputFile, output)) {
        cout << "Error saving BMP file." << endl;
        return 1;
    }

    cout << "Image processing completed successfully." << endl;
    return 0;
}