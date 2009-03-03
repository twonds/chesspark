#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "info.h"
#include "i18n.h"
#include "namedlist.h"
#include "leak.h"
#include "log.h"

void Info_DestroyTimeControl(struct timecontrol_s *tc)
{
	if (tc)
	{
		free(tc->controlarray);
		free(tc);
	}
}

struct timecontrol_s *Info_DupeTimeControl(struct timecontrol_s *src)
{
	int numcontrols, *psrc, *pdst;
	struct timecontrol_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));

	dst->correspondence = src->correspondence;
	dst->delayinc = src->delayinc;

	psrc = src->controlarray;
	numcontrols = *psrc;
	dst->controlarray = malloc(sizeof(int) * (numcontrols * 2 + 1));
	pdst = dst->controlarray;

	*pdst++ = *psrc++;
	while (numcontrols)
	{
		*pdst++ = *psrc++;
		*pdst++ = *psrc++;
		numcontrols--;
	}

	return dst;
}

int Info_TimeControlsEqual(struct timecontrol_s *wtc, struct timecontrol_s *btc)
{
	int numcontrols, *pw, *pb;

	if (!wtc && !btc)
	{
		return 1;
	}

	if ((wtc && !btc) || (btc && !wtc))
	{
		return 0;
	}

	if (wtc->delayinc != btc->delayinc)
	{
		return 0;
	}

	if (wtc->correspondence != btc->correspondence)
	{
		return 0;
	}

	pw = wtc->controlarray;
	pb = btc->controlarray;

	if (*pw != *pb)
	{
		return 0;
	}

	numcontrols = *pb;
	pw++; pb++;
	while (numcontrols)
	{
		if (*pw++ != *pb++)
		{
			return 0;
		}
		if (*pw++ != *pb++)
		{
			return 0;
		}
		numcontrols--;
	}

	return 1;
}

#if 0
char *Info_TimeControlToStringList(struct timecontrol_s *src)
{
	char txt[512];
	char num[512];
	int numcontrols, *p;

	if (!src)
	{
		return NULL;
	}

	txt[0] = '\0';

	if (src->correspondence)
	{
		strcat(txt, "c");
	}

	sprintf(num, "%d", src->delayinc);
	strcat(txt, num);
	strcat(txt, ":");

	p = src->controlarray;
	numcontrols = *p++;

	sprintf(num, "%d", numcontrols);
	strcat(txt, num);
	strcat(txt, ":");

	while (numcontrols)
	{
		sprintf(num, "%d", *p++);
		strcat(txt, num);
		strcat(txt, ":");
		sprintf(num, "%d", *p++);
		strcat(txt, num);
		strcat(txt, ":");
		numcontrols--;
	}
	return strdup(txt);
}

struct timecontrol_s *Info_StringListToTimeControl(char *src)
{
	char txt[512];
	char *p, *pn;
	int *p2;
	struct timecontrol_s *tc;
	int numcontrols;

	if (!src || strlen(src) == 0)
	{
		return NULL;
	}

	tc = malloc(sizeof(*tc));
	memset(tc, 0, sizeof(*tc));

	strcpy(txt, src);
	p = txt;

	if (*p == 'c')
	{
		p++;
		tc->correspondence = 1;
	}

	pn = strchr(p, ':');

	if (pn)
	{
		*pn = '\0';
		pn++;
	}
	else
	{
		Log_Write(0, "Error parsing text time control %s\n", src);
		return NULL;
	}

	sscanf(p, "%d", &(tc->delayinc));

	p = pn;
	pn = strchr(p, ':');

	if (pn)
	{
		*pn = '\0';
		pn++;
	}
	else
	{
		Log_Write(0, "Error parsing text time control %s\n", src);
		return NULL;
	}

	sscanf(p, "%d", &numcontrols);

	tc->controlarray = malloc(sizeof(int) * (numcontrols * 2 + 1));
	p2 = tc->controlarray;
	*p2++ = numcontrols;

	while (numcontrols)
	{
		p = pn;
		pn = strchr(p, ':');

		if (pn)
		{
			*pn = '\0';
			pn++;
		}
		else
		{
			Log_Write(0, "Error parsing text time control %s\n", src);
			return NULL;
		}

		sscanf(p, "%d", p2++);
		
		p = pn;
		pn = strchr(p, ':');

		if (pn)
		{
			*pn = '\0';
			pn++;
		}
		else
		{
			Log_Write(0, "Error parsing text time control %s\n", src);
			return NULL;
		}

		sscanf(p, "%d", p2++);

		numcontrols--;
	}

	return tc;
}
#endif
char *Info_SecsToText1(float fsec)
{
	int sign, day, hour, min, sec, msec;
	char showtime[120], txt[80];

	if (fsec < 0)
	{
		sign = -1;
		fsec = -fsec;
	}
	else
	{
		sign = 1;
	}

	day =  (int)(fsec / 86400.0f);
	hour = (int)(fsec / 3600.0f) - (day * 24);
	min =  (int)(fsec / 60.0f)   - (day * 1440)     - (hour * 60);
	sec =  (int)(fsec)           - (day * 86400)    - (hour * 3600)    - (min * 60);
	msec = (int)(fsec * 1000.0f) - (day * 86400000) - (hour * 3600000) - (min * 60000) - (sec * 1000);

	showtime[0] = '\0';

	if (sign == -1)
	{
		strcat(showtime, "-");
	}

	if (day > 0)
	{
		if (day > 99)
		{
			strcat(showtime, "??d ");
		}
		else
		{
			sprintf(txt, "%dd ", day);
			strcat(showtime, txt);
		}
	}

	if (hour > 0)
	{
			sprintf(txt, "%dh ", hour);
			strcat(showtime, txt);
	}

	if (min > 0)
	{
			sprintf(txt, "%dm ", min);
			strcat(showtime, txt);
	}

	if (sec > 0 || strlen(showtime) <= 1)
	{
			sprintf(txt, "%ds ", sec);
			strcat(showtime, txt);
	}

	if (strlen(showtime) > 0)
	{
                showtime[strlen(showtime)-1] = '\0';
	}

	return strdup(showtime);
}

