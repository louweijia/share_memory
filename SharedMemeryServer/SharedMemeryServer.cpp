// SharedMemeryServer.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <string>
#include <tchar.h>
using namespace std;

#define BUF_SIZE			1024
#define OBJECT_NUM	2

TCHAR szShareMemorySWriteName[] = TEXT("Global\\MyFileMappingSWriteObject");    //ָ������д�����ڴ������
TCHAR szShareMemoryCWriteName[] = TEXT("Global\\MyFileMappingCWriteObject");    //ָ��ͻ���д�����ڴ������
TCHAR szMutexName[] = TEXT("Global\\MyMutexObject");    //ָ�򻥳���������
TCHAR szSemaphoreReadCName[] = TEXT("Global\\MySemaphoreReadCObject");    //ָ��ͻ��˶��ź���������
TCHAR szSemaphoreWriteCName[] = TEXT("Global\\MySemaphoreWriteCObject");    //ָ��ͻ���д�ź���������
TCHAR szSemaphoreReadSName[] = TEXT("Global\\MySemaphoreReadSObject");    //ָ����������ź���������
TCHAR szSemaphoreWriteSName[] = TEXT("Global\\MySemaphoreWriteSObject");    //ָ�������д�ź���������

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
	//ע����Щ����
	HANDLE handleObject[OBJECT_NUM];
	DWORD res = 0xFFFFEEEE;
	DWORD  readthreadId;
	DWORD  writethreadId;

#pragma region Mutex
	//���������壬���������̵ȴ�������
	hMutex = CreateMutex(NULL, TRUE, szMutexName);
	if (hMutex)
	{
		cout<<"�����������ɹ���"<<endl;
	}
	else
	{
		cout << "����������ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}
	ReleaseMutex(hMutex);	//�뿪������
#pragma endregion Mutex

#pragma region Semphore
	hSemaphoreReadC = CreateSemaphore(NULL, 0, 10, szSemaphoreReadCName);
	if (hSemaphoreReadC == NULL)
	{
		cout << "�ͻ��˶��ź�������ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}
	//ReleaseSemaphore(hSemaphoreRead, 1, NULL);  //�ź�����Դ����һ

	hSemaphoreWriteC = CreateSemaphore(NULL, 0, 1, szSemaphoreWriteCName);
	if (hSemaphoreWriteC == NULL)
	{
		cout << "�ͻ���д�ź�������ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreReadS = CreateSemaphore(NULL, 0, 10, szSemaphoreReadSName);
	if (hSemaphoreReadS == NULL)
	{
		cout << "���������ź�������ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}
	//ReleaseSemaphore(hSemaphoreRead, 1, NULL);  //�ź�����Դ����һ

	hSemaphoreWriteS = CreateSemaphore(NULL, 0, 1, szSemaphoreWriteSName);
	if (hSemaphoreWriteS == NULL)
	{
		cout << "������д�ź�������ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}

	ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //�ͻ��˿ɶ��ź�����Դ����һ
	ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //����˿�д�ź�����Դ����һ
#pragma endregion Semphore

#pragma region ShareMemory
	//�����ڴ湲������
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

	//���ڴ�ӳ�䵽�ý���
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

	//�����ڴ湲������
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

	//���ڴ�ӳ�䵽�ý���
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
	cout << "�߳��˳���"<<endl;
	//while (1)
	//{
	//	res = WaitForMultipleObjects(OBJECT_NUM, handleObject, false, INFINITE);
	//	switch (res)
	//	{
	//	case WAIT_OBJECT_0:
	//		//�ɶ�
	//		memcpy(Transdata, (PVOID)pBuf, sizeof(float) * 5);
	//		ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //�ź�����Դ����һ
	//		cout << "���������� " << Transdata[0] << endl;
	//		Sleep(500);
	//		break;
	//	case (WAIT_OBJECT_0 + 1) :
	//		//��д
	//		memcpy((PVOID)pBuf, Transdata, sizeof(float) * 5);
	//		ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //�ź�����Դ����һ
	//		cout << "������д�� " << Transdata[0] << endl;
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
		
	CloseHandle(hSemaphoreWriteC); //�����ź���
	CloseHandle(hSemaphoreReadC); //�����ź���
	CloseHandle(hSemaphoreWriteS); //�����ź���
	CloseHandle(hSemaphoreReadS); //�����ź���
	CloseHandle(hMutex);
	CloseHandle(hMapFileSWrite);
	CloseHandle(hMapFileCWrite);

	getchar();
	return 0;
}

DWORD WINAPI ReadClientDataThreadFunc(LPVOID p)
{
	//���´���
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
			//�ɶ�
			memcpy(Transdata, (PVOID)pBufCWrite, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //�ź�����Դ����һ
			cout << "���������� " << Transdata[0] << endl;
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
	//���´���
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
			//��д
			memcpy((PVOID)pBufSWrite, Transdata, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreReadS, 1, NULL);  //�ź�����Դ����һ
			cout << "������д�� " << Transdata[0] << endl;
			for (int i = 0; i < 5; i++)
			{
				Transdata[i] += 1.0f;
			}
			break;
		//case (WAIT_OBJECT_0 + 1) :
		//	//��д
		//	memcpy((PVOID)pBufSWrite, Transdata, sizeof(float) * 5);
		//	ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //�ź�����Դ����һ
		//	cout << "������д�� " << Transdata[0] << endl;
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