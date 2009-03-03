#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "box.h"

#include "edit.h"
#include "titledrag.h"
#include "button.h"
#include "sizer.h"
#include "tabs.h"
#include "text.h"

#include "audio.h"
#include "autodialog.h"
#include "autosize.h"
#include "boxtypes.h"
#include "chesslogic.h"
#include "constants.h"
#include "ctrl.h"
#include "edit2.h"
#include "i18n.h"
#include "imagemgr.h"
#include "info.h"
#include "link.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "model.h"
#include "namedlist.h"
#include "options.h"
#include "participantentry.h"
#include "subchat.h"
#include "titlebar.h"
#include "util.h"
#include "view.h"

#include "chessbox.h"

#define NORMAL_WHITE_CLOCK_COLOR  GameWhiteTimeFG 
#define NORMAL_BLACK_CLOCK_COLOR  GameBlackTimeFG
/* RGB(225, 225, 225), RGB(156, 156, 156) */
#define FADED_WHITE_CLOCK_COLOR   GameWhiteTimeFG
#define FADED_BLACK_CLOCK_COLOR   GameBlackTimeFG

static float estimatedarbiterlag = 0.0f;

void ChessBoxBorder_RedoBorder(struct Box_s *boardborder, int rotated);
void ChessBox_SetSizeChatLists(struct Box_s *dialog, int chatw);
void ChessBoxLists_OnSizeWidth(struct Box_s *pbox, int dw);
void ChessBox_UpdateControlMsg(struct Box_s *chessbox);
void ChessBox_ActivateClock(struct Box_s *chessbox, enum ChessTurn turn);
void ChessBoard_ClearHighlights(struct Box_s *board);
void ChessBoard_CreateHighlight(struct Box_s *board, int x, int y, int color, int rotated, int gameover, enum ChessPiece piece, int capture);
float ChessBox_CalcClock(struct Box_s *chessbox, int white);
int ChessBox_SetVisibleClock(struct Box_s *chessbox, float fsec, int iswhite);
void ChessBox_ArrangeChats(struct Box_s *dialog);
void ChessBox_UpdateMenu(struct Box_s *dialog);
void ChessBoard_UpdateHighlights(struct Box_s *board, int rotated, int gameover);
void ChessBox_UpdateCaptures(struct Box_s *chessbox);

struct chessboxdata_s
{
	struct Box_s *sizerset;
	struct Box_s *vsizerbar;
	struct Box_s *hsizerbar;
	struct Box_s *chessboard;
	struct Box_s *whitetimebox;
	struct Box_s *blacktimebox;
	struct Box_s *whiteplayerbox;
	struct Box_s *blackplayerbox;
	struct Box_s *whitetopspacer;
	struct Box_s *blacktopspacer;
	struct Box_s *whiteclock;
	struct Box_s *blackclock;
	struct Box_s *whitedelay;
	struct Box_s *blackdelay;
	struct Box_s *whitetimeinfo;
	struct Box_s *blacktimeinfo;
	struct Box_s *whiteavatar;
	struct Box_s *blackavatar;
	struct Box_s *whitename;
	struct Box_s *blackname;
	struct Box_s *whiterating;
	struct Box_s *blackrating;
	struct Box_s *whitedot;
	struct Box_s *blackdot;
	struct Box_s *whitebottomspacer;
	struct Box_s *blackbottomspacer;
	struct Box_s *chatlist;
	struct Box_s *chatborder;
	struct Box_s *chatbottomline;
	struct Box_s *participantlist;
	struct Box_s *participantborder;
	struct Box_s *edit;
	struct Box_s *editborder;
	struct Box_s *message;
	struct Box_s *title;
	struct Box_s *boardborder;
	struct Box_s *gameinfobox;
	struct Box_s *whiteinfobox;
	struct Box_s *blackinfobox;
	struct Box_s *whitecapturebox;
	struct Box_s *blackcapturebox;
	struct Box_s *corrbuttonbox;
	struct Box_s *sendmovebutton;
	struct Box_s *clearmovebutton;
	/*struct Box_s *correspondencebox;*/
	struct Box_s *menutext;
	struct Box_s *menutextborder;
	struct Box_s *buttons;
	struct Box_s *menumovetabs;
	int chatwidth;
	int hidechat;
	char *gameid;
	char *newgameid;
	char *roomjid;
	char *nick;
	char *tourneyid;
	int lastperiod;
	enum ChessTurn turn;
	float whitetime;
	float blacktime;
	int gameover;
	int canabort;
	unsigned int gameovertime;
	int initialconnect;
	int disconnected;
	int requesteddraw;
	int requestedadjourn;
	int requestedabort;
	int requestedtakeback;
	int accepteddraw;
	int acceptedadjourn;
	int acceptedabort;
	int acceptedtakeback;
	int declineddraw;
	int declinedadjourn;
	int declinedabort;
	int declinedtakeback;
	int cancelleddraw;
	int cancelledadjourn;
	int cancelledabort;
	int cancelledtakeback;
	int whitelocal;
	int blacklocal;
	int sentflag;
	char storedmove[9];
	int ignorenextmove;
	int whitecurrentcontrol;
	int blackcurrentcontrol;
	char *lastlistmove;
	int lastlistmovecolor;
	int gotstate;
	char *initialfen;
	char *queuedmove;
	char *queuedmoveannotation;
	int queuednumtakebacks;
	int queuedply;
	unsigned int lastmovetick;
	int ishidinglag;
	int whiteratingset;
	int blackratingset;
	char *whitecaptured;
	char *blackcaptured;

	struct Box_s *drawer;
	int drawervisible;
	int draweranimating;
	struct Box_s *movelistbox;
	struct Box_s *movelistborder;
	struct namedlist_s *movelist;
	int whitemovenum;
	int blackmovenum;
	int movenum;
	char *rotateifblackjid;
	char *lastmove;
	unsigned int tickwhiteset;
	unsigned int tickblackset;
	int postswitch;
	unsigned int arbitermovelag;
	unsigned int movearbiterlag;
	
	struct gamesearchinfo_s *info;

	int normx;
	int normy;
	int normw;
	int normh;

	int isinroom;
	int localnotactivated;
	int lastply;
	int gamechatonbottom;
	int participantlistonbottom;
	int movelisthidden;

	int oldchatw;
	struct Box_s *promotionbox;

	enum ChessCastling castling;
	char *enpassanttarget;

	char *piecethemepng;
};

struct chessboxdrawerdata_s
{
	struct Box_s *chessbox;
	struct Box_s *movelist;
	struct Box_s *movelistsorticon;
	int reversesort;
};

void ChessBox_RefreshTitle(struct Box_s *chessbox);
void ChessBox_OnClickPlayer(struct Box_s *playerbox, void *userdata);
void ChessBox_HighlightMove(struct Box_s *chessbox, char *submove);

struct chessboardhighlight_s
{
	int x;
	int y;
	int color;
	enum ChessPiece piece;
	int capture;
};

struct chessboarddata_s
{
	enum ChessPiece board[8][8];
	struct Box_s *dragtarget;
	int showrotated;
	struct namedlist_s *animpieces;
	struct BoxImage_s *boardimg;
	struct BoxImage_s *boarddimimg;
	struct BoxImage_s *piecesimg;
	struct BoxImage_s *dimpiecesimg;
	struct BoxImage_s *highlightpiecesimg;
	struct BoxImage_s *capturepiecesimg;
	int pieceheight;
	int boardimgheight;
	unsigned int animstart;
	int movefinishsound;
	unsigned int piecepickuptime;
	int cachedimages;
};

static struct BoxImage_s *cachedboardimg = NULL;
static struct BoxImage_s *cachedboarddimimg = NULL;
static int cachedboardimgheight = 0;

static struct BoxImage_s *cachedpiecesimg = NULL;
static struct BoxImage_s *cacheddimpiecesimg = NULL;
static struct BoxImage_s *cachedhighlightpiecesimg = NULL;
static struct BoxImage_s *cachedcapturepiecesimg = NULL;
static int cachedpieceheight = 0;

static struct BoxImage_s *promotionpieces = NULL;
static struct BoxImage_s *capturedpiecesimg = NULL;

void ChessBoard_UpdateBoard(struct Box_s *board);
void ChessBoard_Animate(struct Box_s *board, void *userdata);

struct chesspieceboxdata_s
{
	enum ChessPiece type;
	int x;
	int y;
	int startdragx;
	int startdragy;
	int mousedownx;
	int mousedowny;
	int dragging;
	int twoclickmove;
	int newx;
	int newy;
	int highlightedlegal;
};

#define CHESSPIECEBOX_ID 0x07
#define HIGHLIGHTBOX_ID 0x08

void ChessPieceBox_OnDestroy(struct Box_s *piece)
{
	struct chesspieceboxdata_s *data = piece->boxdata;
	struct Box_s *dialog = Box_GetRoot(piece);
	struct chessboxdata_s *ddata = dialog->boxdata;
	struct Box_s *board = ddata->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	/*if (bdata->dragtarget == piece)*/
	{
		Box_ReleaseMouse(board);
	}

	BoxImage_Destroy(piece->img);
}

void ChessPieceBox_OnLButtonDown(struct Box_s *piece, int x, int y)
{
	struct chesspieceboxdata_s *data = piece->boxdata;
	struct Box_s *dialog = Box_GetRoot(piece);
	struct chessboxdata_s *ddata = dialog->boxdata;
	struct Box_s *board = ddata->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;
/*
	if (timeGetTime() - Box_GetLastMouseMoveTime() < 300 && Model_GetOption(OPTION_ENABLEANTISLIP))
	{
		Log_Write(0, "Denying move due to antislip");
		return;
	}
*/
	if (data->x == -1 && data->y == -1)
	{
		if (!(ddata->info && ddata->info->variant && stricmp(ddata->info->variant, "crazyhouse") == 0))
		{
			return;
		}
	}
	
	if (!(ddata->turn == CT_WHITE && ddata->whitelocal &&  IsPieceWhite(data->type)) &&
	    !(ddata->turn == CT_BLACK && ddata->blacklocal && !IsPieceWhite(data->type)))
	{
		Log_Write(0, "Denying move, turn %d whitelocal %d blacklocal %d ispiecewhite %d\n", ddata->turn, ddata->whitelocal, ddata->blacklocal, IsPieceWhite(data->type));
		Log_Write(0, "Piece was %d, at %d, %d", data->type, data->x, data->y);
		return;
	}
	
	if (ddata->disconnected)
	{
		return;
	}

	if (ddata->gameover)
	{
		Log_Write(0, "Denying move, game over\n");
		return;
	}

	if (!bdata->dragtarget)
	{
		bdata->dragtarget = piece;
		bdata->piecepickuptime = timeGetTime();
		data->startdragx = piece->x;
		data->startdragy = piece->y;
		data->mousedownx = x;
		data->mousedowny = y;
		data->twoclickmove = 1;
		Box_ToFront(piece);
	}
	else if (bdata->dragtarget == piece)
	{
		data->twoclickmove = 0;
	}
}

void ChessPieceBox_OnMouseMove(struct Box_s *piece, int x, int y)
{
	struct chesspieceboxdata_s *data = piece->boxdata;
	struct Box_s *dialog = Box_GetRoot(piece);
	struct chessboxdata_s *ddata = dialog->boxdata;
	struct Box_s *board = ddata->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	if (bdata->dragtarget == piece)
	{
		int standard, unrated;

		if (data->twoclickmove && abs(piece->x - data->startdragx) < 5 && abs(piece->y - data->startdragy) < 5)
		{
			/* clickmove */
			data->twoclickmove = 0;
		}

		unrated = ddata->info && !ddata->info->rated;
		standard = ddata->info && (!ddata->info->variant || (ddata->info->variant && stricmp(ddata->info->variant, "standard") == 0));

		Log_Write(0, "highlighting: %d %d %s", standard, ddata->info, ddata->info->variant);

		if (data->x != -1 && data->y != -1 && !data->highlightedlegal && Model_GetOption(OPTION_SHOWLEGALMOVES) && standard && unrated)
		{
			int newx = 0, newy = 0, oldx, oldy;
			ChessBoard_ClearHighlights(board);

			if (bdata->showrotated)
			{
				oldx = 8 - data->x;
				oldy = 8 - data->y;
			}
			else
			{
				oldx = data->x - 1;
				oldy = data->y - 1;
			}

			for (newx = 0; newx < 8; newx++)
			{
				for (newy = 0; newy < 8; newy++)
				{
					if (IsMoveLegal(bdata->board, oldx, oldy, newx, newy, &(ddata->castling), &(ddata->enpassanttarget)))
					{
						enum ChessPiece srcpiece = bdata->board[oldx][oldy];
						enum ChessPiece dstpiece = bdata->board[newx][newy];

						if (dstpiece != CP_EMPTY)
						{
							ChessBoard_CreateHighlight(board, newx + 1, newy + 1, 0, bdata->showrotated, ddata->gameover, dstpiece, 1);
						}
						else
						{
							ChessBoard_CreateHighlight(board, newx + 1, newy + 1, 0, bdata->showrotated, ddata->gameover, srcpiece, 0);
						}
					}
				}
			}
			data->highlightedlegal = 1;

			/* Move piece to top again, so it's above highlights */
			Box_Unlink(piece);
			Box_AddChild(board, piece);
		}

		piece->x += x - data->mousedownx;
		piece->y += y - data->mousedowny;
		Box_Repaint(board);
	}
}

struct promotionboxdata_s
{
	char *roomjid;
	char *gameid;
	char move[6];
	struct Box_s *parentbox;
};

struct promotedata_s
{
	enum ChessPiece type;
};

void Promote(struct Box_s *pieceselect)
{
	struct Box_s *parentbox = pieceselect->parent;
	struct Box_s *rootbox = parentbox->parent;
	struct promotedata_s *pdata = parentbox->boxdata;
	struct promotionboxdata_s *rdata = rootbox->boxdata;
	struct Box_s *chessbox = Box_GetRoot(rdata->parentbox);
	struct chessboxdata_s *cbdata = chessbox->boxdata;

	char *pieces = " pnbrqkpnbrqk";

	rdata->move[4] = pieces[pdata->type];
	rdata->move[5] = '\0';

	if (rdata->roomjid)
	{
		Ctrl_SendMove(rdata->gameid, rdata->move);
		Text_SetText(cbdata->message, _("Move sent; waiting for reply..."));
		Box_Repaint(cbdata->message);
	}
	else
	{
		Ctrl_SendCorMove(rdata->gameid, rdata->move);
	}
	
	EnableWindow(rdata->parentbox->hwnd, 1);
	cbdata->promotionbox = NULL;
	Box_Destroy(rootbox);
}

void PromoteButton_Create(struct Box_s *dialog, int x, int y, struct BoxImage_s *img, enum ChessPiece type)
{
	struct Box_s *button, *subbox;
	struct promotedata_s *data;

	button = Box_Create(x, y, 41, 41, BOX_VISIBLE | BOX_TRANSPARENT | BOX_BORDER);

	data = malloc(sizeof(*data));
	data->type = type;
	button->boxdata = data;

	subbox = Button_Create(0, 0, 41, 41, BOX_VISIBLE | BOX_TRANSPARENT | BOX_STRETCHIMG);
	subbox->img = img;
	Button_SetOnButtonHit(subbox, Promote);
	Box_AddChild(button, subbox);

	Box_AddChild(dialog, button);
}

struct Box_s *PromotionBox_Create(struct Box_s *parent, char *roomjid, char *gameid, int x, int y, int iswhite, char *oldmove, char *piecethemepng)
{
	struct Box_s *popup;
	struct promotionboxdata_s *data;
	
	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->parentbox = parent;
	data->gameid = strdup(gameid);
	data->roomjid = strdup(roomjid);
	strcpy(data->move, oldmove);
	
	popup = Box_Create(x, y, 82, 82, BOX_VISIBLE);
	popup->bgcol = TabBG2;
	popup->boxdata = data;

	if (!promotionpieces)
	{
		promotionpieces = ImageMgr_GetScaledImage("piecespromotion", piecethemepng, 41 * 6, 41 * 2);
	}

	if (iswhite)
	{
		PromoteButton_Create(popup, 0,  0,  ImageMgr_GetRootSubImage("whiteknightpromotion", "piecespromotion", promotionpieces->w * 1 / 6, promotionpieces->h / 2, promotionpieces->w / 6, promotionpieces->h / 2), CP_WKNIGHT);
		PromoteButton_Create(popup, 41, 0,  ImageMgr_GetRootSubImage("whitebishoppromotion", "piecespromotion", promotionpieces->w * 2 / 6, promotionpieces->h / 2, promotionpieces->w / 6, promotionpieces->h / 2), CP_WBISHOP);
		PromoteButton_Create(popup, 0,  41, ImageMgr_GetRootSubImage("whiterookpromotion",   "piecespromotion", promotionpieces->w * 3 / 6, promotionpieces->h / 2, promotionpieces->w / 6, promotionpieces->h / 2), CP_WROOK);
		PromoteButton_Create(popup, 41, 41, ImageMgr_GetRootSubImage("whitequeenpromotion",  "piecespromotion", promotionpieces->w * 4 / 6, promotionpieces->h / 2, promotionpieces->w / 6, promotionpieces->h / 2), CP_WQUEEN);
	}
	else
	{
		PromoteButton_Create(popup, 0,  0,  ImageMgr_GetRootSubImage("blackknightpromotion", "piecespromotion", promotionpieces->w * 1 / 6, 0, promotionpieces->w / 6, promotionpieces->h / 2), CP_BKNIGHT);
		PromoteButton_Create(popup, 41, 0,  ImageMgr_GetRootSubImage("blackbishoppromotion", "piecespromotion", promotionpieces->w * 2 / 6, 0, promotionpieces->w / 6, promotionpieces->h / 2), CP_BBISHOP);
		PromoteButton_Create(popup, 0,  41, ImageMgr_GetRootSubImage("blackrookpromotion",   "piecespromotion", promotionpieces->w * 3 / 6, 0, promotionpieces->w / 6, promotionpieces->h / 2), CP_BROOK);
		PromoteButton_Create(popup, 41, 41, ImageMgr_GetRootSubImage("blackqueenpromotion",  "piecespromotion", promotionpieces->w * 4 / 6, 0, promotionpieces->w / 6, promotionpieces->h / 2), CP_BQUEEN);
	}

	Box_CreateWndMenu(popup, parent->hwnd);
	SetWindowPos(popup->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
	/*
	SetForegroundWindow(popup->hwnd);
	BringWindowToTop(popup->hwnd);
	*/
	EnableWindow(parent->hwnd, 0);

	return popup;
}

void ChessPieceBox_OnLButtonUp(struct Box_s *piece, int x, int y)
{
	struct chesspieceboxdata_s *data = piece->boxdata;
	struct Box_s *dialog = Box_GetRoot(piece);
	struct chessboxdata_s *cbdata = dialog->boxdata;
	struct Box_s *board = cbdata->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	if (bdata->dragtarget == piece)
	{
		int promote = 0;

		int newx, newy, oldx, oldy;
		int capture = 0;
		char move[5];

		char *letters = "abcdefgh";
		char *numbers = "87654321";
		char *pieces = "opnbrqkpnbrqk";
		char *pieces2 = "oPNBRQKpnbrqk";

		if (data->twoclickmove && data->x != -1)
		{
			ChessBoard_CreateHighlight(board, data->x, data->y, 0, 0, cbdata->gameover, CP_EMPTY, 0);
			return;
		}

		data->highlightedlegal = 0;

		ChessBoard_ClearHighlights(board);
		{
			struct namedlist_s *current;

			current = cbdata->movelist;

			while (current && current->next)
			{
				current = current->next;
			}

			if (current)
			{
				ChessBox_HighlightMove(dialog, current->name);
			}
		}
		/*ChessBoard_UpdateHighlights(board, bdata->showrotated, cbdata->gameover);*/

		if (bdata->piecepickuptime && timeGetTime() - bdata->piecepickuptime < 300 && Model_GetOption(OPTION_ENABLEANTISLIP))
		{
			Log_Write(0, "Denying move due to antislip");
			return;
		}

		bdata->piecepickuptime = 0;

		data->dragging = FALSE;
		oldx = data->x;
		oldy = data->y;
		if (data->x == -1 && data->y == -1)
		{
			int rootpiecex, rootpiecey, rootboardx, rootboardy;

			Box_GetRootCoords(piece, &rootpiecex, &rootpiecey);
			Box_GetRootCoords(board, &rootboardx, &rootboardy);

			newx = (rootpiecex - rootboardx + piece->w / 2) * 8 / board->w + 1;
			newy = (rootpiecey - rootboardy + piece->h / 2) * 8 / board->h + 1;
		}
		else
		{
			newx = (piece->x + piece->w / 2) * 8 / board->w + 1;
			newy = (piece->y + piece->h / 2) * 8 / board->h + 1;
		}

		if (newx < 1 || newx > 8 || newy < 1 || newy > 8)
		{
			newx = data->x;
			newy = data->y;
		}

		data->x = newx;
		data->y = newy;
		piece->x = (newx - 1) * board->w / 8;
		piece->y = (newy - 1) * board->h / 8;

		Box_Repaint(piece->parent);

		if (newx == oldx && newy == oldy)
		{
			bdata->dragtarget = NULL;
			ChessBox_ResetGamePosition(dialog, 0);
			return;
		}

		if (bdata->showrotated)
		{
			if (oldx != -1)
			{
				oldx = 9 - oldx;
			}
			if (oldy != -1)
			{
                                oldy = 9 - oldy;
			}
			if (newx != -1)
			{
				newx = 9 - newx;
			}
			if (newy != -1)
			{
                                newy = 9 - newy;
			}
		}

		if (oldx == -1 && oldy == -1)
		{
			move[0] = letters[newx - 1];
			move[1] = numbers[newy - 1];
			move[2] = pieces2[data->type];
			move[3] = '\0';
		}
		else
		{
			move[0] = letters[oldx - 1];
			move[1] = numbers[oldy - 1];
			move[2] = letters[newx - 1];
			move[3] = numbers[newy - 1];
			move[4] = '\0';
		}

		if (cbdata->promotionbox)
		{
			Box_Destroy(cbdata->promotionbox);
			cbdata->promotionbox = NULL;
		}

		promote = (data->type == CP_WPAWN && newy == 1) || (data->type == CP_BPAWN && newy == 8);

		if (promote && oldx != -1 && oldy != -1)
		{
			POINT pt;

			GetCursorPos(&pt);
			/*
			int sx, sy;
			Box_GetScreenCoords(piece, &sx, &sy);

			sx += x;
			sy += y;
			*/

			cbdata->promotionbox = PromotionBox_Create(piece, cbdata->roomjid, cbdata->gameid, pt.x - 41, pt.y - 41, /*sx - piece->w / 2, sy - piece->h / 2,*/ data->type == CP_WPAWN, move, cbdata->piecethemepng);
		}
		else
		{
			free(cbdata->lastmove);
			cbdata->lastmove = strdup(move);
			if (cbdata->roomjid)
			{
				Ctrl_SendMove(cbdata->gameid, move);
				Text_SetText(cbdata->message, _("Move sent; waiting for reply..."));
				Box_Repaint(cbdata->message);
			}
			else
			{
				Ctrl_SendCorLookAheadMove(cbdata->gameid, move);
				strcpy(cbdata->storedmove, move);
				Button_SetDisabledState(cbdata->sendmovebutton, 0);
			}
		}

		if (bdata->board[newx - 1][newy - 1] != CP_EMPTY)
		{
			capture = 1;
		}

		Box_ReleaseMouse(board);
		bdata->dragtarget = NULL;

		if (cbdata->roomjid && !promote && Model_GetOption(OPTION_ENABLELAGHIDING))
		{
			ChessBoard_ClearHighlights(board);
			ChessBoard_CreateHighlight(board, oldx, oldy, 0, bdata->showrotated, cbdata->gameover, CP_EMPTY, 0);
			ChessBoard_CreateHighlight(board, newx, newy, 0, bdata->showrotated, cbdata->gameover, CP_EMPTY, 0);

			if (!Model_GetOption(OPTION_DISABLEGAMESOUNDS))
			{
				if (capture)
				{
					Audio_PlayWav("sounds/capture.wav");
				}
				else
				{
					Audio_PlayWav("sounds/move.wav");
				}
			}

			cbdata->ishidinglag = 1;
			if (cbdata->info)
			{
				if (cbdata->whitelocal)
				{
					if (cbdata->info->blacktimecontrol && cbdata->info->blacktimecontrol->delayinc > 0)
					{
						cbdata->blacktime = cbdata->blacktime + (float)(cbdata->info->blacktimecontrol->delayinc);
					}
					else if (cbdata->info->timecontrol && cbdata->info->timecontrol->delayinc > 0)
					{
						cbdata->blacktime = cbdata->blacktime + (float)(cbdata->info->timecontrol->delayinc);
					}
				}
				else
				{
					if (cbdata->info->timecontrol && cbdata->info->timecontrol->delayinc > 0)
					{
						cbdata->whitetime = cbdata->whitetime + (float)(cbdata->info->timecontrol->delayinc);
					}
				}
			}
			ChessBox_ActivateClock(Box_GetRoot(board), cbdata->whitelocal ? CT_BLACK : CT_WHITE);
		}
	}
}

void ChessPieceBox_ResetImage(struct Box_s *piece, enum ChessPiece type,
	struct BoxImage_s *piecesimg)
{
	switch(type)
	{
		case CP_WPAWN:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 0, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WKNIGHT:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 1, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WBISHOP:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 2, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WROOK:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 3, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WQUEEN:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 4, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WKING:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 5, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BPAWN:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 0, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BKNIGHT:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 1, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BBISHOP:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 2, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BROOK:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 3, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BQUEEN:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 4, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BKING:
			piece->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 5, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
	}
}

struct Box_s *ChessPieceBox_Create(struct Box_s *board, int x, int y, enum ChessPiece type,
			  struct BoxImage_s *piecesimg)
{
	struct Box_s *piece;
	struct chesspieceboxdata_s *data;
	int sx, sy, w, h;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->type = type;

	if (board)
	{
		data->x = x;
		data->y = y;
		sx = (x - 1) * board->w / 8;
		sy = (y - 1) * board->h / 8;
		w = board->w / 8;
		h = board->h / 8;
	}
	else
	{
		data->x = -1;
		data->y = -1;
		sx = 0;
		sy = 0;
		w = x;
		h = y;
	}

	piece = Box_Create(sx, sy, w, h, BOX_VISIBLE | BOX_TRANSPARENT | BOX_STRETCHIMG | BOX_NOCLIP);
	piece->OnLButtonDown = ChessPieceBox_OnLButtonDown;
	piece->OnMouseMove = ChessPieceBox_OnMouseMove;
	piece->OnLButtonUp = ChessPieceBox_OnLButtonUp;

