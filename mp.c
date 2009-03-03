#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

struct mpmessage_s
{
	struct mpmessage_s *next;
	struct mpmessage_s *prev;
	unsigned int type;
	void *data1;
	void *data2;
	void (*destroydata1)(void *);
	void (*destroydata2)(void *);
	/*
	unsigned int lastcall;
	unsigned int callevery;
	*/
};

struct mpmessage_s *MPMessage_Create(unsigned int type, void *data1,
	void *data2, void (*destroydata1)(void *),
	void (*destroydata2)(void *), unsigned int callevery)
{
	struct mpmessage_s *msg;

	msg = malloc(sizeof(*msg));
	memset(msg, 0, sizeof(msg));

	msg->next         = NULL;
	msg->prev         = NULL;
	msg->type         = type;
	msg->data1        = data1;
	msg->data2        = data2;
	msg->destroydata1 = destroydata1;
	msg->destroydata2 = destroydata2;
	/*
	msg->lastcall     = 0;
	msg->callevery    = callevery;
	*/

	return msg;
}

struct mpmessage_s *MPMessage_CreateSimple(unsigned int type)
{
	return MPMessage_Create(type, NULL, NULL, NULL, NULL, 0);
}

void MPMessage_AddToTop(struct mpmessage_s *queue, struct mpmessage_s *msg)
{
	msg->prev = queue;
	msg->next = queue->next;

	queue->next->prev = msg;
	queue->next = msg;
}

void MPMessage_AddToBottom(struct mpmessage_s *queue, struct mpmessage_s *msg)
{
	msg->prev = queue->prev;
	msg->next = queue;

	queue->prev->next = msg;
	queue->prev = msg;
}


void MPMessage_Destroy(struct mpmessage_s *msg)
{
	struct mpmessage_s *next, *prev;

	if (msg == NULL)
	{
		return;
	}

	next = msg->next;
	prev = msg->prev;
	prev->next = next;
	next->prev = prev;

	if (msg->destroydata1)
	{
		msg->destroydata1(msg->data1);
	}

	if (msg->destroydata2)
	{
		msg->destroydata2(msg->data2);
	}

	free(msg);
}

struct mp_s
{
	struct mpmessage_s **queue;
	int (**callbacks)(void *, void *);
	unsigned int numpriorities;
	unsigned int nummessagetypes;
};

struct mp_s *MP_Create(int numpriorities, int nummessagetypes)
{
	struct mp_s *mp;
	int i;
	
	mp = malloc(sizeof(*mp));
	memset(mp, 0, sizeof(*mp));

	mp->queue = malloc(sizeof(*(mp->queue)) * numpriorities);

	for (i = 0; i < numpriorities; i++)
	{
		struct mpmessage_s *msg = MPMessage_Create(0, NULL, NULL, NULL, NULL, 0);

		msg = malloc(sizeof(*msg));
		memset(msg, 0, sizeof(*msg));

		msg->next = msg;
		msg->prev = msg;
		mp->queue[i] = msg;
	}

	mp->callbacks = malloc(sizeof(*(mp->callbacks)) * nummessagetypes);
	memset(mp->callbacks, 0, sizeof(*(mp->callbacks)) * nummessagetypes);

	mp->numpriorities = numpriorities;
	mp->nummessagetypes = nummessagetypes;

	return mp;
}

void MP_Destroy(struct mp_s *mp)
{
	unsigned int i;

	if (mp == NULL)
	{
		return;
	}

	for (i = 0; i < mp->numpriorities; i++)
	{
		while (mp->queue[i]->next != mp->queue[i])
		{
			MPMessage_Destroy(mp->queue[i]->next);
		}
		MPMessage_Destroy(mp->queue[i]);
	}

	free(mp->callbacks);

	free(mp);
}

void MP_QueueMessage(struct mp_s *mp, struct mpmessage_s *msg, unsigned int priority)
{
	MPMessage_AddToBottom(mp->queue[priority], msg);
}

void MP_SetCallback(struct mp_s *mp, unsigned int type,	int (*callback)(void *, void *))
{
	mp->callbacks[type] = callback;
}

void MP_HandleMessages(struct mp_s *mp, unsigned int timeout)
{
	unsigned int tick = GetTickCount();
	int empty = 0;

	while (GetTickCount() - tick < timeout && !empty)
	{
		struct mpmessage_s *msg = NULL;
		unsigned int i;

		for (i = 0; i < mp->numpriorities; i++)
		{
			if (mp->queue[i]->next != mp->queue[i])
			{
				msg = mp->queue[i]->next;
			}
		}

		if (msg)
		{
			int result = mp->callbacks[msg->type](msg->data1, msg->data2);

			/*if (result)*/
			{
				MPMessage_Destroy(msg);
			}
		}
		else
		{
			empty = 1;
		}
	}
}