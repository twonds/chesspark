#ifndef __GAMERECONVENE_H__
#define __GAMERECONVENE_H__

struct Box_s *GameReconvene_Create(struct Box_s *roster, char *fromjid,
	char *gameid, struct gamesearchinfo_s *info, int inviting);
void GameReconvene_SetProfile(struct Box_s *dialog, char *jid, struct namedlist_s *profileinfo);
void GameReconvene_SetError(struct Box_s *dialog, char *gameid, char *error, char *actor);

#endif