	piece->OnDestroy = ChessPieceBox_OnDestroy;
	piece->boxtypeid = CHESSPIECEBOX_ID;

	piece->boxdata = data;

	ChessPieceBox_ResetImage(piece, type, piecesimg);

	if (board)
	{
		Box_AddChild(board, piece);
	}

	return piece;
}

void ChessBoard_CacheImages(struct Box_s *board)
{
	struct chessboarddata_s *data = board->boxdata;

	if (data->cachedimages)
	{
		return;
	}

	BoxImage_Destroy(cachedboardimg);
	BoxImage_Destroy(cachedboarddimimg);

	cachedboardimg = data->boardimg;
	cachedboarddimimg = data->boarddimimg;
	cachedboardimgheight = data->boardimgheight;

	BoxImage_Destroy(cachedpiecesimg);
	BoxImage_Destroy(cacheddimpiecesimg);
	BoxImage_Destroy(cachedhighlightpiecesimg);
	BoxImage_Destroy(cachedcapturepiecesimg);

	cachedpiecesimg          = data->piecesimg;
	cacheddimpiecesimg       = data->dimpiecesimg;
	cachedhighlightpiecesimg = data->highlightpiecesimg;
	cachedcapturepiecesimg   = data->capturepiecesimg;
	cachedpieceheight        = data->pieceheight;

	data->cachedimages = 1;
}

void ChessBoard_OnDestroy(struct Box_s *board)
{
	struct chessboarddata_s *data = board->boxdata;

	Box_RemoveTimedFunc(board, ChessBoard_Animate, 17);

	Box_ReleaseMouse(board);

	ChessBoard_CacheImages(board);
}

struct Box_s *ChessBoard_Create(int x, int y)
{
	struct Box_s *board;
	struct chessboarddata_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	board = Box_Create(x, y, 328, 328, BOX_VISIBLE | BOX_STRETCHIMG);
	board->img = ImageMgr_GetImage("chessboard.png");
	board->boxdata = data;
	board->OnDestroy = ChessBoard_OnDestroy;

	return board;
}

void Highlight_Update(struct Box_s *highlight, struct Box_s *board, int rotated, int gameover)
{
	struct chessboardhighlight_s *data = highlight->boxdata;
	int x, y, color;
	enum ChessPiece piece;
	struct BoxImage_s *piecesimg;
	struct chessboarddata_s *cbdata = board->boxdata;

	if (data->capture)
	{
		piecesimg = cbdata->capturepiecesimg;
	}
	else
	{
		piecesimg = cbdata->highlightpiecesimg;
	}

	x = data->x;
	y = data->y;
	color = data->color;
	piece = data->piece;

	if (rotated)
	{
		highlight->x = (8 - x) * board->w / 8;
		highlight->y = (8 - y) * board->h / 8;
		highlight->w = board->w / 8;
		highlight->h = board->h / 8;
	}
	else
	{
		highlight->x = (x - 1) * board->w / 8;
		highlight->y = (y - 1) * board->h / 8;
		highlight->w = board->w / 8;
		highlight->h = board->h / 8;
	}
	
	switch (piece)
	{
		default:
		case CP_EMPTY:
			if ((x + y) % 2)
			{
				if (gameover)
				{
					highlight->img = ImageMgr_GetRootSubImage("ChessSquareHighlightOrange2Dim", "ChessSquareHighlightDim", 41, 82, 41, 41);
				}
				else
				{
					highlight->img = ImageMgr_GetSubImage("ChessSquareHighlightOrange2", "ChessSquareHighlight.png", 41, 82, 41, 41);
				}
			}
			else
			{
				if (gameover)
				{
					highlight->img = ImageMgr_GetRootSubImage("ChessSquareHighlightOrange1Dim", "ChessSquareHighlightDim", 0, 82, 41, 41);
				}
				else
				{
					highlight->img = ImageMgr_GetSubImage("ChessSquareHighlightOrange1", "ChessSquareHighlight.png", 0, 82, 41, 41);
				}
			}
			break;
		case CP_WPAWN:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 0, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WKNIGHT:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 1, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WBISHOP:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 2, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WROOK:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 3, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WQUEEN:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 4, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_WKING:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 5, piecesimg->h / 2, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BPAWN:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 0, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BKNIGHT:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 1, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BBISHOP:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 2, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BROOK:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 3, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BQUEEN:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 4, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
		case CP_BKING:
			highlight->img = BoxImage_SubImage(piecesimg, piecesimg->w / 6 * 5, 0, piecesimg->w / 6, piecesimg->h / 2);
			break;
	}

}

void ChessBoard_CreateHighlight(struct Box_s *board, int x, int y, int color, int rotated, int gameover, enum ChessPiece piece, int capture)
{
	struct Box_s *highlight = Box_Create(0, 0, 300, 300, BOX_VISIBLE | BOX_TRANSPARENT | BOX_STRETCHIMG);
	struct chessboardhighlight_s *data;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	data->x = x;
	data->y = y;
	data->color = color;
	data->piece = piece;
	data->capture = capture;

	highlight->boxdata = data;
	highlight->boxtypeid = HIGHLIGHTBOX_ID;

	Highlight_Update(highlight, board, rotated, gameover);
	
	if (piece == CP_EMPTY)
	{
		Box_AddChildToBottom(board, highlight);
	}
	else
	{
		Box_AddChild(board, highlight);
	}
}

void ChessBoard_ClearHighlights(struct Box_s *board)
{
	/* Only clear highlights and pieces that aren't there */
	struct Box_s *pbox = board->child;
	while (pbox)
	{
		struct Box_s *next = pbox->sibling;

		if (pbox->boxtypeid == HIGHLIGHTBOX_ID)
		{
                        Box_Destroy(pbox);
		}
		pbox = next;
	}
}

void ChessBoard_UpdateHighlights(struct Box_s *board, int rotated, int gameover)
{
	struct Box_s *pbox = board->child;

	while (pbox)
	{
		struct Box_s *next = pbox->sibling;

		if (pbox->boxtypeid == HIGHLIGHTBOX_ID)
		{
			Highlight_Update(pbox, board, rotated, gameover);
		}
		pbox = next;
	}
}

void ChessBoard_UpdateBoard(struct Box_s *board)
{
	struct Box_s *chessbox = Box_GetRoot(board);
	struct chessboarddata_s *data = board->boxdata;
	struct chessboxdata_s *rootdata = chessbox->boxdata;
	struct Box_s *pbox;
	int x, y;
	struct BoxImage_s *piecesimg;

	if (!Model_GetOption(OPTION_DISABLEGAMESOUNDS))
	{
		switch(data->movefinishsound)
		{
			case 1:
				Audio_PlayWav("sounds/move.wav");
				break;
			case 2:
				Audio_PlayWav("sounds/capture.wav");
				break;
			case 3:
				Audio_PlayWav("sounds/boom.wav");
				break;
			default:
			case 0:
				break;
		}
	}
	data->movefinishsound = 0;

	Box_ReleaseMouse(board);
	data->dragtarget = NULL;

	NamedList_Destroy(&(data->animpieces));

	if (board->w != data->boardimgheight)
	{
		BoxImage_Destroy(data->boardimg);
		BoxImage_Destroy(data->boarddimimg);

		data->boardimgheight = board->h;

		if (board->h == cachedboardimgheight)
		{
			data->boarddimimg = cachedboarddimimg;
			data->boardimg    = cachedboardimg;
			cachedboarddimimg = NULL;
			cachedboardimg = NULL;
			cachedboardimgheight = 0;
		}
		else
		{
                        data->boarddimimg = BoxImage_Scale(ImageMgr_GetDimmedImage("chessboarddim", "chessboard.png", 70), board->w, board->h);
			data->boardimg = BoxImage_Scale(ImageMgr_GetImage("chessboard.png"), board->w, board->h);
		}

		board->img = data->boardimg;
	}

	if (board->w / 8 != data->pieceheight || !data->piecesimg)
	{
		int h = board->h / 8;
		int w = board->w / 8;

		data->pieceheight = h;

		BoxImage_Destroy(data->piecesimg);
		BoxImage_Destroy(data->dimpiecesimg);
		BoxImage_Destroy(data->highlightpiecesimg);
		BoxImage_Destroy(data->capturepiecesimg);

		if (h == cachedpieceheight)
		{
			data->piecesimg          = cachedpiecesimg;
			data->dimpiecesimg       = cacheddimpiecesimg;
			data->highlightpiecesimg = cachedhighlightpiecesimg;
			data->capturepiecesimg   = cachedcapturepiecesimg;
			
			cachedpiecesimg = NULL;
			cacheddimpiecesimg = NULL;
			cachedhighlightpiecesimg = NULL;
			cachedcapturepiecesimg = NULL;
			cachedpieceheight = 0;
		}
		else
		{
			data->piecesimg = BoxImage_Scale(ImageMgr_GetImage(rootdata->piecethemepng), w * 6, h * 2);
			data->dimpiecesimg = BoxImage_Dim(data->piecesimg, 70);
			data->highlightpiecesimg = BoxImage_Trans(data->piecesimg, 30);
			data->capturepiecesimg = BoxImage_Tint(data->piecesimg, RGB(255, 0, 0));
		}
	}

	if (rootdata->gameover)
	{
		board->img = data->boarddimimg;
		piecesimg = data->dimpiecesimg;
	}
	else
	{
		board->img = data->boardimg;
		piecesimg = data->piecesimg;
	}

	ChessBoard_UpdateHighlights(board, data->showrotated, rootdata->gameover);

	pbox = board->child;
	while (pbox)
	{
		struct Box_s *next = pbox->sibling;

		if (pbox->boxtypeid == CHESSPIECEBOX_ID)
		{
			struct chesspieceboxdata_s *pdata = pbox->boxdata;
			if (data->showrotated)
			{
				if (pdata->x < 1 || pdata->x > 8 || pdata->y < 1 || pdata->y > 8 || data->board[8 - pdata->x][8 - pdata->y] != pdata->type)
				{
					Box_Destroy(pbox);
				}
				else
				{
					ChessPieceBox_ResetImage(pbox, pdata->type, piecesimg);
					pbox->x = board->w * (pdata->x - 1) / 8;
					pbox->y = board->h * (pdata->y - 1) / 8;
					pbox->w = board->w / 8;
					pbox->h = board->h / 8;
				}
			}
			else
			{
				if (pdata->x < 1 || pdata->x > 8 || pdata->y < 1 || pdata->y > 8 || data->board[pdata->x - 1][pdata->y - 1] != pdata->type)
				{
					Box_Destroy(pbox);
				}
				else
				{
					ChessPieceBox_ResetImage(pbox, pdata->type, piecesimg);
					pbox->x = board->w * (pdata->x - 1) / 8;
					pbox->y = board->h * (pdata->y - 1) / 8;
					pbox->w = board->w / 8;
					pbox->h = board->h / 8;
				}
			}
		}

		pbox = next;
	}
/*
	{
		struct namedlist_s *entry = data->highlights;

		while (entry)
		{
			struct chessboardhighlight_s *highlightdata = entry->data;
			ChessBoard_CreateHighlight(board, highlightdata->x, highlightdata->y, highlightdata->color, data->showrotated, rootdata->gameover);
			entry = entry->next;
		}
	}
*/

	for (x = 1; x < 9; x++)
	{
		for (y = 1; y < 9; y++)
		{
			enum ChessPiece type = data->board[x-1][y-1];
			if (type != CP_EMPTY)
			{
				/* search for an existing piece first */
				int found = 0;
				pbox = board->child;

				while (pbox && !found)
				{
					if (pbox->boxtypeid == CHESSPIECEBOX_ID)
					{
						struct chesspieceboxdata_s *pdata = pbox->boxdata;
						if (data->showrotated)
						{
							if (pdata->x == 9 - x && pdata->y == 9 - y && pdata->type == type)
							{
								found = 1;
							}
						}
						else
						{
							if (pdata->x == x && pdata->y == y && pdata->type == type)
							{
								found = 1;
							}
						}
					}

					pbox = pbox->sibling;
				}

				if (!found)
				{
					if (data->showrotated)
					{
						ChessPieceBox_Create(board, 9 - x, 9 - y, type, piecesimg);
					}
					else
					{
						ChessPieceBox_Create(board, x, y, type, piecesimg);
					}
				}
			}
		}
	}



	Box_Repaint(board);
}

void ChessBoard_DeferUpdateBoardCallback(struct Box_s *chessboard, void *dummy)
{
	Box_RemoveTimedFunc(chessboard, ChessBoard_DeferUpdateBoardCallback, 50);
	ChessBoard_UpdateBoard(chessboard);
}

void ChessBoard_DeferUpdateBoard(struct Box_s *chessboard)
{
	/* do a quick resize job on the pieces, and do the detailed resize later */
	struct chessboarddata_s *bdata = chessboard->boxdata;
	struct Box_s *child;

	child = chessboard->child;
	while (child)
	{
		if (child->boxtypeid == CHESSPIECEBOX_ID)
		{
			struct chesspieceboxdata_s *data = child->boxdata;
			child->x = (data->x - 1) * chessboard->w / 8;
			child->y = (data->y - 1) * chessboard->h / 8;
			child->w = chessboard->w / 8;
			child->h = chessboard->h / 8;
		}
		else if (child->boxtypeid == HIGHLIGHTBOX_ID)
		{
			struct chessboardhighlight_s *data = child->boxdata;
			child->x = (bdata->showrotated ? (8 - data->x) : (data->x - 1)) * chessboard->w / 8;
			child->y = (bdata->showrotated ? (8 - data->y) : (data->y - 1)) * chessboard->h / 8;
			child->w = chessboard->w / 8;
			child->h = chessboard->h / 8;
		}

		child = child->sibling;
	}
	Box_Repaint(Box_GetRoot(chessboard));
	Box_RemoveTimedFunc(chessboard, ChessBoard_DeferUpdateBoardCallback, 50);
	Box_AddTimedFunc(chessboard, ChessBoard_DeferUpdateBoardCallback, NULL, 50);
}


void ChessBox_UpdateTime(struct Box_s *chessbox, void *userdata);
void TimeDelay_ShrinkDelay(struct Box_s *delaybox, void *userdata);

void ChessBox_OnMove(struct Box_s *chessbox, int x, int y)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	Box_OnMove(chessbox, x, y);

	if (data->drawervisible)
	{
		Box_MoveWndCustom2(data->drawer, x - data->drawer->w, y + 20, data->drawer->w, chessbox->h - 40);
	}
}

void ChessBox_AdjustMenuAndMovelist(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	int top, totalheight;
	int minmenuheight;

	ChessBox_UpdateMenu(chessbox); /* get the right size */

	top = data->gameinfobox->y + data->gameinfobox->h + 5;

	if (data->gamechatonbottom)
	{
		totalheight = data->message->y + data->message->h - top - 5;
	}
	else
	{
		totalheight = chessbox->h - top - 5;
	}

	data->menutextborder->x = data->gameinfobox->x;
	data->movelistborder->x = data->gameinfobox->x;
	minmenuheight = data->menutext->h + 10;

	if (totalheight < minmenuheight + 19) /*< minmenuheight + data->menumovetabs->h)*/
	{
		data->menutextborder->flags &= ~BOX_VISIBLE;
		data->movelistborder->flags &= ~BOX_VISIBLE;
		data->buttons->flags |= BOX_VISIBLE;
		data->menumovetabs->flags &= ~BOX_VISIBLE;

		data->buttons->x = data->gameinfobox->x;
		data->buttons->y = data->gameinfobox->y + data->gameinfobox->h + 5;
	}
	else if (totalheight < minmenuheight + 5 + 80 + 19)
	{
		data->menutextborder->flags |= BOX_VISIBLE;
		data->movelistborder->flags &= ~BOX_VISIBLE;
		data->buttons->flags &= ~BOX_VISIBLE;
		/*data->menumovetabs->flags |= BOX_VISIBLE;*/

		data->menutextborder->y = data->gameinfobox->y + data->gameinfobox->h + 5;
/*
		data->menutextborder->flags &= ~BOX_BORDERALL;
		data->menutextborder->flags |= BOX_BORDER6;
		data->movelistborder->flags &= ~BOX_BORDERALL;
		data->movelistborder->flags |= BOX_BORDER6;

		data->menumovetabs->x = data->gameinfobox->x;
		data->menumovetabs->y = data->gameinfobox->y + data->gameinfobox->h + 5;
		TabCtrl_ActivateTabByName(data->menumovetabs, "menu");

		data->menutextborder->y = data->menumovetabs->y + data->menumovetabs->h - 1;
		Box_OnSizeHeight_Stretch(data->menutextborder, totalheight - data->menumovetabs->h - data->menutextborder->h);

		data->movelistborder->y = data->menumovetabs->y + data->menumovetabs->h - 1;
		Box_OnSizeHeight_Stretch(data->movelistborder, totalheight - data->menumovetabs->h - data->movelistborder->h);
*/
	}
	else
	{
		data->menutextborder->flags |= BOX_VISIBLE;
		if (!data->movelisthidden)
		{
			/*data->movelistborder->flags |= BOX_VISIBLE;*/
		}
		data->buttons->flags &= ~BOX_VISIBLE;
		data->menumovetabs->flags &= ~BOX_VISIBLE;
/*
		data->menutextborder->flags &= ~BOX_BORDERALL;
		data->menutextborder->flags |= BOX_BORDER5;
		data->movelistborder->flags &= ~BOX_BORDERALL;
		data->movelistborder->flags |= BOX_BORDER5;
*/

		data->menutextborder->y = data->gameinfobox->y + data->gameinfobox->h + 5;
		Box_OnSizeHeight_Stretch(data->menutextborder, minmenuheight + 30 - data->menutextborder->h);

		data->movelistborder->y = data->menutextborder->y + data->menutextborder->h + 5;
		Box_OnSizeHeight_Stretch(data->movelistborder, totalheight - (data->menutextborder->h + 5) - data->movelistborder->h);
	}

	ChessBox_UpdateMenu(chessbox); /* update for visible/invisible move list */

	Box_Repaint(chessbox);
}

void ChessBox_ResizeBoard(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *chessboard = data->chessboard;
	struct chessboarddata_s *bdata = chessboard->boxdata;
	int gameinfoheight, leftpanelwidth;
	/* calculate the size we need for everything */
	gameinfoheight = 148;

	leftpanelwidth = chessbox->h - data->boardborder->y - 5 - 5 - data->message->h;/*chessbox->h - 20 - TitleBarHeight - 4 - 40;*/
	if (leftpanelwidth > chessbox->w - data->gameinfobox->w - 5 - ((data->hidechat) ? 15 : (150 + 5)))
	{
		leftpanelwidth = chessbox->w - data->gameinfobox->w - 5 - ((data->hidechat) ? 15 : (150 + 5));
	}

	Box_OnSizeWidth_Stretch(data->boardborder, leftpanelwidth - data->boardborder->w);
	Box_OnSizeHeight_Stretch(data->boardborder, leftpanelwidth - data->boardborder->h);
	ChessBoxBorder_RedoBorder(data->boardborder, bdata->showrotated);

	Box_OnSizeWidth_Stretch(data->chessboard, leftpanelwidth - 40 - data->chessboard->w);
	Box_OnSizeHeight_Stretch(data->chessboard, leftpanelwidth - 40 - data->chessboard->h);
	ChessBoard_DeferUpdateBoard(data->chessboard);

	Text_OnSizeWidth_Stretch(data->message, leftpanelwidth - 20 - data->message->w);
	data->message->y = data->boardborder->y + data->boardborder->h + 5;

	data->gameinfobox->x = data->boardborder->x + data->boardborder->w + 5;
	ChessBox_AdjustMenuAndMovelist(chessbox);

	data->chatborder->x = data->gameinfobox->x + data->gameinfobox->w + 5;
	data->chatborder->y = data->boardborder->y;
	ChessBox_ArrangeChats(chessbox);

	Box_Repaint(chessbox);

	if (data->drawervisible)
	{
		Box_MoveWndCustom2(data->drawer, chessbox->x - data->drawer->w, chessbox->y + 20, data->drawer->w, chessbox->h - 40);
	}

	if (data->hidechat)
	{
		chessbox->minw = 490;
	}
	else
	{
                chessbox->minw = data->boardborder->w + data->gameinfobox->w + 5 + 210 + 5;
	}
}


void ChessBox_OnSizeWidth(struct Box_s *chessbox, int dwidth)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	Box_OnSizeWidth_Stretch(chessbox, dwidth);

	ChessBox_ResizeBoard(chessbox);
	View_SetSavedWindowPos2("ChessBox3", chessbox->x, chessbox->y, chessbox->w, chessbox->h, data->hidechat ? -data->oldchatw : 0, 0);
}

void ChessBox_OnSizeHeight(struct Box_s *chessbox, int dheight)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	Box_OnSizeHeight_Stretch(chessbox, dheight);

	ChessBox_ResizeBoard(chessbox);
	View_SetSavedWindowPos2("ChessBox3", chessbox->x, chessbox->y, chessbox->w, chessbox->h, data->hidechat ? -data->oldchatw : 0, 0);
}


void ChessBox_ResetChatPositions(struct Box_s *chessbox)
{
	struct chessboxdata_s *data;

	chessbox = Box_GetRoot(chessbox);
	
	data = chessbox->boxdata;

	if (!data->gamechatonbottom)
	{
		ChessBox_OnSizeHeight(chessbox, 0);
	}
	else
	{
		ChessBox_OnSizeWidth(chessbox, 0);
	}
}

void ChessBox_AnimateDrawerOpen(struct Box_s *pbox, void *userdata)
{
	struct chessboxdata_s *data = pbox->boxdata;

	data->drawer->w += 10;
	Box_MoveWndCustom2(data->drawer, pbox->x - data->drawer->w, pbox->y + 20, data->drawer->w, pbox->h - 40);

	if (data->drawer->w >= 150)
	{
		data->draweranimating = 0;
		data->drawervisible = 1;
		Box_RemoveTimedFunc(pbox, ChessBox_AnimateDrawerOpen, 20);
		ChessBox_UpdateMenu(pbox);
	}
}

void ChessBox_AnimateDrawerClose(struct Box_s *pbox, void *userdata)
{
	struct chessboxdata_s *data = pbox->boxdata;

	data->drawer->w -= 10;
	Box_MoveWndCustom2(data->drawer, pbox->x - data->drawer->w, pbox->y + 20, data->drawer->w, pbox->h - 40);

	if (data->drawer->w <= 0)
	{
		DestroyWindow(data->drawer->hwnd);
		data->draweranimating = 0;
		data->drawervisible = 0; 
		Box_RemoveTimedFunc(pbox, ChessBox_AnimateDrawerClose, 20);
		ChessBox_UpdateMenu(pbox);
	}

}

void ChessBox_ToggleMoveList(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->draweranimating)
	{
		return;
	}

	if (!data->drawervisible)
	{
		data->drawer->x = chessbox->x;
		data->drawer->y = chessbox->y - 20;
		data->drawer->w = 0;
		Box_OnSizeHeight_Stretch(data->drawer, chessbox->h - 40 - data->drawer->h);
		/*data->drawer->h = chessbox->h - 40;*/
		
		data->draweranimating = 1;
		Box_CreateWndCustom2(data->drawer, NULL, chessbox);
		
		Box_AddTimedFunc(chessbox, ChessBox_AnimateDrawerOpen, NULL, 20);
		Box_RemoveTimedFunc(chessbox, ChessBox_AnimateDrawerClose, 20);
		/*Button_SetNormalImg (pbox, ImageMgr_GetSubImage("moveListToggle-active",  "moveListToggle.png", 112, 0, 28, 29));*/
		Box_Repaint(pbox);
	}
	else
	{
		data->draweranimating = 1;
		Box_AddTimedFunc(chessbox, ChessBox_AnimateDrawerClose, NULL, 20);
		Box_RemoveTimedFunc(chessbox, ChessBox_AnimateDrawerOpen, 20);
		/*Button_SetNormalImg (pbox, ImageMgr_GetSubImage("moveListToggle-normal",  "moveListToggle.png",  0, 0, 28, 29));*/
		Box_Repaint(pbox);
	}
}

void ChessBox_ResetGamePosition(struct Box_s *chessbox, int illegalmove)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;

	Box_RemoveTimedFunc(data->whitedelay, TimeDelay_ShrinkDelay, 100);
	Box_RemoveTimedFunc(data->blackdelay, TimeDelay_ShrinkDelay, 100);

	if (data->ishidinglag)
	{
		data->ishidinglag = 0;
		Log_Write(0, "ResetGamePosition() ticking updatetime\n");
		Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);
		Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, 1000);
		data->lastperiod = 1000;
	}

	ChessBoard_UpdateBoard(board);

	if (illegalmove && !data->gameover)
	{
		Text_SetText(data->message, _("That is an illegal move."));
	}

	ChessBox_UpdateCaptures(chessbox);
}

void ChessBox_UpdateCaptures(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct chessboarddata_s *ddata = data->chessboard->boxdata;
	char *a;
	int x, y;
	int iscrazyhouse;

	iscrazyhouse = (data->info && data->info->variant && stricmp(data->info->variant, "crazyhouse") == 0);

	if (!capturedpiecesimg)
	{
		capturedpiecesimg = BoxImage_Scale(ImageMgr_GetImage(data->piecethemepng), 24 * 6, 24 * 2);
	}

	while (data->whitecapturebox->child)
	{
		Box_Destroy(data->whitecapturebox->child);
	}

	a = data->blackcaptured;
	x = 4;
	y = 0;

	if (a && *a)
	{
		data->whitecapturebox->flags |= BOX_VISIBLE;
		Box_SetText(data->whitecapturebox, NULL);
	}
	else
	{
		/*data->whitecapturebox->flags &= ~BOX_VISIBLE;*/
		Box_SetText(data->whitecapturebox, _("No pieces taken."));
	}

	while (a && *a)
	{
		struct Box_s *piecebox;
		enum ChessPiece type;
		
		type = ConvertCharToPiece(*a);

		if (iscrazyhouse)
		{
			type = SwapPieceColor(type);
		}

		piecebox = ChessPieceBox_Create(NULL, 24, 24, type, capturedpiecesimg);
		piecebox->x = x;
		piecebox->y = y;

		if (data->whitecapturebox->w < piecebox->x + 24)
		{
			data->whitecapturebox->w = piecebox->x + 24;
		}

		if (data->whitecapturebox->h < piecebox->y + 24)
		{
			data->whitecapturebox->h = piecebox->y + 24;
		}

		x += 22;

		if (x + 22 >= 22 * 9 + 4)
		{
			x = 4;
			y += 24;
		}

		Box_AddChild(data->whitecapturebox, piecebox);
		a += 3;
	}

	while (data->blackcapturebox->child)
	{
		Box_Destroy(data->blackcapturebox->child);
	}

	a = data->whitecaptured;
	x = 4;
	y = 0;

	if (a && *a)
	{
		data->blackcapturebox->flags |= BOX_VISIBLE;
		Box_SetText(data->blackcapturebox, NULL);
	}
	else
	{
		Box_SetText(data->blackcapturebox, _("No pieces taken."));
		/*data->blackcapturebox->flags &= ~BOX_VISIBLE;*/
	}

	while (a && *a)
	{
		struct Box_s *piecebox;
		enum ChessPiece type;
		
		type = ConvertCharToPiece(*a);

		if (iscrazyhouse)
		{
			type = SwapPieceColor(type);
		}

		piecebox = ChessPieceBox_Create(NULL, 24, 24, type, capturedpiecesimg);

		piecebox->x = x;
		piecebox->y = y;

		if (data->blackcapturebox->w < piecebox->x + 24)
		{
			data->blackcapturebox->w = piecebox->x + 24;
		}

		if (data->blackcapturebox->h < piecebox->y + 24)
		{
			data->blackcapturebox->h = piecebox->y + 24;
		}

		x += 22;

		if (x + 22 >= 22 * 9 + 4)
		{
			x = 4;
			y += 24;
		}

		Box_AddChild(data->blackcapturebox, piecebox);
		a += 3;
	}

	AutoSize_Fit(data->gameinfobox);

	ChessBox_AdjustMenuAndMovelist(chessbox);

	Box_Repaint(chessbox);
}


