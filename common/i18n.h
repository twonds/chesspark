#ifndef __I18N_H__
#define __I18N_H__

#define _(x) i18n_TranslateText(x)
#define _L(x) i18n_TranslateTextWide(x)

void i18n_InitLanguage(char *langcode);
void i18n_ResetLanguage(char *langcode);
char *i18n_TranslateText(char *keytext);
WCHAR *i18n_TranslateTextWide(char *keytext);
void i18n_stringsub(char *dest, int len, const char *v, ...);
char *i18n_GetCurrentLangCode();

#endif