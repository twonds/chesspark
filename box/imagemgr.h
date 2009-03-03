#ifndef __IMAGEMGR_H__
#define __IMAGEMGR_H__

struct BoxImage_s *ImageMgr_GetImage(char *filename);
struct BoxImage_s *ImageMgr_GetSubImage(char *name, char *parentname, int x, int y, int w, int h);
struct BoxImage_s *ImageMgr_GetSubAnim(char *name, char *parentname, int numframes, int w, int h);
struct BoxImage_s *ImageMgr_GetScaledImage(char *name, char *parentname, int w, int h);
struct BoxImage_s *ImageMgr_GetDimmedImage(char *name, char *parentname, int strength);

struct BoxImage_s *ImageMgr_GetRootImage(char *filename);
struct BoxImage_s *ImageMgr_GetRootSubImage(char *name, char *parentname, int x, int y, int w, int h);
struct BoxImage_s *ImageMgr_GetRootSubAnim(char *name, char *parentname, int numframes, int w, int h);
struct BoxImage_s *ImageMgr_GetRootScaledImage(char *name, char *parentname, int w, int h);
struct BoxImage_s *ImageMgr_GetRootDimmedImage(char *name, char *parentname, int strength);
struct BoxImage_s *ImageMgr_GetRootAspectScaledImage(char *name, char *parentname, int maxw, int maxh);

void ImageMgr_Animate();

#endif