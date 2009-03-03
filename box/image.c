#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <commctrl.h>

#include <png.h>

#include <jpeglib.h>

#include <gif_lib.h>

#include "box.h"

#include "image.h"

#include "log.h"

HBITMAP BoxImage_CreateHBMPFromData(int width, int height, unsigned char *imagedata)
{
	BITMAPINFO bmi;
	HDC hdc;
	HBITMAP hbmp;
	char *bits;

	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth       = width;
	bmi.bmiHeader.biHeight      = -(int)height;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	hdc = GetDC(NULL);
	/*hbmp = CreateDIBitmap(hdc, &(bmi.bmiHeader), CBM_INIT, imagedata, &bmi, 0);*/
	hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);

	if (!hbmp)
	{
		Log_Write2(0, "CreateDIBSection failed!  width %d, height %d\n", width, height);
		return NULL;
	}
	memcpy(bits, imagedata, width*height*4);
	ReleaseDC(NULL, hdc);

	return hbmp;
}

unsigned char *BoxImage_CreateDataFromHBMP(int width, int height, HBITMAP hbmp)
{
	HDC hdc;
	BITMAPINFO bmi;
	unsigned char *imagedata = malloc(width * height * 4);

	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth       = width;
	bmi.bmiHeader.biHeight      = -(int)height;
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	hdc = GetDC(NULL);
	GetDIBits(hdc, hbmp, 0, height, imagedata, &bmi, DIB_RGB_COLORS);
	ReleaseDC(NULL, hdc);

	return imagedata;
}

struct BoxImage_s *BoxImage_LoadPNG(const char *filename)
{
	png_structp ppng;
	png_infop pinfo;
	FILE *fp;
	png_uint_32 width, height;
	unsigned int row;
	int depth, color_type, interlace_type, bytewidth;
	char *imagedata;
	png_bytep *prows;
	struct BoxImage_s *image = NULL;
	BOOL hasmask = FALSE;
	
	if (!(fp = fopen(filename, "rb")))
	{
		Log_Write(0, "PNG ERROR: Couldn't find image file: %s\n", filename);
		return NULL;
	}

