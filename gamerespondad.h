#ifndef __GAMERESPONDAD_H__
#define __GAMERESPONDAD_H__

struct Box_s *GameRespondAd_Create(struct Box_s *roster, char *fromjid, struct gamesearchinfo_s *info);
void GameRespondAd_SetProfile(struct Box_s *dialog, char *jid, struct namedlist_s *profileinfo);
void GameRespondAd_SetError(struct Box_s *dialog, char *opponentjid, char *error);

#endif