#ifndef __GAMEINVITEERROR_H__
#define __GAMEINVITEERROR_H__

struct Box_s *GameInviteError_Create(struct Box_s *roster, char *fromjid, char *error, char *actor, int sent);

#endif