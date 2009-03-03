#ifndef __HTTPPOST_H__
#define __HTTPPOST_H__

void NetTransfer_Init();
void NetTransfer_Poll();
void NetTransfer_Uninit();
void NetTransfer_PostFile(char *url, char *user, char *category, char *reason, char *filename, void *data, int len, void (*finishcallback)(void *), void *userdata, void (*progresscallback)(void *, float progress), void *userdata2);
void NetTransfer_AddDownload(char *url, char *dest, void (*finishcallback)(void *), void *userdata);
int NetTransfer_AlreadyDownloading(char *url);

#endif