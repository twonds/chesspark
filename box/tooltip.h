#ifndef __TOOLTIP_H__
#define __TOOLTIP_H__

void ToolTip_Popup(struct Box_s *pbox, char *tiptext);
void ToolTip_PopDown(struct Box_s *pbox);
void ToolTipParent_OnDestroy(struct Box_s *pbox);

#endif