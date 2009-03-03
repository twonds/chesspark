#ifndef __PROFILE_H__
#define __PROFILE_H__

struct Box_s *Profile_Create(struct Box_s *roster, char *jid, char *nickname, int local);
void Profile_SetProfile(struct Box_s *dialog, char *jid, struct namedlist_s *profileinfo);

struct Box_s *MiniProfile_Create(int x, int y, int w, int h, char *jid);
void MiniProfile_SetProfile(struct Box_s *miniprofilebox, char *jid, struct profile_s *profile);

#endif