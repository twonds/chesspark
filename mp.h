#ifndef __MP_H__
#define __MP_H__

struct mpmessage_s *MPMessage_Create(unsigned int type, void *data1,
	void *data2, void (*destroydata1)(void *),
	void (*destroydata2)(void *), unsigned int callevery);
struct mpmessage_s *MPMessage_CreateSimple(unsigned int type);
void MPMessage_AddToTop(struct mpmessage_s *queue, struct mpmessage_s *msg);
void MPMessage_AddToBottom(struct mpmessage_s *queue, struct mpmessage_s *msg);
void MPMessage_Destroy(struct mpmessage_s *msg);

struct mp_s *MP_Create(int numpriorities, int nummessagetypes);
void MP_Destroy(struct mp_s *mp);
void MP_QueueMessage(struct mp_s *mp, struct mpmessage_s *msg, unsigned int priority);
void MP_SetCallback(struct mp_s *mp, unsigned int type,	int (*callback)(void *, void *));
void MP_HandleMessages(struct mp_s *mp, unsigned int timeout);

#endif