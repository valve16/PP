#include <windows.h>
#include <string>
#include <iostream>
#include "tchar.h"
#include <fstream>

HANDLE FileMutex;

int ReadFromFile() {
    WaitForSingleObject(FileMutex, INFINITE);

    std::fstream myfile("balance.txt", std::ios_base::in);
    int result;
    myfile >> result;
    myfile.close();

    ReleaseMutex(FileMutex);

    return result;
}

void WriteToFile(int data) {
    WaitForSingleObject(FileMutex, INFINITE);

    std::fstream myfile("balance.txt", std::ios_base::out);
    myfile << data << std::endl;
    myfile.close();

    ReleaseMutex(FileMutex);
}

int GetBalance() {
    return ReadFromFile();
}

void Deposit(int money) {
    WaitForSingleObject(FileMutex, INFINITE);

    int balance = GetBalance();
    balance += money;
    WriteToFile(balance);
    printf("Balance after deposit: %d\n", balance);

    ReleaseMutex(FileMutex);
}

void Withdraw(int money) {
    WaitForSingleObject(FileMutex, INFINITE);

    if (GetBalance() < money) {
        printf("Cannot withdraw money, balance lower than %d\n", money);

        ReleaseMutex(FileMutex);
        return;
    }
    Sleep(20);
    int balance = GetBalance();
    balance -= money;
    WriteToFile(balance);
    printf("Balance after withdraw: %d\n", balance);

    ReleaseMutex(FileMutex);
}

DWORD WINAPI DoDeposit(CONST LPVOID lpParameter) {
    Deposit((int)lpParameter);
    ExitThread(0);
}

DWORD WINAPI DoWithdraw(CONST LPVOID lpParameter) {
    Withdraw((int)lpParameter);
    ExitThread(0);
}

int _tmain(int argc, _TCHAR* argv[]) {
    HANDLE* handles = new HANDLE[49];
    FileMutex = CreateMutex(NULL, FALSE, L"FileMutex");
    if (FileMutex == NULL) {
        printf("CreateMutex error: %d\n", GetLastError());
        return 1;
    }

    WriteToFile(0);

    SetProcessAffinityMask(GetCurrentProcess(), 1);
    for (int i = 0; i < 50; i++) {
        handles[i] = (i % 2 == 0)
            ? CreateThread(NULL, 0, &DoDeposit, (LPVOID)230, CREATE_SUSPENDED, NULL)
            : CreateThread(NULL, 0, &DoWithdraw, (LPVOID)1000, CREATE_SUSPENDED, NULL);
        ResumeThread(handles[i]);
    }

    WaitForMultipleObjects(50, handles, TRUE, INFINITE);
    printf("Final Balance: %d\n", GetBalance());

    getchar();

    CloseHandle(FileMutex);
    return 0;
}