	if (!(ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
	{
		Log_Write(0, "PNG ERROR: Couldn't create read struct for file: %s\n", filename);
		fclose(fp);
		return NULL;
	}

	if (!(pinfo = png_create_info_struct(ppng)))
	{
		Log_Write(0, "PNG ERROR: Couldn't create info struct for file: %s\n", filename);
		png_destroy_read_struct(&ppng, NULL, NULL);
		fclose(fp);
		return NULL;
	}

	if (setjmp(png_jmpbuf(ppng)))
	{
		Log_Write(0, "PNG ERROR: Couldn't setjmp for file: %s\n", filename);
		png_destroy_read_struct(&ppng, NULL, NULL);
		fclose(fp);
		return NULL;
	}

	png_init_io(ppng, fp);

	png_set_sig_bytes(ppng, 0);

	png_read_info(ppng, pinfo);

	png_get_IHDR(ppng, pinfo, &width, &height, &depth, &color_type,
				 &interlace_type, NULL, NULL);

	if (depth < 8)
	{
		png_set_packing(ppng);
	}
        else if (depth == 16)
	{
		png_set_strip_16(ppng);
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(ppng);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY && depth < 8)
	{
		png_set_gray_1_2_4_to_8(ppng);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(ppng);
	}

	if (png_get_valid(ppng, pinfo, PNG_INFO_tRNS))
	{
		png_set_tRNS_to_alpha(ppng);
	}

	png_set_filler(ppng, 0xff, PNG_FILLER_AFTER);

	png_set_bgr(ppng);

	png_read_update_info(ppng, pinfo);

	png_get_IHDR(ppng, pinfo, &width, &height, &depth, &color_type,
				 &interlace_type, NULL, NULL);

	bytewidth = png_get_rowbytes(ppng, pinfo);

	imagedata = malloc(bytewidth * height);
	prows = malloc(sizeof(*prows) * height);

	for (row = 0; row < height; row++)
	{
		prows[row] = malloc(bytewidth);
	}

	png_read_image(ppng, prows);

	png_read_end(ppng, pinfo);

	png_destroy_read_struct(&ppng, NULL, NULL);
	fclose(fp);

	image = malloc(sizeof(*image));
	memset(image, 0, sizeof(*image));

	image->w = width;
	image->h = height;
	for (row = 0; row < height; row++)
	{
		unsigned int col;
		unsigned char *src, *dst;

		src = prows[row];
		dst = imagedata + bytewidth * row;

		for (col = 0; col < width; col++)
		{
			int alpha = src[3];
			*dst++ = *src++ * alpha / 255;
			*dst++ = *src++ * alpha / 255;
			*dst++ = *src++ * alpha / 255;
			*dst++ = alpha;
			src++;

			if (alpha <= 0x7f)
			{
				hasmask = 1;
			}

			if (alpha != 0xff && alpha != 0x00)
			{
				image->complexalpha = 1;
			}
		}
	}

	image->hbmp = BoxImage_CreateHBMPFromData(width, height, imagedata);

	if (!(image->hbmp))
	{
		Log_Write(0, "PNG ERROR: Couldn't convert image data to HBITMAP for file: %s\n", filename);
	}

	if (hasmask)
	{
		for (row = 0; row < height; row++)
		{
			unsigned int col;
			unsigned char *src, *dst;

			src = prows[row];
			dst = imagedata + bytewidth * row;

			for (col = 0; col < width; col++)
			{
				if (*(src + 3) > 0x7f)
				{
					*dst++ = 0x00;
					*dst++ = 0x00;
					*dst++ = 0x00;
					*dst++ = 0x00;
				}
				else
				{
					*dst++ = 0xff;
					*dst++ = 0xff;
					*dst++ = 0xff;
					*dst++ = 0xff;
				}

				src += 4;
			}
		}
		image->hbmpmask = BoxImage_CreateHBMPFromData(width, height, imagedata);
	}

	for (row = 0; row < height; row++)
	{
		free(prows[row]);
	}

	free(prows);
	free(imagedata);

	return image;
}

static jmp_buf jpegloadfailedenv;

void BoxImage_LoadJPGFatalError(j_common_ptr cinfo)
{
	int error = cinfo->err;
	jpeg_destroy(cinfo);

	longjmp(jpegloadfailedenv, error);
}

struct BoxImage_s *BoxImage_LoadJPG(const char *filename)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE *fp;
	unsigned char *imagedata;
	struct BoxImage_s *image = NULL;
	unsigned int width, height, bytewidth, row;
	char **prows;
	int err;
	
	if (!(fp = fopen(filename, "rb")))
	{
		Log_Write(0, "JPG ERROR: Couldn't find image file: %s\n", filename);
		return NULL;
	}

	if ((err = setjmp(jpegloadfailedenv)))
	{
		Log_Write(0, "JPG ERROR: fatal jpg error %d", err);
		return NULL;
	}

	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = BoxImage_LoadJPGFatalError;

	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, fp);
	jpeg_read_header(&cinfo, TRUE);

	jpeg_start_decompress(&cinfo);

	width = cinfo.output_width;
	height = cinfo.output_height;
	bytewidth = width * 4;

	imagedata = malloc(bytewidth * height);
	prows = malloc(sizeof(*prows) * height);

	for (row = 0; row < height; row++)
	{
		prows[row] = malloc(bytewidth);
	}

	imagedata = malloc(bytewidth * height);

