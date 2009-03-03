#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#include "util.h"

char *Jid_Strip(const char *jid)
{
	char *barejid = strdup(jid);
	
	if (barejid)
	{
		char *slash = strchr(barejid, '/');
		if (slash)
		{
			*slash = 0;
		}
	}
	return barejid;
}

char *Jid_GetBeforeAt(const char *jid)
{
	char *barejid = Jid_Strip(jid);

	if (barejid)
	{
		char *at = strrchr(barejid, '@');
		if (at)
		{
			*at = 0;
		}
	}
	return barejid;
}


char *Jid_GetResource(const char *jid)
{
	char *resource = NULL;

	if (jid)
	{
		char *slash = strchr(jid, '/');
		if (slash)
		{
			resource = strdup(slash + 1);
		}
	}

	return resource;
}

int Node_IsValid(const char *node)
{
	static char *invalidchars = "\"&'/:<>@ ";

	if (!node)
	{
		return 1;
	}

	return (strcspn(node, invalidchars) == strlen(node));
}

int Domain_IsValid(const char *domain)
{
	static char *invalidchars = "\"&'/:<>@ ";

	if (!domain)
	{
		return 0;
	}

	return (strcspn(domain, invalidchars) == strlen(domain));
}

int Resource_IsValid(const char *resource)
{
	static char *invalidchars = "\"&'/:<>@ ";

	if (!resource)
	{
		return 1;
	}
	return (strcspn(resource, invalidchars) == strlen(resource));
}

int Jid_IsValid(const char *jid)
{
	char *node, *domain, *resource;
	int valid = 0;

	if (!jid)
	{
		return 0;
	}

	node = strdup(jid);

	domain = strchr(node, '@');

	if (domain)
	{
		*domain = '\0';
		domain++;
	}
	else
	{
		domain = node;
		node = NULL;
	}

	resource = strchr(domain, '/');

	if (resource)
	{
		*resource = '\0';
		resource++;
	}

	valid = Node_IsValid(node) && Domain_IsValid(domain) && Resource_IsValid(resource);

	if (node)
	{
		free(node);
	}
	else
	{
		free(domain);
	}

	return valid;
}

HKEY OpenClientRegKey(int access)
{
	HKEY currentUser, clientkey;

/*	if (RegOpenCurrentUser(KEY_NOTIFY, &currentUser) != ERROR_SUCCESS)*/
	if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0, KEY_NOTIFY, &currentUser) != ERROR_SUCCESS)
	{
		return NULL;
	}

	if (RegCreateKeyEx(currentUser, "Software\\ChessPark\\client", 0, NULL, REG_OPTION_NON_VOLATILE, access, NULL, &clientkey, NULL) != ERROR_SUCCESS)
	{
		RegCloseKey(currentUser);
		return NULL;
	}

	RegCloseKey(currentUser);
	return clientkey;
}


BOOL SetRegString(char *key, char *value)
{
	HKEY clientkey;

	if (!(clientkey = OpenClientRegKey(KEY_WRITE)))
	{
		return FALSE;
	}

	if (RegSetValueEx(clientkey, key, 0, REG_SZ, value, (DWORD)(strlen(value) + 1)) != ERROR_SUCCESS);
	{
		RegCloseKey(clientkey);
		return FALSE;
	}
	
	RegCloseKey(clientkey);
	return TRUE;
}


char *GetRegString(char *key, char *in, int size)
{
	HKEY clientkey;
	DWORD type;

	if (!(clientkey = OpenClientRegKey(KEY_READ)))
	{
		return NULL;
	}
		
	if (RegQueryValueEx(clientkey, key, NULL, &type, in, &size) != ERROR_SUCCESS)
	{
		RegCloseKey(clientkey);
		return NULL;
	}

	if (type != REG_SZ)
	{
		RegCloseKey(clientkey);
		return NULL;
	}
	
	RegCloseKey(clientkey);
	return in;
}


BOOL SetRegInt(char *key, int value)
{
	HKEY clientkey;
	DWORD out = value;

	if (!(clientkey = OpenClientRegKey(KEY_WRITE)))
	{
		return FALSE;
	}

	if (RegSetValueEx(clientkey, key, 0, REG_DWORD, (BYTE *)&out, sizeof(out)) != ERROR_SUCCESS);
	{
		RegCloseKey(clientkey);
		return FALSE;
	}
	
	RegCloseKey(clientkey);
	return TRUE;
}


int GetRegInt(char *key)
{
	HKEY clientkey;
	DWORD in, size, type;

	size = sizeof(in);
	
	if (!(clientkey = OpenClientRegKey(KEY_READ)))
	{
		return 0;
	}
		
	if (RegQueryValueEx(clientkey, key, NULL, &type, (BYTE *)&in, &size) != ERROR_SUCCESS)
	{
		RegCloseKey(clientkey);
		return 0;
	}

	if (type != REG_DWORD)
	{
		RegCloseKey(clientkey);
		return 0;
	}
	
	RegCloseKey(clientkey);
	return in;
}