void Info_SecsToText2(float fsec, char *showtime, int *msec, int longestonly)
{
	int sign, year, mon, day, hour, min, sec;

	if (fsec < 0)
	{
		sign = -1;
		fsec = -fsec;
	}
	else
	{
		sign = 1;
	}

	year = (int)(fsec / 31558464.0f);
	fsec -= year * 31558464.0f;
	mon =  (int)(fsec / 2629872.0f);
	fsec -= mon  * 2629872.0f;
	day =  (int)(fsec / 86400.0f);
	fsec -= day  * 86400.0f;
	hour = (int)(fsec / 3600.0f);
	fsec -= hour * 3600.0f;
	min =  (int)(fsec / 60.0f);
	fsec -= min  * 60.0f;
	sec =  (int)(fsec);
	fsec -= (float)sec;

	if (msec)
	{
		*msec = (int)(fsec * 1000.0f);
	}

	if (year > 0)
	{
		if (year > 99)
		{
			strncat(showtime, _("Many years"), year);
		}
		else if (longestonly)
		{
			if (year == 1)
			{
				sprintf(showtime, _("1 year"));
			}
			else
			{
				sprintf(showtime, _("%d years"), year);
			}
		}
		else
		{
			sprintf(showtime, _("%02d:%02d:%02d.%02d:%02d:%02d"), year, mon, day, hour, min, sec);
		}
	}
	else if (mon > 0)
	{
		if (longestonly)
		{
			if (mon == 1)
			{
				sprintf(showtime, _("1 month"));
			}
			else
			{
				sprintf(showtime, _("%d months"), mon);
			}
		}
		else
		{
			sprintf(showtime, _("%02d.%02d.%02d:%02d:%02d"), mon, day, hour, min, sec);
		}
	}
	else if (day > 0)
	{
		if (longestonly)
		{
			if (day == 1)
			{
				sprintf(showtime, _("1 day"));
			}
			else
			{
				sprintf(showtime, _("%d days"), day);
			}
		}
		else
		{
			sprintf(showtime, _("%02d.%02d:%02d:%02d"), day, hour, min, sec);
		}
	}
	else if (hour > 0)
	{
		if (longestonly)
		{
			if (hour == 1)
			{
				sprintf(showtime, _("1 hour"));
			}
			else
			{
				sprintf(showtime, _("%d hours"), hour);
			}
		}
		else
		{
                        sprintf(showtime, _("%02d:%02d:%02d"), hour, min, sec);
		}
	}
	else if (longestonly == 2)
	{
		if (min > 0)
		{
			if (min == 1)
			{
				sprintf(showtime, _("1 minute"));
			}
			else
			{
				sprintf(showtime, _("%d minutes"), min);
			}
		}
		else
		{
			if (sec == 1)
			{
				sprintf(showtime, _("1 second"));
			}
			else
			{
				sprintf(showtime, _("%d seconds"), sec);
			}
		}
	}
	else
	{
		sprintf(showtime, _("%02d:%02d"), min, sec);
	}

	if (sign == -1)
	{
		char oldtime[512];

		strcpy(oldtime, showtime);
		sprintf(showtime, "-%s", oldtime);
	}
}


void Info_SecsToText3(float fsec, char *showtime, int *msec, int longestonly, int offset)
{
	int sign, year, mon, day, hour, min, sec;
	float fsec2;

	if (fsec < 0)
	{
		sign = -1;
		fsec = -fsec;
	}
	else
	{
		sign = 1;
	}

	fsec2 = fsec;

	year = (int)(fsec / 31558464.0f);
	fsec -= year * 31558464.0f;
	mon =  (int)(fsec / 2629872.0f);
	fsec -= mon  * 2629872.0f;
	day =  (int)(fsec / 86400.0f);
	fsec -= day  * 86400.0f;
	hour = (int)(fsec / 3600.0f);
	fsec -= hour * 3600.0f;
	min =  (int)(fsec / 60.0f);
	fsec -= min  * 60.0f;
	sec =  (int)(fsec);
	fsec -= (float)sec;

	if (msec)
	{
		*msec = (int)(fsec * 1000.0f);
	}

	fsec = fsec2 + (float)(1000 - offset) / 1000.0f;

	year = (int)(fsec / 31558464.0f);
	fsec -= year * 31558464.0f;
	mon =  (int)(fsec / 2629872.0f);
	fsec -= mon  * 2629872.0f;
	day =  (int)(fsec / 86400.0f);
	fsec -= day  * 86400.0f;
	hour = (int)(fsec / 3600.0f);
	fsec -= hour * 3600.0f;
	min =  (int)(fsec / 60.0f);
	fsec -= min  * 60.0f;
	sec =  (int)(fsec);
	fsec -= (float)sec;

	if (year > 0)
	{
		if (year > 99)
		{
			strncat(showtime, _("Many years"), year);
		}
		else if (longestonly)
		{
			if (year == 1)
			{
				sprintf(showtime, _("1 year"));
			}
			else
			{
				sprintf(showtime, _("%d years"), year);
			}
		}
		else
		{
			sprintf(showtime, _("%02d:%02d:%02d.%02d:%02d:%02d"), year, mon, day, hour, min, sec);
		}
	}
	else if (mon > 0)
	{
		if (longestonly)
		{
			if (mon == 1)
			{
				sprintf(showtime, _("1 month"));
			}
			else
			{
				sprintf(showtime, _("%d months"), mon);
			}
		}
		else
		{
			sprintf(showtime, _("%02d.%02d.%02d:%02d:%02d"), mon, day, hour, min, sec);
		}
	}
	else if (day > 0)
	{
		if (longestonly)
		{
			if (day == 1)
			{
				sprintf(showtime, _("1 day"));
			}
			else
			{
				sprintf(showtime, _("%d days"), day);
			}
		}
		else
		{
			sprintf(showtime, _("%02d.%02d:%02d:%02d"), day, hour, min, sec);
		}
	}
	else if (hour > 0)
	{
		if (longestonly)
		{
			if (hour == 1)
			{
				sprintf(showtime, _("1 hour"));
			}
			else
			{
				sprintf(showtime, _("%d hours"), hour);
			}
		}
		else
		{
                        sprintf(showtime, _("%02d:%02d:%02d"), hour, min, sec);
		}
	}
	else if (longestonly == 2)
	{
		if (min > 0)
		{
			if (min == 1)
			{
				sprintf(showtime, _("1 minute"));
			}
			else
			{
				sprintf(showtime, _("%d minutes"), min);
			}
		}
		else
		{
			if (sec == 1)
			{
				sprintf(showtime, _("1 second"));
			}
			else
			{
				sprintf(showtime, _("%d seconds"), sec);
			}
		}
	}
	else
	{
		sprintf(showtime, _("%02d:%02d"), min, sec);
	}

	if (sign == -1)
	{
		char oldtime[512];

		strcpy(oldtime, showtime);
		sprintf(showtime, "-%s", oldtime);

		*msec = -*msec;
	}
}


