// SharedMemeryServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <string>
#include <tchar.h>
using namespace std;

#define BUF_SIZE			1024
#define OBJECT_NUM	2

TCHAR szShareMemoryName[] = TEXT("Global\\MyFileMappingObject");    //指向同一块共享内存的名字
TCHAR szMutexName[] = TEXT("Global\\MyMutexObject");    //指向互斥量的名字
TCHAR szSemaphoreReadCName[] = TEXT("Global\\MySemaphoreReadCObject");    //指向客户端读信号量的名字
TCHAR szSemaphoreWriteCName[] = TEXT("Global\\MySemaphoreWriteCObject");    //指向客户端写信号量的名字
TCHAR szSemaphoreReadSName[] = TEXT("Global\\MySemaphoreReadSObject");    //指向服务器读信号量的名字
TCHAR szSemaphoreWriteSName[] = TEXT("Global\\MySemaphoreWriteSObject");    //指向服务器写信号量的名字

int _tmain(int argc, _TCHAR* argv[])
{
	//注意这些声明
	HANDLE hMapFile = NULL;
	HANDLE hMutex = NULL;
	HANDLE hSemaphoreReadC = NULL;
	HANDLE hSemaphoreWriteC = NULL;
	HANDLE hSemaphoreReadS = NULL;
	HANDLE hSemaphoreWriteS = NULL;
	HANDLE handleObject[OBJECT_NUM];
	LPCTSTR pBuf;
	DWORD res = 0xFFFFEEEE;

#pragma region Mutex
	//创建互斥体，是其他进程等待互斥体
	hMutex = CreateMutex(NULL, TRUE, szMutexName);
	if (hMutex)
	{
		cout<<"互斥量创建成功！"<<endl;
	}
	else
	{
		cout << "互斥量创建失败！" << endl;
		system("pause");
		return 1;
	}
	ReleaseMutex(hMutex);	//离开互斥区
#pragma endregion Mutex

#pragma region Semphore
	hSemaphoreReadC = CreateSemaphore(NULL, 10, 10, szSemaphoreReadCName);
	if (hSemaphoreReadC == NULL)
	{
		cout << "客户端读信号量创建失败！" << endl;
		system("pause");
		return 1;
	}
	//ReleaseSemaphore(hSemaphoreRead, 1, NULL);  //信号量资源数加一

	hSemaphoreWriteC = CreateSemaphore(NULL, 0, 1, szSemaphoreWriteCName);
	if (hSemaphoreWriteC == NULL)
	{
		cout << "客户端写信号量创建失败！" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreReadS = CreateSemaphore(NULL, 0, 10, szSemaphoreReadSName);
	if (hSemaphoreReadS == NULL)
	{
		cout << "服务器读信号量创建失败！" << endl;
		system("pause");
		return 1;
	}
	//ReleaseSemaphore(hSemaphoreRead, 1, NULL);  //信号量资源数加一

	hSemaphoreWriteS = CreateSemaphore(NULL, 0, 1, szSemaphoreWriteSName);
	if (hSemaphoreWriteS == NULL)
	{
		cout << "服务器写信号量创建失败！" << endl;
		system("pause");
		return 1;
	}

	ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //信号量资源数加一
	ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //信号量资源数加一
#pragma endregion Semphore

	handleObject[0] = hSemaphoreReadS;
	handleObject[1] = hSemaphoreWriteS;

#pragma region ShareMemory
	//创建内存共享区域
	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		BUF_SIZE,                // maximum object size (low-order DWORD)
		szShareMemoryName);                 // name of mapping object

	if (hMapFile == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
			GetLastError());
		system("pause");
		return 1;
	}

	//将内存映射到该进程
	pBuf = (LPTSTR)MapViewOfFile(hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		BUF_SIZE);

	if (pBuf == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());
		CloseHandle(hMapFile);
		system("pause");
		return 1;
	}
#pragma endregion ShareMemory

	//以下代码，B不停写共享内存pBuf
	float *Transdata = new float[5];
	for (int i = 0; i < 5; i++)
	{
		Transdata[i] = 0.0f;
	}
	//res = WaitForSingleObject(hMutex, 20000);
	//cout << "客户端已准备好！ " << endl;
	while (1)
	{
		res = WaitForMultipleObjects(OBJECT_NUM, handleObject, false, INFINITE);
		switch (res)
		{
		case WAIT_OBJECT_0:
			//可读
			memcpy(Transdata, (PVOID)pBuf, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //信号量资源数加一
			cout << "服务器读： " << Transdata[0] << endl;
			break;
		case (WAIT_OBJECT_0 + 1) :
			//可写
			memcpy((PVOID)pBuf, Transdata, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //信号量资源数加一
			cout << "服务器写： " << Transdata[0] << endl;
			for (int i = 0; i < 5; i++)
			{
				Transdata[i] += 1.0f;
			}
			break;
		default:
			break;
		}
		if (Transdata[0] >= 100.0f) break;

		Sleep(1000);
	}
		
	CloseHandle(hSemaphoreWriteC); //销毁信号量
	CloseHandle(hSemaphoreReadC); //销毁信号量
	CloseHandle(hSemaphoreWriteS); //销毁信号量
	CloseHandle(hSemaphoreReadS); //销毁信号量
	CloseHandle(hMutex);
	CloseHandle(hMapFile);
	delete[] Transdata;
	Transdata = nullptr;

	getchar();
	return 0;
}