HKEY OpenUserRegKey(char *jid, int access)
{
	HKEY currentUser, userkey;
	char userstring[256];

/*	if (RegOpenCurrentUser(KEY_NOTIFY, &currentUser) != ERROR_SUCCESS)*/
	if (RegOpenKeyEx(HKEY_CURRENT_USER, NULL, 0, KEY_NOTIFY, &currentUser) != ERROR_SUCCESS)
	{
		return NULL;
	}

	userstring[0] = 0;

	strcat(userstring, "Software\\ChessPark\\client\\");
	strcat(userstring, jid);

	if (RegCreateKeyEx(currentUser, userstring, 0, NULL, REG_OPTION_NON_VOLATILE, access, NULL, &userkey, NULL) != ERROR_SUCCESS)
	{
		RegCloseKey(currentUser);
		return NULL;
	}

	RegCloseKey(currentUser);
	return userkey;
}


BOOL SetUserRegString(char *jid, char *key, char *value)
{
	HKEY userkey;
	int len = 0;

	if (!(userkey = OpenUserRegKey(jid, KEY_WRITE)))
	{
		return FALSE;
	}

	if (value)
	{
		len = (int)(strlen(value)) + 1;
	}

	if (RegSetValueEx(userkey, key, 0, REG_SZ, value, (DWORD)(len)) != ERROR_SUCCESS)
	{
		RegCloseKey(userkey);
		return FALSE;
	}
	
	RegCloseKey(userkey);
	return TRUE;
}


char *GetUserRegString(char *jid, char *key, char *in, int size)
{
	HKEY userkey;
	DWORD type, error;

	in[0] = 0;

	if (!(userkey = OpenUserRegKey(jid, KEY_READ)))
	{
		return NULL;
	}
		
	if ((error = RegQueryValueEx(userkey, key, NULL, &type, in, &size)) != ERROR_SUCCESS)
	{
		RegCloseKey(userkey);
		return NULL;
	}

	if (type != REG_SZ)
	{
		RegCloseKey(userkey);
		return NULL;
	}
	
	RegCloseKey(userkey);

	if (in[0] == 0)
	{
		return NULL;
	}
	
	return in;
}


BOOL SetUserRegInt(char *jid, char *key, int value)
{
	HKEY userkey;
	DWORD out = value;

	if (!(userkey = OpenUserRegKey(jid, KEY_WRITE)))
	{
		return FALSE;
	}

	if (RegSetValueEx(userkey, key, 0, REG_DWORD, (BYTE *)&out, sizeof(out)) != ERROR_SUCCESS);
	{
		RegCloseKey(userkey);
		return FALSE;
	}
	
	RegCloseKey(userkey);
	return TRUE;
}


int GetUserRegInt(char *jid, char *key)
{
	HKEY userkey;
	DWORD in, size, type;

	size = sizeof(in);
	
	if (!(userkey = OpenUserRegKey(jid, KEY_READ)))
	{
		return 0;
	}
		
	if (RegQueryValueEx(userkey, key, NULL, &type, (BYTE *)&in, &size) != ERROR_SUCCESS)
	{
		RegCloseKey(userkey);
		return 0;
	}

	if (type != REG_DWORD)
	{
		RegCloseKey(userkey);
		return 0;
	}
	
	RegCloseKey(userkey);
	return in;
}


struct StatusList_s *StatusList_Copy(struct StatusList_s *list)
{
	struct StatusList_s *newlist = NULL, **entry;

	entry = &newlist;

	while (list)
	{
		*entry = malloc(sizeof(**entry));
		(*entry)->next = NULL;
		(*entry)->status = list->status;
		(*entry)->statusmsg = strdup(list->statusmsg);

		list = list->next;
		entry = &((*entry)->next);
	}

	return newlist;
}


void StatusList_Add(struct StatusList_s **list, enum SStatus sstatus, char *statusmsg)
{
	while (*list)
	{
		list = &((*list)->next);
	}

	*list = malloc(sizeof(**list));

	(*list)->next = NULL;
	(*list)->status = sstatus;
	(*list)->statusmsg = strdup(statusmsg);
}

void StatusList_Remove(struct StatusList_s **list, enum SStatus sstatus, char *statusmsg)
{
	while (*list)
	{
		if ((*list)->status == sstatus 
			&& ((statusmsg == NULL && (*list)->statusmsg == NULL)
				|| (statusmsg != NULL && (*list)->statusmsg != NULL && strcmp(statusmsg, (*list)->statusmsg) == 0)))
		{
			struct StatusList_s *old = *list;
			*list = (*list)->next;
			free(old->statusmsg);
			free(old);
			return;
		}
		list = &((*list)->next);
	}
}

struct StatusList_s *StatusList_GetNum(struct StatusList_s *list, int num)
{
	num -= 1;

	while (num)
	{
		num--;
		list = list->next;
	}