int MoveList_Update(struct Box_s *movelistbox)
{
	struct Box_s *drawer = Box_GetRoot(movelistbox);
	struct chessboxdrawerdata_s *ddata = drawer->boxdata;
	struct Box_s *chessbox = ddata->chessbox;
#if 0
	struct Box_s *chessbox = Box_GetRoot(movelistbox);/*ddata->chessbox;*/
#endif
	struct chessboxdata_s *data = chessbox->boxdata;
	char captured[1024];

	struct namedlist_s *entry;
	enum ChessPiece tempboard[8][8];

	int movenum = 1, showmovenum = 1, whitemovenum = 0, blackmovenum = 0;

	if (data->initialfen)
	{
		ChessLogic_ParseFEN(data->initialfen, tempboard, NULL, NULL, NULL);
	}
	else
	{
		ChessLogic_LoadDefaultBoard(tempboard);
	}

	List_RemoveAllEntries(movelistbox);

	captured[0] = '\0';

	entry = data->movelist;
	while (entry)
	{
		struct namedlist_s *whitelistentry, *blacklistentry, *next = NULL;
		struct Box_s *entrybox, *subbox;
		char txt[80];

		whitelistentry = entry;
		blacklistentry = entry->next;
		if (blacklistentry)
		{
			next = blacklistentry->next;
		}

		sprintf(txt, "%d.", showmovenum);
		showmovenum++;

		entrybox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->userdata = (void *)showmovenum;

		subbox = Box_Create(5, 0, 20, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = MoveListFG1;
		Box_SetText(subbox, txt);
		Box_AddChild(entrybox, subbox);

		if (whitelistentry)
		{
			char *move = InterpretMove(tempboard, whitelistentry->name, whitelistentry->data);
			/*strcat(captured, GetCaptureText(tempboard, whitelistentry->name));*/
			ExecuteMove(tempboard, whitelistentry->name, 0, NULL, NULL);

			subbox = Box_Create(25, 0, 50, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->fgcol = MoveListFG2;
			Box_SetText(subbox, move);
			Box_AddChild(entrybox, subbox);
			movenum++;
			whitemovenum++;

			free(data->lastlistmove);
			data->lastlistmove = strdup(move);
			data->lastlistmovecolor = 1;

			free(move);
		}

		if (blacklistentry)
		{
			char *move = InterpretMove(tempboard, blacklistentry->name, blacklistentry->data);
			/*strcat(captured, GetCaptureText(tempboard, blacklistentry->name));*/
			ExecuteMove(tempboard, blacklistentry->name, 0, NULL, NULL);

			subbox = Box_Create(80, 0, 50, 16, BOX_VISIBLE | BOX_TRANSPARENT);
			subbox->fgcol = MoveListFG2;
			Box_SetText(subbox, move);
			Box_AddChild(entrybox, subbox);
			movenum++;
			blackmovenum++;

			free(data->lastlistmove);
			data->lastlistmove = strdup(move);
			data->lastlistmovecolor = 0;

			free(move);
		}

		List_AddEntry(movelistbox, NULL, NULL, entrybox);

		entry = next;
	}

	if (movenum % 2)
	{
		struct Box_s *entrybox, *subbox;

		char txt[80];
		sprintf(txt, "%d.", showmovenum);

		entrybox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->userdata = (void *)showmovenum;

		subbox = Box_Create(5, 0, 20, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = MoveListFG1;
		Box_SetText(subbox, txt);
		Box_AddChild(entrybox, subbox);

		List_AddEntry(movelistbox, NULL, NULL, entrybox);
	}

	List_RedoEntries(movelistbox);
	Box_Repaint(movelistbox);

	data->whitemovenum = whitemovenum;
	data->blackmovenum = blackmovenum;

	{
		int whitemove = 1;
		struct namedlist_s *capturedlist = NULL;

		if (data->initialfen)
		{
			ChessLogic_ParseFEN(data->initialfen, tempboard, NULL, NULL, NULL);
		}
		else
		{
			ChessLogic_LoadDefaultBoard(tempboard);
		}
	
		if (data->info && data->info->variant && stricmp(data->info->variant, "crazyhouse") == 0)
		{
			entry = data->movelist;
			while (entry)
			{
				char *cappiece = GetCaptureText(tempboard, entry->name);

				if (IsPiecePlacement(entry->name))
				{
					struct namedlist_s **ppentry;
					char droppiece[2];

					droppiece[0] = entry->name[2];
					droppiece[1] = '\0';
					if (!whitemove)
					{
						Log_Write(0, "is black's move, so remove a white piece");
						strupr(droppiece);
					}
					else
					{
						strlwr(droppiece);
					}

					Log_Write(0, "removing %s\n", droppiece);

					/* normal namedlist remove isn't case sensitive, so we make our own */
					ppentry = &capturedlist;
					while (ppentry && *ppentry)
					{
						if (strcmp((*ppentry)->name, droppiece) == 0)
						{
							NamedList_Remove(ppentry);
							ppentry = NULL;
						}
						else
						{
                                                        ppentry = &((*ppentry)->next);
						}
					}
				}
				else if (cappiece && strlen(cappiece))
				{
					char name[2];
					name[0] = cappiece[0];
					name[1] = '\0';
					Log_Write(0, "adding %s %s\n", name, &(cappiece[1]));
					NamedList_AddString(&capturedlist, name, &(cappiece[1]));
				}

				ExecuteMove(tempboard, entry->name, 1, NULL, NULL);

				whitemove = !whitemove;
				entry = entry->next;
			}

			entry = capturedlist;
			while (entry)
			{
				strcat(captured, entry->name);
				strcat(captured, entry->data);
				entry = entry->next;
			}

			Log_Write(0, "captured %s\n", captured);
		}
		else
		{
			entry = data->movelist;
			while (entry)
			{
				strcat(captured, GetCaptureText(tempboard, entry->name));
				ExecuteMove(tempboard, entry->name, 0, NULL, NULL);
				entry = entry->next;
			}
		}
	}

	{
		char whitecaptured[512], blackcaptured[512];
		char *p;
		char *pieces = "oPNBRQKpnbrqk";

		whitecaptured[0] = '\0';
		blackcaptured[0] = '\0';

		p = captured;

		while (*p)
		{
			int loc;

			loc = strchr(pieces, *p) - pieces;

			if (loc <= 6)
			{
				strncat(whitecaptured, p, 3);
			}
			else
			{
				strncat(blackcaptured, p, 3);
			}
			p += 3;
		}

		data->whitecaptured = strdup(whitecaptured);
		data->blackcaptured = strdup(blackcaptured);
	}

	ChessBox_UpdateCaptures(chessbox);

	return movenum;
}

void ChessBox_ClearMoveList(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	NamedList_Destroy(&(data->movelist));

	data->movenum = MoveList_Update(data->movelistbox);
	ChessBox_RefreshTitle(chessbox);
}


void ChessBox_AddMoveToList(struct Box_s *chessbox, char *longmove, char *annotation, int numtakebacks)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char *move, side;
	char txt[512];
	char txt2[256];

	if (numtakebacks)
	{
		MoveList_Update(data->movelistbox);
		move = strdup(data->lastlistmove);
		side = data->lastlistmovecolor;
		sprintf(txt2, "%d", numtakebacks);
		while (numtakebacks)
		{
			NamedList_RemoveLast(&(data->movelist));
			numtakebacks--;
		}
		data->movenum = MoveList_Update(data->movelistbox);
		i18n_stringsub(txt, 512, _("%1 moves have been taken back"), txt2);
		SubChat_AddSmallText(data->chatlist, txt, 0, data->edit);
	}
	else
	{
		NamedList_Add(&(data->movelist), longmove, strdup(annotation), NULL);
		data->movenum = MoveList_Update(data->movelistbox);
		move = strdup(data->lastlistmove);
		side = data->lastlistmovecolor;
		i18n_stringsub(txt, 512, _("%1 has moved %2"), side ? _("White") : _("Black"), move);
		SubChat_AddSmallText(data->chatlist, txt, 0, data->edit);
	}

	free(move);

	ChessBox_UpdateControlMsg(chessbox);
	ChessBox_RefreshTitle(chessbox);
}

void ChessBox_SetMoveList(struct Box_s *chessbox, struct namedlist_s *textmovelist)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct namedlist_s *current = textmovelist;

	NamedList_Destroy(&(data->movelist));

	data->movelist = NamedList_DupeStringList(textmovelist);

	data->movenum = MoveList_Update(data->movelistbox);

	ChessBox_UpdateControlMsg(chessbox);
	ChessBox_RefreshTitle(chessbox);

	if (!current)
	{
		return;
	}

	while (current->next)
	{
		current = current->next;
	}

	ChessBox_HighlightMove(chessbox, current->name);

	ChessBox_ActivateClock(chessbox, data->turn);
}

void ChessBox_ShowScore(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;
	int whitescore = 0;
	int blackscore = 0;
	char txt[256];

	if (data->gameover || data->disconnected)
	{
		return;
	}

	ChessLogic_CalcScore(bdata->board, &whitescore, &blackscore);

	{
		char whitescoretxt[4];
		char blackscoretxt[4];

		sprintf(whitescoretxt, "%d", whitescore);
		sprintf(blackscoretxt, "%d", blackscore);

		i18n_stringsub(txt, 256, _("Current score: %1 to %2"), whitescoretxt, blackscoretxt);
	}

	Text_SetText(data->message, txt);

	Box_Repaint(data->message);
}

void TimeDelay_ShrinkDelay(struct Box_s *delaybox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(delaybox);
	struct chessboxdata_s *data = chessbox->boxdata;
	int delayinc;
	int w;

	if (data->gameover || !data->info || !data->info->timecontrol)
	{
		delaybox->w = 0;
		Box_Repaint(delaybox->parent);
		Box_RemoveTimedFunc(delaybox, TimeDelay_ShrinkDelay, 100);
		return;
	}

	if (delaybox == data->whitedelay)
	{
		delayinc = data->info->timecontrol->delayinc;
	}
	else
	{
		if (data->info->blacktimecontrol)
		{
			delayinc = data->info->blacktimecontrol->delayinc;
		}
		else
		{
			delayinc = data->info->timecontrol->delayinc;
		}
	}

	w = (-delayinc * 1000 - (timeGetTime() - data->lastmovetick));

	if (w <= 0)
	{
		int msec = 0;

		Box_RemoveTimedFunc(delaybox, TimeDelay_ShrinkDelay, 100);
		delaybox->w = 0;
		if (delaybox == data->whitedelay)
		{
			data->whiteclock->fgcol = NORMAL_WHITE_CLOCK_COLOR;
			data->whitedot->img = ImageMgr_GetImage("active.png");
			Log_Write(0, "starting white clock\n");
			data->whitetime = ChessBox_CalcClock(chessbox, 1);
			data->tickwhiteset = timeGetTime();
		}
		else if (delaybox == data->blackdelay)
		{
			data->blackclock->fgcol = NORMAL_BLACK_CLOCK_COLOR;
			data->blackdot->img = ImageMgr_GetImage("active.png");
			Log_Write(0, "starting black clock\n");
			data->blacktime = ChessBox_CalcClock(chessbox, 0);
			data->tickblackset = timeGetTime();
		}

		Box_Repaint(delaybox->parent);
		Log_Write(0, "ShrinkDelay() ticking updatetime\n");

		if (delaybox == data->whitedelay)
		{
			msec = ChessBox_SetVisibleClock(chessbox, data->whitetime, 1);
		}
		else if (delaybox == data->blackdelay)
		{
			msec = ChessBox_SetVisibleClock(chessbox, data->blacktime, 0);
		}

		Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);

		if (msec > 0)
		{
			if (msec <= 100)
			{
				msec = 900 + msec;
			}
			else
			{
				msec = msec - 100;
			}
			Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
			data->lastperiod = msec;
		}
		else
		{
			if (msec <= -900)
			{
				msec = 900 + (1000 + msec);
			}
			else
			{
				msec = 900 + msec;
			}

			msec = 900 + msec;
			Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
			data->lastperiod = msec;
		}

		return;
	}

	w *= delaybox->parent->w - 10;
	w /= (-delayinc * 1000);

	delaybox->w = w;

	Box_Repaint(delaybox->parent);
}

void ChessBoard_Animate(struct Box_s *board, void *userdata)
{
	struct chessboarddata_s *bdata = board->boxdata;
	struct namedlist_s *entry;
	unsigned int tm;

	tm = timeGetTime() - bdata->animstart;
	if (tm > 200)
	{
		tm = 200;
	}

	entry = bdata->animpieces;
	while (entry)
	{
		struct Box_s *piece;
		struct chesspieceboxdata_s *pdata;
		int destx, desty;

		piece = entry->data;
		pdata = piece->boxdata;

		if (pdata->newx == -1)
		{
			if (pdata->x <= 4)
			{
				destx = pdata->x - 1;
			}
			else
			{
				destx = pdata->x + 1;
			}

			if (pdata->y <= 4)
			{
				desty = pdata->y - 1;
			}
			else
			{
				desty = pdata->y + 1;
			}
		}
		else
		{
			destx = pdata->newx;
			desty = pdata->newy;
		}

		piece->x = ((pdata->x - 1) * board->w * (200 - tm) + (destx - 1) * board->w * tm) / 200 / 8;
		piece->y = ((pdata->y - 1) * board->w * (200 - tm) + (desty - 1) * board->w * tm) / 200 / 8;

		if (pdata->newx == -1)
		{
			piece->w = board->w / 8 * (tm + 70) / 70;
			piece->h = board->w / 8 * (tm + 70) / 70;
		}

		entry = entry->next;
	}

	if (tm == 200)
	{
		entry = bdata->animpieces;
		while (entry)
		{
			struct Box_s *piece;
			struct chesspieceboxdata_s *pdata;
			
			piece = entry->data;
			pdata = piece->boxdata;

			pdata->x = pdata->newx;
			pdata->y = pdata->newy;
			entry = entry->next;
		}
		Box_RemoveTimedFunc(board, ChessBoard_Animate, 17);
		ChessBoard_UpdateBoard(board);
		return;
	}

	Box_Repaint(board);
}

void ChessBox_AnimateMove(struct Box_s *chessbox, char *move)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;
	int oldx, oldy, newx, newy;
	struct Box_s *piece;
	struct chesspieceboxdata_s *pdata;
	char *space  = strchr(move, ' ');
	int found = 0;
	int len = (int)(strlen(move));

	char *letters = "abcdefgh";
	char *numbers = "87654321";

	if (space)
	{
		len = (int)(space - move);
	}

	if (len < 3)
	{
		ChessBoard_UpdateBoard(board);
		return;
	}

	oldx = (int)(strchr(letters, move[0]) - letters) + 1;
	oldy = (int)(strchr(numbers, move[1]) - numbers) + 1;
	if (len < 4)
	{
		newx = -1;
		newy = -1;
	}
	else
	{
		newx = (int)(strchr(letters, move[2]) - letters) + 1;
		newy = (int)(strchr(numbers, move[3]) - numbers) + 1;
	}

	if (bdata->showrotated)
	{
		oldx = 9 - oldx;
		oldy = 9 - oldy;
		if (len >= 3)
		{
			newx = 9 - newx;
			newy = 9 - newy;
		}
	}

	piece = board->child;
	while (piece && !found)
	{
		if (piece->boxtypeid == CHESSPIECEBOX_ID)
		{
			pdata = piece->boxdata;
			if (pdata->x == oldx && pdata->y == oldy)
			{
				found = 1;
			}
			else
			{
				piece = piece->sibling;
			}
		}
		else
		{
			piece = piece->sibling;
		}
	}

	if (!found)
	{
		ChessBoard_UpdateBoard(board);
		return;
	}

	pdata->newx = newx;
	pdata->newy = newy;
	
	bdata->animstart = timeGetTime();
	NamedList_Add(&(bdata->animpieces), NULL, piece, NULL);

	Box_RemoveTimedFunc(board, ChessBoard_Animate, (void *)17);
	Box_AddTimedFunc(board, ChessBoard_Animate, NULL, (void *)17);
}

void ChessBox_ActivateClock(struct Box_s *chessbox, enum ChessTurn turn)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct timecontrol_s *tc = NULL;

	if (turn == CT_WHITE)
	{
		if (data && data->info && data->info->timecontrol)
		{
			tc = data->info->timecontrol;
		}
	}
	else
	{
		if (data && data->info)
		{
			if (data->info->blacktimecontrol)
			{
				tc = data->info->blacktimecontrol;
			}
			else if (data->info->timecontrol)
			{
				tc = data->info->timecontrol;
			}
		}
	}

	Box_RemoveTimedFunc(data->whitedelay, TimeDelay_ShrinkDelay, 100);
	Box_RemoveTimedFunc(data->blackdelay, TimeDelay_ShrinkDelay, 100);

	if (tc && tc->delayinc < 0)
	{
		data->whiteclock->fgcol = (turn == CT_WHITE) ? NORMAL_WHITE_CLOCK_COLOR : FADED_WHITE_CLOCK_COLOR;
		data->blackclock->fgcol = (turn == CT_BLACK) ? NORMAL_BLACK_CLOCK_COLOR : FADED_BLACK_CLOCK_COLOR;
		data->whitedelay->w =     (turn == CT_WHITE) ? data->whitetimebox->w - 20 : 0;
		data->blackdelay->w =     (turn == CT_BLACK) ? data->blacktimebox->w - 20 : 0;
		data->whitedot->img =     (turn == CT_WHITE) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->blackdot->img =     (turn == CT_BLACK) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		Box_Repaint(data->whitetimebox);
		Box_Repaint(data->blacktimebox);
		Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);
		data->lastperiod = 0;
		data->lastmovetick = timeGetTime();
		Box_AddTimedFunc   (turn == CT_WHITE ? data->whitedelay : data->blackdelay, TimeDelay_ShrinkDelay, NULL, 100);
	}
	else
	{
		int msec;

		data->whiteclock->fgcol = (turn == CT_WHITE) ? NORMAL_WHITE_CLOCK_COLOR : FADED_WHITE_CLOCK_COLOR;
		data->blackclock->fgcol = (turn == CT_BLACK) ? NORMAL_BLACK_CLOCK_COLOR : FADED_BLACK_CLOCK_COLOR;
		data->whitedot->img =     (turn == CT_WHITE) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->blackdot->img =     (turn == CT_BLACK) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->whitedelay->w =     0;
		data->blackdelay->w =     0;

		if (turn == CT_WHITE)
		{
			data->whitetime = ChessBox_CalcClock(chessbox, 1);
			data->tickwhiteset = timeGetTime();
		}
		else 
		{
			data->blacktime = ChessBox_CalcClock(chessbox, 0);
			data->tickblackset = timeGetTime();
		}
		Log_Write(0, "ActivateClock() ticking updatetime\n");

		if (turn == CT_WHITE)
		{
			msec = ChessBox_SetVisibleClock(chessbox, data->whitetime, 1);
		}
		else if (turn == CT_BLACK)
		{
			msec = ChessBox_SetVisibleClock(chessbox, data->blacktime, 0);
		}
		else
		{
			msec = 0;
		}

		Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);

		if (msec > 0)
		{
			if (msec <= 100)
			{
				msec = 900 + msec;
			}
			else
			{
				msec = msec - 100;
			}
			Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
			data->lastperiod = msec;
		}
		else
		{
			if (msec <= -900)
			{
				msec = 900 + (1000 + msec);
			}
			else
			{
				msec = 900 + msec;
			}

			msec = 900 + msec;
			Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
			data->lastperiod = msec;
		}
	}
}

void ChessBox_HighlightMove(struct Box_s *chessbox, char *submove)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	char *letters = "abcdefgh";
	char *numbers = "87654321";

	while (submove && *submove)
	{
		int length;
		char *space;
		int x, y;
		
		length = (int)strlen(submove);
		space = strchr(submove, ' ');

		if (space && length > space - submove)
		{
			length = (int)(space - submove);
		}

		x = (int)(strchr(letters, submove[0]) - letters) + 1;
		y = (int)(strchr(numbers, submove[1]) - numbers) + 1;

		ChessBoard_CreateHighlight(board, x, y, 0, bdata->showrotated, data->gameover, CP_EMPTY, 0);

		if (length > 3)
		{
			x = (int)(strchr(letters, submove[2]) - letters) + 1;
			y = (int)(strchr(numbers, submove[3]) - numbers) + 1;

			ChessBoard_CreateHighlight(board, x, y, 0, bdata->showrotated, data->gameover, CP_EMPTY, 0);
		}

		while (space && *space == ' ')
		{
			space++;
		}

		submove = space;
	}
}

void ChessBox_SwitchClockToNonLocalPlayer(struct Box_s *chessbox, unsigned int lag)
{
	struct chessboxdata_s *data = chessbox->boxdata;


	Log_Write(0, "SCTNLP %s\n", data->lastmove);

	data->movearbiterlag = lag;
	data->arbitermovelag = timeGetTime();

	/* forget this, it just causes more skew */
	return;

	if (data->whitelocal)
	{
		data->whitetime = ChessBox_CalcClock(chessbox, 1);
		data->tickwhiteset = 0;
		data->blacktime = ChessBox_CalcClock(chessbox, 0);
		data->tickblackset = timeGetTime();
		Log_Write(0, "Switching clock, I think white time is %fs (minus %dms lag)\n", data->whitetime, lag);
		data->turn = CT_BLACK;
		ChessBox_ActivateClock(chessbox, CT_BLACK);
		ChessBoard_ClearHighlights(data->chessboard);
		ChessBox_HighlightMove(chessbox, data->lastmove);
		Box_Repaint(chessbox);
		/*data->postswitch = 1;*/
	}
	else if (data->blacklocal)
	{
		
		data->blacktime = ChessBox_CalcClock(chessbox, 0);
		data->tickblackset = 0;
		data->whitetime = ChessBox_CalcClock(chessbox, 1);
		data->tickwhiteset = timeGetTime();
		Log_Write(0, "Switching clock, I think black time is %fs (minus %dms lag)\n", ChessBox_CalcClock(chessbox, 0), lag);
		data->turn = CT_WHITE;
		ChessBox_ActivateClock(chessbox, CT_WHITE);
		ChessBoard_ClearHighlights(data->chessboard);
		ChessBox_HighlightMove(chessbox, data->lastmove);
		Box_Repaint(chessbox);
		/*data->postswitch = 2;*/
	}
}

