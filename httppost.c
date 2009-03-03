#define WIN32_LEAN_AND_MEAN

#include <stdlib.h>
#include <time.h>

#include <windows.h>
#include <wingdi.h>

#include <zlib.h>

#include "box.h"

#include "constants.h"
#include "log.h"

#include "namedlist.h"

#include <curl/curl.h>

struct nettransferdata_s
{
	char *url;
	CURL *curlhandle;
	FILE *fp;
	void (*finishcallback)(void *);
	void (*progresscallback)(void *, float);
	void *userdata;
	void *userdata2;
	struct curl_httppost *formpost;
	struct curl_httppost *lastptr;
	int upload;
};

struct namedlist_s *opentransfers = NULL;
CURLM *curlmultihandle;
int bytesdownloaded = 0;
unsigned int lasttimedownloadupdated = 0;

void NetTransfer_Init()
{
	curlmultihandle = curl_multi_init();
	if (!curlmultihandle)
	{
		Log_Write(0, "libcurl error: can't create multi handle\n");
	}
}

void NetTransferData_Destroy(struct nettransferdata_s *data)
{
	if (data->fp)
	{
		fclose(data->fp);
		data->fp = 0;
	}

	curl_multi_remove_handle(curlmultihandle, data->curlhandle);
	curl_easy_cleanup(data->curlhandle);

	free(data->url);
}

static size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	FILE *fp = data;

	fwrite(ptr, size, nmemb, fp);
	bytesdownloaded = size * nmemb;
	lasttimedownloadupdated = GetTickCount();
	return size * nmemb;
}

 static int ProgressCallback(struct nettransferdata_s *data, double dltotal, double dlnow, double ultotal, double ulnow)
 {
	 if (data->upload)
	 {
		 if (data->progresscallback)
		 {
			 data->progresscallback(data->userdata2, (float)(ulnow / ultotal));
		 }
	 }

	 return 0;
 }

void NetTransfer_AddDownload(char *url, char *dest, void (*finishcallback)(void *), void *userdata)
{
	struct nettransferdata_s *data;
	CURLMcode err;

	data = malloc(sizeof(*data));
	memset(data, 0, sizeof(*data));

	if ((data->fp = fopen(dest, "wb")) == NULL)
	{
		Log_Write(0, "NetTransfer error: Can't fopen %s\n", dest);
		return;
	}

	data->url = strdup(url);

	data->curlhandle = curl_easy_init();
	if (!data->curlhandle)
	{
		Log_Write(0, "libcurl error: can't create easy handle\n");
	}

	if (err = curl_multi_add_handle(curlmultihandle, data->curlhandle))
	{
		Log_Write(0, "libcurl error: curl_multi_add_handle() error %d\n", err);
	}

	if ((err = curl_easy_setopt(data->curlhandle, CURLOPT_URL, data->url)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_URL) error %d\n", err);
	}

	if ((err = curl_easy_setopt(data->curlhandle, CURLOPT_WRITEFUNCTION, WriteCallback)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_WRITEFUNCTION) error %d\n", err);
	}

	if ((err = curl_easy_setopt(data->curlhandle, CURLOPT_WRITEDATA, (void *)(data->fp))))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_WRITEDATA) error %d\n", err);
	}

	if ((err = curl_easy_setopt(data->curlhandle, CURLOPT_USERAGENT, "ChessparkClient/1.0")))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_USERAGENT) error %d\n", err);
	}

	if ((err = curl_easy_setopt(data->curlhandle, CURLOPT_FOLLOWLOCATION, 1)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_FOLLOWLOCATION) error %d\n", err);
	}

	/* disable dns cache */
	if ((err = curl_easy_setopt(data->curlhandle, CURLOPT_DNS_CACHE_TIMEOUT, 0)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_DNS_CACHE_TIMEOUT) error %d\n", err);
	}

	data->finishcallback = finishcallback;
	data->userdata = userdata;

	NamedList_Add(&opentransfers, url, data, NetTransferData_Destroy);
}

