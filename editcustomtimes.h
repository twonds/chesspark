#ifndef __EDITCUSTOMTIMES_H__
#define __EDITCUSTOMTIMES_H__

struct Box_s *EditCustomTimes_Create(struct Box_s *parent, void (*refreshCallback)(struct Box_s *), void (*destroyCallback)(struct Box_s *));

#endif