void ChessBox_ParseGameMove(struct Box_s *chessbox, char *move, char *annotation, int numtakebacks, int ply)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;
	char *space, *submove;
	int oldx, oldy;
	enum ChessPiece piece;
	int length;
	int capture = 0;

	char *letters = "abcdefgh";
	char *numbers = "87654321";
	char *pieces = "oPNBRQKpnbrqk";

	if (!(data->gotstate) || (data->lastply != 0 && ply != 0 && numtakebacks == 0 && data->lastply != ply - 1))
	{
		data->queuedmove = strdup(move);
		data->queuedmoveannotation = strdup(annotation);
		data->queuednumtakebacks = numtakebacks;
		data->queuedply = ply;
		return;
	}

	if (data->ignorenextmove)
	{
		data->ignorenextmove = 0;
		return;
	}

	length = (int)strlen(move);
	space = strchr(move, ' ');

	if (space && length > space - move)
	{
		length = (int)(space - move);
	}

	oldx = (int)(strchr(letters, move[0]) - letters);
	oldy = (int)(strchr(numbers, move[1]) - numbers);
	piece = bdata->board[oldx][oldy];

	/* Ignore moves that start from an empty square and aren't piece placement */
	if (piece == CP_EMPTY && length != 3)
	{
		return;
	}

	data->ishidinglag = 0;

	ChessBox_AddMoveToList(chessbox, move, annotation, numtakebacks);

	capture = ExecuteMove(bdata->board, move, 0, &(data->castling), &(data->enpassanttarget));

	bdata->movefinishsound = 0;
	if (length == 3)
	{
		piece = ConvertCharToPiece(move[2]);
		if (piece <= CP_WKING && (ply % 2) == 0)
		{
			piece += 6;
		}
		else if (piece >= CP_BPAWN && (ply % 2) == 1)
		{
			piece -= 6;
		}
	}
	if (piece != CP_EMPTY)
	{
		/* no sound if lag compensation is on */
		if ((piece <= CP_WKING && (!Model_GetOption(OPTION_ENABLELAGHIDING) || !data->whitelocal))
		   || (piece > CP_WKING && (!Model_GetOption(OPTION_ENABLELAGHIDING) || !data->blacklocal)))
		{
			bdata->movefinishsound = 1;
			if (capture && length != 3)
			{
				bdata->movefinishsound = 2;
			}
		}
		/* queue up an explosion if this is atomic */
		if (!Model_GetOption(OPTION_DISABLEGAMESOUNDS))
		{
			if (capture == 2 && length != 3)
			{
				Audio_PlayWav("sounds/boom.wav");
			}
		}
	}

	ChessBoard_ClearHighlights(board);

	submove = move;
	while (submove && *submove)
	{
		int length;
		char *space;
		int x, y;
		
		length = (int)strlen(submove);
		space = strchr(submove, ' ');

		if (space && length > space - submove)
		{
			length = (int)(space - submove);
		}

		if (!Model_GetOption(OPTION_DISABLEANIMATION) && !numtakebacks)
		{
			ChessBox_AnimateMove(chessbox, submove);
		}

		if (!numtakebacks)
		{
			x = (int)(strchr(letters, submove[0]) - letters) + 1;
			y = (int)(strchr(numbers, submove[1]) - numbers) + 1;

			ChessBoard_CreateHighlight(board, x, y, 0, bdata->showrotated, data->gameover, CP_EMPTY, 0);

			if (length > 3)
			{
				x = (int)(strchr(letters, submove[2]) - letters) + 1;
				y = (int)(strchr(numbers, submove[3]) - numbers) + 1;

				ChessBoard_CreateHighlight(board, x, y, 0, bdata->showrotated, data->gameover, CP_EMPTY, 0);
			}
		}

		while (space && *space == ' ')
		{
			space++;
		}

		submove = space;
	}

	if (numtakebacks)
	{
		struct namedlist_s *current = data->movelist;
		int nummoves = data->movenum - numtakebacks;

		while (current && nummoves)
		{
			current = current->next;
			nummoves--;
		}

		if (current)
		{
			ChessBox_HighlightMove(chessbox, current->name);
		}
	}

	if (Model_GetOption(OPTION_DISABLEANIMATION) || numtakebacks)
	{
		ChessBoard_UpdateBoard(board);
	}

	if (data->roomjid)
	{
		if (piece != CP_EMPTY && ((!IsPieceWhite(piece) && data->whitelocal) || (IsPieceWhite(piece) && data->blacklocal)) && annotation && stricmp(annotation, "check") == 0)
		{
			Text_SetText(data->message, _("You are in check."));
		}
		else
		{
			ChessBox_ShowScore(chessbox);
		}
	}

	if (numtakebacks)
	{
		if (numtakebacks % 2)
		{
			if (data->turn == CT_BLACK)
			{
				data->turn = CT_WHITE;
				ChessBox_ActivateClock(chessbox, data->turn);
			}
			else
			{
				data->turn = CT_BLACK;
				ChessBox_ActivateClock(chessbox, data->turn);
			}
		}
	}
	else if (piece != CP_EMPTY)
	{
		if (piece <= CP_WKING)
		{
			data->turn = CT_BLACK;
			if (data->blacklocal && !data->roomjid)
			{
				data->whiteclock->fgcol = FADED_WHITE_CLOCK_COLOR;
				data->blackclock->fgcol = NORMAL_BLACK_CLOCK_COLOR;
				data->whitedot->img = ImageMgr_GetImage("inactive.png");
				data->blackdot->img = ImageMgr_GetImage("active.png");
				Box_Repaint(data->whiteclock);
				Box_Repaint(data->blackclock);
				Button_SetDisabledState(data->sendmovebutton, 1);
				/*
				data->sendmovebutton->flags  |= BOX_VISIBLE;
				data->clearmovebutton->flags |= BOX_VISIBLE;*/
				Text_SetText(data->message, _("It's your turn.  Make a move, then press send move."));
				Box_Repaint(data->message);
				Box_Repaint(data->gameinfobox);
			}
			else if (!Model_GetOption(OPTION_ENABLELAGHIDING) || !data->whitelocal) /* Don't tick if lag compensation is on, already ticking */
			{
				ChessBox_ActivateClock(chessbox, data->turn);
			}
		}
		else
		{
			data->turn = CT_WHITE;
			if (data->whitelocal && !data->roomjid)
			{
				data->whiteclock->fgcol = NORMAL_WHITE_CLOCK_COLOR;
				data->blackclock->fgcol = FADED_BLACK_CLOCK_COLOR;
				data->whitedot->img = ImageMgr_GetImage("active.png");
				data->blackdot->img = ImageMgr_GetImage("inactive.png");
				Box_Repaint(data->whiteclock);
				Box_Repaint(data->blackclock);
				Button_SetDisabledState(data->sendmovebutton, 1);
				/*
				data->sendmovebutton->flags  |= BOX_VISIBLE;
				data->clearmovebutton->flags |= BOX_VISIBLE;*/
				Text_SetText(data->message, _("It's your turn.  Make a move, then press send move."));
				Box_Repaint(data->message);
				Box_Repaint(data->gameinfobox);
			}
			else if (!Model_GetOption(OPTION_ENABLELAGHIDING) || !data->blacklocal) /* Don't tick if lag compensation is on, already ticking */
			{
				ChessBox_ActivateClock(chessbox, data->turn);
			}
		}	
	}

	if (numtakebacks)
	{
		data->lastply -= numtakebacks;
		if (data->lastply < 0)
		{
			data->lastply = 0;
		}
	}
	else
	{
		data->lastply = ply;
	}

	Box_Repaint(data->whiteclock);
	Box_Repaint(data->blackclock);

	if (data->queuedply == ply + 1)
	{
		ChessBox_ParseGameMove(chessbox, data->queuedmove, data->queuedmoveannotation, data->queuednumtakebacks, data->queuedply);
	}

	if (data->movenum < 4)
	{
		ChessBox_UpdateMenu(chessbox);
		Box_Repaint(chessbox);
	}
}

void ChessBox_EnableAbort(struct Box_s *chessbox, void *dummy)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	Box_RemoveTimedFunc(chessbox, ChessBox_EnableAbort, 15000);

	data->canabort = 1;

	ChessBox_UpdateMenu(chessbox);
	Box_Repaint(chessbox);
}

void ChessBox_SetState(struct Box_s *chessbox, char *initialstate, char *state)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;
	enum ChessTurn turn;

	data->initialfen = strdup(initialstate);

	ChessLogic_ParseFEN(state, bdata->board, &turn, &(data->castling), &(data->enpassanttarget));

	if (turn == CT_WHITE)
	{
		data->turn = CT_WHITE;
		data->whiteclock->fgcol = (turn == CT_WHITE) ? NORMAL_WHITE_CLOCK_COLOR : FADED_WHITE_CLOCK_COLOR;
		data->blackclock->fgcol = (turn == CT_BLACK) ? NORMAL_BLACK_CLOCK_COLOR : FADED_BLACK_CLOCK_COLOR;
		data->whitedot->img =     (turn == CT_WHITE) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->blackdot->img =     (turn == CT_BLACK) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		Box_Repaint(data->whiteclock);
		Box_Repaint(data->blackclock);
		if (!data->roomjid)
		{
			if (data->whitelocal)
			{
				Button_SetDisabledState(data->sendmovebutton, 1);
				/*
				data->sendmovebutton->flags  |= BOX_VISIBLE;
				data->clearmovebutton->flags |= BOX_VISIBLE;*/
				Text_SetText(data->message, _("It's your turn.  Make a move, then press send move."));
				Box_Repaint(data->message);
				Box_Repaint(data->gameinfobox);
			}
			else if (data->blacklocal)
			{
				Button_SetDisabledState(data->sendmovebutton, 1);
				/*
				data->sendmovebutton->flags  &= ~BOX_VISIBLE;
				data->clearmovebutton->flags &= ~BOX_VISIBLE;*/
				Text_SetText(data->message, _("It's your opponent's turn.  You'll be notified when it's complete."));
				Box_Repaint(data->message);
				Box_Repaint(data->gameinfobox);
			}
		}
	}
	else
	{
		data->turn = CT_BLACK;
		data->whiteclock->fgcol = (turn == CT_WHITE) ? NORMAL_WHITE_CLOCK_COLOR : FADED_WHITE_CLOCK_COLOR;
		data->blackclock->fgcol = (turn == CT_BLACK) ? NORMAL_BLACK_CLOCK_COLOR : FADED_BLACK_CLOCK_COLOR;
		data->whitedot->img =     (turn == CT_WHITE) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->blackdot->img =     (turn == CT_BLACK) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		Box_Repaint(data->whiteclock);
		Box_Repaint(data->blackclock);
		if (!data->roomjid)
		{
			if (data->whitelocal)
			{
				Button_SetDisabledState(data->sendmovebutton, 1);
				/*
				data->sendmovebutton->flags  &= ~BOX_VISIBLE;
				data->clearmovebutton->flags &= ~BOX_VISIBLE;*/
				Text_SetText(data->message, _("It's your opponent's turn.  You'll be notified when it's complete."));
				Box_Repaint(data->message);
				Box_Repaint(data->gameinfobox);
			}
			else if (data->blacklocal)
			{
				Button_SetDisabledState(data->sendmovebutton, 1);
				/*
				data->sendmovebutton->flags  |= BOX_VISIBLE;
				data->clearmovebutton->flags |= BOX_VISIBLE;*/
				Text_SetText(data->message, _("It's your turn.  Make a move, then press send move."));
				Box_Repaint(data->message);
				Box_Repaint(data->gameinfobox);
			}
		}
	}

	ChessBoard_UpdateBoard(board);

	if (data->roomjid)
	{
		ChessBox_ShowScore(chessbox);
	}

	data->gotstate = 1;

	if (data->queuedmove)
	{
		ChessBox_ParseGameMove(chessbox, data->queuedmove, data->queuedmoveannotation, data->queuednumtakebacks, data->queuedply);
		free(data->queuedmove);
		free(data->queuedmoveannotation);
		data->queuedmove = NULL;
	}

	if (data->roomjid)
	{
		/*ChessBox_ActivateClock(chessbox, data->turn);*/
	}

	Box_AddTimedFunc(chessbox, ChessBox_EnableAbort, NULL, 15000);
}

float ChessBox_CalcClock(struct Box_s *chessbox, int white)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	unsigned int tick = timeGetTime();

	if (white)
	{
		unsigned int diff = data->tickwhiteset ? (tick - data->tickwhiteset) : 0;
		Log_Write(0, "ChessBox_CalcClock white %f - %dms\n", data->whitetime, diff);
		return data->whitetime - diff / 1000.0f;
	}
	else
	{
		unsigned int diff = data->tickblackset ? (tick - data->tickblackset) : 0;
		Log_Write(0, "ChessBox_CalcClock black %f - %dms\n", data->blacktime, diff);
		return data->blacktime - diff / 1000.0f;
	}
}

int ChessBox_SetVisibleClock(struct Box_s *chessbox, float fsec, int iswhite)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	int msec;
	char showtime[256];

	Info_SecsToText3(fsec, showtime, &msec, data->roomjid == NULL, 100);

	if (fsec < 0)
	{
		strcpy(showtime, "00:00");
	}

	Log_Write(0, "Showing time %s\n", showtime);

	if (iswhite)
	{
		Box_SetText(data->whiteclock, showtime);
		Box_Repaint(data->whiteclock);
	}
	else
	{
		Box_SetText(data->blackclock, showtime);
		Box_Repaint(data->blackclock);
	}

	return msec * (fsec < 0 ? -1 : 1);
}

void ChessBox_UpdateTime(struct Box_s *chessbox, void *userdata)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	enum ChessTurn turn = data->turn;
	int msec = 1000;

	Log_Write(0, "ChessBox_UpdateTime() ishidinglag%d\n", data->ishidinglag);

	if (data->ishidinglag)
	{
		if (turn == CT_WHITE)
		{
			turn = CT_BLACK;
		}
		else if (turn == CT_BLACK)
		{
			turn = CT_WHITE;
		}
	}

	if (turn == CT_WHITE)
	{
		float fsec = ChessBox_CalcClock(chessbox, 1);
		data->whiteclock->fgcol = (turn == CT_WHITE) ? NORMAL_WHITE_CLOCK_COLOR : FADED_WHITE_CLOCK_COLOR;
		data->blackclock->fgcol = (turn == CT_BLACK) ? NORMAL_BLACK_CLOCK_COLOR : FADED_BLACK_CLOCK_COLOR;
		data->whitedot->img =     (turn == CT_WHITE) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->blackdot->img =     (turn == CT_BLACK) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		msec = ChessBox_SetVisibleClock(chessbox, fsec, TRUE);
		if (fsec <= 0 && data->blacklocal && !Model_GetOption(OPTION_NOGAMEAUTOFLAG) && !data->sentflag && !data->ishidinglag) /* autoflag */
		{
			Ctrl_SendGameFlag(data->gameid, data->roomjid == NULL);
			data->sentflag = 1;
		}
	}
	else if (turn == CT_BLACK)
	{
		float fsec = ChessBox_CalcClock(chessbox, 0);
		data->whiteclock->fgcol = (turn == CT_WHITE) ? NORMAL_WHITE_CLOCK_COLOR : FADED_WHITE_CLOCK_COLOR;
		data->blackclock->fgcol = (turn == CT_BLACK) ? NORMAL_BLACK_CLOCK_COLOR : FADED_BLACK_CLOCK_COLOR;
		data->whitedot->img =     (turn == CT_WHITE) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		data->blackdot->img =     (turn == CT_BLACK) ? ImageMgr_GetImage("active.png") : ImageMgr_GetImage("inactive.png");
		msec = ChessBox_SetVisibleClock(chessbox, fsec, FALSE);
		if (fsec <= 0 && data->whitelocal && !Model_GetOption(OPTION_NOGAMEAUTOFLAG) && !data->sentflag && !data->ishidinglag) /* autoflag */
		{
			Ctrl_SendGameFlag(data->gameid, data->roomjid == NULL);
			data->sentflag = 1;
		}
	}

	Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);

	if (msec > 0)
	{
		if (msec <= 100)
		{
			msec = 900 + msec;
		}
		else
		{
			msec = msec - 100;
		}
		Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
		data->lastperiod = msec;
	}
	else
	{
		if (msec <= -900)
		{
			msec = 900 + (1000 + msec);
		}
		else
		{
			msec = 900 + msec;
		}

		msec = 900 + msec;
		Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
		data->lastperiod = msec;
	}
}

void ChessBox_UpdateControlMsg(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct timecontrol_s *tc = NULL;

	if (!data->info)
	{
		return;
	}

	if (data->info->timecontrol)
	{
		tc = data->info->timecontrol;
	}

	if (tc)
	{
		int moves = tc->controlarray[data->whitecurrentcontrol * 2 + 1];
		int numcontrols = tc->controlarray[0];

		if (!data->roomjid)
		{
			char txt[256];
			int result = moves - data->whitemovenum % moves;

			if (result == 1)
			{
				sprintf(txt, _("left to make 1 move"));
			}
			else
			{
				char movestxt[5];

				sprintf(movestxt, "%d", result);

				i18n_stringsub(txt, 256, _("left to make %1 moves"), movestxt);
			}

			Box_SetText(data->whitetimeinfo, txt);
		}
		else if (moves == -1)
		{
			Box_SetText(data->whitetimeinfo, _("left in game"));
		}
		else
		{
			int i;
			int movestil = 0;
			char txt[256];
			int result = -data->whitemovenum;

			for (i = 0; i <= data->whitecurrentcontrol; i++)
			{
				result += tc->controlarray[i * 2 + 1];
			}

			if (result == 0 && i < numcontrols && tc->controlarray[i * 2 + 1] != -1)
			{
				result += tc->controlarray[i * 2 + 1];
			}

			if (result == 0)
			{
				sprintf(txt, _("left in game"));
			}
			else if (result == 1)
			{
                                sprintf(txt, _("1 move until next control"));
			}
			else
			{
				char movestxt[5];

				sprintf(movestxt, "%d", result);

                                i18n_stringsub(txt, 256, _("%1 moves until next control"), movestxt);
			}

			Box_SetText(data->whitetimeinfo, txt);
		}
		Box_Repaint(data->whitetimeinfo);
	}

	if (data->info->blacktimecontrol)
	{
		tc = data->info->blacktimecontrol;
	}
	else if (data->info->timecontrol)
	{
		tc = data->info->timecontrol;
	}

	if (tc)
	{
		int moves = tc->controlarray[data->blackcurrentcontrol * 2 + 1];
		int numcontrols = tc->controlarray[0];

		if (!data->roomjid)
		{
			char txt[256];
			int result = moves - data->blackmovenum % moves;

			if (result == 1)
			{
				sprintf(txt, _("left to make 1 move"));
			}
			else
			{
                                char movestxt[5];

				sprintf(movestxt, "%d", result);

				i18n_stringsub(txt, 256, _("left to make %1 moves"), movestxt);
			}

			Box_SetText(data->blacktimeinfo, txt);
		}
		else if (moves == -1)
		{
			Box_SetText(data->blacktimeinfo, _("left in game"));
		}
		else
		{
			int i;
			int movestil = 0;
			char txt[256];
			int result = -data->blackmovenum;

			for (i = 0; i <= data->blackcurrentcontrol; i++)
			{
				result += tc->controlarray[i * 2 + 1];
			}

			if (result == 0 && i < numcontrols && tc->controlarray[i * 2 + 1] != -1)
			{
				result += tc->controlarray[i * 2 + 1];
			}

			if (result == 0)
			{
				sprintf(txt, _("left in game"));
			}
			else if (result == 1)
			{
                                sprintf(txt, _("1 move until next control"));
			}
			else
			{
                                char movestxt[5];

				sprintf(movestxt, "%d", result);

                                i18n_stringsub(txt, 256, _("%1 moves until next control"), movestxt);
			}

			Box_SetText(data->blacktimeinfo, txt);
		}
		Box_Repaint(data->blacktimeinfo);
	}
}


void ChessBox_SyncClock(struct Box_s *chessbox, char *time, char *side, char *control, int tick)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	float fsec;
	int msec;
	int iswhite = strcmp(side, "white") == 0;
	sscanf(time, "%f", &fsec);

	if (data->arbitermovelag)
	{
		estimatedarbiterlag = (float)(data->movearbiterlag + (timeGetTime() - data->arbitermovelag)) / 1000.0f;
		Log_Write(0, "movearbiterlag %dms arbitermovelag %dms\n", data->movearbiterlag, timeGetTime() - data->arbitermovelag);
		Log_Write(0, "Estimated arbiter lag: %fs\n", estimatedarbiterlag);
		Model_AddPing(data->movearbiterlag + (timeGetTime() - data->arbitermovelag));
		data->arbitermovelag = 0;
	}

	/*if (tick || (!tick && ((!iswhite && data->whitelocal) || (iswhite && data->blacklocal))))*/
	if (tick || (!tick && ((iswhite && data->turn == CT_BLACK) || (!iswhite && data->turn == CT_WHITE))))
	{
		fsec -= estimatedarbiterlag;
	}

	if (iswhite)
	{
		float thinktime = ChessBox_CalcClock(chessbox, 1);
		Log_Write(0, "Server says white clock is %fs, I think it's %fs, skew is %fs\n", fsec, thinktime, fsec - thinktime);
/*
		if (data->postswitch == 1)
		{
			FILE *fp;
			fp = fopen("skew", "a");

			fprintf(fp, "%f\n", fsec - thinktime);

			fclose(fp);

			fp = fopen("skew2", "a");

			fprintf(fp, "\n");

			fclose(fp);
			data->postswitch = 0;
		}
		else
		{
			FILE *fp;
			fp = fopen("skew", "a");

			fprintf(fp, "\n");

			fclose(fp);

			fp = fopen("skew2", "a");

			fprintf(fp, "%f\n", fsec - thinktime);

			fclose(fp);
		}
*/
		/*
		if (data->arbitermovelag)
		{
			FILE *fp;

			fp = fopen("amlag", "a");

			fprintf(fp, "%f\n", (timeGetTime() - data->arbitermovelag) / 1000.0f);

			fclose(fp);

			fp = fopen("malag", "a");

			fprintf(fp, "%f\n", (data->movearbiterlag) / 1000.0f);

			fclose(fp);

			data->arbitermovelag = 0;
		}
		*/

		msec = ChessBox_SetVisibleClock(chessbox, fsec, TRUE);
		data->whitetime = fsec;

		if (tick && !data->gameover)
		{
			data->whiteclock->fgcol = NORMAL_WHITE_CLOCK_COLOR;
			data->whitedot->img = ImageMgr_GetImage("active.png");
			data->turn = CT_WHITE;
		}

		if (control)
		{
			sscanf(control, "%d", &data->whitecurrentcontrol);
		}

		if (fsec <= 0 && data->blacklocal && !Model_GetOption(OPTION_NOGAMEAUTOFLAG) && !data->sentflag) /* autoflag */
		{
			Ctrl_SendGameFlag(data->gameid, data->roomjid == NULL);
			data->sentflag = 1;
		}
	}
	else
	{
		float thinktime = ChessBox_CalcClock(chessbox, 0);

		Log_Write(0, "Server says black clock is %fs, I think it's %fs, skew is %fs\n", fsec, thinktime, fsec - thinktime);
/*
		if (data->postswitch == 2)
		{
			FILE *fp;
			fp = fopen("skew", "a");

			fprintf(fp, "%f\n", fsec - thinktime);

			fclose(fp);

			fp = fopen("skew2", "a");

			fprintf(fp, "\n");

			fclose(fp);
			data->postswitch = 0;
		}
		else
		{
			FILE *fp;
			fp = fopen("skew", "a");

			fprintf(fp, "\n");

			fclose(fp);

			fp = fopen("skew2", "a");

			fprintf(fp, "%f\n", fsec - thinktime);

			fclose(fp);
		}
*/
		/*
		if (data->arbitermovelag)
		{
			FILE *fp;

			fp = fopen("amlag", "a");

			fprintf(fp, "%f\n", (timeGetTime() - data->arbitermovelag) / 1000.0f);

			fclose(fp);

			fp = fopen("malag", "a");

			fprintf(fp, "%f\n", (data->movearbiterlag) / 1000.0f);

			fclose(fp);

			data->arbitermovelag = 0;
		}
*/
		msec = ChessBox_SetVisibleClock(chessbox, fsec, FALSE);
		data->blacktime = fsec;

		if (tick && !data->gameover)
		{
			data->blackclock->fgcol = NORMAL_BLACK_CLOCK_COLOR;
			data->turn = CT_BLACK;
		}

		if (control)
		{
			sscanf(control, "%d", &data->blackcurrentcontrol);
		}

		if (fsec <= 0 && data->whitelocal && !Model_GetOption(OPTION_NOGAMEAUTOFLAG) && !data->sentflag) /* autoflag */
		{
			Ctrl_SendGameFlag(data->gameid, data->roomjid == NULL);
			data->sentflag = 1;
		}
	}

	if (!data->gameover)
	{
		if (tick)
		{
			data->whitedelay->w = 0;
			data->blackdelay->w = 0;
			Box_RemoveTimedFunc(data->whitedelay, TimeDelay_ShrinkDelay, 100);
			Box_RemoveTimedFunc(data->blackdelay, TimeDelay_ShrinkDelay, 100);

			Log_Write(0, "SyncClock() ticking updatetime\n");
			Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);

			if (msec > 0)
			{
				if (msec <= 100)
				{
					msec = 900 + msec;
				}
				else
				{
					msec = msec - 100;
				}
				Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
				data->lastperiod = msec;
			}
			else
			{
				if (msec <= -900)
				{
					msec = 900 + (1000 + msec);
				}
				else
				{
					msec = 900 + msec;
				}

				msec = 900 + msec;
				Box_AddTimedFunc(chessbox, ChessBox_UpdateTime, NULL, msec);
				data->lastperiod = msec;
			}

			if (iswhite)
			{
				data->tickwhiteset = timeGetTime();
			}
			else
			{
				data->tickblackset = timeGetTime();
			}
		}
		else
		{
			if (iswhite)
			{
				Log_Write(0, "Stopping white clock\n");
				data->tickwhiteset = 0;
			}
			else
			{
				Log_Write(0, "Stopping black clock\n");
				data->tickblackset = 0;
			}
		}
	}
	Box_Repaint(data->whitetimebox);
	Box_Repaint(data->blacktimebox);

	ChessBox_UpdateControlMsg(chessbox);
}

void ChessBox_AddTimeNonLocal(struct Box_s *chessbox, int itime)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char *side;
	char control[80];
	char time[256];

	if (data->whitelocal)
	{
		side = "black";
		sprintf(control, "%d", data->blackcurrentcontrol);
		sprintf(time, "%f", data->blacktime + (float)itime);

	}
	else if (data->blacklocal)
	{
		side = "white";
		sprintf(control, "%d", data->whitecurrentcontrol);
		sprintf(time, "%f", data->whitetime + (float)itime);
	}
	else
	{
		return;
	}

	ChessBox_SyncClock(chessbox, time, side, control, 0);
}

void ChessBox_HandleFlagError(struct Box_s *chessbox, char *time)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char *side;
	char control[80];

	if (data->whitelocal)
	{
		side = "black";
		sprintf(control, "%d", data->blackcurrentcontrol);
	}
	else if (data->blacklocal)
	{
		side = "white";
		sprintf(control, "%d", data->whitecurrentcontrol);
	}
	else
	{
		/* shouldn't happen, but just in case */
		return;
	}
		
	ChessBox_SyncClock(chessbox, time, side, control, 0);
	data->sentflag = 0;
}


/* FIXME: use all controls to get total time */

void ChessBox_SetClockControl(struct Box_s *chessbox, char *side, int *controlarray)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	
	int *p = controlarray + 1;
	int numcontrols = *controlarray;
	int time, moves;
	moves = *p++;
	time = *p++;

	if (strcmp(side, "white") == 0)
	{
		ChessBox_SetVisibleClock(chessbox, (float)time, TRUE);
		if (data->whitetime == 0.0f)
		{
			data->whitetime = (float)time;
		}
		data->whiteclock->fgcol = FADED_WHITE_CLOCK_COLOR;
		data->whitedot->img = ImageMgr_GetImage("inactive.png");
		Box_Repaint(data->whiteclock);
		Box_Repaint(data->whitedot);
	}
	else
	{
		ChessBox_SetVisibleClock(chessbox, (float)time, FALSE);
		if (data->blacktime == 0.0f)
		{
			data->blacktime = (float)time;
		}
		data->blackclock->fgcol = FADED_BLACK_CLOCK_COLOR;
		data->blackdot->img = ImageMgr_GetImage("inactive.png");
		Box_Repaint(data->blackclock);
		Box_Repaint(data->blackdot);
	}
}

void ChessBox_OnGameMenu(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	int x, y;

	Box_GetScreenCoords(pbox, &x, &y);
	x += pbox->w / 2;
	y += pbox->h / 2;

	Menu_PopupGameMenu(pbox, data->gameid, data->roomjid, data->whitelocal || data->blacklocal, data->gameover, x, y);
}

void ChessBoxLists_OnSizeWidth(struct Box_s *pbox, int dw);

