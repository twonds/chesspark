#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <time.h>

#include <windows.h>
#include <wingdi.h>
#include <shellapi.h>

#include <strophe.h>

#include "box.h"

#include "audio.h"
#include "constants.h"
#include "ctrl.h"
#include "httppost.h"
#include "i18n.h"
#include "view.h"
#include "mem.h"
#include "model.h"
#include "login.h"
#include "util.h"

#include "dbghelp.h"

LONG ExceptionFilter(struct _EXCEPTION_POINTERS *pep)
{
	char dllfilename[_MAX_PATH];
	HANDLE hdbghelp = NULL;
	HANDLE hfile = NULL;
	BOOL (WINAPI * pMiniDumpWriteDump)();
	MINIDUMP_EXCEPTION_INFORMATION mdei;

	if (GetModuleFileName(NULL, dllfilename, _MAX_PATH))
	{
		strcat(dllfilename, "dbghelp.dll");
		hdbghelp = LoadLibrary(dllfilename);
	}

	if (!hdbghelp)
	{
		hdbghelp = LoadLibrary("dbghelp.dll");
	}

	if (!hdbghelp)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	pMiniDumpWriteDump = GetProcAddress(hdbghelp, "MiniDumpWriteDump");

	if (!pMiniDumpWriteDump)
	{
		FreeLibrary(hdbghelp);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	hfile = CreateFile("crashdump.dmp", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		FreeLibrary(hdbghelp);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	mdei.ThreadId = GetCurrentThreadId();
	mdei.ExceptionPointers = pep;
	mdei.ClientPointers = FALSE;

	pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hfile, MiniDumpNormal, &mdei, NULL, NULL);

	CloseHandle(hfile);

	FreeLibrary(hdbghelp);

	ShellExecute(NULL, NULL, "./CrashReport2.exe", NULL, ".", SW_SHOW);

	return EXCEPTION_CONTINUE_SEARCH;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
					LPTSTR lpCmdLine, int nShowCmd)
{
	HANDLE mutex;
	DWORD error;
	char buffer[256];

	/* Check for existing instances. */

	mutex = CreateMutex(NULL, FALSE, "Chesspark-{4F9860F3-41C9-4009-8B8E-D6990D205E84}");

	error = GetLastError();

	if ((error == ERROR_ALREADY_EXISTS || error == ERROR_ACCESS_DENIED) && !GetRegInt("AllowMultipleInstances"))
	{
		/* FIXME: FindWindow may hang, think up a better way to do this */
		SetForegroundWindow(FindWindow("CustomBox", "ChessPark"));
		return 0;
	}

	/* Set up post-crash debug dumper */
	CopyFile("./CrashReport.exe", "./CrashReport2.exe", 0);
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ExceptionFilter);

	/* Autoupdate pending?  If so, launch the updater and quit. */
	if (GetRegInt("UpgradeOnStartup"))
	{
		Model_LaunchUpdater();
		return 0;
	}

	Mem_InitMemRec();

	tzset();
	srand((unsigned int)(time(NULL)));
	i18n_InitLanguage(GetRegString("language", buffer, 256));

	Box_Init(GetRegInt("DisableAlphaBlending"));
	Ctrl_Init();
	View_Init();

	Audio_Init();
	Audio_PlayWav("sounds/song.wav");

	NetTransfer_Init();

	Ctrl_Start();
	View_Start();

	while(Ctrl_Poll())
	{
		NetTransfer_Poll();
		Audio_Poll();
		Sleep(1);
	}

	Audio_Uninit();
	NetTransfer_Uninit();

	View_End();

	Box_Uninit();

	Mem_DumpLeaks();

	return 0;
}