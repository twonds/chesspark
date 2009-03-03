#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

#include "box.h"

#include "i18n.h"
#include "util.h"

#define MENU_CUT				9997
#define MENU_COPY				9996
#define MENU_PASTE				9995
#define MENU_DELETE				9994

HMENU edithmenu;

struct Box_s *edittarget = NULL;

BOOL AppendMenuWrapper(HMENU hMenu, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	BOOL result;

	if ((uFlags & MF_BITMAP) != MF_BITMAP && (uFlags & MF_OWNERDRAW) != MF_OWNERDRAW)
	{
		WCHAR *wideitem = Util_ConvertUTF8ToWCHAR(lpNewItem);

		if (Util_OldWinVer())
		{
			char *ansiitem = Util_ConvertWCHARToANSI(wideitem);

			result = AppendMenuA(hMenu, uFlags, uIDNewItem, ansiitem);

			free(ansiitem);
		}
		else
		{
			result = AppendMenuW(hMenu, uFlags, uIDNewItem, wideitem);
		}

		free (wideitem);
	}
	else
	{
		result = AppendMenuA(hMenu, uFlags, uIDNewItem, lpNewItem);
	}

	return result;
}

BOOL ModifyMenuWrapper(HMENU hMenu, UINT uPosition, UINT uFlags, UINT_PTR uIDNewItem, LPCTSTR lpNewItem)
{
	BOOL result;

	if ((uFlags & MF_BITMAP) != MF_BITMAP && (uFlags & MF_OWNERDRAW) != MF_OWNERDRAW)
	{
		WCHAR *wideitem = Util_ConvertUTF8ToWCHAR(lpNewItem);

		if (Util_OldWinVer())
		{
			char *ansiitem = Util_ConvertWCHARToANSI(wideitem);

			result = ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, ansiitem);

			free(ansiitem);
		}
		else
		{
			result = ModifyMenuW(hMenu, uPosition, uFlags, uIDNewItem, wideitem);
		}

		free (wideitem);
	}
	else
	{
		result = ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
	}

	return result;
}

void Menu_PopupEditMenu(struct Box_s *pbox, int x, int y, int canedit, int cancopy, int canselect)
{
	edittarget = pbox;
	SetForegroundWindow(pbox->hwnd);

	DestroyMenu(edithmenu);
	edithmenu = CreatePopupMenu();

	if (canedit)
	{
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_CUT, _("Cut"));
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_COPY, _("Copy"));
		AppendMenuWrapper (edithmenu, MF_STRING, MENU_PASTE, _("Paste"));
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_DELETE, _("Delete"));
	}
	else
	{
		AppendMenuWrapper (edithmenu, MF_STRING | (cancopy ? 0 : MF_GRAYED), MENU_COPY, _("Copy"));
		/*AppendMenuWrapper (edithmenu, MF_STRING | (canselect ? 0 : MF_GRAYED), MENU_COPY, _("Select All"));*/
	}	

	TrackPopupMenuEx(edithmenu, TPM_LEFTALIGN | TPM_VCENTERALIGN | TPM_LEFTBUTTON, x, y, pbox->hwnd, NULL);
	
	PostMessage(pbox->hwnd, WM_NULL, 0, 0);
}

void Menu_OnCommand(struct Box_s *pbox, int command)
{
	switch (command)
	{
		case MENU_CUT:
		{
			if (edittarget && edittarget->OnCut)
			{
				edittarget->OnCut(edittarget);
			}
		}
		break;

		case MENU_COPY:
		{
			if (edittarget && edittarget->OnCopy)
			{
				edittarget->OnCopy(edittarget);
			}
		}
		break;

		case MENU_PASTE:
		{
			if (edittarget && edittarget->OnPaste)
			{
				edittarget->OnPaste(edittarget);
			}
		}
		break;

		case MENU_DELETE:
		{
			if (edittarget && edittarget->OnDelete)
			{
				edittarget->OnDelete(edittarget);
			}
		}
		break;

		default:
		break;
	}

	return;
}

void Menu_Init()
{
	edithmenu = CreatePopupMenu();
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_CUT, _("Cut"));
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_COPY, _("Copy"));
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_PASTE, _("Paste"));
	AppendMenuWrapper (edithmenu, MF_STRING, MENU_DELETE, _("Delete"));
}