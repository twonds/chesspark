#ifndef __GAMEINVITEFRIEND_H__
#define __GAMEINVITEFRIEND_H__

struct Box_s *GameInviteFriend_Create(struct Box_s *roster, char *jid, struct gamesearchinfo_s *info, int replace, int playnow);
void GameInviteFriend_SetProfile(struct Box_s *dialog, char *jid, struct namedlist_s *profileinfo);
void GameInviteFriend_SetError(struct Box_s *dialog, char *opponentjid, char *error, char *actor);
void GameInviteFriend_SetGameID(struct Box_s *dialog, char *opponentjid, char *gameid);
void GameInviteFriend_ShowCorGameAccepted(struct Box_s *dialog, char *gameid);

#endif