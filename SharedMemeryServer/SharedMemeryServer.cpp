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

TCHAR szShareMemorySWriteName[] = TEXT("Global\\MyFileMappingSWriteObject");    //指向服务端写共享内存的名字
TCHAR szShareMemoryCWriteName[] = TEXT("Global\\MyFileMappingCWriteObject");    //指向客户端写共享内存的名字
TCHAR szMutexName[] = TEXT("Global\\MyMutexObject");    //指向互斥量的名字
TCHAR szSemaphoreReadCName[] = TEXT("Global\\MySemaphoreReadCObject");    //指向客户端读信号量的名字
TCHAR szSemaphoreWriteCName[] = TEXT("Global\\MySemaphoreWriteCObject");    //指向客户端写信号量的名字
TCHAR szSemaphoreReadSName[] = TEXT("Global\\MySemaphoreReadSObject");    //指向服务器读信号量的名字
TCHAR szSemaphoreWriteSName[] = TEXT("Global\\MySemaphoreWriteSObject");    //指向服务器写信号量的名字

DWORD WINAPI ReadClientDataThreadFunc(LPVOID p);
DWORD WINAPI WriteServerDataThreadFunc(LPVOID p);

HANDLE hMapFileSWrite = NULL;
HANDLE hMapFileCWrite = NULL;
HANDLE hMutex = NULL;
HANDLE hSemaphoreReadC = NULL;
HANDLE hSemaphoreWriteC = NULL;
HANDLE hSemaphoreReadS = NULL;
HANDLE hSemaphoreWriteS = NULL;
HANDLE hThreadWrite = NULL;
HANDLE hThreadRead = NULL;
LPCTSTR pBufSWrite;
LPCTSTR pBufCWrite;

int _tmain(int argc, _TCHAR* argv[])
{
	//注意这些声明
	HANDLE handleObject[OBJECT_NUM];
	DWORD res = 0xFFFFEEEE;
	DWORD  readthreadId;
	DWORD  writethreadId;

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
	hSemaphoreReadC = CreateSemaphore(NULL, 0, 10, szSemaphoreReadCName);
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

	ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //客户端可读信号量资源数加一
	ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //服务端可写信号量资源数加一
#pragma endregion Semphore

#pragma region ShareMemory
	//创建内存共享区域
	hMapFileSWrite = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		BUF_SIZE,                // maximum object size (low-order DWORD)
		szShareMemorySWriteName);                 // name of mapping object
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
	hMapFileCWrite = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		BUF_SIZE,                // maximum object size (low-order DWORD)
		szShareMemoryCWriteName);                 // name of mapping object
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

#pragma region thread
	hThreadRead = CreateThread(NULL, 0, ReadClientDataThreadFunc, 0, 0, &readthreadId);
	if (hThreadRead == NULL)
	{
		_tprintf(TEXT("Could not create read client data thread (%d).\n"),
			GetLastError());
		system("pause");
		return 1;
	}

	hThreadWrite = CreateThread(NULL, 0, WriteServerDataThreadFunc, 0, 0, &writethreadId);
	if (hThreadWrite == NULL)
	{
		_tprintf(TEXT("Could not create write server data thread (%d).\n"),
			GetLastError());
		system("pause");
		return 1;
	}

#pragma endregion thread

	handleObject[0] = hThreadRead;
	handleObject[1] = hThreadWrite;

	res = WaitForMultipleObjects(OBJECT_NUM, handleObject, true, INFINITE);
	cout << "线程退出！"<<endl;
	//while (1)
	//{
	//	res = WaitForMultipleObjects(OBJECT_NUM, handleObject, false, INFINITE);
	//	switch (res)
	//	{
	//	case WAIT_OBJECT_0:
	//		//可读
	//		memcpy(Transdata, (PVOID)pBuf, sizeof(float) * 5);
	//		ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //信号量资源数加一
	//		cout << "服务器读： " << Transdata[0] << endl;
	//		Sleep(500);
	//		break;
	//	case (WAIT_OBJECT_0 + 1) :
	//		//可写
	//		memcpy((PVOID)pBuf, Transdata, sizeof(float) * 5);
	//		ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //信号量资源数加一
	//		cout << "服务器写： " << Transdata[0] << endl;
	//		for (int i = 0; i < 5; i++)
	//		{
	//			Transdata[i] += 1.0f;
	//		}
	//		Sleep(1000);
	//		break;
	//	default:
	//		break;
	//	}
	//	if (Transdata[0] >= 100.0f) break;

	//	//Sleep(1000);
	//}
		
	CloseHandle(hSemaphoreWriteC); //销毁信号量
	CloseHandle(hSemaphoreReadC); //销毁信号量
	CloseHandle(hSemaphoreWriteS); //销毁信号量
	CloseHandle(hSemaphoreReadS); //销毁信号量
	CloseHandle(hMutex);
	CloseHandle(hMapFileSWrite);
	CloseHandle(hMapFileCWrite);

	getchar();
	return 0;
}

DWORD WINAPI ReadClientDataThreadFunc(LPVOID p)
{
	//以下代码
	float *Transdata = new float[5];
	for (int i = 0; i < 5; i++)
	{
		Transdata[i] = 0.0f;
	}

	DWORD res = 0xFFFFEEEE;
	while (1)
	{
		res = WaitForSingleObject(hSemaphoreReadC, INFINITE);
		switch (res)
		{
		case WAIT_OBJECT_0:
			//可读
			memcpy(Transdata, (PVOID)pBufCWrite, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //信号量资源数加一
			cout << "服务器读： " << Transdata[0] << endl;
			break;
		default:
			break;
		}
		if (Transdata[0] > 50.5f) break;
		Sleep(1000);
	}
	delete[] Transdata;
	Transdata = nullptr;

	ExitThread(0);

	return 0;
}
DWORD WINAPI WriteServerDataThreadFunc(LPVOID p)
{
	//以下代码
	float *Transdata = new float[5];
	for (int i = 0; i < 5; i++)
	{
		Transdata[i] = 0.0f;
	}

	DWORD res = 0xFFFFEEEE;
	while (1)
	{
		res = WaitForSingleObject(hSemaphoreWriteS, INFINITE);
		switch (res)
		{
		case WAIT_OBJECT_0:
			//可写
			memcpy((PVOID)pBufSWrite, Transdata, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreReadS, 1, NULL);  //信号量资源数加一
			cout << "服务器写： " << Transdata[0] << endl;
			for (int i = 0; i < 5; i++)
			{
				Transdata[i] += 1.0f;
			}
			break;
		//case (WAIT_OBJECT_0 + 1) :
		//	//可写
		//	memcpy((PVOID)pBufSWrite, Transdata, sizeof(float) * 5);
		//	ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //信号量资源数加一
		//	cout << "服务器写： " << Transdata[0] << endl;
		//	for (int i = 0; i < 5; i++)
		//	{
		//		Transdata[i] += 1.0f;
		//	}
		//	Sleep(1000);
		//	break;
		default:
			break;
		}
		if (Transdata[0] >= 50.5f) break;
		Sleep(1000);
	}
	delete[] Transdata;
	Transdata = nullptr;

	ExitThread(0);

	return 0;
}