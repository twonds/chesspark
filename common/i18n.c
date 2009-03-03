#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "i18n.h"
#include "util.h"
#include "utilhash.h"
#include "leak.h"

utilhash_t *textlookup;
char *i18n_langcode;

void i18n_EscapeQuotedText(char *input, char *output)
{
	char *pin, *pout;

	pin = input;
	pout = output;

	if (*pin == '"')
	{
		pin++;
	}
	else
	{
		*pout = '\0';
		return;
	}

	while (*pin != '"')
	{
		if (*pin == '\\')
		{
			pin++;

			switch(*pin)
			{
				case '"':
					pin++;
					*pout++ = '"';
					break;
				case '\'':
					pin++;
					*pout++ = '\'';
					break;
				case '\\':
					pin++;
					*pout++ = '\\';
					break;
				case 'a':
					pin++;
					*pout++ = '\a';
					break;
				case 'b':
					pin++;
					*pout++ = '\b';
					break;
				case 'f':
					pin++;
					*pout++ = '\f';
					break;
				case 'n':
					pin++;
					*pout++ = '\n';
					break;
				case 'r':
					pin++;
					*pout++ = '\r';
					break;
				case 't':
					pin++;
					*pout++ = '\t';
					break;
				case 'v':
					pin++;
					*pout++ = '\v';
					break;
			}
		}
		else
		{
			*pout++ = *pin++;
		}
	}

	*pout = '\0';
}


void i18n_InitLanguage(char *langcode)
{
	FILE *fp;
	char line[9001], key[9001], translation[9001], temp[9001], filename[256];
	int lasttype = 0;

	textlookup = NULL;
	i18n_langcode = NULL;

	sprintf(filename, "po/%s.po", langcode);

	fp = fopen(filename, "r");

	if (!fp)
	{
		return;
	}

	i18n_langcode = strdup(langcode);

	textlookup = utilhash_new(9001, NULL);

	key[0] = '\0';
	translation[0] = '\0';

	while(fgets(line, 9000, fp))
	{
		if (line[0] == '#')
		{
		}
		else if (line[0] == '\n')
		{
			if (key[0])
			{
				utilhash_add(textlookup, key, strdup(translation));
				key[0] = '\0';
				translation[0] = '\0';
			}
		}
		else if (line[0] == '"')
		{
			i18n_EscapeQuotedText(line, temp);
			if (lasttype)
			{
				strcat(key, temp);
			}
			else
			{
				strcat(translation, temp);
			}
		}
		else if (strncmp(line, "msgid ", 6) == 0)
		{
			lasttype = 1;
			i18n_EscapeQuotedText(line + 6, temp);
			strcat(key, temp);
		}
		else if (strncmp(line, "msgstr ", 7) == 0)
		{
			lasttype = 0;
			i18n_EscapeQuotedText(line + 7, temp);
			strcat(translation, temp);
		}
	}

	if (key[0])
	{
		utilhash_add(textlookup, key, strdup(translation));
		key[0] = '\0';
		translation[0] = '\0';
	}
	fclose(fp);
}

void i18n_ResetLanguage(char *langcode)
{
	if (textlookup)
	{
		utilhash_release(textlookup);
		textlookup = NULL;
	}

	if (i18n_langcode)
	{
		free(i18n_langcode);
		i18n_langcode = NULL;
	}

	i18n_InitLanguage(langcode);
}

char *i18n_TranslateText(char *keytext)
{
	char *translation;

	if (!textlookup)
	{
		return keytext;
	}

	translation = utilhash_get(textlookup, keytext);

	if (!translation || translation[0] == '\0')
	{
		return keytext;
	}

	return translation;
}


WCHAR *i18n_TranslateTextWide(char *keytext)
{
	return Util_ConvertUTF8ToWCHAR(i18n_TranslateText(keytext));
}

char *i18n_GetCurrentLangCode()
{
	if (i18n_langcode)
	{
		return i18n_langcode;
	}
	else
	{
		return "en";
	}
}

void i18n_stringsub(char *dest, int len, const char *v, ...)
{
	va_list ap;
	const char *psrc = v;
	char *pdest = dest;
	int argnum;
	char *subtext = NULL;
	len--;

	while (*psrc && len)
	{
		switch(*psrc)
		{
			case '%':
				psrc++;

				argnum = 0;
				while (*psrc >= '0' && *psrc <= '9')
				{
					argnum *= 10;
					argnum += *psrc++ - '0';
				}

				va_start(ap, v);

				subtext = NULL;

				while (argnum--)
				{
					subtext = va_arg(ap, char *);
				}

				if (!subtext)
				{
					subtext = "(NULL)";
				}

				if (len > 0)
				{
					int sublen = (int)(strlen(subtext));
					if (sublen > len)
					{
						strncpy(pdest, subtext, len);
						pdest += len;
						len = 0;
					}
					else
					{
						strcpy(pdest, subtext);
						pdest += sublen;
						len -= sublen;
					}
				}

				va_end(ap);

				break;

			default:
				len--;
				*pdest++ = *psrc++;
				break;
		}
	}
		
	*pdest = '\0';
}