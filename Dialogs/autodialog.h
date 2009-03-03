#ifndef __AUTODIALOG_H__
#define __AUTODIALOG_H__

struct Box_s *AutoDialog_Create(struct Box_s *parent, int w, char *titletext,
  char *dialogtext, char *button1text, char *button2text,
  void (*button1callback)(struct Box_s *, void *),
  void (*button2callback)(struct Box_s *, void *), void *userdata);
struct Box_s *AutoDialog_Create2(struct Box_s *parent, int w, char *titletext,
  char *dialogtext, char *button1text, char *button2text,
  void (*button1callback)(struct Box_s *, void *),
  void (*button2callback)(struct Box_s *, void *), void *userdata, void (*dontshowcallback)(struct Box_s *, char *name), char *name);

void AutoDialog_SetLinkCallback(struct Box_s *dialog, int linknum, void(*OnClick)(struct Box_s *,void *), void *userdata);

void AutoDialog_ResetText(struct Box_s *dialog, char *titletext, char *dialogtext);

#endif
