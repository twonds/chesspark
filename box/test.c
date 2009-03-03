#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>

#include <windows.h>
#include <wingdi.h>

#include "box.h"
#include "constants.h"
#include "login.h"

BOOL app_eventpoll()
{
	MSG msg;
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
	{
		if (msg.message==WM_QUIT)		// Have We Received A Quit Message?
		{
			return FALSE;			// If So, return FALSE.
		}
		TranslateMessage(&msg);			// Translate The Message
		DispatchMessage(&msg);			// Dispatch The Message
	}
	return TRUE;
}

HWND rosterwnd;
HWND login;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
					LPTSTR lpCmdLine, int nShowCmd)
{
	struct Box_s *login;
	Box_Init();

	/*roster = Roster_Create(100, 30, 256, 256, BOX_VISIBLE);*/
	login = Login_Create(100, 30, 360, 416, BOX_VISIBLE);
	
	rosterwnd = Box_CreateWindow(login, NULL,  WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION | WS_VISIBLE | WS_THICKFRAME,
							WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

	while(app_eventpoll())
	{
		Sleep(1);
	}

	DestroyWindow(rosterwnd);

	Box_Uninit();
}