#ifndef __AUDIO_H__
#define __AUDIO_H__

void Audio_Init();
void Audio_Uninit();

void Audio_Poll();

void Audio_PlayWav(char *filename);
int Audio_GetVolume();
void Audio_SetVolume(int vol);

#endif