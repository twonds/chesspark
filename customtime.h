#ifndef __CUSTOMTIME_H__
#define __CUSTOMTIME_H__

struct Box_s *CustomTime_Create(struct Box_s *parent, 
  void (*onSetCustomTimeControl)(struct Box_s *,
    struct timecontrol_s *, struct timecontrol_s *, char *name),
  char *name,
  struct timecontrol_s *whitetimecontrol,
  struct timecontrol_s *blacktimecontrol,
  void (*refreshCallback)(struct Box_s *),
  void (*destroyCallback)(struct Box_s *),
  int hassave);

#endif