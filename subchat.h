#ifndef __SUBCHAT_H__
#define __SUBCHAT_H__

void SubChat_AddCustom(struct Box_s *subchat,struct Box_s *custombox);
void SubChat_AddText(struct Box_s *subchat, struct Box_s *participantlist, char *targetjid, char *name, char *text, int local, struct Box_s *redirectfocus);
struct Box_s *SubChat_AddText2(struct Box_s *subchat, struct Box_s *participantlist,
	char *targetjid, char *showname, char *text, int local,
	struct Box_s *redirectfocus, char *logfile, char *timestamp, int skipmatchlinks);
int SubChat_AddTimeStamp(struct Box_s *subchat, char *nick, char *timestamp);
int SubChat_AddTimeStamp2(struct Box_s *subchat, char *nick, char *timestamp, struct Box_s *redirectfocus, char *logfile);
struct Box_s *SubChat_AddHistory(struct Box_s *subchat, char *text);
void SubChat_AddCustom(struct Box_s *subchat, struct Box_s *custombox);
struct Box_s *SubChat_AddSmallText(struct Box_s *subchat, char *txt, int emote, struct Box_s *redirectfocus);

#endif