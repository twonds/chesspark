#ifndef __box_h__
#define __box_h__

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wingdi.h>

#include <stdlib.h>
#include <string.h>
#include "leak.h"

#include "image.h"

/***************************************************************************
 * Box_flags
 *
 * Bitfield for box characteristics.  Ensure every unique (not combination)
 * entry is a power of two.
 * 
 ***************************************************************************/
enum Box_flags
{
	BOX_TRANSPARENT	  = 0x0001, /* Box is drawn without painting background color */
	BOX_VISIBLE       = 0x0002, /* Box is drawn */
	BOX_TILEIMAGE	  = 0x0004, /* Image is tiled */
	BOX_CENTERTEXT	  = 0x0008, /* Text is centered in the box */
	BOX_BORDER        = 0x0010, /* Box has a border and uses brcol */
	BOX_BORDER2       = 0x0020, /* Box has a border for rounded top side */
	BOX_BORDER3       = 0x0030, /* Box has a border for rounded left side */
	BOX_BORDER4       = 0x0040, /* Box has dashed border */
	BOX_BORDER5       = 0x0050, /* Box has a rounded border */
	BOX_BORDER6       = 0x0060, /* Box has a rounded border, except the top left */
	BOX_BORDERALL     = 0x0070, /* */
	BOX_STRETCHIMG	  = 0x0080, /* Stretch image to fit box */
	BOX_CENTERIMG	  = 0x0100, /* Center image in box */
	BOX_FITASPECTIMG  = 0x0180, /* Stretch image to aspect ratio, and center */
	BOX_RIGHTTEXT     = 0x0200, /* Right align text */
	BOX_NOCLIP        = 0x0400, /* Don't clip */
};

/***************************************************************************
 * Box_s
 *
 * Structure for each box.
 * 
 ***************************************************************************/
struct Box_s
{
	/* Box tree pointers */
	struct Box_s *parent;	   /* Parent box */
	struct Box_s *sibling;	   /* Next sibling */
	struct Box_s *prevsibling;
	struct Box_s *child;	   /* First child */
	struct Box_s *lastchild;

	/* Internal box information */
	int x;					/* X coord of box relative to parent */
	int y;					/* Y coord of box relative to parent */
	int w;					/* Width of box */
	int h;					/* Height of box */
	enum Box_flags flags;	/* Flags */
	HWND hwnd;				/* Window where this box is drawn */
	int xoffset;
	int yoffset;
	int realw;
	int realh;
	int minw;
	int minh;
	int active;
	int lastmousex;
	int lastmousey;

	/* Box painting information */
	HFONT font;				/* Font used for text */
	char *text;				/* Text, drawn at top left corner on paint */
	COLORREF fgcol;			/* Foreground color, used for text */
	COLORREF bgcol;			/* Background color, drawn when not BOX_TRANSPARENT */
	COLORREF brcol;			/* Border color, drawn when BOX_BORDER */
	struct BoxImage_s *img;	/* Image drawn at top left corner on paint */
	
	/* Pointers used for "subclassing" boxes */
	int boxtypeid;			/* For when we need to know what type of box this is */
	void *boxdata;			/* Internal data specific to box type */
	void *userdata;			/* Unused. */
	
	/* Message handler function pointers */
	void (*OnPaint)(struct Box_s *pbox, HDC hdc, RECT rcClip, int x, int y, int paintmask);
	void (*OnCommand)(struct Box_s *pbox, int command);
	void (*OnDestroy)(struct Box_s *pbox);
	void (*OnKeyDown)(struct Box_s *pbox, int vk, int scan);
	void (*OnKeyShortcut)(struct Box_s *pbox, unsigned int vk, unsigned int scan);
	void (*OnLButtonDown)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnLButtonDblClk)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnLButtonTrpClk)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnLButtonUp)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnRButtonDown)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnRButtonUp)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnMouseMove)(struct Box_s *pbox, int xmouse, int ymouse);
	void (*OnMove)(struct Box_s *pbox, int x, int y);
	void (*OnSizeWidth)(struct Box_s *pbox, int dwidth);
	void (*OnSizeHeight)(struct Box_s *pbox, int dheight);
	void (*OnClose)(struct Box_s *pbox);
	int (*OnDragDrop)(struct Box_s *pdst, struct Box_s *psrc, int xmouse, int ymouse, int id, void *data);
	void (*OnScrollWheel)(struct Box_s *pbox, float delta);
	void (*OnActive)(struct Box_s *pbox);
	void (*OnInactive)(struct Box_s *pbox);
	void (*OnRestore)(struct Box_s *pbox);
	void (*OnMinimize)(struct Box_s *pbox);
	void (*OnMaximize)(struct Box_s *pbox);

	void (*OnCut)(struct Box_s *pbox);
	void (*OnCopy)(struct Box_s *pbox);
	void (*OnPaste)(struct Box_s *pbox);
	void (*OnDelete)(struct Box_s *pbox);

	void (*OnGetFocus)(struct Box_s *pbox);
	void (*OnLoseFocus)(struct Box_s *pbox);

	void (*OnLoseMouseCapture)(struct Box_s *pbox);

	int lastclickx;
	int lastclicky;
	unsigned int lastlclicktime;
	unsigned int lastldblclktime;

	struct Box_s *titlebar;
	struct Box_s *focus;
	struct Box_s *parentactive;

	struct Box_s *prevfocus;
	struct Box_s *nextfocus;
	struct Box_s *redirectfocus;

	struct Box_s *cursorimglockbox;
	HCURSOR cursorimg;

	int tooltipvisible;
	struct Box_s *tooltipbox;

	/* Debug text pointer */
	char *debug;

	struct Box_s *nextwindow;
	char *windowname;
};

