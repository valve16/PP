
#include <iostream>
#include <omp.h>
#include <cmath>
#include <windows.h>
#include <chrono>


constexpr long long NUM_STEPS = 100'000'000;


double calculatePiSync() 
{
    double res = 0.0;
    for (long long i = 0; i < NUM_STEPS; ++i) 
    {
        double term = pow(-1.0, i) / (2.0 * i + 1.0);
        res += term;
    }
    return res * 4.0;
}

double calculatePiParallelWrong() 
{
    double res = 0.0;
#pragma omp parallel for
    for (long long i = 0; i < NUM_STEPS; ++i) 
    {
        double term = pow(-1.0, i) / (2.0 * i + 1.0);
        res += term; 

    }
    return res * 4.0;
}

double calculatePiParallelAtomic() 
{
    double res = 0.0;
#pragma omp parallel for
    for (long long i = 0; i < NUM_STEPS; ++i) 
    {
        double term = pow(-1.0, i) / (2.0 * i + 1.0);
#pragma omp atomic
        res += term;
    }
    return res * 4.0;
}

double calculatePiReduction() 
{
    double res = 0.0;
#pragma omp parallel for reduction(+:res)
    for (long long i = 0; i < NUM_STEPS; ++i) 
    {
        double term = pow(-1.0, i) / (2.0 * i + 1.0);
        res += term;
    }
    return res * 4.0;
}

int main() 
{
    SetConsoleOutputCP(1251);
    auto start = std::chrono::high_resolution_clock::now();
    double piSync = calculatePiSync();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "PI (синхронно): " << piSync << " Time: " << std::chrono::duration<double>(end - start).count() << std::endl;

    // Параллельный расчет (неправильно)
    start = std::chrono::high_resolution_clock::now();
    double piParallelWrong = calculatePiParallelWrong();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "PI (parallel for, неверно): " << piParallelWrong << " Time: " << std::chrono::duration<double>(end - start).count() << std::endl;

    // Параллельный расчет с atomic
    start = std::chrono::high_resolution_clock::now();
    double piParallelAtomic = calculatePiParallelAtomic();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "PI (parallel for с atomic): " << piParallelAtomic << " Time: " << std::chrono::duration<double>(end - start).count() << std::endl;

    // Параллельный расчет с reduction
    start = std::chrono::high_resolution_clock::now();
    double piReduction = calculatePiReduction();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "PI (reduction): " << piReduction << " Time: " << std::chrono::duration<double>(end - start).count() << std::endl;

    return 0;
}