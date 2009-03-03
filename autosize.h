#ifndef __AUTOSIZE_H__
#define __AUTOSIZE_H__

enum autosizeflags
{
	AUTOSIZE_HORIZ              = 0x00,
	AUTOSIZE_VERT               = 0x01,
	AUTOSIZE_EVENSPACING        = 0x02,
	AUTOSIZE_ALIGNRIGHT         = 0x04,
	AUTOSIZE_ALIGNBOTTOM        = 0x08
};

struct Box_s *AutoSize_Create(int x, int y, int minw, int minh, int maxw, int maxh, enum autosizeflags flags);
struct Box_s *AutoSizeSpace_Create(int x, int y, int minw, int minh, int maxw, int maxh, int spacing, enum autosizeflags flags);
void AutoSize_Fit(struct Box_s *autosize);
void AutoSize_Fill(struct Box_s *autosize);
struct Box_s *AutoSize_AddSpacer(struct Box_s *autosize, int spacersize);

#endif