	/* Read in a row at a time, for some reason libjpeg doesn't like reading in more than 2 at a time */
	row = 0;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, &(prows[row]), 1);
		row++;
	}

	jpeg_finish_decompress(&cinfo);

	fclose(fp);

	jpeg_destroy_decompress(&cinfo);

	image = malloc(sizeof(*image));
	image->parent	 = NULL;
	image->hbmpmask  = NULL;
	image->x		 = 0;
	image->y		 = 0;
	image->w		 = width;
	image->h		 = height;
	image->numframes = 0;
	image->currframe = 0;

	for (row = 0; row < height; row++)
	{
		unsigned int col;
		char *p = imagedata + bytewidth * row;
		for (col = 0; col < width; col++)
		{
			*p++ = prows[row][col * 3 + 2];
			*p++ = prows[row][col * 3 + 1];
			*p++ = prows[row][col * 3];
			*p++ = 0xff;
		}
	}

	image->hbmp = BoxImage_CreateHBMPFromData(width, height, imagedata);

	if (!(image->hbmp))
	{
		Log_Write(0, "JPG ERROR: Couldn't convert image data to HBITMAP for file: %s\n", filename);
	}

	image->hbmpmask = NULL;

	for (row = 0; row < height; row++)
	{
		free(prows[row]);
	}

	free(prows);
	free(imagedata);

	return image;
}

static int InterlacedOffset[] = { 0, 4, 2, 1 };
static int InterlacedJumps[] = { 8, 8, 4, 2 };

struct BoxImage_s *BoxImage_LoadGIF(const char *filename)
{
	GifFileType *gfp;
	GifRecordType type;
	ColorMapObject *cmo;
	char *imagedata;
	struct BoxImage_s *image = NULL;
	unsigned int width, height, bytewidth, row;
	unsigned char **prows;
	
	if (!(gfp = DGifOpenFileName(filename)))
	{
		Log_Write(0, "GIF ERROR: Couldn't find image file: %s\n", filename);
		return NULL;
	}

	width = gfp->SWidth;
	height = gfp->SHeight;
	bytewidth = width * 4;

	imagedata = malloc(bytewidth * height);
	prows = malloc(sizeof(*prows) * height);

	for (row = 0; row < height; row++)
	{
		prows[row] = malloc(width);
	}

	do {
	
		if (DGifGetRecordType(gfp, &type) != GIF_OK)
		{
			Log_Write(0, "GIF ERROR: Couldn't get record type for image file: %s\n", filename);
			DGifCloseFile(gfp);
			return NULL;
		}

		if (type == IMAGE_DESC_RECORD_TYPE)
		{
			int row, col, width, height, i, j;

			if (DGifGetImageDesc(gfp) != GIF_OK)
			{
				DGifCloseFile(gfp);
				return NULL;
			}

			row = gfp->Image.Top;
			col = gfp->Image.Left;
			width = gfp->Image.Width;
			height = gfp->Image.Height;

			if (gfp->Image.Interlace)
			{
				for (i = 0; i < 4; i++)
				{
					for (j = row + InterlacedOffset[i]; j < row + height; j += InterlacedJumps[i])
					{
						if (DGifGetLine(gfp, &prows[j][col], width) != GIF_OK)
						{
							Log_Write(0, "GIF ERROR: Couldn't load line in image file: %s\n", filename);
							DGifCloseFile(gfp);
							return NULL;
						}
					}
				}
			}
			else
			{
			    for (i = 0; i < height; i++)
				{
					if (DGifGetLine(gfp, &prows[row + i][col], width) != GIF_OK)
					{
						DGifCloseFile(gfp);
						return NULL;
					}
			    }
			}
		}
		else if (type == EXTENSION_RECORD_TYPE)
		{
			GifByteType *Extension;
			int ExtCode;
			if (DGifGetExtension(gfp, &ExtCode, &Extension) == GIF_ERROR)
			{
				Log_Write(0, "GIF ERROR: Error getting extension record in image file: %s\n", filename);
				DGifCloseFile(gfp);
				return NULL;
			}
			while (Extension != NULL) {
				if (DGifGetExtensionNext(gfp, &Extension) == GIF_ERROR) {
					Log_Write(0, "GIF ERROR: Error getting next extension record in image file: %s\n", filename);
					DGifCloseFile(gfp);
					return NULL;
				}
		    }
		}
	} while (type != TERMINATE_RECORD_TYPE);

