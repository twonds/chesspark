#ifndef __GAMESLIST_H__
#define __GAMESLIST_H__

void GamesList_AddSearchGame(struct Box_s *gameslist_box, char *itemid, char *node, char *jid, struct gamesearchinfo_s *info);
struct Box_s *GamesList_Create();
void GamesList_NoSearchGameResults(struct Box_s *gameslist_box);
void GamesList_SetTotalGamesCount(struct Box_s *gameslist_box, int count);
void GamesList_RefreshPage(struct Box_s *gameslist_box);
void GamesListSearch_OnRatingTypeCombo(struct Box_s *combo, char *type);
void GamesList_ExternalSearch(struct Box_s *gameslist_box, char *node);
void GamesList_ClearSearch(struct Box_s *gameslist_box);
void GamesList_AddSearchTournament(struct Box_s *gameslist_box, struct tournamentinfo_s *info);
void GamesList_FinishGameResults(struct Box_s *gameslist_box, int noresults);

#endif