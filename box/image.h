#ifndef __IMAGE_H__
#define __IMAGE_H__
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <wingdi.h>

struct BoxImage_s
{
	struct BoxImage_s *parent;
	HBITMAP hbmp;
	HBITMAP hbmpmask;
	int x;
	int y;
	int w;
	int h;
	int complexalpha;
	int numframes;
	int currframe;
};

HBITMAP BoxImage_CreateHBMPFromData(int width, int height, unsigned char *imagedata);

struct BoxImage_s *BoxImage_LoadPNG(const char *filename);
struct BoxImage_s *BoxImage_LoadJPG(const char *filename);
struct BoxImage_s *BoxImage_LoadGIF(const char *filename);
struct BoxImage_s *BoxImage_SubImage(struct BoxImage_s* pimg, int x, int y, int w, int h);
struct BoxImage_s *BoxImage_SubAnim(struct BoxImage_s* pimg, int numframes, int w, int h);
struct BoxImage_s *BoxImage_NextFrame(struct BoxImage_s *pimg);
struct BoxImage_s *BoxImage_Scale(struct BoxImage_s* pimg, int w, int h);
struct BoxImage_s *BoxImage_Dim(struct BoxImage_s* pimg, int strength);

int BoxImage_SavePNG(struct BoxImage_s *pimg, const char *filename);
struct BoxImage_s *BoxImage_CreateFromBox(struct Box_s *pbox, int mask);
void BoxImage_Destroy(struct BoxImage_s *pimg);
#endif