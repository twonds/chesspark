#ifndef __CHESSBOX_H__
#define __CHESSBOX_H__

struct Box_s *ChessBox_Create(int x, int y, int w, int h, char *gameid, char *roomjid);
void ChessBox_ParseGameMove(struct Box_s *chessbox, char *move, char *annotation, int numtakebacks, int ply);

void ChessBox_ResetGamePosition(struct Box_s *chessbox, int illegalmove);
void ChessBox_ParseFEN(struct Box_s *chessbox, char *fen);
void ChessBox_SyncClock(struct Box_s *chessbox, char *time, char *side, char *control, int tick);
void ChessBox_SetClockControl(struct Box_s *chessbox, char *side, int *controlarray);

void ChessBox_SetGameViewRotated(struct Box_s *chessbox, int rotated);
void ChessBox_SetGameViewRotatedIfBlack(struct Box_s *chessbox, char *blackjid);

struct Box_s *ChessBox_AddChatMessage(struct Box_s *chessbox, char *name, char *text);
void ChessBox_SetParticipantStatus(struct Box_s *chessbox, char *targetjid,
  char *name, enum SStatus status, char *statusmsg, char *role,
  char *affiliation, char *realjid, char *nickchange,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist,
  int notactivated, char *membertype);

void ChessBox_SetWhitePlayer(struct Box_s *chessbox, char *jid, char *name, int islocalplayer);
void ChessBox_SetBlackPlayer(struct Box_s *chessbox, char *jid, char *name, int islocalplayer);
void ChessBox_SetProfile(char *jid, struct profile_s *profile, struct Box_s *chessbox);

void ChessBox_AddMoveToList(struct Box_s *chessbox, char *longmove, char *annotation, int numtakebacks);
void ChessBox_SetMoveList(struct Box_s *chessbox, struct namedlist_s *textmovelist);

void ChessBox_HandleAdjourn(struct Box_s *chessbox, char *white, char *black);
void ChessBox_HandleDraw(struct Box_s *chessbox, int whiteaccept, int blackaccept);
void ChessBox_HandleRejectAdjourn(struct Box_s *chessbox);
void ChessBox_HandleRejectDraw(struct Box_s *chessbox);
void ChessBox_HandleGameOver(struct Box_s *chessbox, char *type, char *win, char *lose, char *reason);
void ChessBox_HandleAbort(struct Box_s *chessbox, char *white, char *black);
void ChessBox_HandleTakeback(struct Box_s *chessbox, char *white, char *black);
void ChessBox_HandleRejectAbort(struct Box_s *chessbox);
void ChessBox_HandleRejectTakeback(struct Box_s *chessbox);

void ChessBox_CloseIfSamePlayers(struct Box_s *chessbox, char *newgameid, char *player1, char *player2);
void ChessBox_ShowDisconnect(struct Box_s *chessbox);
void ChessBox_ClearMoveList(struct Box_s *chessbox);
void ChessBox_SetGameInfo(struct Box_s *chessbox, struct gamesearchinfo_s *info, int whitelocal, int blacklocal, char *tourneyid);

void ChessBox_ShowRematchRequest(struct Box_s *chessbox, struct gamesearchinfo_s *info);
void ChessBox_SetRematchError(struct Box_s *chessbox, char *error, char *actor);

struct gamesearchinfo_s *ChessBox_GetGameInfo(struct Box_s *chessbox);

void ChessBox_SetDisconnect(struct Box_s *chessbox);
void ChessBox_ShowDisconnect(struct Box_s *chessbox);
void ChessBox_SetStatusOnChat(struct Box_s *chessbox, enum SStatus status, char *statusmsg);
void ChessBox_SwitchClockToNonLocalPlayer(struct Box_s *chessbox, unsigned int lag);
void ChessBox_SetState(struct Box_s *chessbox, char *initialstate, char *state);

int ChessBox_IsInRoom(struct Box_s *chessbox);
int ChessBox_IsPlaying(struct Box_s *chessbox);
void ChessBox_ShowError(struct Box_s *chessbox, int icode);

void ChessBox_ShowRatingUpdate(struct Box_s *chessbox, char *jid, struct rating_s *rating);

#endif