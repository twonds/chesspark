#ifndef __TOURNEYBOX_H__
#define __TOURNEYBOX_H__

void TourneyBox_UpdatePlayer(struct Box_s *tourneybox, char *jid, struct tournamentplayerinfo_s *pinfo);
void TourneyBox_GameStoppedPlaying(struct Box_s *tourneybox,char *gameid,int round,char *white,char *black,char *winner);
void TourneyBox_GamePlaying(struct Box_s *tourneybox,char *gameid,int round,char *white,char *black);
void TourneyBox_End(struct Box_s *tourneybox,char *winner);
void TourneyBox_EndRound(struct Box_s *tourneybox,int round);
void TourneyBox_StartRound(struct Box_s *tourneybox,int round,struct namedlist_s *pairinglist);
void TourneyBox_RemovePlayer(struct Box_s *tourneybox,char *playerjid);
void TourneyBox_AddPlayer(struct Box_s *tourneybox,char *playerjid);
void TourneyBox_SetTournamentInfo(struct Box_s *tourneybox,struct tournamentinfo_s *info);
void TourneyBox_RefreshInfo(struct Box_s *tourneybox);
void TourneyBox_SetParticipantStatus(struct Box_s *tourneybox, char *targetjid,
  char *name, enum SStatus status, char *statusmsg, char *role,
  char *affiliation, char *realjid, char *nickchange,
  struct namedlist_s *roleslist, struct namedlist_s *titleslist);
void TourneyBox_AddChatMessage(struct Box_s *tourneybox,char *name,char *text);
struct Box_s *TourneyBox_Create(int x,int y,char *tourneyid,char *tourneychatjid);
void TourneyBox_UpdateNextRoundMsg(struct Box_s *dialog,void *userdata);
void TourneyBox_UpdateNextRoundMsg(struct Box_s *dialog,void *userdata);

struct Box_s *RoundStartBox_Create(struct Box_s *parent, char *tourneyid);

#endif