	cmo = (gfp->Image.ColorMap ? gfp->Image.ColorMap : gfp->SColorMap);

	image = malloc(sizeof(*image));
	image->parent	 = NULL;
	image->hbmpmask  = NULL;
	image->x		 = 0;
	image->y		 = 0;
	image->w		 = width;
	image->h		 = height;
	image->numframes = 0;
	image->currframe = 0;

	for (row = 0; row < height; row++)
	{
		unsigned int col;
		char *p = imagedata + bytewidth * row;
		for (col = 0; col < width; col++)
		{
			int index = prows[row][col];
			GifColorType color = cmo->Colors[index];

			*p++ = color.Blue;
			*p++ = color.Green;
			*p++ = color.Red;

			if (index == gfp->SBackGroundColor)
			{
				*p++ = 0;
			}
			else
			{
				*p++ = 0xff;
			}
		}
	}

	image->hbmp = BoxImage_CreateHBMPFromData(width, height, imagedata);

	if (!(image->hbmp))
	{
		Log_Write(0, "GIF ERROR: Couldn't convert image data to HBITMAP for file: %s\n", filename);
	}

	image->hbmpmask = NULL;

	for (row = 0; row < height; row++)
	{
		free(prows[row]);
	}

	free(prows);
	free(imagedata);

	return image;

	DGifCloseFile(gfp);
}

struct BoxImage_s *BoxImage_LoadImageNoExtension(char *filename)
{
	struct BoxImage_s *img;
	char filename2[MAX_PATH];
	FILE *fp;
	
	strcpy(filename2, filename);
	strcat(filename2, ".png");

	img = BoxImage_LoadPNG(filename2);

	if (img)
	{
		return img;
	}

	strcpy(filename2, filename);
	strcat(filename2, ".jpg");

	img = BoxImage_LoadJPG(filename2);

	if (img)
	{
		return img;
	}

	strcpy(filename2, filename);
	strcat(filename2, ".gif");

	img = BoxImage_LoadGIF(filename2);

	if (img)
	{
		return img;
	}
}


struct BoxImage_s *BoxImage_SubImage(struct BoxImage_s* pimg, int x, int y, int w, int h)
{
	struct BoxImage_s *newimg = malloc(sizeof(*newimg));

	memset(newimg, 0, sizeof(*newimg));

	if (pimg)
	{
		newimg->parent	  = pimg;
		newimg->hbmp	  = pimg->hbmp;
		newimg->hbmpmask  = pimg->hbmpmask;
		newimg->x         = x + pimg->x;
		newimg->y         = y + pimg->y;
	}

	newimg->w         = w;
	newimg->h         = h;
	newimg->numframes = 0;
	newimg->currframe = 0;

	return newimg;
}

struct BoxImage_s *BoxImage_SubAnim(struct BoxImage_s *pimg, int numframes, int w, int h)
{
	struct BoxImage_s *newimg = malloc(sizeof(*newimg));

	memset(newimg, 0, sizeof(*newimg));

	if (pimg)
	{
		newimg->parent	  = pimg;
		newimg->hbmp	  = pimg->hbmp;
		newimg->hbmpmask  = pimg->hbmpmask;
		newimg->x         = pimg->x;
		newimg->y         = pimg->y;
	}

	newimg->w         = w;
	newimg->h         = h;
	newimg->numframes = numframes;
	newimg->currframe = 0;

	return newimg;
}

struct BoxImage_s *BoxImage_NextFrame(struct BoxImage_s *pimg)
{
	pimg->currframe = (pimg->currframe + 1) % pimg->numframes;

	if (pimg->parent)
	{
		pimg->x = pimg->parent->x + pimg->w * (pimg->currframe % (pimg->parent->w / pimg->w));
		pimg->y = pimg->parent->y + pimg->h * (pimg->currframe / (pimg->parent->w / pimg->w));
	}

	return pimg;
}

