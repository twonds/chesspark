#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include <windows.h>

#include <gdiplus.h>
using namespace Gdiplus;

extern "C"
{
	void GPWrap_Init();
	void GPWrap_End();
	void Log_Write(int level, const char *v, ...);
	char *Util_ConvertWCHARToUTF8(const WCHAR *intext);
};

GdiplusStartupInput gpsi;
ULONG_PTR gpt;

void GPWrap_Init()
{
	GdiplusStartup(&gpt, &gpsi, NULL);

   UINT  num;        // number of image decoders
   UINT  size;       // size, in bytes, of the image decoder array

   ImageCodecInfo* pImageCodecInfo;

   // How many decoders are there?
   // How big (in bytes) is the array of all ImageCodecInfo objects?
   GetImageDecodersSize(&num, &size);

   // Create a buffer large enough to hold the array of ImageCodecInfo
   // objects that will be returned by GetImageDecoders.
   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));

   // GetImageDecoders creates an array of ImageCodecInfo objects
   // and copies that array into a previously allocated buffer. 
   // The third argument, imageCodecInfo, is a pointer to that buffer. 
   GetImageDecoders(num, size, pImageCodecInfo);

   // Display the graphics file format (MimeType)
   // for each ImageCodecInfo object.

   for(UINT j = 0; j < num; ++j)
   {
		char *txt = Util_ConvertWCHARToUTF8(pImageCodecInfo[j].MimeType);
		Log_Write(0, "%s\n", txt);
   }

   free(pImageCodecInfo);
}

void GPWrap_End()
{
	GdiplusShutdown(gpt);
}