/* Box system functions */
void Box_Init();
BOOL Box_Poll();
void Box_Uninit();

/* Timed functions */
void Box_AddTimedFunc(struct Box_s *pbox, void (*func)(struct Box_s *, void *), void *userdata, int period);
void Box_RemoveTimedFunc(struct Box_s *pbox, void (*func)(struct Box_s *, void *), int period);

/* Box creation, deletion, and tree functions */

struct Box_s *Box_CreateReal(int x, int y, int w, int h, enum Box_flags flags);
#if 1
#define Box_Create(x, y, w, h, f) Box_CreateReal(x, y, w, h, f)
#else
#define Box_Create(x, y, w, h, f) Box_CreateWrap(x, y, w, h, f, __FILE__, __LINE__)
#endif

void Box_AddChild(struct Box_s *parent, struct Box_s *child);
void Box_AddChildToBottom(struct Box_s *parent, struct Box_s *child);
void Box_Unlink(struct Box_s *pbox);
void Box_Destroy(struct Box_s *pbox);
struct Box_s *Box_GetRoot(struct Box_s *pbox);
void Box_FlushDead();

/* Box window functions */
HWND Box_CreateWindow(struct Box_s *pbox, char *titlebar, HWND parent, DWORD dwStyle, DWORD dwExStyle, int closeapp);
void Box_CreateWndCustom(struct Box_s *pbox, char *titlebar, HWND parent);
void Box_CreateWndCustom2(struct Box_s *pbox, char *titlebar, struct Box_s *parent);
void Box_CreateWndMenu(struct Box_s *pbox, HWND parent);
void Box_CreateWndMenu2(struct Box_s *pbox, HWND parent);

/* Box accessor functions */
void Box_GetScreenCoords(struct Box_s *pbox, int *x, int *y);
void Box_GetRootCoords(struct Box_s *pbox, int *x, int *y);

void Box_SetText(struct Box_s *pbox, const char *text);

/* Box painting functions */
void Box_OnPaint(struct Box_s *pbox, HDC hdc, RECT rcClip, int x, int y, int paintmask);
void Box_Repaint(struct Box_s *pbox);
HBITMAP Box_Draw_Hbmp(struct Box_s *pbox, BOOL mask);
void Box_ForceDraw(struct Box_s *pbox);

/* Box message handlers */
void Box_OnLButtonDblClk(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnLButtonTrpClk(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnLButtonDown(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnLButtonUp(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnRButtonDown(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnRButtonUp(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnMouseMove(struct Box_s *pbox, int xmouse, int ymouse);
void Box_OnSizeWidth_Stretch(struct Box_s *pbox, int dwidth);
void Box_OnSizeHeight_Stretch(struct Box_s *pbox, int dheight);
void Box_OnSizeWidth_StickRight(struct Box_s *pbox, int dwidth);
void Box_OnSizeHeight_StickBottom(struct Box_s *pbox, int dheight);
void Box_OnSizeWidth_Center(struct Box_s *pbox, int dwidth);
void Box_OnSizeHeight_Center(struct Box_s *pbox, int dheight);
int Box_OnDragDrop(struct Box_s *pdst, struct Box_s *psrc, int xmouse, int ymouse, int id, void *data);
void Box_OnMove(struct Box_s *pbox, int x, int y);
void Box_OnScrollWheel(struct Box_s *pbox, float delta);
void Box_OnClose(struct Box_s *pbox);
void Box_OnMinimize(struct Box_s *pbox);
void Box_OnRestore(struct Box_s *pbox);

/* Mouse capture functions */
BOOL Box_CaptureMouse(struct Box_s *pbox);
void Box_ReleaseMouse(struct Box_s *pbox);
int Box_IsMouseCaptured();

/* Drag and drop functions */
void Box_DragStart(struct Box_s *pbox, int x, int y, int newdragid);
void Box_DragMove(struct Box_s *pbox, int x, int y);
void Box_DragEnd(struct Box_s *pbox);
BOOL Box_IsDragging();
void Box_HandleDragDrop(struct Box_s *psrc, int xmouse, int ymouse, int id, void *data, void (*ondropempty)(struct Box_s *psrc, int xmouse, int ymouse, int id, void *data));

void Box_MoveWndCustom(struct Box_s *pbox, int x, int y, int w, int h);
void Box_MoveWndCustom2(struct Box_s *pbox, int x, int y, int w, int h);

int Box_IsVisible(struct Box_s *pbox);
void Box_ToFront(struct Box_s *pbox);

void Box_SetFocus(struct Box_s *pbox);
void Box_SetNextFocus(struct Box_s *pbox, int reverse);
int Box_HasFocus(struct Box_s *pbox);

int Box_CheckXYInBox(struct Box_s *pbox, int x, int y);

BOOL Box_LockMouseCursorImg(struct Box_s *pbox, HCURSOR cursor);
BOOL Box_UnlockMouseCursorImg(struct Box_s *pbox);

void Box_MeasureText(struct Box_s *pbox, char *text, int *w, int *h);
void Box_MeasureText2(struct Box_s *pbox, HFONT hfont, char *text, int *w, int *h);
int Box_GetDragId();

void Box_RegisterWindowName(struct Box_s *pbox, char *name);
struct Box_s *Box_GetWindowByName(char *name);

struct Box_s *Box_CheckLastActive();
struct Box_s *Box_GetChessparkBox(HWND hwnd);
int Box_GetIdleTime();

#endif
