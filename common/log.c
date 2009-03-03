#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>

#include <strophe.h>

#include "util.h"
#include "info.h"

#include "log.h"

static int logfileopen = 0;
static int nolog = 0;
static int logstripe = 0;
static int runningsize = 0;

void Log_SetNoLog(int log)
{
	nolog = log;
}

void Log_WriteString(int level, const char *string)
{
	va_list ap;
	FILE *plogfile;
	char initialbuf[8192], *buf;
	char *color;
	char *clas;
	int buflen;
	int needed;
	char appdata[MAX_PATH];
	char filename[MAX_PATH];

	if (GetEnvironmentVariable("appdata", appdata, MAX_PATH) <= 0)
	{
		strcpy(appdata, ".");
	}
	else
	{
		strcat(appdata, "/chesspark/");
	}

	if (!logfileopen || runningsize > 15000000)
	{
		char filename2[MAX_PATH];

		strcpy(filename, appdata);
		strcat(filename, "debug.5.log");		
		DeleteFile(filename);

		strcpy(filename2, appdata);
		strcat(filename2, "debug.4.log");
		MoveFile(filename2, filename);

		strcpy(filename, appdata);
		strcat(filename, "debug.3.log");		
		MoveFile(filename, filename2);

		strcpy(filename2, appdata);
		strcat(filename2, "debug.2.log");		
		MoveFile(filename2, filename);

		strcpy(filename, appdata);
		strcat(filename, "debug.1.log");		
		MoveFile(filename, filename2);

		strcpy(filename2, appdata);
		strcat(filename2, "debug.log");		
		MoveFile(filename2, filename);

		logfileopen = 1;
		runningsize = 0;
	}

	buf = initialbuf;
	buflen = 8192;
	buf[0] = '\0';
	needed = EscapeToHTML(buf, buflen, string);

	if (needed > buflen)
	{
		buflen = needed + 1;
		buf = malloc(buflen);
		buf[0] = '\0';
		needed = EscapeToHTML(buf, buflen, string);
	}

	color = "grey";
	clas = "debug";

	if (strnicmp(string, "(xmpp)", 6) == 0)
	{
		color = "black";
		clas = "info";
	}

	if (strnicmp(string, "(conn)", 6) == 0)
	{
		color = "black";
		clas = "info";
	}

	if (strnicmp(string, "(xmpp) SENT:", 12) == 0)
	{
		color = "green";
	}

	if (strnicmp(string, "(conn) SENT:", 12) == 0)
	{
		color = "green";
	}

	if (strnicmp(string, "(xmpp) RECV:", 12) == 0)
	{
		color = "blue";
	}

	if (strnicmp(string, "(conn) RECV:", 12) == 0)
	{
		color = "blue";
	}

	if (strstr(string, "type=\"error\""))
	{
		color = "red";
		clas = "warn";
	}

	if (strnicmp(string, "Connect error ", 14) == 0)
	{
		color = "red";
		clas = "error";
	}

	strcpy(filename, appdata);
	strcat(filename, "debug.log");

	runningsize += needed;

	if ((plogfile = fopen(filename, "at")))
	{
		fprintf(plogfile, "<p style=\"color: %s;\" class=\"%s%d\">(%d)%s: %s</p>\n", color, clas, logstripe, level, Info_GetCurrentTimestamp2(), buf);
		logstripe = (logstripe + 1) % 2;
		fclose(plogfile);
	}

	if (buf != initialbuf)
	{
		free(buf);
	}
}

void Log_Write(int level, const char *v, ...)
{
	va_list ap;
	FILE *plogfile;
	char initialbuf[8192], *buf;
	int buflen;
	int needed;

	if (nolog)
	{
		return;
	}

	if (!GetRegInt("ExtendedLogging"))
	{
		return;
	}

	/* set no log so we don't cause a loop */
	nolog = 1;

	buf = initialbuf;
	buflen = 8192;

	va_start(ap, v);
	needed = xmpp_vsnprintf(buf, buflen, v, ap);
	va_end(ap);

	if (needed > buflen)
	{
		buflen = needed + 1;
		buf = malloc(buflen);
		va_start(ap, v);
		xmpp_vsnprintf(buf, buflen, v, ap);
		va_end(ap);
	}

	Log_WriteString(level, buf);

	if (buf != initialbuf)
	{
		free(buf);
	}

	nolog = 0;
}

void Log_Write2(int level, const char *v, ...)
{
	va_list ap;
	FILE *plogfile;
	char initialbuf[8192], *buf;
	int buflen;
	int needed;

	if (nolog)
	{
		return;
	}

	/* set no log so we don't cause a loop */
	nolog = 1;

	buf = initialbuf;
	buflen = 8192;

	va_start(ap, v);
	needed = xmpp_vsnprintf(buf, buflen, v, ap);
	va_end(ap);

	if (needed > buflen)
	{
		buflen = needed + 1;
		buf = malloc(buflen);
		va_start(ap, v);
		xmpp_vsnprintf(buf, buflen, v, ap);
		va_end(ap);
	}

	Log_WriteString(level, buf);

	if (buf != initialbuf)
	{
		free(buf);
	}

	nolog = 0;
}

void Log_xmppLogCallback(void * const userdata,
	const xmpp_log_level_t level,
	const char * const area,
	const char * const msg)
{
	Log_Write2(level, "(%s) %s\n", area, msg);
}