char *Info_SecsToTextShort(char *showtime, int len, float fsec)
{
	int sign, year, mon, day, hour, min, sec, msec;
	char txt[80];
	int first = 6;

	if (fsec < 0)
	{
		sign = -1;
		fsec = -fsec;
	}
	else
	{
		sign = 1;
	}

	year = (int)(fsec / 31558464.0f);
	fsec -= year * 31558464.0f;
	mon =  (int)(fsec / 2629872.0f);
	fsec -= mon  * 2629872.0f;
	day =  (int)(fsec / 86400.0f);
	fsec -= day  * 86400.0f;
	hour = (int)(fsec / 3600.0f);
	fsec -= hour * 3600.0f;
	min =  (int)(fsec / 60.0f);
	fsec -= day  * 60.0f;
	sec =  (int)(fsec);
	fsec -= (float)sec;
	msec = (int)(fsec * 1000.0f);

	showtime[0] = '\0';

	if (sign == -1)
	{
		strncat(showtime, "-", len - 1 - strlen(showtime));
	}

	if (year > 0)
	{
		if (year > 99)
		{
			strncat(showtime, _("??y "), year);
		}
		else
		{
			sprintf(txt, _("%dy "), year);
			strncat(showtime, txt, len - 1 - strlen(showtime));
		}
	}
	else if (mon > 0)
	{
		sprintf(txt, _("%dM "), mon);
		strncat(showtime, txt, len - 1 - strlen(showtime));
	}
	else if (day > 0)
	{
		sprintf(txt, _("%dd "), day);
		strncat(showtime, txt, len - 1 - strlen(showtime));
	}
	else if (hour > 0)
	{
		sprintf(txt, _("%dh "), hour);
		strncat(showtime, txt, len - 1 - strlen(showtime));
	}
	else if (min > 0)
	{
		sprintf(txt, _("%dm "), min);
		strncat(showtime, txt, len - 1 - strlen(showtime));
	}
	else if (sec > 0)
	{
		sprintf(txt, _("%ds "), sec);
		strncat(showtime, txt, len - 1 - strlen(showtime));
	}

	if (strlen(showtime) > 0)
	{
                showtime[strlen(showtime)-1] = '\0';
	}

	return showtime;
}


int Info_TimeControlToEstimatedTime(struct timecontrol_s *tc)
{
	int numcontrols, *p;
	int estimatedtime = 0;
	int i;

	if (!tc)
	{
		return 0;
	}

	p = tc->controlarray;

	numcontrols = *p++;

	for (i = 0; i < numcontrols; i++);
	{
		int imoves = *p++;
		int itime = *p++;

		if (imoves == -1 && itime != -1)
		{
			estimatedtime += itime;
		}
		else if (imoves != -1 && itime == -1)
		{
			/*estimatedtime += imoves * abs(tc->delayinc);*/
		}
		else if (imoves != -1 && itime != -1)
		{
			estimatedtime += itime;
			/*estimatedtime += imoves * abs(tc->delayinc);*/
		}
	}

	estimatedtime += 60 * abs(tc->delayinc);

	return estimatedtime;
}


char *Info_TimeControlToCategory(struct timecontrol_s *tc)
{
	int estimatedtime = Info_TimeControlToEstimatedTime(tc);

	if (!tc)
	{
		return _("No Time Limit");
	}

	if (tc->correspondence)
	{
		return _("Correspondence");
	}

	if (estimatedtime >= 30 * 60)
	{
		return _("Long");
	}
	else if (estimatedtime >= 15 * 60)
	{
		return _("Speed");
	}/*
	else if (estimatedtime >= 10 * 60)
	{
		return _("Rapid");
	}*/
	else if (estimatedtime >= 5 * 60)
	{
		return _("Blitz");
	}
	else if (estimatedtime > 0)
	{
		return _("Bullet");
	}
	else
	{
		return _("No Time Limit");
	}
}

char *Info_TimeControlToExactText(struct timecontrol_s *tc)
{
	char *outtxt;
	int numcontrols, *p, totaltime = 0, totalmoves = 0, firstcontrol;

	if (!tc)
	{
		return NULL;
	}

	p = tc->controlarray;

	numcontrols = *p++;

	outtxt = malloc(numcontrols * 80 + 8);
	outtxt[0] = '\0';

	firstcontrol = 1;

	if (tc->correspondence && numcontrols)
	{
		int imoves = *p++;
		int itime = *p++;

		sprintf(outtxt, _("^b%d moves^n within ^b%d days^n."), imoves, itime / (60 * 60 * 24));
	}
	else
	{
		char txt[80];

		while (numcontrols)
		{
			int imoves = *p++;
			int itime = *p++;

			if (imoves == -1 && itime != -1)
			{
				totaltime += itime;
				if (firstcontrol)
				{
					strcat(outtxt, _("^bGame^n in ^b"));
				}
				else
				{
					strcat(outtxt, _("^bSD^n in ^b"));
				}
				strcat(outtxt, Info_SecsToText1((float)itime));
				strcat(outtxt, "^n");
			}
			else if (imoves != -1 && itime == -1)
			{
				totalmoves += imoves;
				sprintf(txt, _("^b%d^n moves"), imoves);
				strcat(outtxt, txt);
			}
			else if (imoves != -1 && itime != -1)
			{
				totaltime += itime;
				totalmoves += imoves;
				sprintf(txt, _("^b%d^n in ^b%s^n"), imoves, Info_SecsToText1((float)itime));
				strcat(outtxt, txt);
			}
			
			firstcontrol = 0;
			numcontrols--;

			if (numcontrols)
			{
				strcat(outtxt, _(", "));
			}
		}

		if (tc->delayinc > 0)
		{
			sprintf(txt, _(" (+%ds)"), tc->delayinc);
		}
		else
		{
			sprintf(txt, _(" (%ds)"), -tc->delayinc);
		}
		strcat(outtxt, txt);
	}

	return outtxt;
}

