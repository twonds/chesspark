#ifndef __ROSTER_H__
#define __ROSTER_H__

struct Box_s *Roster_Create(int x, int y, int w, int h, enum Box_flags flags);
void Roster_SetUser(struct Box_s *pbox, char *jid, char *nick);
void Roster_SetPresence(struct Box_s *pbox, enum FriendStatus_s status, char *statusmsg);
void Roster_SetError(struct Box_s *pbox, char *error, int notify);
void Roster_SetReconnect(struct Box_s *pbox);

void Roster_SetNub(struct Box_s *pbox);
void Roster_UnsetNub(struct Box_s *pbox);

void Roster_AddTab(struct Box_s *pbox, char *name, struct Box_s *target);
void Roster_RemoveTab(struct Box_s *pbox, char *name);
void Roster_ActivateTab(struct Box_s *pbox, char *name);
void Roster_ActivateFirstTab(struct Box_s *pbox);

void Roster_SetAvatar(struct Box_s *pbox, char *filename);
void Roster_SetRating(struct Box_s *roster, int rating);
void Roster_SetProfile(struct Box_s *roster, struct namedlist_s *ratinglist, struct namedlist_s *roleslist, struct namedlist_s *titleslist);

void Roster_SetTabActivateFunc(struct Box_s *roster, void (*OnTabActivate)(struct Box_s *, char *));
int Roster_IsNub(struct Box_s *roster);
void Roster_UpdateToOptions(struct Box_s *roster);
void Roster_SetNewGamesButtonActive(struct Box_s *roster, int active);

#endif