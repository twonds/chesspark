#ifndef __LOG_H__
#define __LOG_H__

void Log_SetNoLog(int log);

void Log_Write(int level, const char *v, ...);
void Log_Write2(int level, const char *v, ...);
/*
void Log_xmppLogCallback(void * const userdata,
	const xmpp_log_level_t level,
	const char * const area,
	const char * const msg);
*/
#endif