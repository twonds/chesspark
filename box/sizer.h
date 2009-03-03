#ifndef __SIZER_H__
#define __SIZER_H__

struct Box_s *NWSizer_Create(int x, int y, int w, int h);
struct Box_s *NSizer_Create(int x, int y, int w, int h);
struct Box_s *NESizer_Create(int x, int y, int w, int h);
struct Box_s *WSizer_Create(int x, int y, int w, int h);
struct Box_s *ESizer_Create(int x, int y, int w, int h);
struct Box_s *SWSizer_Create(int x, int y, int w, int h);
struct Box_s *SSizer_Create(int x, int y, int w, int h);
struct Box_s *SESizer_Create(int x, int y, int w, int h);
struct Box_s *SizerSet_Create(struct Box_s *parent);

void SizerSet_SetDisabled(struct Box_s *sizerset, int disabled);
struct Box_s *VSizerBar_Create(int x, int y, int w, int h, struct Box_s *left, struct Box_s *right);

struct Box_s *DrawerSizer_Create(int x, int y, int w, int h);

#endif