	return list;
}

struct StatusList_s *StatusList_Find(struct StatusList_s *list, char *name)
{
	while (list)
	{
		if (strcmp(list->statusmsg, name) == 0)
		{
			return list;
		}
		list = list->next;
	}
	
	return list;
}

void StatusList_Destroy(struct StatusList_s **list)
{
	if (!*list)
	{
		return;
	}

	while ((*list)->next)
	{
		StatusList_Destroy(&(*list)->next);
	}

	free((*list)->statusmsg);
	free((*list));
	*list = NULL;
}
#if 0
void StatusList_Save(struct StatusList_s *list, char *jid, char *name)
{
	char statusstring[2048];

	statusstring[0] = 0;

	while (list)
	{
		int len;
		strcat(statusstring, list->statusmsg);
		len = (int)strlen(statusstring);
		statusstring[len] = 27;
		statusstring[len+1] = 0;

		list = list->next;
	}

	SetUserRegString(jid, name, statusstring);
}

struct StatusList_s *StatusList_Load(char *jid, char *name, enum SStatus status)
{
	struct StatusList_s *list = NULL;
	char *statusstring, *statusend, currentstatus[512];
	char buffer[2048];

	statusstring = GetUserRegString(jid, name, buffer, 2048);

	if (!statusstring)
	{
		return NULL;
	}

	while (statusend = strchr(statusstring, 27))
	{
		strncpy(currentstatus, statusstring, statusend - statusstring);
		currentstatus[statusend - statusstring] = 0;
		StatusList_Add(&list, status, currentstatus);
		statusstring = statusend + 1;
	}

	return list;
}
#endif
char *EscapeUnparsedText(char *src)
{
	char *p = src, *dst;
	int size = 1;

	if (!src)
	{
		return NULL;
	}

	while (*p)
	{
		switch(*p)
		{
			case '"':
				size += (int)strlen("\\\"");
				break;
			case '\'':
				size += (int)strlen("&apos;");
				break;
			case '&':
				size += (int)strlen("&amp;");
				break;
			case '<':
				size += (int)strlen("&lt;");
				break;
			case '>':
				size += (int)strlen("&gt;");
				break;
			case '\\':
				size += (int)strlen("\\\\");
			default:
				size++;
				break;
		}
		p++;
	}

	dst = malloc(size);
	dst[0] = '\0';
	p = src;

	while (*p)
	{
		switch(*p)
		{
			case '"':
				strcat(dst, "\\\"");
				break;
			case '\'':
				strcat(dst, "&apos;");
				break;
			case '&':
				strcat(dst, "&amp;");
				break;
			case '<':
				strcat(dst, "&lt;");
				break;
			case '>':
				strcat(dst, "&gt;");
				break;
			case '\\':
				strcat(dst, "\\\\");
				break;
			default:
				strncat(dst, p, 1);
				break;
		}
		p++;
	}

	return dst;
}

char *UnescapeUnparsedText(char *src)
{
	char *dst;
	char *p = src;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(strlen(src));
	dst[0] = 0;

	while (*p)
	{
		switch(*p)
		{
			case '\\':
				p++;
				switch (*p)
				{
					case '\"':
						strcat(dst, "\"");
						p++;
						break;
					case '\\':
						strcat(dst, "\\");
						p++;
						break;
					default:
						p++;
						break;
				}
				break;

			case '&':
				if (strncmp(p, "&apos;", 6) == 0)
				{
					strcat(dst, "'");
					p += 6;
				}
				else if (strncmp(p, "&amp;", 5) == 0)
				{
					strcat(dst, "&");
					p += 5;
				}
				else if (strncmp(p, "&lt;", 4) == 0)
				{
					strcat(dst, "<");
					p += 4;
				}
				else if (strncmp(p, "&gt;", 4) == 0)
				{
					strcat(dst, ">");
					p += 4;
				}
				else
				{
					strncat(dst, p, 1);
					p++;
				}
				break;
			default:
				strncat(dst, p, 1);
				p++;
				break;
		}
	}

	return dst;
}

