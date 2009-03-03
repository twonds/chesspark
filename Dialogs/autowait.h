#ifndef __AUTOWAIT_H__
#define __AUTOWAIT_H__

struct Box_s *AutoWait_Create(struct Box_s *parent, int w, char *titlebartxt,
  char *topinfotext, char *bottominfotext);
void AutoWait_SetProgress(struct Box_s *pbox, float progress);
void AutoWaitName_SetProgress(char *name, float progress);

#endif