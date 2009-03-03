#ifndef __INVITETOCHAT_H__
#define __INVITETOCHAT_H__

struct Box_s *InviteToChat_Create(struct Box_s *roster,char *friendjid,char *chatjid);
void InviteToChat_Error(struct Box_s *pbox,char *error);

#endif