#ifndef __AUTOEDIT_H__
#define __AUTOEDIT_H__

struct Box_s *AutoEdit_Create(struct Box_s *parent, int w, char *titlebartxt,
  char *topinfotext, char *bottominfotext, char *defaulttext,
  void (*callbackfunc)(char *, void *), void *userdata, char *oktext);

#endif