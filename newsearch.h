#ifndef __NEWSEARCH_H__
#define __NEWSEARCH_H__

struct Box_s *NewGames_Create(int x, int y, int w, int h, struct Box_s *roster);

void NewGames_ExternalSetPage(struct Box_s *pbox, char *name);
void NewGames_ClearAds(struct Box_s *dialog);
void NewGames_ClearSearch(struct Box_s *dialog, char *node);
void NewGames_AddSearchGame(struct Box_s *dialog, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info);
void NewGames_RemovePushGame(struct Box_s *dialog, char *type, char *id);
void NewGames_RefreshPage(struct Box_s *dialog, char *page);
void NewGames_RefreshCurrentPage(struct Box_s *dialog);
void NewGames_AddAdSuccess(struct Box_s *dialog, char *id);

void NewGames_AddPushGame(struct Box_s *dialog, char *type, char *id, struct gamesearchinfo_s *info);
void NewGames_RemovePushGame(struct Box_s *dialog, char *type, char *id);
void NewGames_ClearPushGames(struct Box_s *dialog /*, char *type*/);
void NewGames_KillGameAds(struct Box_s *pbox);

void NewGames_SetGameFinderExpiryReminder(struct Box_s *dialog, int expiry);

#endif