char *Info_TimeControlToShortText(struct timecontrol_s *tc)
{
	char *outtxt;
	int numcontrols, *p, totaltime = 0, totalmoves = 0, firstcontrol;

	if (!tc)
	{
		return NULL;
	}

	p = tc->controlarray;

	numcontrols = *p++;

	outtxt = malloc(numcontrols * 80 + 8);
	outtxt[0] = '\0';

	firstcontrol = 1;

	if (tc->correspondence && numcontrols)
	{
		int imoves = *p++;
		int itime = *p++;

		sprintf(outtxt, _("%d/%dd"), imoves, itime / (60 * 60 * 24));
	}
	else
	{
		while (numcontrols)
		{
			char txt[80];
			int imoves = *p++;
			int itime = *p++;

			if (imoves == -1 && itime != -1)
			{
				totaltime += itime;
				if (firstcontrol)
				{
					strcat(outtxt, _("G/"));
				}
				else
				{
					strcat(outtxt, _("SD/"));
				}
				strcat(outtxt, Info_SecsToText1((float)itime));
			}
			else if (imoves != -1 && itime == -1)
			{
				totalmoves += imoves;
				sprintf(txt, _("%dmv"), imoves);
				strcat(outtxt, txt);
			}
			else if (imoves != -1 && itime != -1)
			{
				totaltime += itime;
				totalmoves += imoves;
				sprintf(txt, _("%d/%s"), imoves, Info_SecsToText1((float)itime));
				strcat(outtxt, txt);
			}
			
			if (tc->delayinc > 0)
			{
				sprintf(txt, _(" (+%ds)"), tc->delayinc);
			}
			else
			{
				sprintf(txt, _(" (%ds)"), -tc->delayinc);
			}
			strcat(outtxt, txt);
			

			firstcontrol = 0;
			numcontrols--;

			if (numcontrols)
			{
				strcat(outtxt, _(" "));
			}
		}
	}

	return outtxt;
}

char *Info_TimeControlsToShortText(struct timecontrol_s *wtc, struct timecontrol_s *btc)
{
	char txt[1024];

	if (!btc || Info_TimeControlsEqual(wtc, btc))
	{
		return Info_TimeControlToShortText(wtc);
	}

	sprintf(txt, "W:%s B:%s", Info_TimeControlToShortText(wtc), Info_TimeControlToShortText(btc));

	return strdup(txt);
}

char *Info_TimeControlToLongText(struct timecontrol_s *tc)
{
	char *category = Info_TimeControlToCategory(tc);
	char *exacttime = Info_TimeControlToExactText(tc);
	char *outtxt;

	if (exacttime)
	{
		outtxt = malloc(strlen(category) + strlen(exacttime) + 4);
		strcpy(outtxt, category);
		strcat(outtxt, " - ");
		strcat(outtxt, exacttime);
	}
	else
	{
		outtxt = strdup(category);
	}

	free(exacttime);

	return outtxt;
}


char *Info_TimeControlsToText(struct timecontrol_s *tc, struct timecontrol_s *blacktc)
{
	int estimatedtime;
	char *whitetime;
	/*char *blacktime;*/
	char *outtxt;


	if (!tc)
	{
		return strdup(_("No Time Limit"));
	}

	whitetime = Info_TimeControlToExactText(tc);

	/*if (!blacktc)*/
	{
		estimatedtime = Info_TimeControlToEstimatedTime(tc);
		outtxt = malloc(strlen(whitetime) + 512);
		strcpy(outtxt, whitetime);
	}
	/*
	else
	{
		estimatedtime = (Info_TimeControlToEstimatedTime(tc) + Info_TimeControlToEstimatedTime(blacktc)) / 2;
		blacktime = Info_TimeControlToExactText(blacktc);
		outtxt = malloc(strlen(whitetime) + strlen(blacktime) + 512);
		sprintf(outtxt, "white: %s  black: %s", whitetime, blacktime);
	}
	*/

	strcat(outtxt, _(" (about "));
	strcat(outtxt, Info_SecsToText1((float)estimatedtime));
	strcat(outtxt, _(")"));

	return outtxt;
}

char *Info_TimeControlsToMultilineText(struct timecontrol_s *tc, struct timecontrol_s *blacktc, char *buffer, int buflen)
{
	int estimatedtime;
	char *whitetime = NULL;
	char *blacktime = NULL;

	if (!tc)
	{
		strncpy(buffer, _("No Time Limit"), buflen);
		return buffer;
	}

	whitetime = Info_TimeControlToExactText(tc);

	if (!blacktc)
	{
		strncpy(buffer, whitetime, buflen);
	}
	else
	{
		blacktime = Info_TimeControlToExactText(blacktc);
		_snprintf(buffer, buflen, "^6%s^n\n%s: %s\n%s: %s", _("This game has asymmetric time"), _("White"), whitetime, _("Black"), blacktime);
	}

	free(whitetime);
	free(blacktime);

	return buffer;
}


char *Info_SearchLimitToText(struct searchlimit_s *limit)
{
	char *outtxt = malloc(120);
	char txt[80];

	if (!limit)
	{
		strcpy(outtxt, _("Any Rating"));
		return outtxt;
	}

	strcpy(outtxt, limit->type);

	if (limit->low != -1 && limit->high != -1)
	{
		sprintf(txt, _(" between %d and %d"), limit->low, limit->high);
	}
	else if (limit->low == -1)
	{
		sprintf(txt, _(" below %d"), limit->high);
	}
	else
	{
		sprintf(txt, _(" above %d"), limit->low);
	}

	strcat(outtxt, txt);

	return outtxt;
}

void Info_DestroyPlayerInfo(struct playerinfo_s *pi)
{
	if (!pi)
	{
		return;
	}

	free(pi->jid);
	free(pi->award);
	free(pi->rating);
	NamedList_Destroy2(pi->titles);
	NamedList_Destroy2(pi->roles);
	free(pi->membertype);
	free(pi);
}

