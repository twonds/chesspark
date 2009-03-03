#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <time.h>

#include <windows.h>
#include <wingdi.h>

#include <zlib.h>

#include "box.h"
#include "autodialog.h"

#include "constants.h"
#include "log.h"
#include "util.h"

#include "sock.h"

/* hack */
int View_IsPlayingAGame()
{
	return 0;
}

void LogError(DWORD err)
{
	char *errbuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &errbuf, 0, NULL );
	Log_Write(0, "error %s\n", errbuf);
}


int sock_wait(sock_t s)
{
	fd_set fds;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 1000;

	FD_ZERO(&fds); 
	FD_SET(s, &fds);

	return (int)(select((int)s + 1, &fds, &fds, NULL, NULL/*&tv*/));
}

#if 0
int LogRead(sock_t s)
{
	char inbuf[4096];
	int bytes;
	FILE *fp;

	fp = fopen("sockrecv", "a");

	while ((bytes = sock_read(s, inbuf, 4095)) > 0)
	{
		inbuf[bytes] = '\0';
		Log_Write(0, "got %d bytes from server\n", bytes);
		fwrite(inbuf, 1, bytes, fp);
	}

	fclose(fp);

	if (bytes == SOCKET_ERROR)
	{
		int err = sock_error();
		Log_Write(0, "Read error\n");
		LogError(err);
	}

	return bytes;
}

int LogSockWrite(sock_t s, void *buf, int len)
{
	FILE *fp;
	int bytes, err;

	fp = fopen("socksend", "ab");

	if ((bytes = sock_write(s, buf, len)) > 0)
	{
		Log_Write(0, "sent %d bytes to server, wanted %d\n", bytes, len);
		fwrite(buf, 1, bytes, fp);
	}

	fclose(fp);

	if (bytes == SOCKET_ERROR)
	{
		int err = sock_error();
		Log_Write(0, "Write error\n");
		LogError(err);
		return bytes;
	}

	err = sock_wait(s);

	if (err == SOCKET_ERROR)
	{
		LogError(err);
		return err;
	}

	return bytes;
}

int LogSockWriteString(sock_t s, char *string)
{
	return LogSockWrite(s, string, strlen(string));
}
#endif

int sock_clearread(sock_t s)
{
	int bytes;
	unsigned char buf[4096];

	while ((bytes = sock_read(s, buf, 4095)) > 0)
	{
	}

	return bytes;
}

int sock_writestring(sock_t s, char *string)
{
	return sock_write(s, string, strlen(string));
}

struct contentlist_s
{
	struct contentlist_s *next;
	char *name;
	char *filename;
	char *contenttype;
	void *data;
	int datalen;
	int binary;
};

char *ContentListEntry_GetHeaders(struct contentlist_s *entry, char *buf)
{
	char buf2[512];
	if (entry->filename)
	{
		sprintf(buf2, "Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n", entry->name, entry->filename);
	}
	else
	{
		sprintf(buf2, "Content-Disposition: form-data; name=\"%s\"\r\n", entry->name);
	}

	strcpy(buf, buf2);

	if (entry->contenttype)
	{
		sprintf(buf2, "Content-Type: %s\r\n", entry->contenttype);
		strcat(buf, buf2);
	}

	if (entry->binary)
	{
		sprintf(buf2, "Content-Transfer-Encoding: binary\r\n");
		strcat(buf, buf2);
	}

	strcat(buf, "\r\n");

	return buf;
}

int ContentList_GetLength(struct contentlist_s *list, char *boundary)
{
	int len = 0;
	char buf[4096];

	while (list)
	{
		len += (int)(strlen(boundary)) + 5;
		len += (int)(strlen(ContentListEntry_GetHeaders(list, buf)));
		len += list->datalen + 1;
		list = list->next;
	}

	len += (int)(strlen(boundary)) + 6;

	return len;
}

void ContentList_AddEntry(struct contentlist_s **list, char *name, char *filename, char *contenttype, void *data, int datalen, int binary)
{
	while (*list)
	{
		list = &((*list)->next);
	}

	*list = malloc(sizeof(**list));
	(*list)->next        = NULL;
	(*list)->name        = strdup(name);
	(*list)->filename    = strdup(filename);
	(*list)->contenttype = strdup(contenttype);
	(*list)->data        = data;
	(*list)->datalen     = datalen;
	(*list)->binary      = binary;
}

void ContentList_AddStringEntry(struct contentlist_s **list, char *name, char *filename, char *contenttype, char *data)
{
	ContentList_AddEntry(list, name, filename, contenttype, data, (int)(strlen(data)), 0);
}