char *EscapeParsedText(char *src)
{
	unsigned char *p = src, *dst;
	int size = 1;

	if (!src)
	{
		return NULL;
	}

	while (*p)
	{
		if (!(*p & 0x80))
		{
			switch(*p)
			{
				case '"':
					size += (int)strlen("&quot;");
					break;
				case '\'':
					size += (int)strlen("&apos;");
					break;
				case '&':
					size += (int)strlen("&amp;");
					break;
				case '<':
					size += (int)strlen("&lt;");
					break;
				case '>':
					size += (int)strlen("&gt;");
					break;
				default:
					size++;
					break;
			}
			p++;
		}
		else
		{
			if ((*p & 0xE0) == 0xC0)
			{
				size += 2;
				p += 2;
			}
			else if ((*p & 0xF0) == 0xE0)
			{
				size += 3;
				p += 3;
			}
			else if ((*p & 0xF8) == 0xF0)
			{
				size += 4;
				p += 4;
			}
			else
			{
				Log_Write(0, "caution: bad UTF8 trying to escape string, p == %d\n", *p);
				size += 1;
				p++;
			}
		}
	}

	dst = malloc(size);
	dst[0] = '\0';
	p = src;

	while (*p)
	{
		if (!(*p & 0x80))
		{
			switch(*p)
			{
				case '"':
					strcat(dst, "&quot;");
					break;
				case '\'':
					strcat(dst, "&apos;");
					break;
				case '&':
					strcat(dst, "&amp;");
					break;
				case '<':
					strcat(dst, "&lt;");
					break;
				case '>':
					strcat(dst, "&gt;");
					break;
				default:
					strncat(dst, p, 1);
					break;
			}
			p++;
		}
		else
		{
			if ((*p & 0xE0) == 0xC0)
			{
				strncat(dst, p, 2);
				p += 2;
			}
			else if ((*p & 0xF0) == 0xE0)
			{
				strncat(dst, p, 3);
				p += 3;
			}
			else if ((*p & 0xF8) == 0xF0)
			{
				strncat(dst, p, 4);
				p += 4;
			}
			else
			{
				strncat(dst, p, 1);
				p++;
			}
		}
	}

	Log_Write(0, "Escaped %s\n", dst);

	return dst;
}

int EscapeToHTML(unsigned char *dst, int dstlen, unsigned char *src)
{
	unsigned char *p = src;
	unsigned char *p2 = dst;
	int size = 1;

	if (!src)
	{
		return 0;
	}

	while (*p)
	{
		if (!(*p & 0x80))
		{
			switch(*p)
			{
				case '"':
					size += 6; /*(int)strlen("&quot;");*/
					break;
				case '\'':
					size += 6; /*(int)strlen("&apos;");*/
					break;
				case '&':
					size += 5; /*(int)strlen("&amp;");*/
					break;
				case '<':
					size += 4; /*(int)strlen("&lt;");*/
					break;
				case '>':
					size += 5; /*(int)strlen("&gt; ");*/
					break;
				default:
					size++;
					break;
			}
			p++;
		}
		else
		{
			if ((*p & 0xE0) == 0xC0)
			{
				size += 2;
				p += 2;
			}
			else if ((*p & 0xF0) == 0xE0)
			{
				size += 3;
				p += 3;
			}
			else if ((*p & 0xF8) == 0xF0)
			{
				size += 4;
				p += 4;
			}
			else
			{
				size += 1;
				p++;
			}
		}
	}

	if (size > dstlen)
	{
		return size;
	}

	p = src;
	p2 = dst;

	while (*p)
	{
		if (!(*p & 0x80))
		{
			switch(*p)
			{
				case '"':
					strcpy(p2, "&quot;");
					p2 += 6;
					break;
				case '\'':
					strcpy(p2, "&apos;");
					p2 += 6;
					break;
				case '&':
					strcpy(p2, "&amp;");
					p2 += 5;
					break;
				case '<':
					strcpy(p2, "&lt;");
					p2 += 4;
					break;
				case '>':
					strcpy(p2, "&gt; ");
					p2 += 5;
					break;
				default:
					*p2++ = *p;
					break;
			}
			p++;
		}
		else
		{
			if ((*p & 0xE0) == 0xC0)
			{
				strncpy(p2, p, 2);
				p2 += 2;
				p += 2;
			}
			else if ((*p & 0xF0) == 0xE0)
			{
				strncpy(p2, p, 3);
				p2 += 3;
				p += 3;
			}
			else if ((*p & 0xF8) == 0xF0)
			{
				strncpy(p2, p, 4);
				p2 += 4;
				p += 4;
			}
			else
			{
				*p2++ = *p++;
			}
		}
	}
	*p2 = '\0';

	return size;
}


char *UnescapeParsedText(char *src)
{
	char *dst;
	char *p = src;

	if (!src)
	{
		return NULL;
	}

	dst = malloc(strlen(src));
	dst[0] = 0;

	while (*p)
	{
		switch(*p)
		{
			case '&':
				if (strncmp(p, "&quot;", 6) == 0)
				{
					strcat(dst, "\"");
					p += 6;
				}
				else if (strncmp(p, "&apos;", 6) == 0)
				{
					strcat(dst, "'");
					p += 6;
				}
				else if (strncmp(p, "&amp;", 5) == 0)
				{
					strcat(dst, "&");
					p += 5;
				}
				else if (strncmp(p, "&lt;", 4) == 0)
				{
					strcat(dst, "<");
					p += 4;
				}
				else if (strncmp(p, "&gt;", 4) == 0)
				{
					strcat(dst, ">");
					p += 4;
				}
				else
				{
					strncat(dst, p, 1);
					p++;
				}
				break;
			default:
				strncat(dst, p, 1);
				p++;
				break;
		}
	}

	return dst;
}