struct playerinfo_s *Info_DupePlayerInfo(struct playerinfo_s *src)
{
	struct playerinfo_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));

	dst->jid    = strdup(src->jid);
	dst->rating = strdup(src->rating);
	dst->award  = strdup(src->award);
	dst->titles = NamedList_DupeStringList(src->titles);
	dst->roles  = NamedList_DupeStringList(src->roles);
	dst->notactivated = src->notactivated;
	dst->membertype = strdup(src->membertype);
	
	return dst;
}

struct gamesearchinfo_s *Info_DupeGameSearchInfo(struct gamesearchinfo_s *src)
{
	struct gamesearchinfo_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));

	memset(dst, 0, sizeof(*dst));

	dst->gameid = strdup(src->gameid);
	dst->node = strdup(src->node);

	dst->adplayer = Info_DupePlayerInfo(src->adplayer);
	dst->white    = Info_DupePlayerInfo(src->white);
	dst->black    = Info_DupePlayerInfo(src->black);

	dst->variant          = strdup(src->variant);
	dst->timecontrol      = Info_DupeTimeControl(src->timecontrol);
	dst->blacktimecontrol = Info_DupeTimeControl(src->blacktimecontrol);
	dst->timecontrolrange = strdup(src->timecontrolrange);

	dst->rated     = src->rated;
	dst->computers = src->computers;
	dst->titled    = src->titled;
	dst->takebacks = strdup(src->takebacks);
	dst->solitaire = src->solitaire;
	
	if (src->limit)
	{
		dst->limit = malloc(sizeof(*(dst->limit)));
		dst->limit->type = strdup(src->limit->type);
		dst->limit->low = src->limit->low;
		dst->limit->high = src->limit->high;
	}
	dst->relativerating = src->relativerating;
	dst->filterunrated = src->filterunrated;

	dst->keywords = strdup(src->keywords);
	dst->comment = strdup(src->comment);
	dst->correspondence = src->correspondence;
	dst->colorpreference = src->colorpreference;
	dst->category = strdup(src->category);

	dst->whiteclock = src->whiteclock;
	dst->blackclock = src->blackclock;
	dst->whitetomove = src->whitetomove;
	dst->blacktomove = src->blacktomove;

	dst->lastmove = strdup(src->lastmove);
	dst->movenum  = src->movenum;
	dst->offline  = src->offline;
	dst->date     = strdup(src->date);

	dst->tournament = src->tournament;
	dst->manager = strdup(src->manager);
	dst->tournamentname = strdup(src->tournamentname);
	dst->pairingtype = strdup(src->pairingtype);

	dst->filteropen   = src->filteropen;
	dst->quickapply   = src->quickapply;
	dst->hideown      = src->hideown;
	dst->showonlymine = src->showonlymine;

	dst->groupids = NamedList_DupeStringList(src->groupids);

	return dst;
}

void Info_DestroyGameSearchInfo(struct gamesearchinfo_s *info)
{
	Info_DestroyPlayerInfo(info->white);
	Info_DestroyPlayerInfo(info->black);
	Info_DestroyPlayerInfo(info->adplayer);

	free(info->variant);
	free(info->timecontrol);
	free(info->blacktimecontrol);
	free(info->timecontrolrange);
	
	if (info->limit)
	{
		free(info->limit->type);
		free(info->limit);
	}

	free(info->keywords);
	free(info->comment);
	free(info->category);

	free(info->lastmove);
	free(info->date);

	free(info->manager);
	free(info->tournamentname);
	free(info->pairingtype);
}

struct rating_s *Info_DupeRating(struct rating_s *src)
{
	struct rating_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));
	memset(dst, 0, sizeof(*dst));

	dst->rating = src->rating;
	dst->rd     = src->rd;
	dst->best   = src->best;
	dst->worst  = src->worst;
	dst->wins   = src->wins;
	dst->losses = src->losses;
	dst->draws  = src->draws;
	dst->prevrating = src->prevrating;

	return dst;
}

void Info_DestroyRating(struct rating_s *rating)
{
	free(rating);
}

void Info_DumpInfo(struct gamesearchinfo_s *info)
{
	Log_Write(0, "GameSearchInfo dump\n");

	if (!info)
	{
		Log_Write(0, "NULL info\n");
		return;
	}

	Log_Write(0, "gameid %s\n", info->gameid);

	if (info->white)
	{
		Log_Write(0, "white jid %s\n", info->white->jid);
		Log_Write(0, "white rating %s\n", info->white->rating);
		Log_Write(0, "white award %s\n", info->white->award);
	}

	if (info->black)
	{
		Log_Write(0, "black jid %s\n", info->black->jid);
		Log_Write(0, "black rating %s\n", info->black->rating);
		Log_Write(0, "black award %s\n", info->black->award);
	}

	Log_Write(0, "variant %s\n", info->variant);

	Log_Write(0, "timecontrol %d\n", info->timecontrol);

	Log_Write(0, "timecontrolrange %s\n", info->timecontrolrange);

	Log_Write(0, "rated %d\n", info->rated);
	Log_Write(0, "computers %d\n", info->computers);
	Log_Write(0, "titled %d\n", info->titled);

	if (info->limit)
	{
		Log_Write(0, "Limit type %s\n", info->limit->type);
		Log_Write(0, "Limit low %d\n", info->limit->low);
		Log_Write(0, "Limit high %d\n", info->limit->high);
	}
	
	Log_Write(0, "keywords %s\n", info->keywords);
}

char *Info_GetHighestRating(struct namedlist_s *ratinglist)
{
	char *rating;
	int highrating = -1;

	while (ratinglist)
	{
		struct rating_s *ratinginfo = ratinglist->data;

		if (ratinginfo->rating > highrating)
		{
			highrating = ratinginfo->rating;
		}

		ratinglist = ratinglist->next;
	}

	if (highrating == -1)
	{
		rating = strdup(_("Unrated"));
	}
	else
	{
		char txt[80];
		sprintf(txt, "%d", highrating);
		rating = strdup(txt);
	}

	return rating;
}


char *Info_GetStandardRating(struct namedlist_s *ratinglist)
{
	int highrating = -1;
	struct namedlist_s **entry = NamedList_GetByName(&ratinglist, "Standard");

	if (entry)
	{
		struct rating_s *ratinginfo = (*entry)->data;
		char txt[80];
		sprintf(txt, "%d", ratinginfo->rating);
		return strdup(txt);
	}

	return strdup(_("Unrated"));
}

