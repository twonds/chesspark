#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

extern const int Margin;
extern const int ContentMargin;
extern const int TitleBarHeight;
extern const int ButtonWidth;
extern const int ButtonHeight;
extern const int PresenceWidth;
extern const int PresenceHeight;
extern const int AvatarWidth;
extern const int AvatarHeight;
extern const int TextHeight;
extern const int TabHeight;
extern const int GrabberWidth;
extern const int GrabberHeight;

extern const int LogoWidth;
extern const int LogoHeight;

extern const COLORREF CR_Black;
extern const COLORREF CR_White;
extern const COLORREF CR_Red;
extern const COLORREF CR_Blue;
extern const COLORREF CR_Brown;
extern const COLORREF CR_DkOrange;
extern const COLORREF CR_LtOrange;
extern const COLORREF CR_Gray;

extern const COLORREF DefaultBG;
extern const COLORREF DefaultBR;
extern const COLORREF DrawerBG;
extern const COLORREF UserInfoFG1;
extern const COLORREF UserInfoFG2;
extern const COLORREF PresenceFG;
extern const COLORREF TabBG1;
extern const COLORREF TabBG2;
extern const COLORREF TabBG3;
extern const COLORREF TabBG4;
extern const COLORREF TabFG1;
extern const COLORREF TabFG2;
extern const COLORREF TabBR;
extern const COLORREF TitleBarBG;
extern const COLORREF TitleBarFG;
extern const COLORREF MenuBG;
extern const COLORREF MenuBR;
extern const COLORREF EditBG;
extern const COLORREF EditFG;
extern const COLORREF ComboBG;
extern const COLORREF ComboFG;
extern const COLORREF ActiveTitleBarBG;
extern const COLORREF ActiveTitleBarFG;
extern const COLORREF InactiveTitleBarBG;
extern const COLORREF InactiveTitleBarFG;
extern const COLORREF EditDisabledBG;
extern const COLORREF AdFG;
extern const COLORREF MoveListBG1;
extern const COLORREF MoveListBG2;
extern const COLORREF MoveListFG1;
extern const COLORREF MoveListFG2;
extern const COLORREF MoveListFG3;
extern const COLORREF BoardBorderBG;
extern const COLORREF BoardBorderFG;
extern const COLORREF BoardBorderBR;
extern const COLORREF GameMessageFG;
extern const COLORREF GameMessageFL;
extern const COLORREF GamePlayerBG;
extern const COLORREF GamePlayerFG;
extern const COLORREF GamePlayerBR;
extern const COLORREF GameBlackTimeBG;
extern const COLORREF GameBlackTimeFG;
extern const COLORREF GameBlackTimeBR;
extern const COLORREF GameWhiteTimeBG;
extern const COLORREF GameWhiteTimeFG;
extern const COLORREF GameWhiteTimeBR;
extern const COLORREF GameChatBG1;
extern const COLORREF GameChatBG2;
extern const COLORREF GameChatFG;
extern const COLORREF GameChatBR;
extern const COLORREF GameChatPrtcpntBG;
extern const COLORREF GameChatPrtcpntBR;
extern const COLORREF GameChatEditBG;
extern const COLORREF GameChatEditFG;
extern const COLORREF GameChatEditBR;
extern const COLORREF GameMenuBG;
extern const COLORREF GameMenuFG1;
extern const COLORREF GameMenuFG2;
extern const COLORREF GameMenuFGL;
extern const COLORREF GameMenuBR;

#define CHESSPARK_LOCALCHATSUSEJIDNICK 1
#undef CHESSPARK_CORRESPONDENCE
#define CHESSPARK_GROUPS 1
#define CHESSPARK_CRAZYHOUSE 1
#define CHESSPARK_LOSERS
#undef CHESSPARK_CHECKERS 
#undef CHESSPARK_RAPIDTIMECONTROL

#define CHESSPARK_VERSION "1.0"
#define CHESSPARK_BUILD "412"

#endif