char jidescapedchars[] = {0x20, 0x22, 0x26, 0x27, 0x2f, 0x3a, 0x3c, 0x3e, 0x40, 0x5c, 0x00 };
char *hex = "0123456789abcdef";

char *EscapeJID(char *src, char *dst, int dstlen)
{
	unsigned char *ps = src;
	unsigned char *pd = dst;
	char *at = strrchr(src, '@');

	if (!src)
	{
		return NULL;
	}

	while (*ps)
	{
		if (dstlen == 1)
		{
			*pd = '\0';
			return dst;
		}

		if ((!at || ps < at) && strchr(jidescapedchars, *ps))
		{
			if (dstlen < 4)
			{
				*pd = '\0';
				return dst;
			}
			else
			{
				*pd++ = '\\';
				*pd++ = hex[*ps / 16];
				*pd++ = hex[*ps % 16];
				ps++;
				dstlen -= 3;
			}
		}
		else
		{
			*pd++ = *ps++;
		}

		dstlen--;
	}
	*pd = '\0';

	return dst;
}

char *UnescapeJID(char *src, char *dst, int dstlen)
{
	unsigned char *ps = src;
	unsigned char *pd = dst;

	if (!src)
	{
		return NULL;
	}

	while (*ps)
	{
		if (dstlen == 1)
		{
			*pd = '\0';
			return 0;
		}

		if (*ps == '\\')
		{
			ps++;
			*pd = (int)(strchr(hex, *ps++) - hex) * 16;
			*pd++ |= (int)(strchr(hex, *ps++) - hex);
		}
		else
		{
			*pd++ = *ps++;
		}

		dstlen--;
	}
	*pd = '\0';

	return dst;
}

char urlescapedchars[] = {0x20, 0x3C, 0x3E, 0x23, 0x25, 0x7B, 0x7D, 0x7C, 0x5C, 0x5E, 0x7E, 0x5B, 0x5D, 0x60, 0x3B, 0x2F, 0x3F, 0x3A, 0x40, 0x3D, 0x26, 0x24, 0x00 };

char *EscapeURL(char *src, char *dst, int dstlen)
{
	unsigned char *ps = src;
	unsigned char *pd = dst;

	if (!src)
	{
		return NULL;
	}

	while (*ps)
	{
		if (dstlen == 1)
		{
			*pd = '\0';
			return dst;
		}

		if (strchr(urlescapedchars, *ps))
		{
			if (dstlen < 4)
			{
				*pd = '\0';
				return dst;
			}
			else
			{
				*pd++ = '%';
				*pd++ = hex[*ps / 16];
				*pd++ = hex[*ps % 16];
				ps++;
				dstlen -= 3;
			}
		}
		else
		{
			*pd++ = *ps++;
		}

		dstlen--;
	}
	*pd = '\0';

	return dst;
}

char *UnescapeURL(char *src, char *dst, int dstlen)
{
	unsigned char *ps = src;
	unsigned char *pd = dst;

	if (!src)
	{
		return NULL;
	}

	while (*ps)
	{
		if (dstlen == 1)
		{
			*pd = '\0';
			return 0;
		}

		if (*ps == '%')
		{
			ps++;
			*pd = (int)(strchr(hex, *ps++) - hex) * 16;
			*pd++ |= (int)(strchr(hex, *ps++) - hex);
		}
		else
		{
			*pd++ = *ps++;
		}

		dstlen--;
	}
	*pd = '\0';

	return dst;
}

int Util_OldWinVer()
{
	OSVERSIONINFO osvi;

	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);

	GetVersionEx(&osvi);

	return (osvi.dwMajorVersion <= 4);
}

char *Util_WinVerText()
{
	char *osname = NULL, *text;

	OSVERSIONINFO osvi;

	memset(&osvi, 0, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);

	GetVersionEx(&osvi);

	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if (osvi.dwMajorVersion == 3 || osvi.dwMajorVersion == 4)
		{
			osname = "Windows NT";
		}
		else if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 0)
			{
				osname = "Windows 2000";
			}
			else if (osvi.dwMinorVersion == 1)
			{
				osname = "Windows XP";
			}
			else if (osvi.dwMinorVersion == 2)
			{
				osname = "Windows Server 2003 or Windows XP Pro x64";
			}

		}
		else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
		{
			osname = "Windows Vista or Windows Server \"Longhorn\"";
		}
	}
	else if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		if (osvi.dwMajorVersion == 4)
		{
			if (osvi.dwMinorVersion == 0)
			{
				osname = "Windows 95";
			}
			else if (osvi.dwMinorVersion == 10)
			{
				osname = "Windows 98";
			}
			else if (osvi.dwMinorVersion == 90)
			{
				osname = "Windows Me";
			}
		}
	}

	if (!osname)
	{
		osname = "Unknown";
	}

	text = malloc(120);

	sprintf(text, "%s %d.%d.%d", osname, osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

	return text;
}


