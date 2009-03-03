#include <stdio.h>
#include <stdlib.h>

#include "image.h"

#include "imagemgr.h"
/*#include "log.h"*/
#include "leak.h"

struct imagemgrlist_s
{
	struct imagemgrlist_s *next;
	struct BoxImage_s *image;
	char *name;
	BOOL animate;
};

struct imagemgrlist_s *imagelist = NULL;

struct BoxImage_s *ImageMgr_GetRootImage(char *filename)
{	
	struct imagemgrlist_s **image = &imagelist;

	if (!filename)
	{
		return NULL;
	}

	while (*image && stricmp(filename, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		char *dot = strrchr(filename, '.');
		struct BoxImage_s *bimage = NULL;

		if (dot)
		{
			if (stricmp(dot, ".png") == 0)
			{
				bimage = BoxImage_LoadPNG(filename);
			}
			else if (stricmp(dot, ".jpg") == 0)
			{
				bimage = BoxImage_LoadJPG(filename);
			}
			else if (stricmp(dot, ".gif") == 0)
			{
				bimage = BoxImage_LoadGIF(filename);
			}
		}

		if (!bimage)
		{
			return NULL;
		}

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(filename);
		(*image)->image = bimage;
		(*image)->animate = FALSE;
		
	}
	
	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetImage(char *filename)
{
	char newfilename[512];
	sprintf(newfilename, "images/%s", filename);

	return ImageMgr_GetRootImage(newfilename);
}

struct BoxImage_s *ImageMgr_GetRootSubImage(char *name, char *parentname, int x, int y, int w, int h)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetRootImage(parentname);

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_SubImage(parentimage, x, y, w, h);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetSubImage(char *name, char *parentname, int x, int y, int w, int h)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetImage(parentname);

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_SubImage(parentimage, x, y, w, h);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetRootScaledImage(char *name, char *parentname, int w, int h)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetRootImage(parentname);

		if (!parentimage)
		{
			return NULL;
		}

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_Scale(parentimage, w, h);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetRootAspectScaledImage(char *name, char *parentname, int maxw, int maxh)
{
	struct imagemgrlist_s **image = &imagelist;
	int w, h;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetRootImage(parentname);

		if (!parentimage)
		{
			return NULL;
		}

		w = maxw;
		h = maxw * parentimage->h / parentimage->w;

		if (h > maxh)
		{
			h = maxh;
			w = maxh * parentimage->w / parentimage->h;
		}

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_Scale(parentimage, w, h);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetScaledImage(char *name, char *parentname, int w, int h)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetImage(parentname);

		if (!parentimage)
		{
			return NULL;
		}

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_Scale(parentimage, w, h);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetRootDimmedImage(char *name, char *parentname, int strength)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetRootImage(parentname);

		if (!parentimage)
		{
			return NULL;
		}

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_Dim(parentimage, strength);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetDimmedImage(char *name, char *parentname, int strength)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetImage(parentname);

		if (!parentimage)
		{
			return NULL;
		}

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_Dim(parentimage, strength);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}

struct BoxImage_s *ImageMgr_GetAspectScaledTransImage(char *name, char *parentname, int maxw, int maxh, int strength)
{
	struct imagemgrlist_s **image = &imagelist;
	int w, h;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetImage(parentname);
		struct BoxImage_s *scaledimage;

		if (!parentimage)
		{
			return NULL;
		}

		w = maxw;
		h = maxw * parentimage->h / parentimage->w;

		if (h > maxh)
		{
			h = maxh;
			w = maxh * parentimage->w / parentimage->h;
		}

		scaledimage = BoxImage_Scale(parentimage, w, h);

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_Trans(parentimage, strength);
		(*image)->animate = FALSE;
	}

	return (*image)->image;
}


struct BoxImage_s *ImageMgr_GetSubAnim(char *name, char *parentname, int numframes, int w, int h)
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image && stricmp(name, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		struct BoxImage_s *parentimage = ImageMgr_GetImage(parentname);

		*image = malloc(sizeof(**image));
		(*image)->next = NULL;
		(*image)->name = strdup(name);
		(*image)->image = BoxImage_SubAnim(parentimage, numframes, w, h);
		(*image)->animate = TRUE;
	}

	return (*image)->image;
}

void ImageMgr_Animate()
{
	struct imagemgrlist_s **image = &imagelist;

	while (*image)
	{
		if ((*image)->animate)
		{
            BoxImage_NextFrame((*image)->image);
		}
		image = &((*image)->next);
	}
}

struct BoxImage_s *ImageMgr_ReloadImage(char *filename)
{
	struct imagemgrlist_s **image = &imagelist;
	char *dot = strchr(filename, '.');
	struct BoxImage_s *bimage = NULL;

	while (*image && stricmp(filename, (*image)->name) != 0)
	{
		image = &((*image)->next);
	}

	if (!*image)
	{
		return ImageMgr_GetImage(filename);
	}

	
	if (dot)
	{
		if (stricmp(dot, ".png") == 0)
		{
			bimage = BoxImage_LoadPNG(filename);
		}
		else if (stricmp(dot, ".jpg") == 0)
		{
			bimage = BoxImage_LoadJPG(filename);
		}
		else if (stricmp(dot, ".gif") == 0)
		{
			bimage = BoxImage_LoadGIF(filename);
		}
	}

	BoxImage_Destroy((*image)->image);

	(*image)->image = bimage;

	return bimage;
}