struct namedlist_s *Info_DupeRatingList(struct namedlist_s *src)
{
	struct namedlist_s *dst = NULL;

	while (src)
	{
		struct rating_s *srcratinginfo = src->data;

		NamedList_Add(&dst, src->name, Info_DupeRating(src->data), Info_DestroyRating);

		src = src->next;
	}

	return dst;
}

void Info_ConvertTimeTToTimeOfDay(time_t t1, 
	int *dstsec, int *dstmin, int *dsthour,
	int *dstday, int *dstmon, int *dstyear)
{
	struct tm *t2;

	t2 = localtime(&t1);

	if (dstsec)
	{
		*dstsec = t2->tm_sec;
	}

	if (dstmin)
	{
		*dstmin = t2->tm_min;
	}

	if (dsthour)
	{
		*dsthour = t2->tm_hour;
		if (t2->tm_isdst > 0)
		{
			*dsthour++;
		}
	}

	if (dstday)
	{
		*dstday = t2->tm_mday;
	}

	if (dstmon)
	{
		*dstmon = t2->tm_mon + 1;
	}

	if (dstyear)
	{
		*dstyear = t2->tm_year + 1900;
	}
}

time_t Info_ConvertTimeOfDayToTimeT(
	int sec, int min, int hour,
	int day, int mon, int year)
{
	time_t t1;
	struct tm t2;

	t2.tm_sec = sec;
	t2.tm_min = min;
	t2.tm_hour = hour;
	t2.tm_mday = day;
	t2.tm_mon = mon - 1;
	t2.tm_year = year - 1900;
	t2.tm_isdst = -1;

	t1 = mktime(&t2);

	return t1;
}

time_t Info_ConvertTimestampToTimeT(char *timestamp)
{
	int sec, min, hour, day, mon, year, hourdiff, mindiff, numparsed;
	float fsec;
	struct tm t2;

	/*YYYY-MM-DD HH:MM:SS.SSSSSS-HH:MM*/
	numparsed = sscanf(timestamp, "%04d-%02d-%02d %02d:%02d:%f-%02d:%02d", &year, &mon, &day, &hour, &min, &fsec, &hourdiff, &mindiff);
	if (numparsed == 8)
	{
		t2.tm_sec = (unsigned int)fsec;
		t2.tm_min = min;
		t2.tm_hour = hour + hourdiff;
		t2.tm_mday = day + mindiff;
		t2.tm_mon = mon - 1;
		t2.tm_year = year - 1900;
		t2.tm_isdst = 0;

		return mktime(&t2) - timezone;
	}

	/*YYYY-MM-DD HH:MM:SS.SSSSSS+HH:MM*/
	numparsed = sscanf(timestamp, "%04d-%02d-%02d %02d:%02d:%f+%02d:%02d", &year, &mon, &day, &hour, &min, &fsec, &hourdiff, &mindiff);
	if (numparsed == 8)
	{
		t2.tm_sec = (unsigned int)fsec;
		t2.tm_min = min;
		t2.tm_hour = hour - hourdiff;
		t2.tm_mday = day - mindiff;
		t2.tm_mon = mon - 1;
		t2.tm_year = year - 1900;
		t2.tm_isdst = 0;

		return mktime(&t2) - timezone;
	}

	/*YYYY-MM-DD HH:MM:SS*/
	numparsed = sscanf(timestamp, "%04d-%02d-%02d %02d:%02d:%d", &year, &mon, &day, &hour, &min, &sec);
	if (numparsed == 6)
	{
		t2.tm_sec = sec;
		t2.tm_min = min;
		t2.tm_hour = hour;
		t2.tm_mday = day;
		t2.tm_mon = mon - 1;
		t2.tm_year = year - 1900;
		t2.tm_isdst = 0;

		return mktime(&t2) - timezone;
	}

	/*YYYYMMDDTHH:MM:SS*/
	numparsed = sscanf(timestamp, "%04d%02d%02dT%02d:%02d:%02d",
		&year, &mon, &day, &hour, &min, &sec);

	if (numparsed == 6)
	{
		t2.tm_sec = sec;
		t2.tm_min = min;
		t2.tm_hour = hour;
		t2.tm_mday = day;
		t2.tm_mon = mon - 1;
		t2.tm_year = year - 1900;
		t2.tm_isdst = 0;

		return mktime(&t2) - timezone;
	}

	/*YYYY-MM-DDTHH:MM:SS*/
	numparsed = sscanf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02d",
		&year, &mon, &day, &hour, &min, &sec);

	if (numparsed == 6)
	{
		t2.tm_sec = sec;
		t2.tm_min = min;
		t2.tm_hour = hour;
		t2.tm_mday = day;
		t2.tm_mon = mon - 1;
		t2.tm_year = year - 1900;
		t2.tm_isdst = 0;

		return mktime(&t2) - timezone;
	}

	/*YYYY-MM-DD*/
	numparsed = sscanf(timestamp, "%04d-%02d-%02d",
		&year, &mon, &day);

	if (numparsed == 3)
	{
		t2.tm_sec = 0;
		t2.tm_min = 0;
		t2.tm_hour = 0;
		t2.tm_mday = day;
		t2.tm_mon = mon - 1;
		t2.tm_year = year - 1900;
		t2.tm_isdst = 0;

		return mktime(&t2) - timezone;
	}

	return 0;
}

char *Info_ConvertTimeTToTimestamp(time_t t1)
{
	char *timestamp = malloc(256);
	struct tm *t2;

	t2 = gmtime(&t1);

	sprintf(timestamp, "%04d%02d%02dT%02d:%02d:%02d",
		t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday, t2->tm_hour, t2->tm_min, t2->tm_sec);

	return timestamp;
}

void Info_ConvertTimestampToTimeOfDay(char *timestamp,
	int *dstsec, int *dstmin, int *dsthour,
	int *dstday, int *dstmon, int *dstyear)
{
	time_t t1;

	t1 = Info_ConvertTimestampToTimeT(timestamp);
	Info_ConvertTimeTToTimeOfDay(t1, dstsec, dstmin, dsthour, dstday, dstmon, dstyear);
}

