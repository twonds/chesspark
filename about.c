#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>

#include "box.h"
#include "text.h"

#include "constants.h"
#include "i18n.h"
#include "imagemgr.h"
#include "titlebar.h"
#include "view.h"

void About_OnClose(struct Box_s *pbox)
{
	Box_Destroy(Box_GetRoot(pbox));
}

extern HFONT tahoma11b_f;

struct Box_s *About_Create(struct Box_s *roster)
{
	struct Box_s *dialog, *subbox;
	char txt[120];
	int x, y;

	{
		RECT windowrect;
		HMONITOR hm;
		MONITORINFO mi;

		windowrect.left = roster->x;
		windowrect.right = windowrect.left + roster->w - 1;
		windowrect.top = roster->y;
		windowrect.bottom = windowrect.top + roster->h - 1;

		hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

		mi.cbSize = sizeof(mi);
		GetMonitorInfo(hm, &mi);

		x = mi.rcWork.left + (mi.rcWork.right - mi.rcWork.left - 360) / 2;
		y = mi.rcWork.top  + (mi.rcWork.bottom - mi.rcWork.top - 270/*490*/) / 2;
	}

	dialog = Box_Create(x, y, 360, 270 + 80/*490*/, BOX_VISIBLE);
	dialog->bgcol = DefaultBG;
	dialog->fgcol = UserInfoFG2;

	dialog->titlebar = TitleBarCloseOnly_Add(dialog, _("About Chesspark"), About_OnClose);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = TitleBarRoot_OnInactive;

	subbox = Box_Create((dialog->w - 300) / 2, 40, 300, 180, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("bigLogo.gif");
	Box_AddChild(dialog, subbox);

	subbox = Box_Create(20, 230, 320, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
	subbox->fgcol = UserInfoFG2;
	subbox->font = tahoma11b_f;
	i18n_stringsub(txt, 120, _("Version %1, Build %2"), CHESSPARK_VERSION, CHESSPARK_BUILD);
	Box_SetText(subbox, txt);
	Box_AddChild(dialog, subbox);

	subbox = Text_Create(20, 250, 320, 100/*240*/, BOX_VISIBLE | BOX_TRANSPARENT, TX_CENTERED | TX_WRAP);
	subbox->fgcol = UserInfoFG2;
	Text_SetText(subbox, 
		_("Copyright 2005, 2006 Chesspark.  All rights reserved.\n\n")
		/*
		"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT "
		"HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRAN"
		"TIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCH"
		"ANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN N"
		"O EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY "
		"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL D"
		"AMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOO"
		"DS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPT"
		"ION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTR"
		"ACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) A"
		"RISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED O"
		"F THE POSSIBILITY OF SUCH DAMAGE.");
		*/
		);
	Box_AddChild(dialog, subbox);
 
	Box_CreateWndCustom(dialog, _("About Chesspark"), NULL);

	BringWindowToTop(dialog->hwnd);

	return dialog;
}