/*
int _sock_writestring(sock_t s, char *string)
{
	return LogSockWriteString(s, string);
}

int _sock_write(sock_t s, void *buf, int len)
{
	return LogSockWrite(s, buf, len);
}
*/

int ContentList_Write(struct contentlist_s *list, sock_t s, char *boundary)
{
	int bytes = 0;
	int err;

	while (list)
	{
		char buf[4096];

		if ((err = sock_writestring(s, "--")) < 0)
		{
			return err;
		}
		else
		{
			bytes += err;
		}

		if ((err = sock_writestring(s, boundary)) < 0)
		{
			return err;
		}
		else
		{
			bytes += err;
		}

		if ((err = sock_writestring(s, "\r\n")) < 0)
		{
			return err;
		}
		else
		{
			bytes += err;
		}

		ContentListEntry_GetHeaders(list, buf);

		if ((err = sock_writestring(s, buf)) < 0)
		{
			return err;
		}
		else
		{
			bytes += err;
		}

		Log_Write(0, "senddata is %d (%d)\n", list->data, *(unsigned int *)(list->data));

		if ((err = sock_write(s, list->data, list->datalen)) < 0)
		{
			return err;
		}
		else
		{
			bytes += err;
		}

		if ((err = sock_writestring(s, "\r\n")) < 0)
		{
			return err;
		}
		else
		{
			bytes += err;
		}

/*
		LogSockWrite(s, "--", 2);
		LogSockWrite(s, boundary, strlen(boundary));
		LogSockWrite(s, "\r\n", 2);
		ContentListEntry_GetHeaders(list, buf);

		LogSockWrite(s, buf, strlen(buf));
		LogSockWrite(s, list->data, list->datalen);
		LogSockWrite(s, "\r\n", 2);
*/
		list = list->next;
	}

	if ((err = sock_writestring(s, "--")) < 0)
	{
		return err;
	}
	else
	{
		bytes += err;
	}

	if ((err = sock_writestring(s, boundary)) < 0)
	{
		return err;
	}
	else
	{
		bytes += err;
	}

	if ((err = sock_writestring(s, "--\r\n")) < 0)
	{
		return err;
	}
	else
	{
		bytes += err;
	}

/*
	LogSockWrite(s, "--", 2);
	LogSockWrite(s, boundary, strlen(boundary));
	LogSockWrite(s, "--\r\n", 2);
*/
	return bytes;
}


int PostBin(void *data, int len)
{
	sock_t s;
	char outbuf[4096];
	/*char inbuf[4096];*/
	char *boundary;
	struct contentlist_s *list = NULL;
	int err;

	if (!data)
	{
		return -1;
	}

	sock_initialize();

	if ((s = sock_connect(/*"127.0.0.1"*/ "www.chesspark.com", 80)) == SOCKET_ERROR)
	{
		sock_shutdown();
		return (int)s;
	}

	if ((err = sock_set_blocking(s)) == SOCKET_ERROR)
	{
		sock_close(s);
		sock_shutdown();
		return err;
	}

	if ((err = sock_wait(s)) == SOCKET_ERROR)
	{
		sock_close(s);
		sock_shutdown();
		return err;
	}

	boundary = "---------------------------12Gn5cG0g1Gr54";
	boundary = "---------------------------9823059293";

	{
		char user[256];
                if (GetRegString("lastlogin", user, 256))
		{
			user[255] = '\0';
			ContentList_AddStringEntry(&list, "user", NULL, NULL, user);
		}
		else
		{
			ContentList_AddStringEntry(&list, "user", NULL, NULL, "crashreport@chesspark.com");
		}
	}
	ContentList_AddStringEntry(&list, "client", NULL, NULL, "Native " CHESSPARK_BUILD);
	ContentList_AddStringEntry(&list, "reason", NULL, NULL, CHESSPARK_BUILD " Crash dump");
	ContentList_AddEntry(&list, "log", "crashdump.dmp.gz", "application/x-gzip", data, len, 1);
	ContentList_AddStringEntry(&list, "mime", NULL, NULL, "application/x-gzip");

	sprintf(outbuf,
		"POST /log/submit/ HTTP/1.1\r\n"
		"Host: www.chesspark.com\r\n"
		"User-Agent: ChessparkCrashReport/0.1\r\n"
		"Content-Type: multipart/form-data; boundary=%s\r\n"
		"Content-Length: %d\r\n\r\n", boundary, ContentList_GetLength(list, boundary));

	if ((err = sock_writestring(s, outbuf)) == SOCKET_ERROR)
	{
		sock_close(s);
		sock_shutdown();
		return err;
	}
	else
	{
		Log_Write(0, "Wrote %d bytes\n", err);
	}

	if ((err = ContentList_Write(list, s, boundary)) == SOCKET_ERROR)
	{
		sock_close(s);
		sock_shutdown();
		return err;
	}
	else
	{
		Log_Write(0, "Wrote %d bytes\n", err);
	}

	if ((err = sock_writestring(s, "\r\n")) == SOCKET_ERROR)
	{
		sock_close(s);
		sock_shutdown();
		return err;
	}
	else
	{
		Log_Write(0, "Wrote %d bytes\n", err);
	}

	if ((err = sock_clearread(s)) == SOCKET_ERROR)
	{
		sock_close(s);
		sock_shutdown();
		return err;
	}

	sock_close(s);
	sock_shutdown();

	return 0;
}