char *Info_ConvertTimeOfDayToTimestamp(
	int sec, int min, int hour,
	int day, int mon, int year)
{
	time_t t1 = Info_ConvertTimeOfDayToTimeT(sec, min, hour, day, mon, year) + timezone;
	return Info_ConvertTimeTToTimestamp(t1);
}

void Info_GetTimeOfDay(int *dstsec, int *dstmin, int *dsthour,
	int *dstday, int *dstmon, int *dstyear)
{
    Info_ConvertTimeTToTimeOfDay(time(NULL), dstsec, dstmin, dsthour, dstday, dstmon, dstyear);
}

int Info_GetTimeDiffFromNow(time_t t1)
{
	return (int)(difftime(time(NULL), t1));
}

int Info_GetTimeDiffFromNowNeg(time_t t1)
{
	return (int)(difftime(t1, time(NULL)));
}

char *Info_GetCurrentTimestamp()
{
	return Info_ConvertTimeTToTimestamp(time(NULL));
}

char *Info_GetCurrentTimestamp2()
{
	SYSTEMTIME st;
	char timestamp[256];
	
	/* Use windows GetLocalTime() for higher resolution */
	GetLocalTime(&st);

	sprintf(timestamp, "%04d%02d%02dT%02d:%02d:%02d.%03d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	return strdup(timestamp);
}

char *Info_GetLocalDate()
{
	char *date = malloc(256);
	time_t t1 = time(NULL);
	struct tm *t2;

	t2 = localtime(&t1);

	if (!t2)
	{
		sprintf(date, "????-??-??");
		return date;
	}

	sprintf(date, "%04d-%02d-%02d", t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday);

	return date;
}

void Info_24HourTo12Hour(int *hour, char *ampm)
{
	if (*hour >= 12)
	{
		strcpy(ampm, _("pm"));
		*hour -= 12;
	}
	else
	{
		strcpy(ampm, _("am"));
	}

	if (*hour == 0)
	{
		*hour = 12;
	}
}

char *Info_GetLocalTime()
{
	char *ttime = malloc(256);
	time_t t1 = time(NULL);
	struct tm *t2;
	int hour;
	char ampm[25];

	t2 = localtime(&t1);

	hour = t2->tm_hour;

	Info_24HourTo12Hour(&hour, ampm);

	sprintf(ttime, "%d:%02d %s", hour, t2->tm_min, ampm);

	return ttime;
}

char *Info_GetLocalTime2()
{
	char *ttime = malloc(256);
	time_t t1 = time(NULL);
	struct tm *t2;
	int hour;
	char ampm[25];

	t2 = localtime(&t1);

	hour = t2->tm_hour;

	Info_24HourTo12Hour(&hour, ampm);

	sprintf(ttime, "%d:%02d:%02d %s", hour, t2->tm_min, t2->tm_sec, ampm);

	return ttime;
}


char *Info_TimestampToLocalTime2(char *timestamp)
{
	char *ttime = malloc(256);
	time_t t1 = Info_ConvertTimestampToTimeT(timestamp);
	struct tm *t2;
	int hour;
	char ampm[25];

	t2 = localtime(&t1);

	hour = t2->tm_hour;

	Info_24HourTo12Hour(&hour, ampm);

	sprintf(ttime, "%d:%02d:%02d %s", hour, t2->tm_min, t2->tm_sec, ampm);

	return ttime;
}

char *Info_ConvertTimeTToLongDate(time_t t1)
{
	char ttime[512];
	struct tm *t2;
	/*char *months = _("January"), _("February"), _("March"), _("April"), _("May"), _("June"), _("July"), _("August"), _("September"), _("October"), _("November"), _("December");*/
	char *months[12] = {_("Jan"), _("Feb"), _("Mar"), _("Apr"), _("May"), _("Jun"), _("Jul"), _("Aug"), _("Sep"), _("Oct"), _("Nov"), _("Dec")};

	t2 = localtime(&t1);

	sprintf(ttime, "%s %d, %d", months[t2->tm_mon], t2->tm_mday, t2->tm_year + 1900);

	return strdup(ttime);
}

char *Info_ConvertTimestampToLongDate(char *timestamp)
{
	return Info_ConvertTimeTToLongDate(Info_ConvertTimestampToTimeT(timestamp));
}

char *Info_ConvertTimeTToLongDateAndTime(time_t t1)
{
	char ttime[512];
	struct tm *t2;
	/*char *months = _("January"), _("February"), _("March"), _("April"), _("May"), _("June"), _("July"), _("August"), _("September"), _("October"), _("November"), _("December");*/
	char *months[12] = {_("Jan"), _("Feb"), _("Mar"), _("Apr"), _("May"), _("Jun"), _("Jul"), _("Aug"), _("Sep"), _("Oct"), _("Nov"), _("Dec")};
	char ampm[25];
	int hour;

	t2 = localtime(&t1);

	hour = t2->tm_hour;

	Info_24HourTo12Hour(&hour, ampm);

	sprintf(ttime, "%d:%02d %s, %s %d, %d", hour, t2->tm_min, ampm, months[t2->tm_mon], t2->tm_mday, t2->tm_year + 1900);

	return strdup(ttime);
}

char *Info_ConvertTimestampToLongDateAndTime(char *timestamp)
{
	return Info_ConvertTimeTToLongDateAndTime(Info_ConvertTimestampToTimeT(timestamp));
}

struct tournamentpairing_s *Info_DupeTournamentPairing(struct tournamentpairing_s *src)
{
	struct tournamentpairing_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));

	dst->bye       = strdup(src->bye);
	dst->black     = strdup(src->black);
	dst->white     = strdup(src->white);
	dst->gamestate = src->gamestate;
	dst->gameid    = strdup(src->gameid);
	dst->winner    = strdup(src->winner);

	return dst;
}

void Info_DestroyTournamentPairing(struct tournamentpairing_s *pairing)
{
	if (pairing)
	{
		free(pairing->black);
		free(pairing->white);
		free(pairing->gameid);
		free(pairing->winner);
	}
	free(pairing);
}

