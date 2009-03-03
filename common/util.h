#ifndef __UTIL_H__
#define __UTIL_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

enum SStatus
{
	SSTAT_AVAILABLE = 0,
	SSTAT_IDLE,
	SSTAT_AWAY,
	SSTAT_PENDING,
	SSTAT_DENIED,
	SSTAT_OFFLINE
};

struct StatusList_s
{
	struct StatusList_s *next;
	enum SStatus status;
	char *statusmsg;
};

char *Jid_Strip(const char *jid);
char *Jid_GetBeforeAt(const char *jid);
int   Jid_IsValid(const char *jid);
char *Jid_GetResource(const char *jid);

int   SetRegString(char *key, char *value);
char *GetRegString(char *key, char *in, int size);

int   SetRegInt(char *key, int value);
int   GetRegInt(char *key);

int   SetUserRegString(char *jid, char *key, char *value);
char *GetUserRegString(char *jid, char *key, char *in, int size);

int   SetUserRegInt(char *jid, char *key, int value);
int   GetUserRegInt(char *jid, char *key);


void StatusList_Add(struct StatusList_s **list, enum SStatus sstatus, char *statusmsg);
void StatusList_Remove(struct StatusList_s **list, enum SStatus sstatus, char *statusmsg);

struct StatusList_s *StatusList_Find(struct StatusList_s *list, char *name);

struct StatusList_s *StatusList_Copy(struct StatusList_s *list);
struct StatusList_s *StatusList_GetNum(struct StatusList_s *list, int num);

void StatusList_Destroy(struct StatusList_s **list);
void StatusList_Save(struct StatusList_s *list, char *jid, char *name);
struct StatusList_s *StatusList_Load(char *jid, char *name, enum SStatus status);

char *EscapeUnparsedText(char *src);
char *UnescapeUnparsedText(char *src);
char *EscapeParsedText(char *src);
char *UnescapeParsedText(char *src);

int util_base64_encoded_len(const int len);
char *util_base64_encode(const unsigned char * const buffer, const int len);
int util_base64_decoded_len(const char * const buffer, const int len);
unsigned char *util_base64_decode(const char * const buffer, const int len);

void Util_OpenURL2(void *box, char *url);

char *Util_Capitalize(char *src);
char *Util_Lowercase(char *src);
char *Util_StripLeadingTrailingSpaces(char *in);

WCHAR *Util_ConvertUTF8ToWCHAR(const char *intext);
char *Util_ConvertWCHARToUTF8(const WCHAR *intext);
char *Util_ConvertWCHARToANSI(const WCHAR *intext);
char *Util_ConvertUTF8ToANSI(const char *intext);

int Util_OldWinVer();
char *Util_WinVerText();

void Util_OpenURL(char *url);
void Util_OpenURL2(void *box, char *url);
void Util_OpenURL3(void *box, char *url);

char *Util_StripInvalidFilenameCharacters(char *src);

#endif