unsigned char *senddata;
int sendlen;

void CrashReportDialog_PostQuit(struct Box_s *pbox, void *dummy)
{
	PostQuitMessage(0);
}

void CrashReportDialog_OnSendInfo(struct Box_s *pbox, void *dummy)
{
	struct Box_s *dialog = Box_GetRoot(pbox);
	int err;
	
	Box_Destroy(dialog);

	err = PostBin(senddata, sendlen);

	if (err < 0)
	{
		dialog = AutoDialog_Create(NULL, 500, "Error!", "There was an error submitting your crash log.\n\nIf you wish to submit your crash dump manually, email the file crashdump.dmp in your Chesspark directory to help@chesspark.com.", NULL, NULL, CrashReportDialog_PostQuit, NULL, NULL);
	}
	else
	{
		dialog = AutoDialog_Create(NULL, 500, "Thanks for your support.", "Thank you for submitting your crash information.\n\nYour contribution will greatly assist our dev team's effort to make Chesspark a better park for you.", NULL, NULL, CrashReportDialog_PostQuit, NULL, NULL);
	}

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
					LPTSTR lpCmdLine, int nShowCmd)
{
	FILE *fp;
	int len;
	/*int clen;*/
	unsigned char *data;
	char *crashenc = NULL;

	Log_SetNoLog(1);

	if (!(fp = fopen("crashdump.dmp", "rb")))
	{
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data = malloc(len);
	fread(data, 1, len, fp);
	fclose(fp);

	{
		/*unsigned int ziplen, b64enclen, urlenclen, urldeclen, b64declen, unziplen, finallen;
		char *zip, *b64enc, *urlenc, *urldec, *b64dec, *unzip;
		FILE *fp;*/
		gzFile gzf;

		gzf = gzopen("crashdump.dmp.gz", "wb");
		gzwrite(gzf, data, len);
		gzclose(gzf);

		/*
		zip = malloc((int)(len * 1.1) + 12 + 4);
		compress (zip + 4, &ziplen, data, len);
		*(unsigned int *)(zip) = len;
		ziplen += 4;

		b64enclen = util_base64_encoded_len(ziplen);
		b64enc  = util_base64_encode(zip, ziplen);

		urlenclen = util_url_encoded_len(b64enc, b64enclen);
		urlenc = util_url_encode(b64enc, b64enclen);

		urldeclen = util_url_decoded_len(urlenc, urlenclen);
		urldec = util_url_decode(urlenc, urlenclen);

		b64declen = util_base64_decoded_len(urldec, urldeclen);
		b64dec = util_base64_decode(urldec, urldeclen);

		unziplen = *(unsigned int *)(b64dec);
		unzip = malloc(unziplen);
		uncompress (unzip, &finallen, b64dec + 4, b64declen - 4);
		*/

	}

	free(data);

	if (!(fp = fopen("crashdump.dmp.gz", "rb")))
	{
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	sendlen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	senddata = malloc(sendlen);
	fread(senddata, 1, sendlen, fp);
	fclose(fp);

	/*PostBin(senddata, sendlen)*/;

	tzset();
	srand((unsigned int)(time(NULL)));
	/*i18n_InitLanguage(GetRegString("language", buffer, 256));*/

	Box_Init(GetRegInt("DisableAlphaBlending"));

	AutoDialog_Create(NULL, 500, "A very bad thing happened!", 
		"Looks like the native chesspark client just had an "
		"unrecoverable error.\n\nWant to help us fix this problem?  "
		"Just hit the send info button below, and we'll automatically "
		"receive the crash information to aid us in fixing this "
		"problem.\n\nThe crash dump contains no personally "
		"identifiable information, but if you'd rather not send the "
		"info, just hit the cancel button.", "Cancel", "Send Info",
		CrashReportDialog_PostQuit, CrashReportDialog_OnSendInfo,
		NULL);

	while(Box_Poll())
	{
		Sleep(1);
	}

	Box_Uninit();

	return 0;
}