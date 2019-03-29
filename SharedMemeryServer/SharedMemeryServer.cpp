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

TCHAR szShareMemoryName[] = TEXT("Global\\MyFileMappingObject");    //ָ��ͬһ�鹲���ڴ������
TCHAR szMutexName[] = TEXT("Global\\MyMutexObject");    //ָ�򻥳���������
TCHAR szSemaphoreReadCName[] = TEXT("Global\\MySemaphoreReadCObject");    //ָ��ͻ��˶��ź���������
TCHAR szSemaphoreWriteCName[] = TEXT("Global\\MySemaphoreWriteCObject");    //ָ��ͻ���д�ź���������
TCHAR szSemaphoreReadSName[] = TEXT("Global\\MySemaphoreReadSObject");    //ָ����������ź���������
TCHAR szSemaphoreWriteSName[] = TEXT("Global\\MySemaphoreWriteSObject");    //ָ�������д�ź���������

int _tmain(int argc, _TCHAR* argv[])
{
	//ע����Щ����
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
	hSemaphoreReadC = CreateSemaphore(NULL, 10, 10, szSemaphoreReadCName);
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

	ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //�ź�����Դ����һ
	ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //�ź�����Դ����һ
#pragma endregion Semphore

	handleObject[0] = hSemaphoreReadS;
	handleObject[1] = hSemaphoreWriteS;

#pragma region ShareMemory
	//�����ڴ湲������
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

	//���ڴ�ӳ�䵽�ý���
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

	//���´��룬B��ͣд�����ڴ�pBuf
	float *Transdata = new float[5];
	for (int i = 0; i < 5; i++)
	{
		Transdata[i] = 0.0f;
	}
	//res = WaitForSingleObject(hMutex, 20000);
	//cout << "�ͻ�����׼���ã� " << endl;
	while (1)
	{
		res = WaitForMultipleObjects(OBJECT_NUM, handleObject, false, INFINITE);
		switch (res)
		{
		case WAIT_OBJECT_0:
			//�ɶ�
			memcpy(Transdata, (PVOID)pBuf, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreWriteC, 1, NULL);  //�ź�����Դ����һ
			cout << "���������� " << Transdata[0] << endl;
			break;
		case (WAIT_OBJECT_0 + 1) :
			//��д
			memcpy((PVOID)pBuf, Transdata, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //�ź�����Դ����һ
			cout << "������д�� " << Transdata[0] << endl;
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
		
	CloseHandle(hSemaphoreWriteC); //�����ź���
	CloseHandle(hSemaphoreReadC); //�����ź���
	CloseHandle(hSemaphoreWriteS); //�����ź���
	CloseHandle(hSemaphoreReadS); //�����ź���
	CloseHandle(hMutex);
	CloseHandle(hMapFile);
	delete[] Transdata;
	Transdata = nullptr;

	getchar();
	return 0;
}

