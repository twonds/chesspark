#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>

void WaitForParentIfChesspark()
{
	PROCESSENTRY32 pe;
	HANDLE hSnapshot, hcpcprocess;
	BOOL next;
	DWORD cpcpid = 0;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	memset(&pe, 0, sizeof(pe));
	pe.dwSize = sizeof(pe);
	next = Process32First(hSnapshot, &pe);
	while(next && cpcpid == 0)
	{
		if (stricmp(pe.szExeFile, "chessparkclient.exe") == 0)
		{
			cpcpid = pe.th32ProcessID;
		}
		memset(&pe, 0, sizeof(pe));
		pe.dwSize = sizeof(pe);
		next = Process32Next(hSnapshot, &pe);
	}

	if (!cpcpid)
	{
		return;
	}

	hcpcprocess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, cpcpid);

	if (!hcpcprocess)
	{
		return;
	}

	/* Wait 30 seconds, that's long enough, right? :) */
	WaitForSingleObject(hcpcprocess, 30000);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
					LPTSTR lpCmdLine, int nShowCmd)
{
	FILE *fp;
	char apppath[MAX_PATH];
	char filename[MAX_PATH];
	char filename2[MAX_PATH];
	
	if (GetEnvironmentVariable("appdata", apppath, MAX_PATH) <= 0)
	{
		strcpy(apppath, ".");
	}

	strcpy(filename, apppath);
	strcat(filename, "/chesspark/installers/setup.exe");
	strcpy(filename2, apppath);
	strcat(filename2, "/chesspark/installers/setup3.exe");

	WaitForParentIfChesspark();

	fp = fopen(filename, "rb");

	if (!fp)
	{
		ShellExecute(NULL, NULL, "./chessparkclient.exe", NULL, ".", SW_SHOW);
		return;
	}

	fclose(fp);

	DeleteFile(filename2);
	MoveFile(filename, filename2);

	ShellExecute(NULL, NULL, filename2, "/silent /nocancel", ".", SW_SHOW);
}