int BoxImage_SavePNG(struct BoxImage_s *pimg, const char *filename)
{
	char *imagedata;
	FILE *fp;
	png_structp ppng;
	png_infop pinfo;
	int row, bytewidth;
	
	imagedata = BoxImage_CreateDataFromHBMP(pimg->w, pimg->h, pimg->hbmp);

	if (!imagedata)
	{
		Log_Write(0, "SAVEPNG ERROR: Couldn't convert HBITMAP to imagedata for file: %s\n", filename);
		return 0;
	}

	bytewidth = pimg->w * 4;

	if (!(fp = fopen(filename, "wb")))
	{
		Log_Write(0, "SAVEPNG ERROR: Couldn't open filename: %s\n", filename);
		return 0;
	}

	if (!(ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
	{
		Log_Write(0, "SAVEPNG ERROR: Couldn't create write struct for file: %s\n", filename);
		fclose(fp);
		return 0;
	}

	if (!(pinfo = png_create_info_struct(ppng)))
	{
		Log_Write(0, "SAVEPNG ERROR: Couldn't create info struct for file: %s\n", filename);
		png_destroy_write_struct(&ppng, NULL);
		fclose(fp);
		return 0;
	}

	if (setjmp(png_jmpbuf(ppng)))
	{
		Log_Write(0, "SAVEPNG ERROR: Couldn't setjmp for file: %s\n", filename);
		png_destroy_write_struct(&ppng, &pinfo);
		fclose(fp);
		return 0;
	}

	png_init_io(ppng, fp);

	png_set_IHDR(ppng, pinfo, pimg->w, pimg->h, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	png_write_info(ppng, pinfo);

	png_set_filler(ppng, 0, PNG_FILLER_AFTER);

	png_set_bgr(ppng);

	for (row = 0; row < pimg->h; row++)
	{
		png_write_row(ppng, &imagedata[row * bytewidth]);
	}

	png_write_end(ppng, pinfo);

	png_destroy_write_struct(&ppng, &pinfo);

	fclose(fp);

	free(imagedata);

	return 1;
}

static jmp_buf jpegsavefailedenv;

void BoxImage_SaveJPGFatalError(j_common_ptr cinfo)
{
	int error = cinfo->err;
	jpeg_destroy(cinfo);

	longjmp(jpegsavefailedenv, error);
}

int BoxImage_SaveJPG(struct BoxImage_s *pimg, const char *filename)
{
	char *imagedata;
	FILE *fp;
	struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;
        JSAMPROW row_pointer[1];
	unsigned char *rowdata;
	int bytewidth;
	int err;

	imagedata = BoxImage_CreateDataFromHBMP(pimg->w, pimg->h, pimg->hbmp);

	if (!imagedata)
	{
		Log_Write(0, "SAVEJPG ERROR: Couldn't convert HBITMAP to imagedata for file: %s\n", filename);
		return 0;
	}

	bytewidth = pimg->w * 4;

	if (!(fp = fopen(filename, "wb")))
	{
		Log_Write(0, "SAVEJPG ERROR: Couldn't open filename: %s\n", filename);
		return 0;
	}

	if ((err = setjmp(jpegsavefailedenv)))
	{
		Log_Write(0, "JPG ERROR: fatal jpg error %d", err);
		return NULL;
	}

	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = BoxImage_SaveJPGFatalError;

	jpeg_create_compress(&cinfo);

	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = pimg->w;
	cinfo.image_height = pimg->h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	rowdata = malloc(3 * pimg->w);

	jpeg_start_compress(&cinfo, TRUE);

        while (cinfo.next_scanline < cinfo.image_height) {
		unsigned char *in, *out;
		int i;

		in = &(imagedata[cinfo.next_scanline * bytewidth]);
		out = rowdata;

		for (i = 0; i < pimg->w; i++)
		{
			*out++ = in[2];
			*out++ = in[1];
			*out++ = in[0];
			in += 4;
		}
		row_pointer[0] = rowdata;
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

	jpeg_finish_compress(&cinfo);

	fclose(fp);

	free(imagedata);

	return 1;
}

struct BoxImage_s *BoxImage_Scale(struct BoxImage_s* pimg, int w, int h)
{
	struct BoxImage_s *newimg = malloc(sizeof(*newimg));

	memset(newimg, 0, sizeof(*newimg));

	if (pimg)
	{
		unsigned char *srcimagedata;
		unsigned char *dstimagedata;
		int srcbytewidth, dstbytewidth;
		int dstx, dsty, srcx, srcy;

		newimg->parent = NULL;

		srcbytewidth = pimg->w * 4;
		dstbytewidth = w * 4;

		if (pimg->hbmp)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(pimg->w, pimg->h, pimg->hbmp);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "SCALE ERROR: Couldn't convert source imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (dsty = 0; dsty < h; dsty++)
			{
				for (dstx = 0; dstx < w; dstx++)
				{
					int srcx1, srcx2, srcy1, srcy2, lx, ly;
					unsigned int i;
					srcx1 = dstx * pimg->w / w;
					srcy1 = dsty * pimg->h / h;
					srcx2 = (dstx + 1) * pimg->w / w - 1;
					srcy2 = (dsty + 1) * pimg->h / h - 1;

					srcx2 = (srcx2 < pimg->w - 1) ? srcx2 : (pimg->w - 1);
					srcx2 = (srcx2 > srcx1)       ? srcx2 : srcx1;
					srcy2 = (srcy2 < pimg->h - 1) ? srcy2 : (pimg->h - 1);
					srcy2 = (srcy2 > srcy1)       ? srcy2 : srcy1;
					
					for (i = 0; i < 4; i++)
					{
						unsigned int col = 0;
						unsigned int count = 0;
						for (ly = srcy1; ly <= srcy2; ly++)
						{
							for (lx = srcx1; lx <= srcx2; lx++)
							{
								col += srcimagedata[lx * 4 + ly * srcbytewidth + i];
								count++;
							}
						}
						col /= count;
						dstimagedata[dstx * 4 + dsty * dstbytewidth + i] = col;
					}
				}
			}

			newimg->hbmp = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmp)
			{
				Log_Write(0, "SCALE ERROR: Couldn't convert imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}

		if (pimg->hbmpmask)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(pimg->w, pimg->h, pimg->hbmpmask);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "SCALE ERROR: Couldn't convert source mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (dsty = 0; dsty < h; dsty++)
			{
				for (dstx = 0; dstx < w; dstx++)
				{
					srcx = dstx * pimg->w / w;
					srcy = dsty * pimg->h / h;
					dstimagedata[dstx * 4 + dsty * dstbytewidth]     = srcimagedata[srcx * 4 + srcy * srcbytewidth];
					dstimagedata[dstx * 4 + dsty * dstbytewidth + 1] = srcimagedata[srcx * 4 + srcy * srcbytewidth + 1];
					dstimagedata[dstx * 4 + dsty * dstbytewidth + 2] = srcimagedata[srcx * 4 + srcy * srcbytewidth + 2];
					dstimagedata[dstx * 4 + dsty * dstbytewidth + 3] = srcimagedata[srcx * 4 + srcy * srcbytewidth + 3];
				}
			}

			newimg->hbmpmask = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmpmask)
			{
				Log_Write(0, "SCALE ERROR: Couldn't convert mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}
	}

	newimg->w		  = w;
	newimg->h		  = h;
	newimg->numframes = 0;
	newimg->currframe = 0;

	return newimg;
}

struct BoxImage_s *BoxImage_Dim(struct BoxImage_s* pimg, int strength)
{
	struct BoxImage_s *newimg = malloc(sizeof(*newimg));

	memset(newimg, 0, sizeof(*newimg));

	if (pimg)
	{
		unsigned char *srcimagedata;
		unsigned char *dstimagedata;
		int bytewidth;
		int x, y, w, h;

		newimg->parent = NULL;

		w = pimg->w;
		h = pimg->h;

		bytewidth = w * 4;
		
		if (pimg->hbmp)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(w, h, pimg->hbmp);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "DIM ERROR: Couldn't convert source imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					dstimagedata[x * 4 + y * bytewidth]     = srcimagedata[x * 4 + y * bytewidth] * strength / 100;
					dstimagedata[x * 4 + y * bytewidth + 1] = srcimagedata[x * 4 + y * bytewidth + 1] * strength / 100;
					dstimagedata[x * 4 + y * bytewidth + 2] = srcimagedata[x * 4 + y * bytewidth + 2] * strength / 100;
					dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3];
				}
			}

			newimg->hbmp = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmp)
			{
				Log_Write(0, "DIM ERROR: Couldn't convert imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}

		if (pimg->hbmpmask)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(w, h, pimg->hbmpmask);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "DIM ERROR: Couldn't convert source mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					dstimagedata[x * 4 + y * bytewidth]     = srcimagedata[x * 4 + y * bytewidth];
					dstimagedata[x * 4 + y * bytewidth + 1] = srcimagedata[x * 4 + y * bytewidth + 1];
					dstimagedata[x * 4 + y * bytewidth + 2] = srcimagedata[x * 4 + y * bytewidth + 2];
					dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3];
				}
			}

			newimg->hbmpmask = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmpmask)
			{
				Log_Write(0, "DIM ERROR: Couldn't convert mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}
	}

	newimg->w		  = pimg->w;
	newimg->h		  = pimg->h;
	newimg->numframes = 0;
	newimg->currframe = 0;

	return newimg;
}

struct BoxImage_s *BoxImage_Trans(struct BoxImage_s* pimg, int strength)
{
	struct BoxImage_s *newimg = malloc(sizeof(*newimg));

	memset(newimg, 0, sizeof(*newimg));

	if (pimg)
	{
		unsigned char *srcimagedata;
		unsigned char *dstimagedata;
		int bytewidth;
		int x, y, w, h;

		newimg->parent = NULL;

		w = pimg->w;
		h = pimg->h;

		bytewidth = w * 4;
		
		if (pimg->hbmp)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(w, h, pimg->hbmp);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "TRANS ERROR: Couldn't convert source imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					dstimagedata[x * 4 + y * bytewidth]     = srcimagedata[x * 4 + y * bytewidth] * strength / 100;
					dstimagedata[x * 4 + y * bytewidth + 1] = srcimagedata[x * 4 + y * bytewidth + 1] * strength / 100;
					dstimagedata[x * 4 + y * bytewidth + 2] = srcimagedata[x * 4 + y * bytewidth + 2] * strength / 100;
					dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3] * strength / 100;
				}
			}

			newimg->hbmp = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmp)
			{
				Log_Write(0, "TRANS ERROR: Couldn't convert imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}

		if (pimg->hbmpmask)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(w, h, pimg->hbmpmask);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "TRANS ERROR: Couldn't convert source mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					dstimagedata[x * 4 + y * bytewidth]     = srcimagedata[x * 4 + y * bytewidth];
					dstimagedata[x * 4 + y * bytewidth + 1] = srcimagedata[x * 4 + y * bytewidth + 1];
					dstimagedata[x * 4 + y * bytewidth + 2] = srcimagedata[x * 4 + y * bytewidth + 2];
					dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3];
				}
			}

			newimg->hbmpmask = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmpmask)
			{
				Log_Write(0, "TRANS ERROR: Couldn't convert mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}
	}

	newimg->w		  = pimg->w;
	newimg->h		  = pimg->h;
	newimg->numframes = 0;
	newimg->currframe = 0;

	return newimg;
}

