// SharedMemeryClient.cpp : �������̨Ӧ�ó������ڵ㡣
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
	//��ѯ���У�����szMutexName �򿪻����� 
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
	hMutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, szMutexName);
	if (hMutex)
	{
		cout << "�������򿪳ɹ���" << endl;
	}
	else
	{
		cout << "��������ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}

	cout << "�ȴ�... ... " << endl;
	res = WaitForSingleObject(hMutex, 20000);
	switch (res)
	{
	case WAIT_OBJECT_0:
		cout << "�յ��ź�... ... " << endl;
		break;
	case WAIT_TIMEOUT:
		cout << "��ʱû���յ�... ... " << endl;
		break;
	case WAIT_ABANDONED:
		cout << "��һ������������ֹ... ... " << endl;
		break;
	default:
		break;
	}
#pragma endregion Mutex

#pragma region Semphore
	hSemaphoreReadC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreReadCName);
	if (hSemaphoreReadC == NULL)
	{
		cout << "�ͻ��˶��ź�����ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreWriteC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreWriteCName);
	if (hSemaphoreWriteC == NULL)
	{
		cout << "�ͻ���д�ź�����ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreWriteS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreWriteSName);
	if (hSemaphoreWriteS == NULL)
	{
		cout << "������д�ź�����ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}

	hSemaphoreReadS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, szSemaphoreReadSName);
	if (hSemaphoreReadS == NULL)
	{
		cout << "���������ź�����ʧ�ܣ�" << endl;
		system("pause");
		return 1;
	}
	//ReleaseSemaphore(hSemaphoreReadC, 1, NULL);  //�ź�����Դ����һ
	//ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //�ź�����Դ����һ
#pragma endregion Semphore

	handleObject[0] = hSemaphoreReadC;
	handleObject[1] = hSemaphoreWriteC;

#pragma region ShareMemory
	//�����ڴ湲������
	hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, TRUE, szShareMemoryName); 
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

	float *Transdata = new float[5];
	for (int i = 0; i < 5; i++)
	{
		Transdata[i] = 0.0f;
	}
	ReleaseMutex(hMutex);	//�뿪������
	cout << "��������׼���ã�" << endl;

	while (true)
	{
		res = WaitForMultipleObjects(OBJECT_NUM, handleObject, false, INFINITE);
		switch (res)
		{
		case WAIT_OBJECT_0:
			//�ɶ�
			memcpy(Transdata, (PVOID)pBuf, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreWriteS, 1, NULL);  //�ź�����Դ����һ
			cout << "�ͻ��˶��� " << Transdata[0] << endl;
			break;
		case (WAIT_OBJECT_0 + 1) :
			//��д
			memcpy((PVOID)pBuf, Transdata, sizeof(float) * 5);
			ReleaseSemaphore(hSemaphoreReadS, 1, NULL);  //�ź�����Դ����һ
			cout << "�ͻ���д�� " << Transdata[0] << endl;
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
	//һ��������Ҫ��ע��һ��Ҫ�� CloseHandle �رջ������������������о�����ѹرգ���ô����Ҳ��ɾ��
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