void ChessBox_OnChatArea(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	/*data->gamechatonbottom = !data->gamechatonbottom;*/

	/*Model_SetOption(OPTION_GAMECHATONBOTTOM, 2, NULL);*/

	/*ChessBox_ResetChatPositions(chessbox);*/

	if (data->hidechat)
	{
		int neww;
		data->hidechat = 0;
		data->chatborder->flags |= BOX_VISIBLE;
		data->participantborder->flags |= BOX_VISIBLE;
		data->edit->flags |= BOX_VISIBLE;
		chessbox->minw = data->boardborder->w + data->gameinfobox->w + 5 + 210 + 5;
		/*chessbox->minw = data->gameinfobox->x + data->gameinfobox->w + 5 + 150;*/
		if (data->oldchatw)
		{
			Log_Write(0, "neww %d %d %d", chessbox->minw, data->oldchatw, data->gameinfobox->x + data->gameinfobox->w + 5 + data->oldchatw + 5);
			neww = data->gameinfobox->x + data->gameinfobox->w + 5 + data->oldchatw + 5;
		}
		else
		{
			neww = chessbox->minw;
		}
		Box_MoveWndCustom(chessbox, chessbox->x, chessbox->y, neww, chessbox->h);
	}
	else
	{
		data->hidechat = 1;
		data->chatborder->flags &= ~BOX_VISIBLE;
		data->participantborder->flags &= ~BOX_VISIBLE;
		data->edit->flags &= ~BOX_VISIBLE;
		data->oldchatw = data->chatborder->w;
		chessbox->minw = 490;
		/*chessbox->minw = data->gameinfobox->x + data->gameinfobox->w + 5;*/
		Box_MoveWndCustom(chessbox, chessbox->x, chessbox->y, chessbox->w - data->oldchatw - 5, chessbox->h);
	}

	ChessBox_AdjustMenuAndMovelist(chessbox);
}

void ChessBoxBorder_RedoBorder(struct Box_s *boardborder, int rotated)
{
	struct Box_s *subbox;
	int i;
	char *letters = "abcdefgh";
	char *numbers = "87654321";
	int squaresize = (boardborder->w - 40);

	while (boardborder->child)
	{
		Box_Destroy(boardborder->child);
	}

	{
		struct Box_s *subbox;
		subbox = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners2ul", "contentcorners2.png", 0, 0, 5, 5);
		Box_AddChild(boardborder, subbox);

		subbox = Box_Create(boardborder->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners2ur", "contentcorners2.png", 5, 0, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(boardborder, subbox);

		subbox = Box_Create(boardborder->w - 5, boardborder->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners2dr", "contentcorners2.png", 5, 5, 5, 5);
		subbox->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(boardborder, subbox);

		subbox = Box_Create(0, boardborder->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->img = ImageMgr_GetSubImage("contentcorners2dl", "contentcorners2.png", 0, 5, 5, 5);
		subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(boardborder, subbox);
	}


	if (!rotated)
	{
		for (i=0; i<8; i++)
		{
			char txt[2];

			txt[0] = letters[i];
			txt[1] = '\0';

			subbox = Box_Create(i * squaresize / 8 + 20 + (squaresize / 8 - 11) / 2, squaresize + 20, 10, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			Box_SetText(subbox, txt);

			Box_AddChild(boardborder, subbox);
		}
		
		for (i=0; i<8; i++)
		{
			char txt[2];
	
			txt[0] = numbers[i];
			txt[1] = '\0';

			subbox = Box_Create(7, i * squaresize / 8 + 20 + (squaresize / 8 - 17) / 2, 10, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			Box_SetText(subbox, txt);

			Box_AddChild(boardborder, subbox);
		}
	}
	else
	{
		for (i=0; i<8; i++)
		{
			char txt[2];

			txt[0] = letters[7-i];
			txt[1] = '\0';

			subbox = Box_Create(i * squaresize / 8 + 20 + (squaresize / 8 - 9) / 2, 0, 10, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			Box_SetText(subbox, txt);

			Box_AddChild(boardborder, subbox);
		}
		
		for (i=0; i<8; i++)
		{
			char txt[2];
	
			txt[0] = numbers[7-i];
			txt[1] = '\0';

			subbox = Box_Create(squaresize + 20 + 6, i * squaresize / 8 + 20 + (squaresize / 8 - 17) / 2, 10, 20, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			Box_SetText(subbox, txt);

			Box_AddChild(boardborder, subbox);
		}
	}

	Box_Repaint(boardborder);
}

void ChessBox_OnRotate(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	bdata->showrotated = !bdata->showrotated;

	ChessBoard_UpdateBoard(board);
	ChessBoxBorder_RedoBorder(data->boardborder, bdata->showrotated);

	if (bdata->showrotated)
	{
		struct Box_s *arrange = data->blackinfobox;
		Box_Unlink(arrange);
		Box_AddChild(data->gameinfobox, arrange);


		arrange = data->whitetimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitecapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whiteplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitetopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);


		arrange = data->blackplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blackcapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blacktimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blacktopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);
	}
	else
	{
		struct Box_s *arrange = data->whiteinfobox;
		Box_Unlink(arrange);
		Box_AddChild(data->gameinfobox, arrange);


		arrange = data->blacktimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blackcapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blackplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blacktopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);


		arrange = data->whiteplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitecapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitetimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitetopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);
	}

	AutoSize_Fit(data->gameinfobox);

	ChessBox_AdjustMenuAndMovelist(chessbox);
	Box_Repaint(chessbox);
}

void ChessBoxMenu_OnRotate(struct Box_s *pbox, void *dummy)
{
	ChessBox_OnRotate(pbox);
}

void ChessBoxMenu_OnPositionChat(struct Box_s *pbox, void *dummy)
{
	ChessBox_OnChatArea(pbox);
}

void ChessBoxMenu_OnHideChat(struct Box_s *pbox, void *dummy)
{
	ChessBox_OnChatArea(pbox);
}

void ChessBoxMenu_OnAdjourn(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameAdjourn(data->gameid, data->roomjid == NULL);
}

void ChessBoxMenu_OnFlag(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameFlag(data->gameid, data->roomjid == NULL);
}

void ChessBoxMenu_OnRequestDraw(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameDraw(data->gameid, data->roomjid == NULL);
}

void ChessBoxMenu_OnRequestAbort(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameAbort(data->gameid, data->roomjid == NULL);
}

void ChessBoxMenu_OnResign(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameResign(data->gameid, data->roomjid == NULL);
}

void ChessBoxMenu_OnGiveTime(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	AutoEdit_Create(chessbox, 500, _("Add time to opponent's clock"), _("Enter the amount of time (in seconds) here."), NULL, "10", Ctrl_AddTimeCallback, data->gameid, NULL);
}

void ChessBoxMenu_OnCastleK(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendMove(data->gameid, "O-O");
}

void ChessBoxMenu_OnCastleQ(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendMove(data->gameid, "O-O=O");
}

void ChessBoxMenu_OnTakeback(struct Box_s *pbox, void *dummy)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameTakeback(data->gameid, data->roomjid == NULL);
}



void ChessBox_OnRequesterResign(struct Box_s *autodialog, char *gameid)
{
	Ctrl_SendGameResign(gameid, 0);
	Box_Destroy(autodialog);
	View_CloseChessGame(gameid);
}

void ChessBox_OnClose(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->gameover || !(data->whitelocal || data->blacklocal) || data->roomjid == NULL || data->disconnected)
	{
		View_CloseChessGame(data->gameid);
	}
	else
	{
		struct chessboxdata_s *data = chessbox->boxdata;

		AutoDialog_Create(chessbox, 500, _("You're still playing!"),
		  _("You are still playing a game.  Would you like to resign or return?"),
		  _("Return"), _("Resign"), NULL, ChessBox_OnRequesterResign, strdup(data->gameid));

		/*
		Text_SetText(data->message, "You can't close this game, you're still playing!");

		Box_Repaint(data->message);
		*/
	}
}

void ChessBox_OnRestore(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_RESTORE);
}

void ChessBox_OnMinimize(struct Box_s *pbox)
{
	ShowWindow(Box_GetRoot(pbox)->hwnd, SW_MINIMIZE);
}

void ChessBox_OnMaximize(struct Box_s *pbox)
{
	int maxx, maxy, maxw, maxh;
	RECT rc;
	struct chessboxdata_s *data;

	RECT windowrect;
	HMONITOR hm;
	MONITORINFO mi;

	pbox = Box_GetRoot(pbox);
	data = pbox->boxdata;

	windowrect.left = pbox->x;
	windowrect.right = windowrect.left + pbox->w - 1;
	windowrect.top = pbox->y;
	windowrect.bottom = windowrect.top + pbox->h - 1;

	hm = MonitorFromRect(&windowrect, MONITOR_DEFAULTTONEAREST);

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hm, &mi);

	rc = mi.rcWork;

	maxx = rc.left;
	maxy = rc.top;
	maxw = rc.right - rc.left;
	maxh = rc.bottom - rc.top;

	if (data->normh == 0)
	{
		data->chatwidth = data->chatborder->w;
		data->normx = pbox->x;
		data->normy = pbox->y;
		data->normw = pbox->w;
		data->normh = pbox->h;

		Box_MoveWndCustom(pbox, maxx, maxy, maxw, maxh);
		
		Box_ForceDraw(pbox);
	}
	else if (data->normh != 0)
	{
		/* reset minw so it doesn't stop us from downsizing the width */
		pbox->minw = 0;
		Box_MoveWndCustom(pbox, data->normx, data->normy, data->normw, data->normh);
		/*ChessBox_SetSizeChatLists(pbox, data->chatwidth);*/
		data->normh = 0;

		Box_ForceDraw(pbox);
	}
}

void ChessBox_OnDestroy(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);

	if (data->info)
	{
		if (data->info->white && data->info->white->jid)
		{
			Model_UnsubscribeProfile(data->info->white->jid, ChessBox_SetProfile, chessbox);
		}
		if (data->info->black && data->info->black->jid)
		{
			Model_UnsubscribeProfile(data->info->black->jid, ChessBox_SetProfile, chessbox);
		}

		Info_DestroyGameSearchInfo(data->info);
		data->info = NULL;
	}

	if (data->drawervisible)
	{
		Box_Destroy(data->drawer);
	}

	if (data->roomjid && !data->disconnected && data->isinroom)
	{
		Ctrl_LeaveChat(data->roomjid);
	}

	if (data->promotionbox)
	{
		Box_Destroy(data->promotionbox);
		data->promotionbox = NULL;
	}

	Box_RemoveTimedFunc(data->whitedelay, TimeDelay_ShrinkDelay, 100);
	Box_RemoveTimedFunc(data->blackdelay, TimeDelay_ShrinkDelay, 100);

	View_SetSavedWindowPos2("ChessBox3", chessbox->x, chessbox->y, chessbox->w, chessbox->h, data->hidechat ? -data->oldchatw : 0, 0);
}


void ChessBox_SetGameViewRotated(struct Box_s *chessbox, int rotated)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	bdata->showrotated = rotated;

	ChessBoard_UpdateBoard(board);
	ChessBoxBorder_RedoBorder(data->boardborder, rotated);

	if (bdata->showrotated)
	{
		struct Box_s *arrange = data->blackinfobox;
		Box_Unlink(arrange);
		Box_AddChild(data->gameinfobox, arrange);


		arrange = data->whitetimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitecapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whiteplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitetopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);


		arrange = data->blackplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blackcapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blacktimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blacktopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);
	}
	else
	{
		struct Box_s *arrange = data->whiteinfobox;
		Box_Unlink(arrange);
		Box_AddChild(data->gameinfobox, arrange);


		arrange = data->blacktimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blackcapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blackplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);

		arrange = data->blacktopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->blackinfobox, arrange);


		arrange = data->whiteplayerbox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitecapturebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitetimebox;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);

		arrange = data->whitetopspacer;
		Box_Unlink(arrange);
		Box_AddChildToBottom(data->whiteinfobox, arrange);
	}

	AutoSize_Fit(data->gameinfobox);

	ChessBox_AdjustMenuAndMovelist(chessbox);
	Box_Repaint(chessbox);
}

void ChessBox_SetGameViewRotatedIfBlack(struct Box_s *chessbox, char *blackjid)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	if (data->info)
	{
		if (data->info->black->jid && blackjid)
		{
			char *barejid1 = Jid_Strip(data->info->black->jid);
			char *barejid2 = Jid_Strip(blackjid);

			if (stricmp(barejid1, barejid2) == 0)
			{
				ChessBox_SetGameViewRotated(chessbox, 1);
			}
		}
	}
	else
	{
		data->rotateifblackjid = strdup(blackjid);
	}
}


extern HFONT tahoma10_f;
extern HFONT tahoma11_f;
extern HFONT tahoma11b_f;
HFONT clockfont;

void ChessChatBox_OnEnter(struct Box_s *pbox, char *text);

void ChessBox_ArrangeChats(struct Box_s *dialog)
{
	struct chessboxdata_s *data = dialog->boxdata;
	int totalwidth = dialog->w - data->chatborder->x - 5;

	if (data->gamechatonbottom)
	{
		/*
		data->participantborder->OnSizeWidth = NULL;
		data->participantborder->OnSizeHeight = Box_OnSizeHeight_Stretch;
		data->chatborder->OnSizeWidth = NULL;
		data->chatborder->OnSizeHeight = Box_OnSizeHeight_Stretch;
		data->editborder->OnSizeWidth = NULL;
		data->edit->OnSizeWidth = NULL;
		*/
	}
	else
	{
		data->participantborder->x = data->chatborder->x;
		data->participantborder->y = data->boardborder->y;
		Box_OnSizeWidth_Stretch(data->participantborder, totalwidth - data->participantborder->w);
		/*Box_OnSizeHeight_Stretch(data->participantborder, 90 - data->participantborder->h);*/

		data->chatborder->y = data->participantborder->y + data->participantborder->h + 5;
		Box_OnSizeWidth_Stretch(data->chatborder, totalwidth - data->chatborder->w);
/*		Box_OnSizeHeight_Stretch(data->chatborder, dialog->h - data->editborder->h - 10  - data->chatborder->y - data->chatborder->h);*/
		Box_OnSizeHeight_Stretch(data->chatborder, dialog->h - data->chatborder->y - 5 - data->chatborder->h);
		Box_OnSizeHeight_Stretch(data->vsizerbar, -data->vsizerbar->h);
/*
		data->editborder->x = data->chatborder->x;
		data->editborder->y = data->chatborder->y + data->chatborder->h + 5;
		Box_OnSizeWidth_Stretch(data->editborder, data->chatborder->w - data->editborder->w);
		Box_OnSizeWidth_Stretch(data->edit, data->editborder->w - 10 - data->edit->w);

		data->edit->x = data->editborder->x + 5;
		data->edit->y = data->editborder->y + 2;
		Box_OnSizeWidth_Stretch(data->edit, data->editborder->w - 10 - data->edit->w);
*/

		data->hsizerbar->x = data->participantborder->x;
		data->hsizerbar->y = data->participantborder->y + data->participantborder->h - 3;
		Box_OnSizeWidth_Stretch(data->hsizerbar, data->participantborder->w - data->hsizerbar->w);

		data->edit->x = data->chatborder->x + 12;
		data->edit->y = data->chatborder->y + data->chatborder->h - data->edit->h - 15;
		Box_OnSizeWidth_Stretch(data->edit, data->chatborder->w - 24 - data->edit->w);

		Box_OnSizeHeight_Stretch(data->chatlist, data->chatborder->h - data->chatlist->y - data->edit->h - 30 - data->chatlist->h);
		data->chatbottomline->y = data->chatlist->y + data->chatlist->h;

		data->participantborder->OnSizeWidth = Box_OnSizeWidth_Stretch;
		data->participantborder->OnSizeHeight = NULL;
		data->chatborder->OnSizeWidth = Box_OnSizeWidth_Stretch;
		data->chatborder->OnSizeHeight = NULL;
		data->editborder->OnSizeWidth = Box_OnSizeWidth_Stretch;
		data->edit->OnSizeWidth = Box_OnSizeWidth_Stretch;
	}
}

void ChessBox_SetSizeChatLists(struct Box_s *dialog, int chatw)
{
	struct chessboxdata_s *data = dialog->boxdata;

	ChessBox_ArrangeChats(dialog);

	Box_Repaint(dialog);
}

void ChessBoxLists_OnSizeWidth(struct Box_s *pbox, int dw)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct chessboxdata_s *data = dialog->boxdata;
	int totalwidth = dialog->w - data->chatborder->x - 5;
	int chatw = data->chatborder->w + dw;

	if (!data->gamechatonbottom)
	{
		return;
	}

	if (totalwidth <= 85 || chatw <= 40)
	{
		chatw = 40;
	}
	else if (chatw >= totalwidth - 45)
	{
		chatw = totalwidth - 45;
	}

	ChessBox_SetSizeChatLists(dialog, chatw);
}

void ChessBoxEdit_EditSizeFunc(struct Box_s *edit, int edith)
{
	struct Box_s *dialog = Box_GetRoot(edit);

	struct chessboxdata_s *data = dialog->boxdata;
	int chath, totalh;

	totalh = data->chatborder->h - 60; /*dialog->h - data->chatborder->y - 5;*/
/*
	if (!data->gamechatonbottom)
	{
		totalh -= 155;
	}
*/
	if (edith > totalh / 2)
	{
		edith = totalh / 2;
	}

	chath = totalh - edith - 5;

	/*Box_OnSizeHeight_Stretch(data->editborder, edith + 4 - data->editborder->h);*/
	Box_OnSizeHeight_Stretch(data->edit, edith - data->edit->h);

	ChessBox_ArrangeChats(dialog);
#if 0
	data->editborder->y = data->chatborder->y + chath + 5;
	data->edit->y = data->editborder->y + 2;

	Box_OnSizeHeight_Stretch(data->chatborder, chath - data->chatborder->h);

	if (data->gamechatonbottom)
	{
		Box_OnSizeHeight_Stretch(data->vsizerbar,         data->chatborder->h - data->vsizerbar->h);
		Box_OnSizeHeight_Stretch(data->participantborder, data->chatborder->h - data->participantborder->h);
	}

	/*Box_OnSizeHeight_Stretch(data->chatlist,          dialog->h - data->chatlist->y          - edith - 8 - 10 - data->chatlist->h);*/
	/*Box_OnSizeHeight_Stretch(data->participantlist,   dialog->h - data->participantlist->y   - edith - 8 - 10 - data->participantlist->h);*/
#endif
	Box_Repaint(dialog);
}

void ChessBoxButton_OnSendMove(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	if (!data->storedmove || strlen(data->storedmove) == 0)
	{
		return;
	}

	data->ignorenextmove = 1;
	Ctrl_SendCorMove(data->gameid, data->storedmove);
	data->storedmove[0] = '\0';
	/*
	data->sendmovebutton->flags  &= ~BOX_VISIBLE;
	data->clearmovebutton->flags &= ~BOX_VISIBLE;
	*/
	Button_SetDisabledState(data->sendmovebutton, 1);
	Text_SetText(data->message, _("It's your opponent's turn.  You'll be notified when it's complete."));
	Box_Repaint(data->message);
	Box_Repaint(data->gameinfobox);
}

void ChessBoxButton_OnClearMove(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_GetCorGameState(data->gameid);
	data->storedmove[0] = '\0';
	Button_SetDisabledState(data->sendmovebutton, 1);
	Box_Repaint(data->gameinfobox);
}

int MoveListEntry_Sort(struct Box_s *lentry, struct Box_s *rentry)
{
	return lentry->userdata < rentry->userdata;
}

int MoveListEntry_SortReverse(struct Box_s *lentry, struct Box_s *rentry)
{
	return lentry->userdata > rentry->userdata;
}

void MoveListSort_OnLButtonDown(struct Box_s *pbox, int x, int y)
{
	/*
	struct Box_s *drawer = Box_GetRoot(pbox);
	struct chessboxdrawerdata_s *ddata = drawer->boxdata;
*/
	return; /* FIXME: drawer has been eliminated */
#if 0
	Box_OnLButtonDown(pbox, x, y);

	if (ddata->reversesort)
	{
		List_SetEntrySortFunc(ddata->movelist, MoveListEntry_Sort);
		ddata->reversesort = 0;
		ddata->movelistsorticon->img = ImageMgr_GetSubImage("sortforward", "SortTriangles.png", 0, 0, 16, 16);
	}
	else
	{
		List_SetEntrySortFunc(ddata->movelist, MoveListEntry_SortReverse);
		ddata->reversesort = 1;
		ddata->movelistsorticon->img = ImageMgr_GetSubImage("sortreverse", "SortTriangles.png", 16, 0, 16, 16);
	}

	List_Resort(ddata->movelist);
	List_RedoEntries(ddata->movelist);
	Box_Repaint(ddata->movelist);
	Box_Repaint(ddata->movelistsorticon);
#endif
}

void ChessBox_OnInactive(struct Box_s *dialog)
{
	struct chessboxdata_s *data = dialog->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;

	TitleBarRoot_OnInactive(dialog);

	if (bdata->dragtarget)
	{
		struct Box_s *piece = bdata->dragtarget;
		struct chesspieceboxdata_s *pdata = piece->boxdata;

		piece->x = (pdata->x - 1) * board->w / 8;
		piece->y = (pdata->y - 1) * board->h / 8;

		bdata->dragtarget = NULL;
	}
}

void ChessBoxMenu_OnToggleMoveList(struct Box_s *pbox)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	struct chessboxdata_s *data = dialog->boxdata;
	int top = data->gameinfobox->y + data->gameinfobox->h + 5;
	int totalheight = dialog->h - top - 5;
	int minmenuheight = data->menutext->h + 10;

	ChessBox_ToggleMoveList(dialog);
	return;
#if 0
	if (data->movelistborder->flags & BOX_VISIBLE)
	{
		data->movelistborder->flags &= ~BOX_VISIBLE;
		data->movelisthidden = 1;
	}
	else 
	{
		data->movelistborder->flags |= BOX_VISIBLE;
		if (totalheight < minmenuheight + 5 + 80)
		{
			Box_MoveWndCustom(dialog, dialog->x, dialog->y, dialog->w, minmenuheight + 5 + 80 + top + 5);
		}
		data->movelisthidden = 0;
	}

	ChessBox_AdjustMenuAndMovelist(dialog);
	Box_Repaint(dialog);
#endif
}