void NetTransfer_PostFile(char *url, char *user, char *category, char *reason, char *filename, void *data, int len, void (*finishcallback)(void *), void *userdata, void (*progresscallback)(void *, float progress), void *userdata2)
{
	struct nettransferdata_s *ntdata;
	CURLMcode err;

	ntdata = malloc(sizeof(*ntdata));
	memset(ntdata, 0, sizeof(*ntdata));

	ntdata->url = strdup(url);

	curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "user", CURLFORM_COPYCONTENTS, strdup(user), CURLFORM_END);
	curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "client", CURLFORM_COPYCONTENTS, "Native " CHESSPARK_BUILD, CURLFORM_END);
	curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "newformat", CURLFORM_COPYCONTENTS, "yes", CURLFORM_END);
	if (Model_IsLocalMemberNotActivated())
	{
		char fullmembertype[256];
		sprintf(fullmembertype, "%s-not-activated", Model_GetLocalMemberType());
		curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "membertype", CURLFORM_COPYCONTENTS, fullmembertype, CURLFORM_END);
	}
	else
	{
		curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "membertype", CURLFORM_COPYCONTENTS, Model_GetLocalMemberType(), CURLFORM_END);
	}

	if (category)
	{
		curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "category", CURLFORM_COPYCONTENTS, strdup(category), CURLFORM_END);
	}

	if (reason)
	{
		curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "reason", CURLFORM_COPYCONTENTS, strdup(reason), CURLFORM_END);
	}

	if (data)
	{
		curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "log", CURLFORM_BUFFER, strdup(filename), CURLFORM_BUFFERPTR, data, CURLFORM_BUFFERLENGTH, len, CURLFORM_END);
		curl_formadd(&(ntdata->formpost), &(ntdata->lastptr), CURLFORM_COPYNAME, "mime", CURLFORM_COPYCONTENTS, "application/x-gzip", CURLFORM_END);
	}

	ntdata->curlhandle = curl_easy_init();
	if (!ntdata->curlhandle)
	{
		Log_Write(0, "libcurl error: can't create easy handle\n");
	}

	if (err = curl_multi_add_handle(curlmultihandle, ntdata->curlhandle))
	{
		Log_Write(0, "libcurl error: curl_multi_add_handle() error %d\n", err);
	}

	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_URL, ntdata->url)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_URL) error %d\n", err);
	}

	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_HTTPPOST, ntdata->formpost)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_HTTPPOST) error %d\n", err);
	}

	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_USERAGENT, "ChessparkClient/1.0")))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_USERAGENT) error %d\n", err);
	}

	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_NOPROGRESS, 0)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_NOPROGRESS) error %d\n", err);
	}

	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_PROGRESSFUNCTION, ProgressCallback)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_PROGRESSFUNCTION) error %d\n", err);
	}

	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_PROGRESSDATA, ntdata)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_PROGRESSDATA) error %d\n", err);
	}

	/* disable dns cache */
	if ((err = curl_easy_setopt(ntdata->curlhandle, CURLOPT_DNS_CACHE_TIMEOUT, 0)))
	{
		Log_Write(0, "libcurl error: curl_easy_setopt(CURLOPT_DNS_CACHE_TIMEOUT) error %d\n", err);
	}

	ntdata->finishcallback = finishcallback;
	ntdata->userdata = userdata;
	ntdata->progresscallback = progresscallback;
	ntdata->userdata2 = userdata2;
	ntdata->upload = 1;

	NamedList_Add(&opentransfers, url, ntdata, NetTransferData_Destroy);
}

static int callagain = 0;

void NetTransfer_Poll()
{
	if (opentransfers)
	{
		CURLMsg *cmsg;
		CURLMcode err;
		int numcmsg;
		int numhandles;

		/* don't perform if we're going over the download cap and a game is in progress */
		if (bytesdownloaded && !callagain && View_IsPlayingAGame())
		{
			unsigned int ms = (GetTickCount() - lasttimedownloadupdated);

                        if (!ms || bytesdownloaded * 1000 / ms > 4096)
			{
				return;
			}

			bytesdownloaded = 0;
		}

		err = curl_multi_perform(curlmultihandle, &numhandles);

		if (err == -1) /* wants to be called again, so we'll call it at next poll */
		{
			callagain = 1;
			return;
		}
		else if (err != 0)
		{
			Log_Write(0, "libcurl error: curl_multi_perform() error %d\n", err);
		}

		callagain = 0;

		cmsg = curl_multi_info_read(curlmultihandle, &numcmsg);
		if (cmsg && cmsg->msg == CURLMSG_DONE)
		{
			struct namedlist_s **ppentry = &opentransfers;

			Log_Write(0, "transfer complete, result %d\n", cmsg->data.result);

			while (*ppentry)
			{
				struct nettransferdata_s *data = (*ppentry)->data;
				if (data->curlhandle == cmsg->easy_handle)
				{
					long rc;
					curl_easy_getinfo(data->curlhandle, CURLINFO_RESPONSE_CODE, &rc);
					Log_Write(0, "CURLINFO_RESPONSE_CODE %d\n", rc);

					curl_easy_getinfo(data->curlhandle, CURLINFO_HTTP_CONNECTCODE, &rc);
					Log_Write(0, "CURLINFO_HTTP_CONNECTCODE %d\n", rc);

					if (data->fp)
					{
						fclose(data->fp);
						data->fp = NULL;
					}

					if (data->upload && data->progresscallback)
					{
						data->progresscallback(data->userdata2, 1.0f);
					}

					if (data->finishcallback && cmsg->data.result == 0)
					{
						data->finishcallback(data->userdata);
					}

					NamedList_Remove(ppentry);
					return;
				}

				ppentry = &((*ppentry)->next);
			}
		}
	}
	else
	{
		bytesdownloaded = 0;
	}
}

int NetTransfer_AlreadyDownloading(char *url)
{
	struct namedlist_s **ppentry = &opentransfers;

	if (!url)
	{
		return 0;
	}

	while (*ppentry)
	{
		struct nettransferdata_s *data = (*ppentry)->data;

		if (data->url && stricmp(data->url, url) == 0)
		{
			return 1;
		}

		ppentry = &((*ppentry)->next);
	}

	return 0;
}

void NetTransfer_Uninit()
{
	NamedList_Destroy(&opentransfers);
	curl_multi_cleanup(curlmultihandle);
}
