// SharedMemeryClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <string>
#include <tchar.h>
using namespace std;


#define BUF_SIZE			1024
#define OBJECT_NUM	2

TCHAR szShareMemorySWriteName[] = TEXT("Global\\MyFileMappingSWriteObject");    //指向服务端写共享内存的名字
TCHAR szShareMemoryCWriteName[] = TEXT("Global\\MyFileMappingCWriteObject");    //指向客户端写共享内存的名字
TCHAR szMutexName[] = TEXT("Global\\MyMutexObject");    //指向互斥量的名字
TCHAR szSemaphoreReadCName[] = TEXT("Global\\MySemaphoreReadCObject");    //指向客户端读信号量的名字
TCHAR szSemaphoreWriteCName[] = TEXT("Global\\MySemaphoreWriteCObject");    //指向客户端写信号量的名字
TCHAR szSemaphoreReadSName[] = TEXT("Global\\MySemaphoreReadSObject");    //指向服务器读信号量的名字
TCHAR szSemaphoreWriteSName[] = TEXT("Global\\MySemaphoreWriteSObject");    //指向服务器写信号量的名字

int _tmain(int argc, _TCHAR* argv[])
{
	//查询所有，根据szMutexName 打开互斥量 
	HANDLE hMapFileSWrite = NULL;
	HANDLE hMapFileCWrite = NULL;
	HANDLE hMutex = NULL;
	HANDLE hSemaphoreReadC = NULL;
	HANDLE hSemaphoreWriteC = NULL;
	HANDLE hSemaphoreReadS = NULL;
	HANDLE hSemaphoreWriteS = NULL;
	HANDLE handleObject[OBJECT_NUM];
	LPCTSTR pBufSWrite;
	LPCTSTR pBufCWrite;
	DWORD res = 0xFFFFEEEE;

#pragma region Mutex
	hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, szMutexName);
	if (hMutex)
	{
		cout << "互斥量打开成功！" << endl;
	}
	else
	{
		cout << "互斥量打开失败！" << endl;
		system("pause");
		return 1;
	}

	cout << "等待... ... " << endl;
	res = WaitForSingleObject(hMutex, 20000);
	switch (res)
	{
	case WAIT_OBJECT_0:
		cout << "收到信号... ... " << endl;
		break;
	case WAIT_TIMEOUT:
		cout << "超时没有收到... ... " << endl;
		break;
	case WAIT_ABANDONED:
		cout << "另一个进程意外终止... ... " << endl;
		break;
	default:
		break;
	}
#pragma endregion Mutex

#pragma region Semphore
	hSemaphoreReadC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreReadCName);
	if (hSemaphoreReadC == NULL)
	{
		cout << "客户端读信号量打开失败！" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreWriteC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreWriteCName);
	if (hSemaphoreWriteC == NULL)
	{
		cout << "客户端写信号量打开失败！" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreWriteS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreWriteSName);
	if (hSemaphoreWriteS == NULL)
	{
		cout << "服务器写信号量打开失败！" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreReadS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreReadSName);
	if (hSemaphoreReadS == NULL)
	{
		cout << "服务器读信号量打开失败！" << endl;
		system("pause");
		return 1;
	}
	ReleaseSemaphore(hSemaphoreReadS, 1, NULL);  //服务端可读信号量资源数加一
	ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //客户端可写信号量资源数加一
#pragma endregion Semphore

	handleObject[0] = hSemaphoreReadS;	//服务端可读信号
	handleObject[1] = hSemaphoreWriteC;	//客户端可写信号

#pragma region ShareMemory
	//创建内存共享区域
	hMapFileSWrite = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, szShareMemorySWriteName);
	if (hMapFileSWrite == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
			GetLastError());
		system("pause");
		return 1;
	}
	//将内存映射到该进程
	pBufSWrite = (LPTSTR)MapViewOfFile(hMapFileSWrite,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		BUF_SIZE);
	if (pBufSWrite == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());
		CloseHandle(hMapFileSWrite);
		system("pause");
		return 1;
	}

	//创建内存共享区域
	hMapFileCWrite = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, szShareMemoryCWriteName);
	if (hMapFileCWrite == NULL)
	{
		_tprintf(TEXT("Could not create file mapping object (%d).\n"),
			GetLastError());
		system("pause");
		return 1;
	}
	//将内存映射到该进程
	pBufCWrite = (LPTSTR)MapViewOfFile(hMapFileCWrite,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		BUF_SIZE);
	if (pBufCWrite == NULL)
	{
		_tprintf(TEXT("Could not map view of file (%d).\n"),
			GetLastError());
		CloseHandle(hMapFileCWrite);
		system("pause");
		return 1;
	}
#pragma endregion ShareMemory

	float *Transdata = new float[5];
	for (int i = 0; i < 5; i++)
	{
		Transdata[i] = 0.0f;
	}
	ReleaseMutex(hMutex);	//离开互斥区
	cout << "服务器已准备好！" << endl;

	while (true)
	{
		res = WaitForMultipleObjects(OBJECT_NUM, handleObject, false, INFINITE);
		switch (res)
		{
		case WAIT_OBJECT_0:
			//服务器可读
			memcpy(Transdata, (PVOID)pBufSWrite, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //服务端可写信号量资源数加一
			cout << "客户端读： " << Transdata[0] << endl;
			break;
		case (WAIT_OBJECT_0 + 1) :
			//客户端可写
			memcpy((PVOID)pBufCWrite, Transdata, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //客户端可读信号量资源数加一
			cout << "客户端写： " << Transdata[0] << endl;
			for (int i = 0; i < 5; i++)
			{
				Transdata[i] += 1.0f;
			}
			//Sleep(350);
			break;
		default:
			break;
		}
		if (Transdata[0] >= 100.0f) break;
		Sleep(1000);
	}
	//一旦不再需要，注意一定要用 CloseHandle 关闭互斥体句柄。如对象的所有句柄都已关闭，那么对象也会删除
	CloseHandle(hSemaphoreWriteC); //销毁信号量
	CloseHandle(hSemaphoreReadC); //销毁信号量
	CloseHandle(hSemaphoreWriteS); //销毁信号量
	CloseHandle(hSemaphoreReadS); //销毁信号量
	CloseHandle(hMutex);
	CloseHandle(hMapFileSWrite);
	CloseHandle(hMapFileCWrite);
	delete[] Transdata;
	Transdata = nullptr;

	getchar();
	return 0;
}