struct BoxImage_s *BoxImage_Tint(struct BoxImage_s* pimg, COLORREF tintblend)
{
	struct BoxImage_s *newimg = malloc(sizeof(*newimg));

	memset(newimg, 0, sizeof(*newimg));

	if (pimg)
	{
		unsigned char *srcimagedata;
		unsigned char *dstimagedata;
		int bytewidth;
		int x, y, w, h;

		newimg->parent = NULL;

		w = pimg->w;
		h = pimg->h;

		bytewidth = w * 4;
		
		if (pimg->hbmp)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(w, h, pimg->hbmp);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "TINT ERROR: Couldn't convert source imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					if (srcimagedata[x * 4 + y * bytewidth + 3] > 70)
					{
						dstimagedata[x * 4 + y * bytewidth]     = (srcimagedata[x * 4 + y * bytewidth] + GetBValue(tintblend)) / 2;
						dstimagedata[x * 4 + y * bytewidth + 1] = (srcimagedata[x * 4 + y * bytewidth + 1] + GetGValue(tintblend)) / 2;
						dstimagedata[x * 4 + y * bytewidth + 2] = (srcimagedata[x * 4 + y * bytewidth + 2] + GetRValue(tintblend)) / 2;
						dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3];
					}
					else
					{
						dstimagedata[x * 4 + y * bytewidth]     = srcimagedata[x * 4 + y * bytewidth];
						dstimagedata[x * 4 + y * bytewidth + 1] = srcimagedata[x * 4 + y * bytewidth + 1];
						dstimagedata[x * 4 + y * bytewidth + 2] = srcimagedata[x * 4 + y * bytewidth + 2];
						dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3];
					}
				}
			}

			newimg->hbmp = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmp)
			{
				Log_Write(0, "TINT ERROR: Couldn't convert imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}

		if (pimg->hbmpmask)
		{
			srcimagedata = BoxImage_CreateDataFromHBMP(w, h, pimg->hbmpmask);
			dstimagedata = malloc(w * 4 * h);

			if (!srcimagedata)
			{
				Log_Write(0, "TINT ERROR: Couldn't convert source mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			for (y = 0; y < h; y++)
			{
				for (x = 0; x < w; x++)
				{
					dstimagedata[x * 4 + y * bytewidth]     = srcimagedata[x * 4 + y * bytewidth];
					dstimagedata[x * 4 + y * bytewidth + 1] = srcimagedata[x * 4 + y * bytewidth + 1];
					dstimagedata[x * 4 + y * bytewidth + 2] = srcimagedata[x * 4 + y * bytewidth + 2];
					dstimagedata[x * 4 + y * bytewidth + 3] = srcimagedata[x * 4 + y * bytewidth + 3];
				}
			}

			newimg->hbmpmask = BoxImage_CreateHBMPFromData(w, h, dstimagedata);

			if (!newimg->hbmpmask)
			{
				Log_Write(0, "TINT ERROR: Couldn't convert mask imagedata to HBITMAP, image resolution is %dx%d\n", w, h);
			}

			free(dstimagedata);
			free(srcimagedata);
		}
	}

	newimg->w		  = pimg->w;
	newimg->h		  = pimg->h;
	newimg->numframes = 0;
	newimg->currframe = 0;

	return newimg;
}


struct BoxImage_s *BoxImage_CreateFromBox(struct Box_s *pbox, int mask)
{
	HBITMAP hb, hbm;
	struct BoxImage_s *newimg;

	hb = Box_Draw_Hbmp(pbox, 0);
	if (mask)
	{
		hbm = Box_Draw_Hbmp(pbox, 1);
	}

	newimg = malloc(sizeof(*newimg));
	memset(newimg, 0, sizeof(*newimg));

	newimg->w = pbox->w;
	newimg->h = pbox->h;
	newimg->hbmp = hb;
	if (mask)
	{
		newimg->hbmpmask = hbm;
	}

	return newimg;
}

void BoxImage_Destroy(struct BoxImage_s *pimg)
{
	if (!pimg)
	{
		return;
	}

	if (pimg->hbmp && !pimg->parent)
	{
		DeleteObject(pimg->hbmp);
	}
	
	if (pimg->hbmpmask && !pimg->parent)
	{
		DeleteObject(pimg->hbmpmask);
	}

	free(pimg);
}