struct tournamentplayerinfo_s *Info_DupeTournamentPlayerInfo(struct tournamentplayerinfo_s *src)
{
	struct tournamentplayerinfo_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));

	dst->jid = strdup(src->jid);
	dst->score  = src->score;
	dst->wins   = src->wins;
	dst->draws  = src->draws;
	dst->losses = src->losses;
	dst->rating = src->rating;

	return dst;
}

void Info_DestroyTournamentPlayerInfo(struct tournamentplayerinfo_s *info)
{
	free(info);
}

struct tournamentinfo_s *Info_DupeTournamentInfo(struct tournamentinfo_s *src)
{
	struct tournamentinfo_s *dst;
	struct namedlist_s *currentry;

	dst = malloc(sizeof(*dst));

	memset(dst, 0, sizeof(*dst));

	dst->id           = strdup(src->id);
	dst->manager      = strdup(src->manager);
	dst->name         = strdup(src->name);
	dst->variant      = strdup(src->variant);
	dst->pairingtype  = strdup(src->pairingtype);
	dst->timecontrol  = strdup(src->timecontrol);
	dst->currentround = src->currentround;
	dst->totalrounds  = src->totalrounds;

	currentry = src->players;

	while (currentry)
	{
		NamedList_Add(&(dst->players), currentry->name, Info_DupeTournamentPlayerInfo(currentry->data), Info_DestroyTournamentPlayerInfo);
		currentry = currentry->next;
	}

	currentry = src->roundstarttimes;

	while (currentry)
	{
		NamedList_Add(&(dst->roundstarttimes), currentry->name, strdup(currentry->data), NULL);
		currentry = currentry->next;
	}

	currentry = src->roundpairings;

	while (currentry)
	{
		struct namedlist_s *currentry2 = currentry->data;
		struct namedlist_s *pairinglist = NULL;

		while (currentry2)
		{
			NamedList_Add(&(pairinglist), currentry2->name, Info_DupeTournamentPairing(currentry2->data), Info_DestroyTournamentPairing);
			currentry2 = currentry2->next;
		}

		NamedList_Add(&(dst->roundpairings), currentry->name, pairinglist, NamedList_Destroy2);
		currentry = currentry->next;
	}
	return dst;
}

struct adhoccommand_s *Info_DupeAdHocCommand(struct adhoccommand_s *src)
{
	struct adhoccommand_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));
	memset(dst, 0, sizeof(*dst));

	dst->name = strdup(src->name);
	dst->command = strdup(src->command);

	return dst;
}

void Info_DestroyAdHocCommand(struct adhoccommand_s *cmd)
{
	if (cmd)
	{
                free(cmd->name);
                free(cmd->command);
	}
	free(cmd);
}

struct profile_s *Info_DupeProfile(struct profile_s *src)
{
	struct profile_s *dst;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(sizeof(*dst));
	memset(dst, 0, sizeof(*dst));

	dst->ratings       = Info_DupeRatingList(src->ratings);
	dst->roles         = NamedList_DupeStringList(src->roles);
	dst->titles        = NamedList_DupeStringList(src->titles);
	dst->groups        = NamedList_DupeList(src->groups, Info_DupeGroupInfo, Info_DestroyGroupInfo);
	dst->nickname      = strdup(src->nickname);
	dst->avatarhash    = strdup(src->avatarhash);
	dst->description   = strdup(src->description);
	dst->membersince   = strdup(src->membersince);
	dst->lastonline    = strdup(src->lastonline);
	dst->clientname    = strdup(src->clientname);
	dst->clientversion = strdup(src->clientversion);
	dst->clientvendor  = strdup(src->clientvendor);
	dst->clientos      = strdup(src->clientos);
	dst->membertype    = strdup(src->membertype);
	dst->countrycode   = strdup(src->countrycode);
	dst->location      = strdup(src->location);
	dst->rank          = strdup(src->rank);

	return dst;
}

void Info_DestroyProfile(struct profile_s *profile)
{
	NamedList_Destroy(&(profile->ratings));
	NamedList_Destroy(&(profile->roles));
	NamedList_Destroy(&(profile->titles));
	free(profile->nickname);
	free(profile->description);
	free(profile->membersince);
	free(profile->lastonline);
	free(profile->clientname);
	free(profile->clientversion);
	free(profile->clientvendor);
	free(profile->clientos);
	free(profile->membertype);
	free(profile->countrycode);
	free(profile->location);
	free(profile->rank);

	free(profile);
}

struct privacylistentry_s *Info_DupePrivacyListEntry(struct privacylistentry_s *src)
{
	struct privacylistentry_s *dst;

	dst = malloc(sizeof(*dst));
	memset(dst, 0, sizeof(*dst));

	dst->type   = strdup(src->type);
	dst->value  = strdup(src->value);
	dst->action = strdup(src->action);
	dst->order  = strdup(src->order);

	return dst;
}

void Info_DestroyPrivacyListEntry(struct privacylistentry_s *entry)
{
	free(entry->type);
	free(entry->value);
	free(entry->action);
	free(entry->order);
	free(entry);
}

void Info_DestroyTCPair(struct tcpair_s *tcp)
{
	Info_DestroyTimeControl(tcp->white);
	Info_DestroyTimeControl(tcp->black);
	free(tcp);
}

struct groupinfo_s *Info_DupeGroupInfo(struct groupinfo_s *src)
{
	struct groupinfo_s *dst;

	dst = malloc(sizeof(*dst));
	memset(dst, 0, sizeof(*dst));

	dst->name        = strdup(src->name);
	dst->chat        = strdup(src->chat);
	dst->forum       = strdup(src->forum);
	dst->type        = strdup(src->type);
	dst->id          = strdup(src->id);
	dst->roles       = NamedList_DupeStringList(src->roles);
	dst->titles      = NamedList_DupeStringList(src->titles);
	dst->avatar      = strdup(src->avatar);
	dst->permissions = NamedList_DupeStringList(src->permissions);

	return dst;
}

struct groupinfo_s *Info_DestroyGroupInfo(struct groupinfo_s *ginfo)
{
	free(ginfo->name);
	free(ginfo->chat);
	free(ginfo->forum);
	free(ginfo->type);
	free(ginfo->id);
	NamedList_Destroy(&(ginfo->roles));
	NamedList_Destroy(&(ginfo->titles));
	free(ginfo->avatar);
	NamedList_Destroy(&(ginfo->permissions));
	free(ginfo);
}