WCHAR *Util_ConvertUTF8ToWCHAR(const char *intext)
{
	WCHAR *outtext = NULL;
	int len = MultiByteToWideChar(CP_UTF8, 0, intext, -1, NULL, 0);

	if (len)
	{
		outtext = malloc((len + 1) * sizeof(WCHAR));

		MultiByteToWideChar(CP_UTF8, 0, intext, -1, outtext, len);

		outtext[len] = L'\0';
	}


	return outtext;
}

char *Util_ConvertWCHARToUTF8(const WCHAR *intext)
{
	char *outtext = NULL;
	int len = WideCharToMultiByte(CP_UTF8, 0, intext, -1, NULL, 0, NULL, NULL);

	if (len)
	{
		outtext = malloc((len + 1) * sizeof(WCHAR));

		WideCharToMultiByte(CP_UTF8, 0, intext, -1, outtext, len, NULL, NULL);

		outtext[len] = '\0';
	}

	return outtext;
}

char *Util_ConvertWCHARToANSI(const WCHAR *intext)
{
	char *outtext = NULL;
	int len = WideCharToMultiByte(CP_ACP, 0, intext, -1, NULL, 0, NULL, NULL);

	if (len)
	{
		outtext = malloc((len + 1) * sizeof(WCHAR));

		WideCharToMultiByte(CP_ACP, 0, intext, -1, outtext, len, NULL, NULL);

		outtext[len] = '\0';
	}

	return outtext;
}

char *Util_ConvertUTF8ToANSI(const char *intext)
{
	WCHAR *midtext = Util_ConvertUTF8ToWCHAR(intext);
	char *outtext = Util_ConvertWCHARToANSI(midtext);
	free(midtext);
	return outtext;
}

/** Base64 encoding routines. Implemented according to RFC 3548 */

/** map of all byte values to the base64 values, or to
    '65' which indicates an invalid character. '=' is '64' */
static const char _base64_invcharmap[256] = {
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,62, 65,65,65,63,
    52,53,54,55, 56,57,58,59, 60,61,65,65, 65,64,65,65,
    65, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,65, 65,65,65,65,
    65,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65,
    65,65,65,65, 65,65,65,65, 65,65,65,65, 65,65,65,65 
};

/** map of all 6-bit values to their corresponding byte
    in the base64 alphabet. Padding char is the value '64' */
static const char _base64_charmap[65] = {
    'A','B','C','D', 'E','F','G','H',
    'I','J','K','L', 'M','N','O','P',
    'Q','R','S','T', 'U','V','W','X',
    'Y','Z','a','b', 'c','d','e','f',
    'g','h','i','j', 'k','l','m','n',
    'o','p','q','r', 's','t','u','v',
    'w','x','y','z', '0','1','2','3',
    '4','5','6','7', '8','9','+','/',
    '='
};

int util_base64_encoded_len(const int len)
{
    /* encoded steam is 4 bytes for every three, rounded up */
    return ((len + 2)/3) << 2;
}

char *util_base64_encode(const unsigned char * const buffer, const int len)
{
    int clen;
    char *cbuf, *c;
    unsigned int word, hextet;
    int i;

    clen = util_base64_encoded_len(len);
    cbuf = malloc(clen + 1);
    if (cbuf != NULL) {
	c = cbuf;
	/* loop over data, turning every 3 bytes into 4 characters */
	for (i = 0; i < len - 2; i += 3) {
	    word = buffer[i] << 16 | buffer[i+1] << 8 | buffer[i+2];
	    hextet = (word & 0x00FC0000) >> 18;
	    *c++ = _base64_charmap[hextet];
	    hextet = (word & 0x0003F000) >> 12;
	    *c++ = _base64_charmap[hextet];
	    hextet = (word & 0x00000FC0) >> 6;
	    *c++ = _base64_charmap[hextet];
	    hextet = (word & 0x000003F);
	    *c++ = _base64_charmap[hextet];
	}
	/* zero, one or two bytes left */
	switch (len - i) {
	    case 0:
		break;
	    case 1:
		hextet = (buffer[len-1] & 0xFC) >> 2;
		*c++ = _base64_charmap[hextet];
		hextet = (buffer[len-1] & 0x03) << 4;
		*c++ = _base64_charmap[hextet];
		*c++ = _base64_charmap[64]; /* pad */
		*c++ = _base64_charmap[64]; /* pad */
		break;
	    case 2:
		hextet = (buffer[len-2] & 0xFC) >> 2;
		*c++ = _base64_charmap[hextet];
		hextet = ((buffer[len-2] & 0x03) << 4) |
			 ((buffer[len-1] & 0xF0) >> 4);
		*c++ = _base64_charmap[hextet];
		hextet = (buffer[len-1] & 0x0F) << 2;
		*c++ = _base64_charmap[hextet];
		*c++ = _base64_charmap[64]; /* pad */
		break;
	}
	/* add a terminal null */
	*c = '\0';
    }
	
    return cbuf;
}

