#ifndef __GAMEINVITATION_H__
#define __GAMEINVITATION_H__

struct Box_s *GameInvitation_Create(struct Box_s *roster, char *fromjid, struct gamesearchinfo_s *info, int replace, int cascade);
void GameInvitation_SetProfile(struct Box_s *dialog, char *jid, struct namedlist_s *profileinfo);
void GameInvitation_SetError(struct Box_s *dialog, char *opponentjid, char *gameid, char *error);
void GameInvitation_ShowCorGameAccepted(struct Box_s *dialog, char *gameid);
char *GameInvitation_GetJid(struct Box_s *dialog);
void GameInvitation_OnClose(struct Box_s *pbox);

#endif