void ChessBox_UpdateMenu(struct Box_s *dialog)
{
	struct chessboxdata_s *data = dialog->boxdata;
	char txt[1024];
	void *clickfuncs[20];
	int link = 0, i;

	txt[0] = '\0';

	/*strcat(txt, "^bGame Options^n");*/

	strcat(txt, "> ^L");
	strcat(txt, _("Flip Board"));
	strcat(txt, "^l");
	clickfuncs[++link] = ChessBoxMenu_OnRotate;

	if (data->hidechat)
	{
		strcat(txt, "\n> ^L");
		strcat(txt, _("Show chat"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnHideChat;
	}
	else
	{
		strcat(txt, "\n> ^L");
		strcat(txt, _("Hide chat"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnHideChat;
	}

	if (!data->gameover && (data->whitelocal || data->blacklocal))
	{
		strcat(txt, "\n> ^L");
		strcat(txt, _("Request Adjourn"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnAdjourn;

		if (Model_GetOption(OPTION_NOGAMEAUTOFLAG))
		{
			strcat(txt, "\n> ^L");
			strcat(txt, _("Flag Opponent's Time"));
			strcat(txt, "^l");
			clickfuncs[++link] =  ChessBoxMenu_OnFlag;
		}

		strcat(txt, "\n> ^L");
		strcat(txt, _("Request Draw"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnRequestDraw;
		
		if ((data->whitelocal && data->movenum < 3) || (data->blacklocal && data->movenum < 2))
		{
			if (data->canabort)
			{
				strcat(txt, "\n> ^L");
				strcat(txt, _("Request Abort"));
				strcat(txt, "^l");
				clickfuncs[++link] = ChessBoxMenu_OnRequestAbort;
			}
			else
			{
				strcat(txt, "\n> ^4");
				strcat(txt, _("Request Abort"));
				strcat(txt, "^n");
			}
		}

		strcat(txt, "\n> ^L");
		strcat(txt, _("Resign Game"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnResign;

		strcat(txt, "\n> ^L");
		strcat(txt, _("Give Time"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnGiveTime;

		if (data->info && data->info->variant && stricmp(data->info->variant, "chess960") == 0)
		{
			strcat(txt, "\n> ^L");
			strcat(txt, _("Quick Castle King's side"));
			strcat(txt, "^l");
			clickfuncs[++link] = ChessBoxMenu_OnCastleK;

			strcat(txt, "\n> ^L");
			strcat(txt, _("Quick Castle Queen's side"));
			strcat(txt, "^l");
			clickfuncs[++link] = ChessBoxMenu_OnCastleQ;
		}

		if (data->info && !data->info->rated)
		{
			strcat(txt, "\n> ^L");
			strcat(txt, _("Request Takeback"));
			strcat(txt, "^l");
			clickfuncs[++link] = ChessBoxMenu_OnTakeback;
		}
	}

	if (data->drawervisible /*data->movelistborder->flags & BOX_VISIBLE*/)
	{
		strcat(txt, "\n> ^L");
		strcat(txt, _("Hide Move List"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnToggleMoveList;
	}
	else
	{
		strcat(txt, "\n> ^L");
		strcat(txt, _("Show Move List"));
		strcat(txt, "^l");
		clickfuncs[++link] = ChessBoxMenu_OnToggleMoveList;
	}

	Text_SetText(data->menutext, txt);

	for (i = 0; i <= link; i++)
	{
		Text_SetLinkCallback(data->menutext, i, clickfuncs[i], NULL);
	}
	Box_OnSizeHeight_Stretch(data->menutextborder, data->menutext->h + 30 - data->menutextborder->h);
}

struct Box_s *ChessBox_Create(int x, int y, int w, int h, int chatw, char *gameid, char *roomjid)
{
	struct Box_s *dialog, *subbox, *drawer, *movelist, *buttons;
	struct chessboxdata_s *data;
	struct chessboxdrawerdata_s *ddata;

	if (chatw < 0)
	{
		chatw = -chatw;
	}

	if (!clockfont)
	{
		/*clockfont = CreateFont(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Tahoma");*/
		clockfont = CreateFont(-19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, VARIABLE_PITCH | FF_SWISS, "Lucida Grande");
	}

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));
	data->gameid = strdup(gameid);
	data->roomjid = strdup(roomjid);
	data->movenum = 1;
	data->nick = Jid_GetResource(roomjid);
	{
		char *piecetheme = Model_GetOptionString(OPTION_PIECESTHEME);
		char *piecethemepng = "pieces.png";

		if (piecetheme)
		{
			if (stricmp(piecetheme, "fantasy") == 0)
			{
				piecethemepng = "pieces-fantasy.png";
			}
			else if (stricmp(piecetheme, "spatial") == 0)
			{
				piecethemepng = "pieces-spatial.png";
			}
			else if (stricmp(piecetheme, "skulls") == 0)
			{
				piecethemepng = "pieces-skulls.png";
			}
			else if (stricmp(piecetheme, "eyes") == 0)
			{
				piecethemepng = "pieces-eyes.png";
			}
		}

		data->piecethemepng = strdup(piecethemepng);
	}

	ddata = malloc(sizeof(*ddata));
	memset(ddata, 0, sizeof(*ddata));

	/**/

	if (roomjid && w > h - 188 - 20 - TitleBarHeight - 4 - 40)
	{
		dialog = Box_Create(x, y, 800, 602, BOX_VISIBLE);
		dialog->minw = 490 /*800*/;
		dialog->minh = 360;
	}
	else
	{
		dialog = Box_Create(x, y, 388, 602, BOX_VISIBLE);
		dialog->minw = 490;
		dialog->minh = 360;
		data->hidechat = 1;
	}
	dialog->bgcol = DefaultBG;
	dialog->OnSizeWidth = Box_OnSizeWidth_Stretch;	

	data->sizerset = SizerSet_Create(dialog);
	dialog->boxtypeid = BOXTYPE_GAME;
	/*
	if (!roomjid)
	{
		SizerSet_SetDisabled(data->sizerset, 1);
	}
*/
	dialog->titlebar = TitleBar_Add(dialog, _("Game"), ChessBox_OnClose, ChessBox_OnMinimize, ChessBox_OnMaximize);
	dialog->OnActive = TitleBarRoot_OnActive;
	dialog->OnInactive = ChessBox_OnInactive;
	
	dialog->OnClose = ChessBox_OnClose;
	dialog->OnMinimize = ChessBox_OnMinimize;
	dialog->OnMaximize = ChessBox_OnMaximize;
	dialog->OnRestore = ChessBox_OnRestore;

	subbox = Box_Create(10, TitleBarHeight + /*25*/ 10, 368, 368, BOX_VISIBLE | BOX_BORDER);
	subbox->bgcol = BoardBorderBG;
	subbox->brcol = BoardBorderBR;
	Box_AddChild(dialog, subbox);
	data->boardborder = subbox;

	ChessBoxBorder_RedoBorder(data->boardborder, FALSE);

	subbox = ChessBoard_Create(30, TitleBarHeight + 30);
	Box_AddChild(dialog, subbox);
	data->chessboard = subbox;

	subbox = Text_Create(20, /*TitleBarHeight - 3*/ 388 + TitleBarHeight + 4, 334, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT, TX_CENTERED | TX_CENTERVERT | TX_WRAP);
	/*subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;*/
	subbox->fgcol = GameMessageFG;
	subbox->font = tahoma11b_f;
	Text_SetPaletteColor(subbox, 6, GameMessageFL);
	Box_AddChild(dialog, subbox);
	data->message = subbox;

	subbox = AutoSizeSpace_Create(800 - 187 - 10, data->boardborder->y, 187, 0, 0, 0, 5, AUTOSIZE_VERT);
	Box_AddChild(dialog, subbox);
	data->gameinfobox = subbox;

	{
		subbox = AutoSizeSpace_Create(0, 0, 187, 0, 187, 0, 5, AUTOSIZE_VERT);
		subbox->flags &= ~BOX_TRANSPARENT;
		subbox->flags |= BOX_BORDER5;
		subbox->bgcol = GamePlayerBG;
		subbox->brcol = GamePlayerBR;
		Box_AddChild(data->gameinfobox, subbox);
		data->blackinfobox = subbox;
		{
			struct Box_s *pinfo;
			
			data->blacktopspacer = AutoSize_AddSpacer(data->blackinfobox, 0);

			pinfo = Box_Create(0, 0, 187, 30, BOX_VISIBLE | BOX_TRANSPARENT);
			data->blackplayerbox = pinfo;
			Box_AddChild(data->blackinfobox, pinfo);
			{
				subbox = Box_Create(8, 0, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
				subbox->img = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
				Box_AddChild(pinfo, subbox);
				data->blackavatar = subbox;
				
				subbox = LinkBox_Create(50, 0, 118, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->fgcol = CR_DkOrange;
				LinkBox_SetClickFunc(subbox, ChessBox_OnClickPlayer, NULL);
				Box_AddChild(pinfo, subbox);
				data->blackname = subbox;
				
				subbox = Box_Create(50, 15, 80, 12, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->fgcol = GamePlayerFG;
				Box_AddChild(pinfo, subbox);
				data->blackrating = subbox;
			}

			subbox = Box_Create(0, 0, 187, 48, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			Box_SetText(subbox, _("No pieces taken."));
			Box_AddChild(data->blackinfobox, subbox);
			data->blackcapturebox = subbox;

			subbox = Box_Create(9, 0, 173, 40, BOX_VISIBLE | BOX_BORDER5);
			subbox->bgcol = GameBlackTimeBG;
			subbox->fgcol = GameBlackTimeFG;
			subbox->brcol = GameBlackTimeBR;
			Box_AddChild(data->blackinfobox, subbox);
			data->blacktimebox = subbox;
			{
				subbox = Box_Create(12, 14, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->img = ImageMgr_GetImage("inactive.png");
				Box_AddChild(data->blacktimebox, subbox);
				data->blackdot = subbox;

				subbox = Box_Create(35, 9, 65, 25, BOX_VISIBLE | BOX_RIGHTTEXT | BOX_TRANSPARENT);
				subbox->fgcol = GameBlackTimeFG;
				subbox->font = clockfont;
				Box_SetText(subbox, "00:00");
				Box_AddChild(data->blacktimebox, subbox);
				data->blackclock = subbox;

				subbox = Box_Create(10, 31, 0, 2, BOX_VISIBLE);
				subbox->bgcol = GameBlackTimeFG;
				Box_AddChild(data->blacktimebox, subbox);
				data->blackdelay = subbox;

				subbox = Box_Create(105, 16, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
				subbox->fgcol = GameBlackTimeFG;/*UserInfoFG2;*/
				subbox->font = tahoma10_f;
				Box_AddChild(data->blacktimebox, subbox);
				data->blacktimeinfo = subbox;
			}
			data->blackbottomspacer = AutoSize_AddSpacer(data->blackinfobox, 5);
		}

		subbox = AutoSizeSpace_Create(0, 0, 187, 0, 187, 0, 5, AUTOSIZE_VERT);
		subbox->flags &= ~BOX_TRANSPARENT;
		subbox->flags |= BOX_BORDER5;
		subbox->bgcol = GamePlayerBG;
		subbox->brcol = GamePlayerBR;
		Box_AddChild(data->gameinfobox, subbox);
		data->whiteinfobox = subbox;
		{
			struct Box_s *pinfo;

			data->whitetopspacer = AutoSize_AddSpacer(data->whiteinfobox, 0);

			subbox = Box_Create(9, 0, 173, 40, BOX_VISIBLE | BOX_BORDER5);
			subbox->bgcol = GameWhiteTimeBG;
			subbox->fgcol = GameWhiteTimeFG;
			subbox->brcol = GameWhiteTimeBR;
			Box_AddChild(data->whiteinfobox, subbox);
			data->whitetimebox = subbox;
			{
				subbox = Box_Create(12, 14, 12, 12, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->img = ImageMgr_GetImage("inactive.png");
				Box_AddChild(data->whitetimebox, subbox);
				data->whitedot = subbox;

				subbox = Box_Create(35, 9, 65, 25, BOX_VISIBLE | BOX_RIGHTTEXT | BOX_TRANSPARENT);
				subbox->fgcol = GameWhiteTimeFG;
				subbox->font = clockfont;
				Box_SetText(subbox, "00:00");
				Box_AddChild(data->whitetimebox, subbox);
				data->whiteclock = subbox;

				subbox = Box_Create(10, 31, 0, 2, BOX_VISIBLE);
				subbox->bgcol = GameWhiteTimeFG;
				Box_AddChild(data->whitetimebox, subbox);
				data->whitedelay = subbox;

				subbox = Box_Create(105, 16, 80, 20, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
				subbox->fgcol = GameWhiteTimeFG;
				subbox->font = tahoma10_f;
				Box_AddChild(data->whitetimebox, subbox);
				data->whitetimeinfo = subbox;
			}

			subbox = Box_Create(0, 0, 187, 48, BOX_VISIBLE | BOX_TRANSPARENT | BOX_CENTERTEXT);
			Box_SetText(subbox, _("No pieces taken."));
			Box_AddChild(data->whiteinfobox, subbox);
			data->whitecapturebox = subbox;

			pinfo = Box_Create(0, 0, 187, 30, BOX_VISIBLE | BOX_TRANSPARENT);
			data->whiteplayerbox = pinfo;
			Box_AddChild(data->whiteinfobox, pinfo);
			{
				subbox = Box_Create(8, 0, 30, 30, BOX_VISIBLE | BOX_TRANSPARENT | BOX_FITASPECTIMG);
				subbox->img = ImageMgr_GetSubImage("DefaultAvatar1", "DefaultAvatar.png", 0, 0, 50, 50);
				Box_AddChild(pinfo, subbox);
				data->whiteavatar = subbox;
				
				subbox = LinkBox_Create(50, 0, 118, 16, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->fgcol = CR_DkOrange;
				LinkBox_SetClickFunc(subbox, ChessBox_OnClickPlayer, NULL);
				Box_AddChild(pinfo, subbox);
				data->whitename = subbox;
				
				subbox = Box_Create(50, 15, 80, 12, BOX_VISIBLE | BOX_TRANSPARENT);
				subbox->fgcol = GamePlayerFG;
				Box_AddChild(pinfo, subbox);
				data->whiterating = subbox;
			}

			data->whitebottomspacer = AutoSize_AddSpacer(data->whiteinfobox, 5);
		}
	}

	AutoSize_Fit(data->gameinfobox);
	AutoSize_Fill(data->gameinfobox);

	buttons = Box_Create(0, 0, 180, 30, BOX_TRANSPARENT);
	Box_AddChild(dialog, buttons);
	data->buttons = buttons;
	{
		subbox = Button_Create(0, 0, 38, 29, BOX_VISIBLE | BOX_TRANSPARENT);
		Button_SetNormalImg (subbox, ImageMgr_GetSubImage("gameMenu-normal",  "gameMenu.png",  0, 0, 38, 29));
		Button_SetPressedImg(subbox, ImageMgr_GetSubImage("gameMenu-pressed", "gameMenu.png", 38, 0, 38, 29));
		Button_SetHoverImg  (subbox, ImageMgr_GetSubImage("gameMenu-hover",   "gameMenu.png", 76, 0, 38, 29));
		Button_SetOnButtonHit(subbox, ChessBox_OnGameMenu);
		Button_SetTooltipText(subbox, _("Game Menu"));
		Box_AddChild(buttons, subbox);

		subbox = Button_Create(45, 0, 28, 29, BOX_VISIBLE | BOX_TRANSPARENT);
		Button_SetNormalImg (subbox, ImageMgr_GetSubImage("reverseBoard-normal",  "reverseBoard.png",  0, 0, 28, 29));
		Button_SetPressedImg(subbox, ImageMgr_GetSubImage("reverseBoard-pressed", "reverseBoard.png", 28, 0, 28, 29));
		Button_SetHoverImg  (subbox, ImageMgr_GetSubImage("reverseBoard-hover",   "reverseBoard.png", 56, 0, 28, 29));
		Button_SetOnButtonHit(subbox, ChessBox_OnRotate);
		Button_SetTooltipText(subbox, _("Flip Board"));
		Box_AddChild(buttons, subbox);

		subbox = Button_Create(80, 0, 28, 29, BOX_VISIBLE | BOX_TRANSPARENT);
		Button_SetNormalImg (subbox, ImageMgr_GetSubImage("chatToggle-normal",  "chatToggle.png",  0, 0, 28, 29));
		Button_SetPressedImg(subbox, ImageMgr_GetSubImage("chatToggle-pressed", "chatToggle.png", 28, 0, 28, 29));
		Button_SetHoverImg  (subbox, ImageMgr_GetSubImage("chatToggle-hover",   "chatToggle.png", 56, 0, 28, 29));
		Button_SetOnButtonHit(subbox, ChessBox_OnChatArea);
		Button_SetTooltipText(subbox, _("Toggle Chat"));
		Box_AddChild(buttons, subbox);
	}

	subbox = Box_Create(data->gameinfobox->x + data->gameinfobox->w + 5, data->boardborder->y, 800 - 120 - 14, dialog->h - 30 - TitleBarHeight - 4, BOX_VISIBLE | BOX_BORDER5);
	/*subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;*/
	subbox->bgcol = GameChatBG1;
	subbox->brcol = GameChatBR;
	Box_AddChild(dialog, subbox);
	data->chatborder = subbox;
/*
	subbox = Box_Create(8, 4, data->chatborder->w - 16, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, _("Chat"));
	Box_AddChild(data->chatborder, subbox);

	subbox = Box_Create(0, 22, data->chatborder->w, 1, BOX_VISIBLE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->bgcol = RGB(181, 184, 167);
	Box_AddChild(data->chatborder, subbox);
*/
	subbox = Box_Create(0, data->chatborder->h - 18 - 30, data->chatborder->w, 1, BOX_VISIBLE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	subbox->bgcol = GameChatFG;
	Box_AddChild(data->chatborder, subbox);
	data->chatbottomline = subbox;

/*	
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}
*/
	/*subbox = List_Create(5, 25, data->chatborder->w - 10, data->chatborder->h - 30 - 30 - 18, BOX_VISIBLE, FALSE);*/
	subbox = List_Create(5, 5, data->chatborder->w - 10, data->chatborder->h - 30 - 5 - 18, BOX_VISIBLE, FALSE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	subbox->bgcol = GameChatBG1; /*TabBG2;*/
	List_SetStripeBG1(subbox, GameChatBG1/*TabBG2*/);
	List_SetStripeBG2(subbox, GameChatBG2/*TabBG3*/);
	Box_AddChild(data->chatborder, subbox);
	List_SetEntrySelectable(subbox, FALSE);
	List_SetStickyBottom(subbox, 1);
	data->chatlist = subbox;

	subbox = Box_Create(data->chatborder->x + data->chatborder->w + 5, data->chatborder->y, 90, 143, BOX_VISIBLE | BOX_BORDER5);
	/*subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;*/
	subbox->bgcol = GameChatPrtcpntBG;
	subbox->brcol = GameChatPrtcpntBR;
	Box_AddChild(dialog, subbox);
	data->participantborder = subbox;

	/*
	subbox = Box_Create(data->participantborder->w - GrabberWidth, data->participantborder->h - GrabberHeight, GrabberWidth, GrabberHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("grippylight.png");
	subbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(data->participantborder, subbox);
	*/
	
/*
	subbox = Box_Create(8, 4, data->participantborder->w - 16, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, _("Players"));
	Box_AddChild(data->participantborder, subbox);

	subbox = Box_Create(0, 22, data->participantborder->w, 1, BOX_VISIBLE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->bgcol = RGB(128, 128, 128);
	Box_AddChild(data->participantborder, subbox);
*/
	/*
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}
	*/

	/*subbox = List_Create(5, 25, data->participantborder->w - 10, data->participantborder->h - 30, BOX_VISIBLE, FALSE);*/
	subbox = List_Create(5, 5, data->participantborder->w - 10, data->participantborder->h - 10, BOX_VISIBLE, FALSE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetStripeBG1(subbox, GameChatPrtcpntBG /*RGB(180, 184, 165)*/);
	List_SetStripeBG2(subbox, GameChatPrtcpntBG /*RGB(180, 184, 165)*/);
	List_SetGroupSelectable(subbox, FALSE);
	List_SetHideDisclosureTriangles(subbox, TRUE);
	List_SetSortGroups(subbox, 0);
	List_SetGroupCollapsible(subbox, FALSE);
	/*List_AddGroup(subbox, NULL);*/
	List_AddGroup(subbox, _("Players"));
	List_AddGroup(subbox, _("Observers"));
	List_SetSpreadHorizSize(subbox, 90);
	List_SetEntrySortFunc(subbox, ParticipantEntry_SortFunc);
	/*List_SetHideNoGroupHeader(subbox, 1);*/
	List_RedoEntries(subbox);
	Box_AddChild(data->participantborder, subbox);
	data->participantlist = subbox;

	subbox = HSizerBar_Create(data->participantborder->x, data->participantborder->y + data->participantborder->h + 2, data->participantborder->w, 5, data->participantborder, data->chatborder);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->OnSizeHeight = NULL;
	Box_AddChild(dialog, subbox);
	data->hsizerbar = subbox;

	subbox = VSizerBar_Create(data->participantborder->x - 5, data->participantborder->y, 5, data->participantborder->h, data->chatborder, data->participantborder);
	/*subbox->OnSizeWidth = ChessBoxLists_OnSizeWidth;*/
	subbox->OnSizeHeight = Box_OnSizeHeight_Stretch;
	Sizer_SetDisabled(subbox, 1);
	Box_AddChild(dialog, subbox);
	data->vsizerbar = subbox;
	
	subbox = Box_Create(4, dialog->h - 26, 800 - 8, 22, BOX_TRANSPARENT);
	/*subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;*/
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	data->editborder = subbox;
	Box_AddChild(dialog, subbox);
#if 0
	{
		struct Box_s *subbox2;

		subbox2 = Box_Create(0, 0, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("editborderul", "SearchBorder.png", 0, 0, 6, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(6, 0, subbox->w - 12, 6, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox2->img = ImageMgr_GetSubImage("editborderu", "SearchBorder.png", 6, 0, 230 - 12, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 6, 0, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->img = ImageMgr_GetSubImage("editborderur", "SearchBorder.png", 230 - 6, 0, 6, 6);
		Box_AddChild(subbox, subbox2);


		subbox2 = Box_Create(0, 6, 6, subbox->h - 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox2->img = ImageMgr_GetSubImage("editborderl", "SearchBorder.png", 0, 6, 6, 25 - 12);
		Box_AddChild(subbox, subbox2);
		
		subbox2 = Box_Create(subbox->w - 6, 6, 6, subbox->h - 12, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_Stretch;
		subbox2->img = ImageMgr_GetSubImage("editborderr", "SearchBorder.png", 230 - 6, 6, 6, 25 - 12);
		Box_AddChild(subbox, subbox2);


		subbox2 = Box_Create(0, subbox->h - 6, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("editborderdl", "SearchBorder.png", 0, 25 - 6, 6, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(6, subbox->h - 6, subbox->w - 12, 6, BOX_VISIBLE | BOX_TRANSPARENT | BOX_TILEIMAGE);
		subbox2->OnSizeWidth = Box_OnSizeWidth_Stretch;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("editborderd", "SearchBorder.png", 6, 25 - 6, 230 - 12, 6);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 6, subbox->h - 6, 6, 6, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		subbox2->img = ImageMgr_GetSubImage("editborderdr", "SearchBorder.png", 230 - 6, 25 - 6, 6, 6);
		Box_AddChild(subbox, subbox2);
	}
#endif
	subbox = Edit2Box_Create(4 + 5, dialog->h - 22 - 2, 800 - 8 - 10, 18, BOX_VISIBLE | BOX_BORDER, 0);
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	subbox->bgcol = GameChatEditBG;
	subbox->fgcol = GameChatEditFG;
	subbox->brcol = GameChatEditBR;
	Edit2Box_SetOnEnter(subbox, ChessChatBox_OnEnter);
	Edit2Box_SetEditSizeFunc(subbox, ChessBoxEdit_EditSizeFunc);
	Box_AddChild(dialog, subbox);
	data->edit = subbox;
	
	data->turn = CT_NONE;

	dialog->boxdata = data;
	dialog->OnCommand = Menu_OnCommand;
	dialog->OnDestroy = ChessBox_OnDestroy;

	drawer = Box_Create(dialog->x - 80, dialog->y + 20, 150, dialog->h - 40, BOX_VISIBLE);
	drawer->bgcol = DrawerBG;
	drawer->OnCommand = Menu_OnCommand;

	subbox = Box_Create(10, 5, drawer->w - 20, 20, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = MoveListFG3;
	subbox->OnLButtonDown = MoveListSort_OnLButtonDown;
	Box_SetText(subbox, _("Move List"));
	Box_AddChild(drawer, subbox);

	subbox = Box_Create(60, 3, 16, 16, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("sortforward", "SortTriangles.png", 0, 0, 16, 16);
	Box_AddChild(drawer, subbox);
	ddata->movelistsorticon = subbox;

	subbox = Box_Create(35, 30, 10, 10, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("whitedot", "sidedots.png", 0, 0, 10, 10);
	Box_AddChild(drawer, subbox);

	subbox = Box_Create(90, 30, 10, 10, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetSubImage("blackdot", "sidedots.png", 10, 0, 10, 10);
	Box_AddChild(drawer, subbox);

	subbox = Box_Create(data->gameinfobox->x, data->gameinfobox->y + data->gameinfobox->h + 5, data->gameinfobox->w, 300, BOX_VISIBLE | BOX_BORDER5);
	subbox->bgcol = GameMenuBG;
	subbox->brcol = GameMenuBR;
	Box_AddChild(dialog, subbox);
	data->menutextborder = subbox;

	subbox = Box_Create(8, 4, data->menutextborder->w - 16, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = GameMenuFG1;
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, _("Game Options"));
	Box_AddChild(data->menutextborder, subbox);

	subbox = Box_Create(0, 22, data->menutextborder->w, 1, BOX_VISIBLE);
	subbox->OnSizeWidth = Box_OnSizeWidth_Stretch;
	subbox->bgcol = GameMenuFG2; 
	Box_AddChild(data->menutextborder, subbox);

	/*
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}
*/
	/*subbox = Text_Create(8, 0, data->menutextborder->w, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);*/
	subbox = Text_Create(8, 25, data->menutextborder->w, 0, BOX_VISIBLE | BOX_TRANSPARENT, TX_WRAP | TX_STRETCHVERT);
	/*subbox->fgcol = RGB(255, 255, 255);*/
	subbox->fgcol = GameMenuFG1;
	subbox->font = tahoma11_f;
	/*Text_SetLinkColor(subbox, CR_LtOrange);*/
	Text_SetPaletteColor(subbox, 6, GameMenuFGL);
#if 0
	{
		char txt[1024];

		txt[0] = '\0';

		/*strcat(txt, "^bBoard Options^n\n");*/

		strcat(txt, "> ^L");
		strcat(txt, _("Flip Board"));
		strcat(txt, "^l\n");
/*
		strcat(txt, "> ^L");
		strcat(txt, _("Change chat position"));
		strcat(txt, "^l\n");
*/
		strcat(txt, "> ^L");
		strcat(txt, _("Hide chat"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Request Adjourn"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Flag Opponent's Time"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Request Draw"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Request Abort"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Resign Game"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Give Time"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Quick Castle King's side"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Quick Castle Queen's side"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Request Takeback"));
		strcat(txt, "^l\n");

		strcat(txt, "> ^L");
		strcat(txt, _("Show Move List"));
		strcat(txt, "^l");

		Text_SetText(subbox, txt);

		Text_SetLinkCallback(subbox,  1, ChessBoxMenu_OnRotate, NULL);
		/*Text_SetLinkCallback(subbox,  2, ChessBoxMenu_OnPositionChat, NULL);*/
		Text_SetLinkCallback(subbox,  2, ChessBoxMenu_OnHideChat, NULL);
		Text_SetLinkCallback(subbox,  3, ChessBoxMenu_OnAdjourn, NULL);
		Text_SetLinkCallback(subbox,  4, ChessBoxMenu_OnFlag, NULL);
		Text_SetLinkCallback(subbox,  5, ChessBoxMenu_OnRequestDraw, NULL);
		Text_SetLinkCallback(subbox,  6, ChessBoxMenu_OnRequestAbort, NULL);
		Text_SetLinkCallback(subbox,  7, ChessBoxMenu_OnResign, NULL);
		Text_SetLinkCallback(subbox,  8, ChessBoxMenu_OnGiveTime, NULL);
		Text_SetLinkCallback(subbox,  9, ChessBoxMenu_OnCastleK, NULL);
		Text_SetLinkCallback(subbox, 10, ChessBoxMenu_OnCastleQ, NULL);
		Text_SetLinkCallback(subbox, 11, ChessBoxMenu_OnTakeback, NULL);
		Text_SetLinkCallback(subbox, 12, ChessBoxMenu_OnToggleMoveList, NULL);
	}
#endif
	data->menutext = subbox;
	Box_AddChild(data->menutextborder, subbox);
	/*Box_OnSizeHeight_Stretch(data->menutextborder, data->menutext->h - data->menutextborder->h);*/
	Box_OnSizeHeight_Stretch(data->menutextborder, data->menutext->h + 30 - data->menutextborder->h);


	subbox = Box_Create(data->gameinfobox->x, data->menutextborder->y + data->menutextborder->h + 5, data->gameinfobox->w, dialog->h - data->menutextborder->y - data->menutextborder->h - 10 - 40, /*BOX_VISIBLE |*/ BOX_BORDER5);
	subbox->bgcol = RGB(226, 229, 212);/*TabBG2;*/
	subbox->brcol = RGB(226, 229, 212);/*RGB(226, 231, 211);*/
	Box_AddChild(dialog, subbox);
	data->movelistborder = subbox;

	subbox = Box_Create(8, 4, data->movelistborder->w - 16, 14, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->fgcol = RGB(0, 0, 0);
	subbox->font = tahoma11b_f;
	Box_SetText(subbox, _("Move List"));
	Box_AddChild(data->movelistborder, subbox);

	subbox = Box_Create(0, 22, data->movelistborder->w, 1, BOX_VISIBLE);
	subbox->bgcol = RGB(181, 184, 167);
	Box_AddChild(data->movelistborder, subbox);

	subbox = Button_Create(data->movelistborder->w - 18, 7, 9, 8, BOX_VISIBLE | BOX_TRANSPARENT);
	Button_SetOnButtonHit(subbox, ChessBoxMenu_OnToggleMoveList);
	Button_SetNormalImg (subbox, ImageMgr_GetSubImage("movelistclose1", "movelistclose.png", 0, 0, 9, 8));
	Button_SetHoverImg  (subbox, ImageMgr_GetSubImage("movelistclose1", "movelistclose.png", 0, 0, 9, 8));
	Button_SetPressedImg(subbox, ImageMgr_GetSubImage("movelistclose1", "movelistclose.png", 0, 0, 9, 8));
	Box_AddChild(data->movelistborder, subbox);
/*	
	{
		struct Box_s *subbox2;
		subbox2 = Box_Create(0, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersul", "contentcorners.png", 0, 0, 5, 5);
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, 0, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersur", "contentcorners.png", 5, 0, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(subbox->w - 5, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdr", "contentcorners.png", 5, 5, 5, 5);
		subbox2->OnSizeWidth = Box_OnSizeWidth_StickRight;
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);

		subbox2 = Box_Create(0, subbox->h - 5, 5, 5, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox2->img = ImageMgr_GetSubImage("contentcornersdl", "contentcorners.png", 0, 5, 5, 5);
		subbox2->OnSizeHeight = Box_OnSizeHeight_StickBottom;
		Box_AddChild(subbox, subbox2);
	}
*/
	/*movelist = List_Create(5, 45, drawer->w - 10, drawer->h - 22 - 5 - 5 - 45, BOX_VISIBLE, 0)*/;

	movelist = List_Create(5, 45, drawer->w - 10, drawer->h - 22 - 5 - 5 - 45, BOX_VISIBLE, 0);
	movelist->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetStripeBG1(movelist, MoveListBG1);
	List_SetStripeBG2(movelist, MoveListBG2);
	List_SetGroupSelectable(movelist, FALSE);
	List_SetEntrySelectable(movelist, 0);
	List_SetEntrySortFunc(movelist, MoveListEntry_Sort);
	Box_AddChild(drawer, movelist);
	{
		struct Box_s *entrybox, *subbox;

		entrybox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->userdata = (void *)1;

		subbox = Box_Create(5, 0, 20, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = RGB(128, 128, 128);
		Box_SetText(subbox, "1.");
		Box_AddChild(entrybox, subbox);

		List_AddEntry(movelist, NULL, NULL, entrybox);
		List_RedoEntries(movelist);
	}
	data->movelistbox = movelist;

#if 0
	movelist = List_Create(5, 25, data->movelistborder->w - 10, data->movelistborder->h - 30, BOX_VISIBLE, 0);
	movelist->OnSizeHeight = Box_OnSizeHeight_Stretch;
	List_SetStripeBG1(movelist, RGB(226, 229, 212)/*TabBG2*/);
	List_SetStripeBG2(movelist, RGB(243, 245, 234)/*TabBG3*/);
	List_SetGroupSelectable(movelist, FALSE);
	List_SetEntrySelectable(movelist, 0);
	List_SetEntrySortFunc(movelist, MoveListEntry_Sort);
	Box_AddChild(data->movelistborder, movelist);
	{
		struct Box_s *entrybox, *subbox;

		entrybox = Box_Create(0, 0, 80, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		entrybox->userdata = (void *)1;

		subbox = Box_Create(5, 0, 20, 16, BOX_VISIBLE | BOX_TRANSPARENT);
		subbox->fgcol = RGB(0, 0, 0);
		Box_SetText(subbox, "1.");
		Box_AddChild(entrybox, subbox);

		List_AddEntry(movelist, NULL, NULL, entrybox);
		List_RedoEntries(movelist);
	}
	data->movelistbox = movelist;
#endif
	subbox = TabCtrl_Create(0, 0, 190, TabHeight, BOX_TRANSPARENT, 0, 1, 0);
	subbox->bgcol = TabBG2;
	subbox->fgcol = TabFG1;
	TabCtrl_AddTab2(subbox, "menu", _("Menu"), data->menutextborder, -58, NULL, NULL, 0, NULL, NULL);
	TabCtrl_AddTab2(subbox, "moves", _("Moves"), data->movelistborder, -58, NULL, NULL, 0, NULL, NULL);
	Box_AddChild(dialog, subbox);
	data->menumovetabs = subbox;

	subbox = Box_Create(dialog->w - GrabberWidth, dialog->h - GrabberHeight, GrabberWidth, GrabberHeight, BOX_VISIBLE | BOX_TRANSPARENT);
	subbox->img = ImageMgr_GetImage("windowResizeHandle.png");
	subbox->OnSizeWidth  = Box_OnSizeWidth_StickRight;
	subbox->OnSizeHeight = Box_OnSizeHeight_StickBottom;
	Box_AddChild(dialog, subbox);

	data->drawer = drawer;

	ddata->chessbox = dialog;
	ddata->movelist = movelist;
	ddata->reversesort = 0;

	drawer->boxdata = ddata;

	if (chatw)
	{
		data->hidechat = 1;
		data->chatborder->flags &= ~BOX_VISIBLE;
		data->participantborder->flags &= ~BOX_VISIBLE;
		data->edit->flags &= ~BOX_VISIBLE;
		data->oldchatw = chatw;
		/*dialog->minw = data->gameinfobox->x + data->gameinfobox->w + 5;*/
	}

	ChessBox_OnSizeHeight(dialog, h - dialog->h);
	Box_OnSizeWidth_Stretch(dialog, w - dialog->w);

	Box_CreateWndCustom(dialog, _("Chess"), NULL);

	dialog->OnMove = ChessBox_OnMove;
	dialog->OnSizeWidth = ChessBox_OnSizeWidth;
	dialog->OnSizeHeight = ChessBox_OnSizeHeight;

	ChessBoard_UpdateBoard(data->chessboard);

	BringWindowToTop(dialog->hwnd);

	if (data->roomjid)
	{
		Box_SetFocus(data->edit);
	}

	ChessBox_ResetChatPositions(dialog);
	ChessBox_AdjustMenuAndMovelist(dialog);

	return dialog;
}

static int IsMove(char *move)
{
	char *letters = "abcdefgh";
	char *numbers = "12345678";
	char *pieces  = "pnbrqk";

	if (strchr(letters, move[0]) == NULL)
	{
		return 0;
	}

	if (strchr(numbers, move[1]) == NULL)
	{
		return 0;
	}

	if (strlen(move) == 3)
	{
		if (strchr(pieces, move[2]) == NULL)
		{
			return 0;
		}
		return 1;
	}

	if (strchr(letters, move[2]) == NULL)
	{
		return 0;
	}

	if (strchr(numbers, move[3]) == NULL)
	{
		return 0;
	}

	if (strlen(move) == 4)
	{
		return 1;
	}

	if (strchr(pieces, move[4]) == NULL)
	{
		return 0;
	}
	return 1;
}

void ChessChatBox_OnEnter(struct Box_s *pbox, char *text)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	char *gamejid;

	if (!text || strlen(text) == 0)
	{
		return;
	}

	if (!data->gameid)
	{
		return;
	}

	gamejid = malloc(strlen(data->gameid) + strlen("@games.chesspark.com") + 1);
	strcpy(gamejid, data->gameid);
	strcat(gamejid, "@games.chesspark.com");

	/* check for moves */

	if (strlen(text) >= 3 && strlen(text) <= 5)
	{
		if (stricmp(text, "O-O") == 0)
		{
			Ctrl_SendMove(data->gameid, "O-O");
			Text_SetText(data->message, _("Move sent; waiting for reply..."));
			Box_Repaint(data->message);
			Edit2Box_ClearText(pbox);
			Box_Repaint(pbox);
			return;
		}
		else	if (stricmp(text, "O-O-O") == 0)
		{
			Ctrl_SendMove(data->gameid, "O-O-O");
			Text_SetText(data->message, _("Move sent; waiting for reply..."));
			Box_Repaint(data->message);
			Edit2Box_ClearText(pbox);
			Box_Repaint(pbox);
		}
		else
		{
			char *move = strdup(text);

			strlwr(move);

			if (IsMove(move))
			{
				Ctrl_SendMove(data->gameid, text);
				Text_SetText(data->message, _("Move sent; waiting for reply..."));
				Box_Repaint(data->message);
				Edit2Box_ClearText(pbox);
				Box_Repaint(pbox);
				return;
			}
		}
	}

	/* check for commands */
	if (text && strlen(text) > 0 && text[0] == '/' && strncmp(text, "/me", 3) != 0)
	{
		char *command = strdup(text);
		char *param1 = NULL;
		char *param2 = NULL;
		char *space = command;

		while (*space != '\0' && *space != ' ')
		{
			space++;
		}

		while (*space != '\0' && *space == ' ')
		{
			space++;
		}

		if (*space != '\0' && *(space -1) == ' ')
		{
			*(space - 1) = '\0';
			param1 = space;
		}

		while (*space != '\0' && *space != ' ')
		{
			space++;
		}

		while (*space != '\0' && *space == ' ')
		{
			space++;
		}

		if (*space != '\0' && *(space -1) == ' ')
		{
			*(space - 1) = 0;
			param2 = space;
		}

		/*if (strcmp(command, "/nick") == 0)
		{
			free(tabdata->nick);
			tabdata->nick = strdup(param1);
			Ctrl_ChangeNick(tabdata->targetjid, param1);
		}
		else*/ if (param1 && strcmp(command, "/kick") == 0)
		{
			Ctrl_KickUser(gamejid, param1, param2);
		}
		else if (param1 && strcmp(command, "/ban") == 0)
		{
			Ctrl_BanUser(gamejid, param1, param2);
		}
		else if (param1 && strcmp(command, "/invite") == 0)
		{
			Ctrl_InviteUser(gamejid, param1, param2);
		}

		free(command);

		Edit2Box_ClearText(pbox);
		Box_Repaint(pbox);
		free(gamejid);
		return;
	}

	Ctrl_SendGroupMessage(gamejid, text);
	
	Edit2Box_ClearText(pbox);
	Box_Repaint(pbox);

	free(gamejid);	
}

struct Box_s *ChessBox_AddChatMessage(struct Box_s *chessbox, char *name, char *text)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *entrybox;
	struct Box_s *textbox;
	char *nick = Jid_GetResource(data->roomjid);
	char *showname;

	if (Model_IsJIDIgnored(nick))
	{
		return NULL;
	}

	entrybox = List_GetEntryBoxAllGroups(data->participantlist, name);

	if (entrybox)
	{
		char *realjid;

		realjid = ParticipantEntry_GetRealJID(entrybox);
		if (Model_IsJIDIgnored(realjid))
		{
			return NULL;
		}
		showname = ParticipantEntry_GetShowName(entrybox);

	}
	else
	{
		showname = Model_GetFriendNick(name);
	}

	textbox = SubChat_AddText2(data->chatlist, data->participantlist, name, showname, text, name && stricmp(name, nick) == 0, data->edit, NULL, NULL, 0);

	free(nick);

	return textbox;
}

void ChessBox_SetProfile(char *jid, struct profile_s *profile, struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char *jid1 = NULL;
	char *jidw = NULL;
	char *jidb = NULL;
	struct namedlist_s **allratingslist = NULL;
	struct namedlist_s *allratings = NULL;
	struct namedlist_s **ratinglist = NULL;
	struct rating_s *rating = NULL;
	struct Box_s *namebox;
	struct Box_s *avatarbox;

	char txt[120];
	struct Box_s *ratingbox;

	if (!data->info)
	{
		return;
	}

	jid1 = Jid_Strip(jid);

	if (data->info->white && data->info->white->jid)
	{
		jidw = Jid_Strip(data->info->white->jid);
	}

	if (data->info->black && data->info->black->jid)
	{
		jidb = Jid_Strip(data->info->black->jid);
	}

	if (jidw && stricmp(jid1, jidw) == 0)
	{
		ratingbox = data->whiterating;
		namebox   = data->whitename;
		avatarbox = data->whiteavatar;
	}
	else if (jidb && stricmp(jid1, jidb) == 0)
	{
		ratingbox = data->blackrating;
		namebox   = data->blackname;
		avatarbox = data->blackavatar;
	}
	else
	{
		free(jid1);
		free(jidw);
		free(jidb);
		return;
	}

	/* don't set the rating after a gameover */
	if (!data->gameover)
	{
		ratinglist = NamedList_GetByName(&(profile->ratings), data->info->category);
		if (ratinglist)
		{
			rating = (*ratinglist)->data;
		}

		if (rating)
		{
			sprintf(txt, "%d", rating->rating);
			Box_SetText(ratingbox, txt);
			Box_Repaint(ratingbox);
		}
		else
		{
			sprintf(txt, _("Unrated"));
			Box_SetText(ratingbox, txt);
			Box_Repaint(ratingbox);
		}

	}

	{
		char txt[512];
		int notfirst = 0;
		struct namedlist_s *entry;

		txt[0] = '\0';
		entry = profile->titles;

		while (entry)
		{
			if (notfirst)
			{
				strcat(txt, " ");
			}
			else
			{
				notfirst = 1;
			}

			strcat(txt, entry->data);

			entry = entry->next;
		}

		if (notfirst)
		{
			strcat(txt, " ");
		}

		strcat(txt, (profile && profile->nickname) ? profile->nickname : Model_GetFriendNick(jid));

		Box_SetText(namebox, txt);
	}

	{
		struct BoxImage_s *img = NULL;
		char *filename = NULL;
		char scaledname[512];

		if (profile && profile->avatarhash)
		{
			char buffer[MAX_PATH];
			filename = Ctrl_GetAvatarFilenameByHash(profile->avatarhash, buffer, MAX_PATH);
			img = ImageMgr_GetRootImage(filename);
		}

		if (img)
		{
			sprintf(scaledname, "%s-30x30", filename);
			img = ImageMgr_GetRootAspectScaledImage(scaledname, filename, 30, 30);
		}

		if (!img)
		{
			img = ImageMgr_GetScaledImage("DefaultAvatar50x50", "DefaultAvatar.png", 30, 30);
		}

		avatarbox->img = img;
		Box_Repaint(avatarbox);

	}

	ChessBox_RefreshTitle(chessbox);
}

void ChessBox_ShowRatingUpdate(struct Box_s *chessbox, char *jid, struct rating_s *rating)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char *jid1 = NULL;
	char *jidw = NULL;
	char *jidb = NULL;
	struct Box_s *ratingbox;
	char txt[120];
	char difftxt[120];

	jid1 = Jid_Strip(jid);

	if (!data->info)
	{
		/* can't show a rating update if no state */
		return;
	}

	if (data->info->white && data->info->white->jid)
	{
		jidw = Jid_Strip(data->info->white->jid);
	}

	if (data->info->black && data->info->black->jid)
	{
		jidb = Jid_Strip(data->info->black->jid);
	}

	if (jidw && stricmp(jid1, jidw) == 0)
	{
		ratingbox = data->whiterating;
		if (data->gameover && data->whiteratingset)
		{
			/* don't set more than once */
			return;
		}
		data->whiteratingset = 1;
	}
	else if (jidb && stricmp(jid1, jidb) == 0)
	{
		ratingbox = data->blackrating;
		if (data->gameover && data->blackratingset)
		{
			/* don't set more than once */
			return;
		}
		data->blackratingset = 1;
	}
	else
	{
		free(jid1);
		free(jidw);
		free(jidb);
		return;
	}
	
	if (!data->gameover)
	{
		/* game isn't over, silently change rating */
		if (rating)
		{
			sprintf(txt, "%d", rating->rating);
			Box_SetText(ratingbox, txt);
			Box_Repaint(ratingbox);
		}
		else
		{
			sprintf(txt, _("Unrated"));
			Box_SetText(ratingbox, txt);
			Box_Repaint(ratingbox);
		}
	}

	if (rating->rating < rating->prevrating)
	{
		sprintf(difftxt, "%d", rating->prevrating - rating->rating);
		sprintf(txt, "%d (-%s)", rating->rating, difftxt);
		Box_SetText(ratingbox, txt);
		Box_Repaint(ratingbox);
		if (ratingbox == data->whiterating)
		{
			i18n_stringsub(txt, 512, _("White's rating has gone down by %1 points."), difftxt);
			SubChat_AddSmallText(data->chatlist, txt, 0, data->edit);
		}
		else
		{
			i18n_stringsub(txt, 512, _("Black's rating has gone down by %1 points."), difftxt);
			SubChat_AddSmallText(data->chatlist, txt, 0, data->edit);
		}
	}
        else
	{
		sprintf(difftxt, "%d", rating->rating - rating->prevrating);
		sprintf(txt, "%d (+%s)", rating->rating, difftxt);
		Box_SetText(ratingbox, txt);
		Box_Repaint(ratingbox);
		if (ratingbox == data->whiterating)
		{
			i18n_stringsub(txt, 512, _("White's rating has gone up by %1 points."), difftxt);
			SubChat_AddSmallText(data->chatlist, txt, 0, data->edit);
		}
		else
		{
			i18n_stringsub(txt, 512, _("Black's rating has gone up by %1 points."), difftxt);
			SubChat_AddSmallText(data->chatlist, txt, 0, data->edit);
		}
	}
}


void ChessBox_SetGameInfo(struct Box_s *chessbox, struct gamesearchinfo_s *info, int whitelocal, int blacklocal, char *tourneyid)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->info)
	{
		if (data->info->white && data->info->white->jid)
		{
			Model_UnsubscribeProfile(data->info->white->jid, ChessBox_SetProfile, chessbox);
		}
		if (data->info->black && data->info->black->jid)
		{
			Model_UnsubscribeProfile(data->info->black->jid, ChessBox_SetProfile, chessbox);
		}
	}

	data->tourneyid = strdup(tourneyid);
	data->info = Info_DupeGameSearchInfo(info);
	data->gameid = strdup(info->gameid);

	data->whitelocal = whitelocal;
	data->blacklocal = blacklocal;

	if (whitelocal || blacklocal)
	{
		BringWindowToTop(chessbox->hwnd);
		if (!Model_GetOption(OPTION_DISABLEGAMESOUNDS))
		{
			Audio_PlayWav("sounds/start.wav");
		}
	}

	Box_SetText(data->whitename, Model_GetFriendNick(info->white->jid));
	Box_SetText(data->blackname, Model_GetFriendNick(info->black->jid));

	Box_Repaint(data->whitename);
	Box_Repaint(data->blackname);

	ChessBox_RefreshTitle(chessbox);

	Model_SubscribeProfileAndAvatar(info->white->jid, ChessBox_SetProfile, chessbox);
	Model_SubscribeProfileAndAvatar(info->black->jid, ChessBox_SetProfile, chessbox);
/*
	Ctrl_RequestProfile(info->white->jid);
	Ctrl_RequestProfile(info->black->jid);
*/
	if (!whitelocal && !blacklocal)
	{
		ChessBox_AddChatMessage(chessbox, NULL, _("Reminder: Players do not receive observer messages, only other observers do."));
	}

	/* FIXME - hack, close all gameover games with the same players */
	View_CloseGamesWithSamePlayers(data->gameid, data->info->white->jid, data->info->black->jid);

	if (data->info->black->jid && data->rotateifblackjid)
	{
		char *barejid1 = Jid_Strip(data->info->black->jid);
		char *barejid2 = Jid_Strip(data->rotateifblackjid);

		if (stricmp(barejid1, barejid2) == 0)
		{
			ChessBox_SetGameViewRotated(chessbox, 1);
		}
	}

	ChessBox_UpdateControlMsg(chessbox);
	ChessBox_AdjustMenuAndMovelist(chessbox);
	ChessBoard_UpdateBoard(data->chessboard);
	MoveList_Update(data->movelistbox);
}

void ChessBox_CloseIfSamePlayers(struct Box_s *chessbox, char *newgameid, char *player1, char *player2)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char *gamejid1, *gamejid2, *closejid1, *closejid2;

	if (!data->info)
	{
		return;
	}

	if (!data->gameover || data->info->correspondence)
	{
		return;
	}

	if (strcmp(data->gameid, newgameid) == 0)
	{
		return;
	}

	gamejid1 = Jid_Strip(data->info->white->jid);
	gamejid2 = Jid_Strip(data->info->black->jid);
	closejid1 = Jid_Strip(player1);
	closejid2 = Jid_Strip(player2);

	if (((stricmp(gamejid1, closejid1) == 0) && (stricmp(gamejid2, closejid2) == 0))
		|| ((stricmp(gamejid1, closejid2) == 0) && (stricmp(gamejid2, closejid1) == 0)))
	{
		View_CloseChessGame(data->gameid);	
	}
}


void ChessBox_OnRematch(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	if (Model_IsLocalMemberFree(0) && (timeGetTime() - data->gameovertime < 5000))
	{
		return;
	}

	/* put the piece images in the cache now, because the new board will appear before the old one is closed */
	ChessBoard_CacheImages(data->chessboard);

	Text_SetText(data->message, _("Rematch request sent.  Hang on a sec."));
	Box_Repaint(data->message);

	Ctrl_RequestRematch(data->gameid);

	/*
	if (data->whitelocal)
	{
		Text_SetText(data->message, "Rematch request sent.  Hang on a sec.");
		Box_Repaint(data->message);

		Ctrl_RequestRematch(data->blackjid);
	}
	else if (data->blacklocal)
	{
		Text_SetText(data->message, "Rematch request sent.  Hang on a sec.");
		Box_Repaint(data->message);

		Ctrl_RequestRematch(data->whitejid);
	}
	*/
}

void ChessBox_ClearMessage(struct Box_s *pbox)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Text_SetText(data->message, NULL);

	Box_Repaint(data->message);

}

void ChessBox_OnAdjournCancel(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;
	Text_SetText(data->message, _("Adjourn request cancel request sent."));
	Box_Repaint(data->message);

	Ctrl_RejectGameAdjourn(data->gameid, data->roomjid == NULL);
	data->cancelledadjourn = 1;
}

void ChessBox_OnAdjournAccept(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameAdjourn(data->gameid, data->roomjid == NULL);
	data->acceptedadjourn = 1;
}

void ChessBox_OnAdjournDecline(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_RejectGameAdjourn(data->gameid, data->roomjid == NULL);
	data->declinedadjourn = 1;
}

void ChessBox_HandleAdjourn(struct Box_s *chessbox, char *white, char *black)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char finaltxt[256];

	if ((white && !black && data->blacklocal) || (black && !white && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 %2", _("Your opponent has requested to adjourn the game."), _("Do you ^laccept^l or ^ldecline^l?"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnAdjournAccept, NULL);
		Text_SetLinkCallback(data->message, 2, ChessBox_OnAdjournDecline, NULL);
	}
	else if ((!white && black && data->blacklocal) || (!black && white && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 ^l%2^l", _("You have requested to adjourn this game."), _("Cancel Request"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnAdjournCancel, NULL);

		data->requestedadjourn = 1;
	}
	else
	{
		if (white && !black)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested to adjourn the game."), _("White"));
			Text_SetText(data->message, finaltxt);
		}
		else if (!white && black)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested to adjourn the game."), _("Black"));
			Text_SetText(data->message, finaltxt);
		}
	}

	if (white && !black)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested to adjourn the game."), _("White"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}
	else if (!white && black)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested to adjourn the game."), _("Black"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}

	Box_Repaint(data->message);
}

void ChessBox_OnAbortCancel(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;
	Text_SetText(data->message, _("Abort request cancel request sent."));
	Box_Repaint(data->message);

	Ctrl_RejectGameAbort(data->gameid, data->roomjid == NULL);
	data->cancelledabort = 1;
}

void ChessBox_OnAbortAccept(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameAbort(data->gameid, data->roomjid == NULL);
	data->acceptedabort = 1;
}

void ChessBox_OnAbortDecline(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_RejectGameAbort(data->gameid, data->roomjid == NULL);
	data->declinedabort = 1;
}


void ChessBox_HandleAbort(struct Box_s *chessbox, char *white, char *black)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char finaltxt[256];

	if ((white && !black && data->blacklocal) || (black && !white && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 %2", _("Your opponent has requested to abort the game."), _("Do you ^laccept^l or ^ldecline^l?"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnAbortAccept, NULL);
		Text_SetLinkCallback(data->message, 2, ChessBox_OnAbortDecline, NULL);
	}
	else if ((!white && black && data->blacklocal) || (!black && white && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 ^l%2^l", _("You have requested to abort this game."), _("Cancel Request"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnAbortCancel, NULL);

		data->requestedabort = 1;
	}
	else
	{
		if (white && !black)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested to abort the game."), _("White"));
			Text_SetText(data->message, finaltxt);
		}
		else if (!white && black)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested to abort the game."), _("Black"));
			Text_SetText(data->message, finaltxt);
		}
	}

	if (white && !black)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested to abort the game."), _("White"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}
	else if (!white && black)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested to abort the game."), _("Black"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}

	Box_Repaint(data->message);
}


void ChessBox_OnTakebackCancel(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;
	Text_SetText(data->message, _("Takeback request cancel request sent."));
	Box_Repaint(data->message);

	Ctrl_RejectGameTakeback(data->gameid, data->roomjid == NULL);
	data->cancelledtakeback = 1;
}

void ChessBox_OnTakebackAccept(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameTakeback(data->gameid, data->roomjid == NULL);
	data->acceptedtakeback = 1;
}

void ChessBox_OnTakebackDecline(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_RejectGameTakeback(data->gameid, data->roomjid == NULL);
	data->declinedtakeback = 1;
}


void ChessBox_HandleTakeback(struct Box_s *chessbox, char *white, char *black)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char finaltxt[256];

	if ((white && !black && data->blacklocal) || (black && !white && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 %2", _("Your opponent has requested to take back a move."), _("Do you ^laccept^l or ^ldecline^l?"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnTakebackAccept, NULL);
		Text_SetLinkCallback(data->message, 2, ChessBox_OnTakebackDecline, NULL);
	}
	else if ((!white && black && data->blacklocal) || (!black && white && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 ^l%2^l", _("You have requested to take back a move."), _("Cancel Request"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnTakebackCancel, NULL);

		data->requestedtakeback = 1;
	}
	else
	{
		if (white && !black)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested to take back a move."), _("White"));
			Text_SetText(data->message, finaltxt);
		}
		else if (!white && black)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested to take back a move."), _("Black"));
			Text_SetText(data->message, finaltxt);
		}
	}

	if (white && !black)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested to take back a move."), _("White"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}
	else if (!white && black)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested to take back a move."), _("Black"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}

	Box_Repaint(data->message);
}

void ChessBox_OnDrawCancel(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	return;
	/*
	Text_SetText(data->message, _("Draw request cancel request sent."));
	Box_Repaint(data->message);

	Ctrl_RejectGameDraw(data->gameid, data->roomjid == NULL);
	data->cancelleddraw = 1;
	*/
}

void ChessBox_OnDrawAccept(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_SendGameDraw(data->gameid, data->roomjid == NULL);
	data->accepteddraw = 1;
}

void ChessBox_OnDrawDecline(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	Ctrl_RejectGameDraw(data->gameid, data->roomjid == NULL);
	data->declineddraw = 1;
}

void ChessBox_HandleDraw(struct Box_s *chessbox, int whiteaccept, int blackaccept)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char finaltxt[256];

	if ((whiteaccept && !blackaccept && data->blacklocal) || (blackaccept && !whiteaccept && data->whitelocal))
	{
		i18n_stringsub(finaltxt, 256, "%1 %2", _("Your opponent has requested a draw."), _("Do you ^laccept^l or ^ldecline^l?"));

		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnDrawAccept, NULL);
		Text_SetLinkCallback(data->message, 2, ChessBox_OnDrawDecline, NULL);
	}
	else if ((!whiteaccept && blackaccept && data->blacklocal) || (!blackaccept && whiteaccept && data->whitelocal))
	{
		/*
		i18n_stringsub(finaltxt, 256, "%1 ^l%2^l", _("You have requested a draw."), _("Cancel Request"));
		*/
		i18n_stringsub(finaltxt, 256, "%1", _("You have requested a draw."));

		Text_SetText(data->message, finaltxt);
		/*
		Text_SetLinkCallback(data->message, 1, ChessBox_OnDrawCancel, NULL);
		*/

		data->requesteddraw = 1;
	}
	else
	{
		if (whiteaccept && !blackaccept)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested a draw"), _("White"));
			Text_SetText(data->message, finaltxt);
		}
		else if (!whiteaccept && blackaccept)
		{
			i18n_stringsub(finaltxt, 256, _("%1 has requested a draw"), _("Black"));
			Text_SetText(data->message, finaltxt);
		}
	}

	if (whiteaccept && !blackaccept)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested a draw"), _("White"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}
	else if (!whiteaccept && blackaccept)
	{
		i18n_stringsub(finaltxt, 256, _("%1 has requested a draw"), _("Black"));
		SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);
	}

	Box_Repaint(data->message);
}

void ChessBox_HandleRejectDraw(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->cancelleddraw)
	{
		Text_SetText(data->message, _("Your draw request was cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Draw request cancelled."), 0, data->edit);
	}
	else if (data->requesteddraw)
	{
		Text_SetText(data->message, _("Your opponent has declined your request for a draw."));
		data->requesteddraw = 0;
		SubChat_AddSmallText(data->chatlist, _("Draw request declined."), 0, data->edit);
	}
	else if (data->declineddraw)
	{
		Text_SetText(data->message, _("You have declined your opponent's request for a draw."));
		SubChat_AddSmallText(data->chatlist, _("Draw request declined."), 0, data->edit);
	}
	else if (data->whitelocal || data->blacklocal)
	{
		Text_SetText(data->message, _("Your opponent has cancelled the draw request."));
		SubChat_AddSmallText(data->chatlist, _("Draw request cancelled."), 0, data->edit);
	}
	else
	{
		Text_SetText(data->message, _("Draw request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Draw request cancelled."), 0, data->edit);
	}

	data->accepteddraw = 0;
	data->requesteddraw = 0;
	data->declineddraw = 0;
	data->cancelleddraw = 0;

	Box_Repaint(data->message);
}


void ChessBox_HandleRejectAdjourn(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->cancelledadjourn)
	{
		Text_SetText(data->message, _("Adjourn request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Adjourn request cancelled."), 0, data->edit);
	}
	else if (data->requestedadjourn)
	{
		Text_SetText(data->message, _("Your opponent has declined your request to adjourn."));
		SubChat_AddSmallText(data->chatlist, _("Adjourn request declined."), 0, data->edit);
	}
	else if (data->declinedadjourn)
	{
		Text_SetText(data->message, _("You have declined your opponent's request to adjourn."));
		SubChat_AddSmallText(data->chatlist, _("Adjourn request declined."), 0, data->edit);
	}
	else
	{
		Text_SetText(data->message, _("Adjourn request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Adjourn request cancelled."), 0, data->edit);
	}

	data->acceptedadjourn = 0;
	data->requestedadjourn = 0;
	data->declinedadjourn = 0;
	data->cancelledadjourn = 0;

	Box_Repaint(data->message);
}

void ChessBox_HandleRejectAbort(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->cancelledabort)
	{
		Text_SetText(data->message, _("Abort request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Abort request cancelled."), 0, data->edit);
	}
	else if (data->requestedabort)
	{
		Text_SetText(data->message, _("Your opponent has declined your request to abort."));
		SubChat_AddSmallText(data->chatlist, _("Abort request declined."), 0, data->edit);
	}
	else if (data->declinedabort)
	{
		Text_SetText(data->message, _("You have declined your opponent's request to abort."));
		SubChat_AddSmallText(data->chatlist, _("Abort request declined."), 0, data->edit);
	}
	else
	{
		Text_SetText(data->message, _("Abort request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Abort request cancelled."), 0, data->edit);
	}

	data->acceptedabort = 0;
	data->requestedabort = 0;
	data->declinedabort = 0;
	data->cancelledabort = 0;

	Box_Repaint(data->message);
}

void ChessBox_HandleRejectTakeback(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->cancelledtakeback)
	{
		Text_SetText(data->message, _("Take back request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Take back request cancelled."), 0, data->edit);
	}
	else if (data->requestedtakeback)
	{
		Text_SetText(data->message, _("Your opponent has declined your request to take back a move."));
		SubChat_AddSmallText(data->chatlist, _("Take back request declined."), 0, data->edit);
	}
	else if (data->declinedabort)
	{
		Text_SetText(data->message, _("You have declined your opponent's request to take back a move."));
		SubChat_AddSmallText(data->chatlist, _("Take back request declined."), 0, data->edit);
	}
	else
	{
		Text_SetText(data->message, _("Take back request cancelled."));
		SubChat_AddSmallText(data->chatlist, _("Take back request cancelled."), 0, data->edit);
	}

	data->acceptedtakeback = 0;
	data->requestedtakeback = 0;
	data->declinedtakeback = 0;
	data->cancelledtakeback = 0;

	Box_Repaint(data->message);
}

void ChessBox_RefreshTitle(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char txt[256];
	char tourneytxt[80];
	char gametxt[80];
	char varianttxt[80];
	char movetxt[80];
	char playingtxt[80];
	char numconvert[5];
	int whitelocal, blacklocal;

	whitelocal = data->whitelocal;
	blacklocal = data->blacklocal;

	i18n_stringsub(gametxt, 80, _("Game %1"), data->gameid);

	sprintf(numconvert, "%d", (data->movenum + 1) / 2);

	if (data->info)
	{
		if (data->info->rated)
		{
			strcpy(varianttxt, _("Rated"));
		}
		else
		{
			strcpy(varianttxt, _("Unrated"));
		}

		strcat(varianttxt, " ");

		strcat(varianttxt, Info_TimeControlToCategory(data->info->timecontrol));

		if (data->info->variant && stricmp(data->info->variant, "Standard") != 0)
		{
			strcat(varianttxt, " ");
			if (stricmp(data->info->variant, "chess960") == 0)
			{
				strcat(varianttxt, _("Chess960"));
			}
			else if (stricmp(data->info->variant, "atomic") == 0)
			{
				strcat(varianttxt, _("Atomic"));
			}
			else if (stricmp(data->info->variant, "losers") == 0)
			{
				strcat(varianttxt, _("Loser's"));
			}
			else if (stricmp(data->info->variant, "crazyhouse") == 0)
			{
				strcat(varianttxt, _("Crazyhouse"));
			}
			else if (stricmp(data->info->variant, "checkers") == 0)
			{
				strcat(varianttxt, _("Checkers"));
			}
			else
			{
				strcat(varianttxt, _("Standard"));
			}
		}
	}
	else
	{
		strcpy(varianttxt, _("Standard"));
	}

	i18n_stringsub(movetxt, 80, _("Move #%1"), numconvert);

	if (whitelocal && !blacklocal)
	{
		i18n_stringsub(playingtxt, 80, _("Playing: %1"), data->blackname->text);
	}
	else if (!whitelocal && blacklocal)
	{
		i18n_stringsub(playingtxt, 80, _("Playing: %1"), data->whitename->text);
	}
	else
	{
		i18n_stringsub(playingtxt, 80, _("Observing: %1 vs. %2"), data->whitename->text, data->blackname->text);
	}

	if (data->tourneyid)
	{
		i18n_stringsub(tourneytxt, 80, _("Tournament %1"), data->tourneyid);

		sprintf(txt, "%s - %s - %s - %s - %s", tourneytxt, varianttxt, gametxt, movetxt, playingtxt);
	}
	else
	{
		sprintf(txt, "%s - %s - %s - %s", gametxt, varianttxt, movetxt, playingtxt);
	}

	TitleBar_SetText(chessbox->titlebar, txt);

	SetWindowText(chessbox->hwnd, txt);

	Box_Repaint(chessbox->titlebar);
}

void ChessBox_HandleGameOver(struct Box_s *chessbox, char *type, char *win, char *lose, char *reason)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	int whitewon, blackwon, whitelocal, blacklocal;
	char finaltxt[512];
	char gameoverstatetxt[256];
	char overtxt[256];

	data->gameovertime = timeGetTime();

	whitewon = win && (strcmp(win, "white") == 0);
	blackwon = win && (strcmp(win, "black") == 0);
	whitelocal = data->whitelocal;
	blacklocal = data->blacklocal;

	if (Model_IsLocalMemberFree(0) && (whitelocal || blacklocal))
	{
		View_AdpopDelay();
	}

	if (type && stricmp(type, "time") == 0)
	{
		ChessBox_SetVisibleClock(chessbox, 0, blackwon);
	}

	data->gameover = 1;

	ChessBox_AdjustMenuAndMovelist(chessbox);

	data->whiteclock->fgcol = FADED_WHITE_CLOCK_COLOR;
	data->blackclock->fgcol = FADED_BLACK_CLOCK_COLOR;
	data->whitedot->img = ImageMgr_GetImage("inactive.png");
	data->blackdot->img = ImageMgr_GetImage("inactive.png");
	Box_Repaint(data->whitetimebox);
	Box_Repaint(data->blacktimebox);

	Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);

	if (data->promotionbox)
	{
		EnableWindow(chessbox->hwnd, 1);
		Box_Destroy(data->promotionbox);
		data->promotionbox = NULL;
	}

	if (!type)
	{
		return;
	}

	if (whitewon)
	{
		if (whitelocal)
		{
			i18n_stringsub(overtxt, 256, _("You win."));
		}
		else if (blacklocal)
		{
			i18n_stringsub(overtxt, 256, _("You lose."));
		}
		else
		{
			i18n_stringsub(overtxt, 256, _("White wins."));
		}
	}
	else if (blackwon)
	{
		if (whitelocal)
		{
			i18n_stringsub(overtxt, 256, _("You lose."));
		}
		else if (blacklocal)
		{
			i18n_stringsub(overtxt, 256, _("You win."));
		}
		else
		{
			i18n_stringsub(overtxt, 256, _("Black wins."));
		}
	}
	else if (!type || (type && strcmp(type, "adjourned") != 0))
	{
		i18n_stringsub(overtxt, 256, _("Game over."));
	}
	else
	{
		overtxt[0] = '\0';
	}

	if (type && strcmp(type, "checkmate") == 0)
	{
		i18n_stringsub(gameoverstatetxt, 256, _("Checkmate."));
	}
	else if (type && strcmp(type, "stalemate") == 0)
	{
		i18n_stringsub(gameoverstatetxt, 256, _("Stalemate."));
		i18n_stringsub(overtxt, 256, _("Game is a draw."));
	}
	else if (type && strcmp(type, "resigned") == 0)
	{
		if (reason && stricmp(reason, "autoresign") == 0)
		{
			if ((whitelocal && whitewon) || (blacklocal && blackwon))
			{
				i18n_stringsub(gameoverstatetxt, 256, _("Your opponent has been forfeited."));
			}
			else if ((whitelocal && blackwon) || (blacklocal && whitewon))
			{
				i18n_stringsub(gameoverstatetxt, 256, _("You have been forfeited."));
			}
			else
			{
				i18n_stringsub(gameoverstatetxt, 256, "%1 has been forfeited.", whitewon ? _("Black") : _("White"));
			}
		}
		else if ((whitelocal && whitewon) || (blacklocal && blackwon))
		{
			i18n_stringsub(gameoverstatetxt, 256, _("Your opponent has resigned."));
		}
		else if ((whitelocal && blackwon) || (blacklocal && whitewon))
		{
			i18n_stringsub(gameoverstatetxt, 256, _("You have resigned."));
		}
		else
		{
			i18n_stringsub(gameoverstatetxt, 256, "%1 has resigned.", whitewon ? _("Black") : _("White"));
		}
	}
	else if (type && strcmp(type, "adjourned") == 0)
	{
		i18n_stringsub(overtxt, 256, _("Game adjourned."));

		if (reason && stricmp(reason, "autoadjourn") == 0)
		{
			i18n_stringsub(gameoverstatetxt, 256, _("Game has been automatically adjourned by the server."));
		}
		else if ((blacklocal || whitelocal) && data->requestedadjourn)
		{
			i18n_stringsub(gameoverstatetxt, 256, _("Your opponent has agreed to adjourn this game."));
		}
		else if ((blacklocal || whitelocal) && data->acceptedadjourn)
		{
			i18n_stringsub(gameoverstatetxt, 256, _("You have agreed to adjourn this game."));
		}
		else
		{
			if (blacklocal || whitelocal)
			{
				i18n_stringsub(gameoverstatetxt, 256, _("Opponent disconnected."));
			}
			else
			{
				i18n_stringsub(gameoverstatetxt, 256, _("The players have agreed to adjourn this game."));
			}
		}
	}
	else if (type && strcmp(type, "abort") == 0)
	{
		i18n_stringsub(gameoverstatetxt, 256, _("Game aborted."));
	}
	else if (type && strcmp(type, "time") == 0)
	{
		i18n_stringsub(gameoverstatetxt, 256, _("Time."));
	} 
	else if (type && strcmp(type, "draw") == 0)
	{
		if (reason && stricmp(reason, "agreed") == 0)
		{
			if ((whitelocal || blacklocal) && data->requesteddraw)
			{
				i18n_stringsub(gameoverstatetxt, 256, _("Your opponent has accepted your request for a draw."));
			}
			else if ((whitelocal || blacklocal) && data->accepteddraw)
			{
				i18n_stringsub(gameoverstatetxt, 256, _("You have agreed that this game is a draw."));
			}
			else
			{
				i18n_stringsub(gameoverstatetxt, 256, _("The players have agreed that this game is a draw."));
			}
		}
		else /*if (reason && stricmp(reason, "autodraw") == 0)*/
		{
			i18n_stringsub(gameoverstatetxt, 256, _("Game is automatically drawn."));
		}
	}
	else
	{
		gameoverstatetxt[0] = '\0';
	}

	if (overtxt[0])
	{
		sprintf(finaltxt, "%s %s", gameoverstatetxt, overtxt);
	}
	else
	{
		sprintf(finaltxt, "%s", gameoverstatetxt);
	}

	SubChat_AddSmallText(data->chatlist, finaltxt, 0, data->edit);

	if ((whitelocal || blacklocal) && type && strcmp(type, "adjourned") != 0 && !data->tourneyid)
	{
		i18n_stringsub(finaltxt, 512, _("%1 %2 ^lRematch?^l"), gameoverstatetxt, overtxt);
		Text_SetText(data->message, finaltxt);
		Text_SetLinkCallback(data->message, 1, ChessBox_OnRematch, NULL);
	}
	else
	{
		Text_SetText(data->message, finaltxt);
	}

	if (data->info && !data->info->rated)
	{
		SubChat_AddSmallText(data->chatlist, _("This game was unrated, so the player ratings have not changed."), 0, data->edit);
	}
	else if (type && stricmp(type, "abort") == 0)
	{
		SubChat_AddSmallText(data->chatlist, _("This game was aborted, so the player ratings have not changed."), 0, data->edit);
	}

	Box_Repaint(data->message);

	ChessBoard_UpdateBoard(data->chessboard);
}

void ChessBox_OnClickPlayer(struct Box_s *playerbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(playerbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->info && data->info->white->jid && playerbox == data->whitename)
	{
		Ctrl_ShowProfile(data->info->white->jid);
	}
	else if (data->info && data->info->black->jid && playerbox == data->blackname)
	{
		Ctrl_ShowProfile(data->info->black->jid);
	}
}

void ChessBox_SetParticipantStatus(struct Box_s *chessbox, char *targetjid,
  char *name, enum SStatus status, char *statusmsg, char *role,
  char *affiliation, char *realjid, char *nickchange,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist,
  int notactivated, char *membertype)
{
	struct Box_s *entrybox;
	struct chessboxdata_s *data = chessbox->boxdata;
	char txt[256];
	char *groupname;
	char *resource;

	/* ignore the arbiter */
	if (realjid && stricmp(realjid, "arbiter.chesspark.com") == 0)
	{
		return;
	}

	resource = Jid_GetResource(name);

	if (resource && strcmp(resource, data->nick) == 0)
	{
		data->isinroom = (status != SSTAT_OFFLINE);
		data->localnotactivated = notactivated;
	}

	if (role && stricmp(role, "participant") == 0)
	{
		groupname = _("Observers");
	}
	else if (role && stricmp(role, "moderator") == 0)
	{
		groupname = _("Moderators");
	}
	else if (role && stricmp(role, "player") == 0)
	{
		/*groupname = NULL;*/
		groupname = _("Players");
	}
	else if (role)
	{
		groupname = role;
	}
	else
	{
		groupname = _("Observers");
	}

	entrybox = List_GetEntryBoxAllGroups(data->participantlist, Jid_GetResource(name));

	if (status == SSTAT_OFFLINE)
	{
		if (nickchange)
		{
			i18n_stringsub(txt, 256, _("%1 is now known as %2"), name, nickchange);
			ChessBox_AddChatMessage(chessbox, NULL, txt);

			/* add a temp (fake) entry so we don't show the entering message again */
			entrybox = ParticipantEntry_Create(targetjid, nickchange, SSTAT_AVAILABLE, statusmsg, role, affiliation, realjid, 1, NULL, NULL, 0, NULL);

			if (!List_GetGroupBox(data->participantlist, groupname))
			{
				List_AddGroup(data->participantlist, groupname);
			}

			List_AddEntry(data->participantlist, nickchange, groupname, entrybox);
		}
		else
		{
			/* show warning if player left */
			if (!data->gameover && entrybox && (data->blacklocal || data->whitelocal))
			{
				char *entryrole = ParticipantEntry_GetRole(entrybox);
				if (entryrole && stricmp(entryrole, "player") == 0)
				{
					ChessBox_AddChatMessage(chessbox, NULL, "Reminder: If your opponent disconnects and does not reconnect within the timeout period, they automatically forfeit.");
				}
			}
			/*i18n_stringsub(txt, 256, _("%1 has left the room"), name);
			ChessBox_AddChatMessage(chessbox, NULL, txt);*/
		}
	}
	else if (!entrybox)
	{
		if (stricmp(resource, data->nick) == 0)
		{
			data->initialconnect = 1;
		}
		/*i18n_stringsub(txt, 256, _("%1 has entered the room"), name);
		ChessBox_AddChatMessage(chessbox, NULL, txt);*/
	}

	List_RemoveEntryByNameAllGroups(data->participantlist, Jid_GetResource(name));

	if (status == SSTAT_OFFLINE)
	{
		List_RedoEntries(data->participantlist);
		Box_Repaint(data->participantlist);
		return;
	}

	entrybox = ParticipantEntry_Create(targetjid, Jid_GetResource(name), status, statusmsg, role, affiliation, realjid, 1, roleslist, titleslist, notactivated, membertype);

	if (!List_GetGroupBox(data->participantlist, groupname))
	{
		List_AddGroup(data->participantlist, groupname);
	}

	List_AddEntry(data->participantlist, Jid_GetResource(name), groupname, entrybox);

	List_RedoEntries(data->participantlist);
	Box_Repaint(data->participantlist);
}

void ChessBox_SetDisconnect(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	data->disconnected = 1;
	data->isinroom = 0;
}

void ChessBox_ShowDisconnect(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	Text_SetText(data->message, _("You have been disconnected."));
	Box_Repaint(data->message);

	data->whiteclock->fgcol = FADED_WHITE_CLOCK_COLOR;
	data->blackclock->fgcol = FADED_BLACK_CLOCK_COLOR;
	data->whitedot->img = ImageMgr_GetImage("inactive.png");
	data->blackdot->img = ImageMgr_GetImage("inactive.png");
	data->whitedelay->w =     0;
	data->blackdelay->w =     0;
	Box_Repaint(data->whitetimebox);
	Box_Repaint(data->blacktimebox);
	Box_RemoveTimedFunc(chessbox, ChessBox_UpdateTime, data->lastperiod);
	data->lastperiod = 0;
	Box_RemoveTimedFunc(data->whitedelay, TimeDelay_ShrinkDelay, 100);
	Box_RemoveTimedFunc(data->blackdelay, TimeDelay_ShrinkDelay, 100);
}

void ChessBox_Reconnect(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->disconnected)
	{
		Text_SetText(data->message, NULL);
		Box_Repaint(data->message);

		data->disconnected = 0;
	}

	data->gameover = 0;

	ChessBoard_UpdateBoard(data->chessboard);
}

void ChessBox_SetRematchError(struct Box_s *chessbox, char *error, char *actor)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char txt[512];

	if (error)
	{
		if (stricmp(error, "offline") == 0)
		{
			if (actor && actor[0])
			{
				i18n_stringsub(txt, 512, _("Rematch cancelled.\n%1 is offline."), Model_GetFriendNick(actor));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Rematch cancelled.\nMember is offline."));
			}
		}
		else if (stricmp(error, "reject") == 0)
		{
			if (actor && actor[0])
			{
				i18n_stringsub(txt, 512, _("Rematch declined by %1."), Model_GetFriendNick(actor));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Rematch declined."));
			}
		}
		else if (stricmp(error, "playing") == 0)
		{
			if (actor && actor[0])
			{
				i18n_stringsub(txt, 512, _("Rematch cancelled.\n%1 is playing a game."), Model_GetFriendNick(actor));
			}
			else
			{
				i18n_stringsub(txt, 512, _("Rematch cancelled.\nMember is playing a game."));
			}
		}
		else if (stricmp(error, "badrequest") == 0)
		{
			i18n_stringsub(txt, 512, _("Bad Request."));
		}
		else if (stricmp(error, "internalservice") == 0)
		{
			i18n_stringsub(txt, 512, _("Internal service error."));
		}
		else
		{
			i18n_stringsub(txt, 512, _("Unknown error: %1"), error);
		}
	}
	else
	{
		i18n_stringsub(txt, 512, _("Unknown error."));
	}

	Text_SetText(data->message, txt);

	Box_Repaint(data->message);
}

void ChessBox_OnRematchAccept(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;

	/* put the piece images in the cache now, because the new board will appear before the old one is closed */
	ChessBoard_CacheImages(data->chessboard);

	Ctrl_AcceptRematch(data->newgameid, data->gameid);
}

void ChessBox_OnRematchDecline(struct Box_s *pbox, void *userdata)
{
	struct Box_s *chessbox = Box_GetRoot(pbox);
	struct chessboxdata_s *data = chessbox->boxdata;
	char finaltxt[256];

	Ctrl_DeclineRematch(data->newgameid, data->gameid);

	sprintf(finaltxt, _("Rematch declined."));

	Text_SetText(data->message, finaltxt);

	Box_Repaint(data->message);
}

void ChessBox_ShowRematchRequest(struct Box_s *chessbox, struct gamesearchinfo_s *info)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	char finaltxt[256];

	i18n_stringsub(finaltxt, 256, "%1 %2", _("Your opponent requests a rematch."), _("Do you ^laccept^l or ^ldecline^l?"));
	Text_SetText(data->message, finaltxt);
	Text_SetLinkCallback(data->message, 1, ChessBox_OnRematchAccept, NULL);
	Text_SetLinkCallback(data->message, 2, ChessBox_OnRematchDecline, NULL);

	data->newgameid = strdup(info->gameid);

	Box_Repaint(data->message);
}

struct gamesearchinfo_s *ChessBox_GetGameInfo(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	return data->info;
}

void ChessBox_SetStatusOnChat(struct Box_s *chessbox, enum SStatus status, char *statusmsg)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (data->roomjid && !data->disconnected)
	{
		Ctrl_SendPresence(status, statusmsg, data->roomjid, 1, NULL, 0, 0);
	}
}

void ChessBox_ShowError(struct Box_s *chessbox, int icode)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (icode == 405 && data->localnotactivated)
	{
		struct Box_s *textbox;

		textbox = ChessBox_AddChatMessage(chessbox, NULL, _("Message blocked.  To chat in public rooms, please confirm your email address.  For more information, click ^lhere^l"));
		Text_SetLinkCallback(textbox, 1, Util_OpenURL2, "http://www.chesspark.com/help/howtoconfirm/");
	}
	else
	{
		char txt[512];

		sprintf(txt, "ERROR: %d", icode);
		ChessBox_AddChatMessage(chessbox, NULL, txt);
	}
}

int ChessBox_IsInRoom(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	return data->initialconnect;
}

int ChessBox_IsPlaying(struct Box_s *chessbox)
{
	struct chessboxdata_s *data = chessbox->boxdata;

	if (!data->gameover && (data->whitelocal || data->blacklocal))
	{
		return 1;
	}

	return 0;
}

void ChessBox_SetTheme(struct Box_s *chessbox, char *piecetheme)
{
	struct chessboxdata_s *data = chessbox->boxdata;
	struct Box_s *board = data->chessboard;
	struct chessboarddata_s *bdata = board->boxdata;
	char *piecethemepng = "pieces.png";

	if (piecetheme && !Model_IsLocalMemberFree(0))
	{
		if (stricmp(piecetheme, "fantasy") == 0)
		{
			piecethemepng = "pieces-fantasy.png";
		}
		else if (stricmp(piecetheme, "spatial") == 0)
		{
                        piecethemepng = "pieces-spatial.png";
		}
		else if (stricmp(piecetheme, "skulls") == 0)
		{
                        piecethemepng = "pieces-skulls.png";
		}
		else if (stricmp(piecetheme, "eyes") == 0)
		{
                        piecethemepng = "pieces-eyes.png";
		}
	}

	/* First, clear out any visible pieces so we don't end up with dangling references */

	while (data->whitecapturebox->child)
	{
		Box_Destroy(data->whitecapturebox->child);
	}

	while (data->blackcapturebox->child)
	{
		Box_Destroy(data->blackcapturebox->child);
	}

	while (board->child)
	{
		Box_Destroy(board->child);
	}

	if (data->promotionbox)
	{
		EnableWindow(chessbox->hwnd, 1);
		Box_Destroy(data->promotionbox);
		data->promotionbox = NULL;
	}

	/* Next, clear the piece image caches */
	BoxImage_Destroy(cachedpiecesimg);
	BoxImage_Destroy(cacheddimpiecesimg);
	BoxImage_Destroy(cachedhighlightpiecesimg);
	BoxImage_Destroy(cachedcapturepiecesimg);

	BoxImage_Destroy(promotionpieces);
	BoxImage_Destroy(capturedpiecesimg);

	cachedpiecesimg = NULL;
	cacheddimpiecesimg = NULL;
	cachedhighlightpiecesimg = NULL;
	cachedcapturepiecesimg = NULL;
	cachedpieceheight = 0;

	promotionpieces = NULL;
	capturedpiecesimg = NULL;

	BoxImage_Destroy(bdata->piecesimg);
	BoxImage_Destroy(bdata->dimpiecesimg);
	BoxImage_Destroy(bdata->highlightpiecesimg);
	BoxImage_Destroy(bdata->capturepiecesimg);
	bdata->piecesimg = NULL;
	bdata->dimpiecesimg = NULL;
	bdata->highlightpiecesimg = NULL;
	bdata->capturepiecesimg = NULL;


	/* Next, set the current theme and call the updates, and all the correct images should load up */

	free(data->piecethemepng);
	data->piecethemepng = strdup(piecethemepng);

	ChessBoard_UpdateBoard(board);
	ChessBox_UpdateCaptures(chessbox);
}