int util_base64_decoded_len(const char * const buffer, const int len)
{
    int nudge;
    int c;

    /* count the padding characters for the remainder */
    nudge = -1;
    c = _base64_invcharmap[(int)buffer[len-1]];
    if (c < 64) nudge = 0;
    else if (c == 64) {
	c = _base64_invcharmap[(int)buffer[len-2]];
	if (c < 64) nudge = 1;
	else if (c == 64) {
	    c = _base64_invcharmap[(int)buffer[len-3]];
	    if (c < 64) nudge = 2;
	} 
    }
    if (nudge < 0) return 0; /* reject bad coding */

    /* decoded steam is 3 bytes for every four */ 
    return 3 * (len >> 2) - nudge;
}

unsigned char *util_base64_decode(const char * const buffer, const int len)
{
    int dlen;
    unsigned char *dbuf, *d;
    unsigned int word, hextet = 0;
    int i;

    /* len must be a multiple of 4 */
    if (len & 0x03) return NULL;

    dlen = util_base64_decoded_len(buffer, len);
    dbuf = malloc(dlen + 1);
    if (dbuf != NULL) {
	d = dbuf;
	/* loop over each set of 4 characters, decoding 3 bytes */
	for (i = 0; i < len - 3; i += 4) {
	    hextet = _base64_invcharmap[(int)buffer[i]];
	    if (hextet & 0xC0) break;
	    word = hextet << 18;
	    hextet = _base64_invcharmap[(int)buffer[i+1]];
	    if (hextet & 0xC0) break;
	    word |= hextet << 12;
	    hextet = _base64_invcharmap[(int)buffer[i+2]];
	    if (hextet & 0xC0) break;
	    word |= hextet << 6;
	    hextet = _base64_invcharmap[(int)buffer[i+3]];
	    if (hextet & 0xC0) break;
	    word |= hextet;
	    *d++ = (word & 0x00FF0000) >> 16;
	    *d++ = (word & 0x0000FF00) >> 8;
	    *d++ = (word & 0x000000FF);
	}
	if (hextet > 64) goto _base64_decode_error;
	/* handle the remainder */
	switch (dlen % 3) {
	    case 0:
		/* nothing to do */
		break;
	    case 1:
		/* redo the last quartet, checking for correctness */
		hextet = _base64_invcharmap[(int)buffer[len-4]];
		if (hextet & 0xC0) goto _base64_decode_error;
		word = hextet << 2;
		hextet = _base64_invcharmap[(int)buffer[len-3]];
		if (hextet & 0xC0) goto _base64_decode_error;
		word |= hextet >> 4;
		*d++ = word & 0xFF;
		hextet = _base64_invcharmap[(int)buffer[len-2]];
		if (hextet != 64) goto _base64_decode_error;
		hextet = _base64_invcharmap[(int)buffer[len-1]];
		if (hextet != 64) goto _base64_decode_error;
		break;
	    case 2:
		/* redo the last quartet, checking for correctness */
		hextet = _base64_invcharmap[(int)buffer[len-4]];
		if (hextet & 0xC0) goto _base64_decode_error;
		word = hextet << 10;		
		hextet = _base64_invcharmap[(int)buffer[len-3]];
		if (hextet & 0xC0) goto _base64_decode_error;
		word |= hextet << 4;		
		hextet = _base64_invcharmap[(int)buffer[len-2]];
		if (hextet & 0xC0) goto _base64_decode_error;
		word |= hextet >> 2;
		*d++ = (word & 0xFF00) >> 8;
		*d++ = (word & 0x00FF);		
		hextet = _base64_invcharmap[(int)buffer[len-1]];
		if (hextet != 64) goto _base64_decode_error;
		break;
	}
    }
    *d = '\0';
    return dbuf;

_base64_decode_error:	
    /* invalid character; abort decoding! */
    free(dbuf);
    return NULL;
}

int util_url_encoded_len(unsigned char *buffer, int len)
{
	int newlen = 0;
	unsigned char *p = buffer;

	while(len--)
	{
		if (  ((*p >=   0) && (*p <=  47))
		   || ((*p >=  58) && (*p <=  64))
		   || ((*p >=  91) && (*p <=  96))
		   || ((*p >= 123) && (*p <= 255)))
		{
			newlen += 3;
		}
		else
		{
			newlen++;
		}
		p++;
	}

	return newlen;
}

int util_url_decoded_len(char *buffer, int len)
{
	int newlen = 0;
	char *p = buffer;

	while(len)
	{
		if (*p == 0x25 /* '%' */)
		{
			p += 3;
			len -= 3;
		}
		else
		{
			p++;
			len--;
		}
		newlen++;
	}

	return newlen;
}

