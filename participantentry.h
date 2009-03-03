#ifndef __PARTICIPANTENTRY_H__
#define __PARTICIPANTENTRY_H__

#include "drop.h"

struct Box_s *ParticipantEntry_Create(char *targetjid, char *name,
  enum SStatus status, char *statusmsg, char *role, char *affiliation,
  char *realjid, int isGroupChat, struct namedlist_s *roleslist,
  struct namedlist_s *titleslist, int notactivated, char *membertype);
int ParticipantEntry_SortFunc(struct Box_s *lbox, struct Box_s *gbox);
char *ParticipantEntry_GetShowName(struct Box_s *entrybox);
enum SStatus ParticipantEntry_GetStatus(struct Box_s *entrybox);
int ParticipantEntry_UserHasMucPower(struct Box_s *entrybox);
void ParticipantEntry_PopupMenu(struct Box_s *pbox, struct Box_s *window, int x, int y);
char *ParticipantEntry_GetRealJID(struct Box_s *entrybox);
char *ParticipantEntry_GetRole(struct Box_s *entrybox);
void ParticipantEntry_SetStatus(struct Box_s *entrybox, enum SStatus status);

#endif