char *util_url_encode(unsigned char *buffer, int len)
{
	unsigned char *p = buffer;
	char *pdst, *dbuf;
	int newlen = util_url_encoded_len(buffer, len);
	char *hex = "0123456789ABCDEF";

	dbuf = malloc(newlen + 1);
	pdst = dbuf;

	while(len--)
	{
		if (  ((*p >=   0) && (*p <=  47))
		   || ((*p >=  58) && (*p <=  64))
		   || ((*p >=  91) && (*p <=  96))
		   || ((*p >= 123) && (*p <= 255)))
		{
			*pdst++ = 0x25; /* '%' */
			*pdst++ = hex[*p / 16];
			*pdst++ = hex[*p % 16];
		}
		else
		{
			*pdst++ = *p;
		}
		p++;
	}
	*pdst = '\0';

	return dbuf;
}

char *util_url_decode(char *buffer, int len)
{
	char *p = buffer;
	unsigned char *pdst, *dbuf;
	int newlen = util_url_decoded_len(buffer, len);
	char *hex = "0123456789ABCDEF";

	dbuf = malloc(newlen);
	pdst = dbuf;

	while(len)
	{
		if (*p == 0x25 /* '%' */)
		{
			*pdst++ = (unsigned int)(strchr(hex, *(p+1)) - hex) * 16 + (unsigned int)(strchr(hex, *(p+2)) - hex);
			p += 3;
			len -= 3;
		}
		else
		{
			*pdst++ = *p++;
			len--;
		}
	}

	return dbuf;
}

void Util_OpenURL(char *url)
{
	/*
	HINSTANCE error = ShellExecute(NULL, NULL, url, NULL, ".", SW_SHOW);

	if (error <= (HINSTANCE)(32))
	{
		Log_Write(0, "error opening url %s: %d\n", url, error);
	}
	*/
	char args[2048];

	_snprintf(args, 2048, "url.dll,FileProtocolHandler %s", url);

	ShellExecute(NULL, "open", "rundll32.exe", args, ".", SW_SHOW);
}

void Util_OpenURL2(void *box, char *url)
{
	Util_OpenURL(url);
}

void Util_OpenURL3(void *box, char *url)
{
	char *url3 = strdup(url);
	char *tilde = strchr(url3, '|');

	if (tilde)
	{
		*tilde = '\0';
	}


	Util_OpenURL(url3);

	free(url3);
}

char *Util_Capitalize(char *src)
{
	char *dst = strdup(src);

	if (src[0] >= 'a' && src[0] <= 'z')
	{
		dst[0] = src[0] + 'A' - 'a';
	}

	return dst;
}

char *Util_Lowercase(char *src)
{
	char *dst = strdup(src);
	char *p = dst;

	while(*p)
	{
		if (*p >= 'A' && *p <= 'Z')
		{
			*p -= 'A' - 'a';
		}
		p++;
	}

	return dst;
}

char *Util_StripInvalidFilenameCharacters(char *src)
{
	char *dst, *p;

	if (!src)
	{
		return NULL;
	}

	dst = strdup(src);
	p = dst;

	while (*p)
	{
		if (strchr("\\/:*?\"<>|", *p))
		{
			*p++ = '_';
		}
		else
		{
			p++;
		}
	}

	return dst;
}

char *Util_StripLeadingTrailingSpaces(char *in)
{
	char *out, *finalout = NULL, *finalout2;
	out = strdup(in);

	if (out)
	{
		char *end;
		finalout = out;

		while (*finalout == ' ')
		{
			finalout++;
		}
		
		end = finalout + strlen(finalout) - 1;
		while (*end == ' ' && end >= finalout)
		{
			*end = '\0';
			end--;
		}

		if (end < finalout)
		{
			finalout = NULL;
		}
	}

	finalout2 = strdup(finalout);
	free(out);

	return finalout2;
}

char *Util_DoubleCarats(char *text)
{
	char *newtext = malloc(strlen(text) * 2 + 2);
	char *p = newtext;

	while (*text)
	{
		if (*text == '^')
		{
			*p++ = '^';
		}
		*p++ = *text++;
	}
	*p = '\0';

	return newtext;		
}

char *Util_GetPrivateSavePath(char *buffer, int bufferlen)
{
	char path[MAX_PATH];
	int found = 0;

	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path) == S_OK)
	{
		strncpy(buffer, path, bufferlen);
		found = 1;
	}
	else if (GetEnvironmentVariable("appdata", buffer, bufferlen) > 0)
	{
		found = 1;
	}

	if (found)
	{
		strncat(buffer, "/Chesspark", bufferlen);

		/* make the directory, it'll fail if it exists anyway */
		mkdir(buffer);
	}
	else
	{
		strcpy(buffer, ".");
	}

	return buffer;
}

char *Util_GetPublicSavePath(char *buffer, int bufferlen)
{
	char path[MAX_PATH];
	if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
	{
		strcpy(buffer, ".");
	}
	else
	{
		strncpy(buffer, path, bufferlen);
		strncat(buffer, "/Chesspark", bufferlen);

		/* make the directory, it'll fail if it exists anyway */
		mkdir(buffer);
	}

	Log_Write(0, "publicsavepath %s\n", buffer);

	return buffer;
}