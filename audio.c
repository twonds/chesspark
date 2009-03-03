#include <stdio.h>
#include <windows.h>

#include "log.h"

#include "options.h"
#include "model.h"

#include "audio.h"

HWAVEOUT hwo;
HWAVEIN hwi;

unsigned char *audiooutbuffer;
unsigned int audiowrittenbuffer  = 0;
unsigned int audioplayedbuffer   = 0;
unsigned int audiotowritebuffer  = 0;

unsigned char *audioinbuffer;
unsigned int audioreadbuffer     = 0;
unsigned int audiorecordedbuffer = 0;
unsigned int audiotoreadbuffer   = 0;


unsigned int audioinit = 0;
unsigned int recordinit = 0;
unsigned int audiorecording = 0;
int audio_volume = 50;

#define AUDIO_BUFFERLENGTH 32768
#define AUDIO_BUFFERS 4
#define AUDIO_MIXINGBUFFERS 2

WAVEHDR audio_outheaders[AUDIO_BUFFERS];
WAVEHDR audio_inheaders[AUDIO_BUFFERS];

void Audio_Init()
{
	WAVEFORMATEX wf;
	HRESULT hr;
	WAVEHDR *whdr;
	int i;

	memset(&wf, 0, sizeof(wf));
	wf.wFormatTag      = WAVE_FORMAT_PCM;
	wf.nChannels       = 2;
	wf.wBitsPerSample  = 16;
	wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nSamplesPerSec  = 44100;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.cbSize = 0;

	hr = waveOutOpen(&hwo, WAVE_MAPPER, &wf, (DWORD_PTR)NULL, (DWORD_PTR)NULL, CALLBACK_NULL);
	if (hr != MMSYSERR_NOERROR)
	{
		Log_Write(0, "waveOutOpen() error %d\n", hr);
		return;
	}

	audiooutbuffer = malloc(AUDIO_BUFFERLENGTH * AUDIO_BUFFERS);
	memset(audiooutbuffer, 0, AUDIO_BUFFERLENGTH * AUDIO_BUFFERS);

	for (i = 0; i < AUDIO_BUFFERS; i++)
	{
		whdr = &(audio_outheaders[i]);
		memset(whdr, 0, sizeof(*whdr));
		whdr->dwBufferLength = AUDIO_BUFFERLENGTH;
		whdr->lpData = audiooutbuffer + i * AUDIO_BUFFERLENGTH;
		hr = waveOutPrepareHeader(hwo, whdr, sizeof(*whdr));
		if (hr != MMSYSERR_NOERROR)
		{
			Log_Write(0, "waveOutPrepareHeader() error %d\n", hr);
			Audio_Uninit();
			return;
		}
	}

	audiowrittenbuffer = 0;
	audioplayedbuffer = 0;
	audioinit = 1;
#if 0
	memset(&wf, 0, sizeof(wf));
	wf.wFormatTag      = WAVE_FORMAT_PCM;
	wf.nChannels       = 1;
	wf.wBitsPerSample  = 8;
	wf.nBlockAlign     = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nSamplesPerSec  = 8000;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.cbSize = 0;

	hr = waveInOpen(&hwi, WAVE_MAPPER, &wf, (DWORD_PTR)NULL, (DWORD_PTR)NULL, CALLBACK_NULL);
	if (hr != MMSYSERR_NOERROR)
	{
		Log_Write(0, "Couldn't open wavein, mic support disabled\n");
		return;
	}


	audioinbuffer = malloc(AUDIO_BUFFERLENGTH * AUDIO_BUFFERS);
	memset(audioinbuffer, 0, AUDIO_BUFFERLENGTH * AUDIO_BUFFERS);

	for (i = 0; i < AUDIO_BUFFERS; i++)
	{
		whdr = &(audio_inheaders[i]);
		memset(whdr, 0, sizeof(*whdr));
		whdr->dwBufferLength = AUDIO_BUFFERLENGTH;
		whdr->lpData = audioinbuffer + i * AUDIO_BUFFERLENGTH;
		hr = waveInPrepareHeader(hwi, whdr, sizeof(*whdr));
		if (hr != MMSYSERR_NOERROR)
		{
			Log_Write(0, "waveInPrepareHeader() error %d\n", hr);
			return;
		}
	}

	recordinit = 1;
#endif
}

struct soundbuffer_s
{
	struct soundbuffer_s *next;
	unsigned char *buffer;
	int len;
	unsigned short channels;
	unsigned short samplerate;
	unsigned short bits;
	unsigned int start;
	unsigned int mixed;
	int finished;
};

struct soundbuffer_s *soundstoplay = NULL;

void Audio_MixSound(struct soundbuffer_s *sb, unsigned int start, unsigned int end)
{
	int srcsamplesperbuffer = AUDIO_BUFFERLENGTH * sb->samplerate / 44100 / 2 / 2; /* sample rate, number of channels, bytes per sample */
	int dstsamplesperbuffer = AUDIO_BUFFERLENGTH / 2 / 2;
	int adjvolume = audio_volume * audio_volume;

	while (start < end && !(sb->finished))
	{
		/* convert chunk of audio to audio buffer format */
		unsigned char *inbyte;
		short *inshort, *outshort;
		int written = 0;
		int parsesamples;
		char buffer1[AUDIO_BUFFERLENGTH * 2], buffer2[AUDIO_BUFFERLENGTH * 2];
		char *inbuffer;
		int i;
		int pos = srcsamplesperbuffer * (start - sb->start) * (sb->bits == 8 ? 1 : 2) * sb->channels;

		/* calculate how many samples of the original sound we need to fill an audio buffer */

		parsesamples = srcsamplesperbuffer;
		if (parsesamples * (sb->bits == 8 ? 1 : 2) * sb->channels >= sb->len - pos)
		{
			parsesamples = (sb->len - pos) / sb->channels / (sb->bits == 8 ? 1 : 2);
			sb->finished = 1;
		}

		/* convert 8 bit to 16 bit */
		if (sb->bits == 8)
		{
			inbyte = sb->buffer + pos;
			outshort = (short *)(&(buffer1[0]));

			for (i = 0; i < parsesamples * sb->channels; i++)
			{
				*outshort++ = (*inbyte++ - 128) << 8;
			}
			inbuffer = &(buffer1[0]);
		}
		else
		{
			inbuffer = sb->buffer + pos;
		}

		/* upmix mono to stereo */
		if (sb->channels == 1)
		{
			inshort = (short *)(inbuffer);
			outshort = (short *)(&(buffer2[0]));

			for (i = 0; i < parsesamples; i++)
			{
				*outshort++ = *inshort;
				*outshort++ = *inshort++;
			}

			inbuffer = buffer2;
		}
		/* crappy upsample to 22khz */
		if (sb->samplerate == 11025)
		{
			inshort =  (short *)(inbuffer);
			outshort = (short *)(audiooutbuffer + (start % AUDIO_BUFFERS) * AUDIO_BUFFERLENGTH);
			if (start >= audiotowritebuffer)
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					inshort += 2;
				}
			}
			else
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					inshort += 2;
				}
			}
		}
		else if (sb->samplerate == 22050)
		{
			inshort =  (short *)(inbuffer);
			outshort = (short *)(audiooutbuffer + (start % AUDIO_BUFFERS) * AUDIO_BUFFERLENGTH);
			if (start >= audiotowritebuffer)
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					inshort += 2;
				}
			}
			else
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					inshort += 2;
				}
			}
		}
		else if (sb->samplerate == 44100)
		{
			inshort =  (short *)(inbuffer);
			outshort = (short *)(audiooutbuffer + (start % AUDIO_BUFFERS) * AUDIO_BUFFERLENGTH);
			if (start >= audiotowritebuffer)
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ = *inshort++ * adjvolume / 10000;
					*outshort++ = *inshort++ * adjvolume / 10000;
				}
			}
			else
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ += *inshort++ * adjvolume / 10000;
					*outshort++ += *inshort++ * adjvolume / 10000;
				}
			}
		}

#if 0
		/* crappy upsample to 22khz */
		if (sb->samplerate == 11025)
		{
			inshort =  (short *)(inbuffer);
			outshort = (short *)(audiooutbuffer + (start % AUDIO_BUFFERS) * AUDIO_BUFFERLENGTH);
			if (start >= audiotowritebuffer)
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					*outshort++ = *(inshort) * adjvolume / 10000;
					*outshort++ = *(inshort + 1) * adjvolume / 10000;
					inshort += 2;
				}
			}
			else
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					*outshort++ += *(inshort) * adjvolume / 10000;
					*outshort++ += *(inshort + 1) * adjvolume / 10000;
					inshort += 2;
				}
			}
		} /* crappy downsample to 22khz */
		else if (sb->samplerate == 44100)
		{
			inshort =  (short *)(inbuffer);
			outshort = (short *)(audiooutbuffer + (start % AUDIO_BUFFERS) * AUDIO_BUFFERLENGTH);

			if (start >= audiotowritebuffer)
			{
				for (i = 0; i < parsesamples / 2; i++)
				{
					*outshort++ = (short)(((((int)(*(inshort    )) + (int)(*(inshort + 2))) / 2)) * adjvolume / 10000);
					*outshort++ = (short)(((((int)(*(inshort + 1)) + (int)(*(inshort + 3))) / 2)) * adjvolume / 10000);
					inshort += 4;
				}
			}
			else
			{
				for (i = 0; i < parsesamples / 2; i++)
				{
					*outshort++ += (short)(((((int)(*(inshort    )) + (int)(*(inshort + 2))) / 2)) * adjvolume / 10000);
					*outshort++ += (short)(((((int)(*(inshort + 1)) + (int)(*(inshort + 3))) / 2)) * adjvolume / 10000);
					inshort += 4;
				}
			}
		}
		else if (sb->samplerate == 22050)
		{
			inshort =  (short *)(inbuffer);
			outshort = (short *)(audiooutbuffer + (start % AUDIO_BUFFERS) * AUDIO_BUFFERLENGTH);
			if (start >= audiotowritebuffer)
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ = *inshort++ * adjvolume / 10000;
					*outshort++ = *inshort++ * adjvolume / 10000;
				}
			}
			else
			{
				for (i = 0; i < parsesamples; i++)
				{
					*outshort++ += *inshort++ * adjvolume / 10000;
					*outshort++ += *inshort++ * adjvolume / 10000;
				}
			}
		}
#endif
		/* zero the rest of the buffer */
		if (start >= audiotowritebuffer)
		{
			while ((unsigned char *)outshort < (audiooutbuffer + (start % AUDIO_BUFFERS + 1) * AUDIO_BUFFERLENGTH))
			{
				*outshort++ = 0;
			}
		}

		start++;
	}
	sb->mixed = start;
	if (audiotowritebuffer < start)
	{
		audiotowritebuffer = start;
	}

}

void Audio_Poll()
{
	HRESULT hr;
	int i = 0;

	struct soundbuffer_s *sb = soundstoplay;

	if (!audioinit)
	{
		return;
	}

	/* remove finished sounds */
	{
		struct soundbuffer_s **sb = &soundstoplay;

		while (*sb)
		{
			if ((*sb)->finished)
			{
				struct soundbuffer_s *old = *sb;
				*sb = (*sb)->next;
				free(old->buffer);
				free(old);
			}
			else
			{
				sb = &((*sb)->next);
			}
		}
	}

	sb = soundstoplay;

	while (sb)
	{
		unsigned int start;

		if (sb->start == 0)
		{
			sb->start = audiowrittenbuffer;
		}

		if (sb->mixed)
		{
			start = sb->mixed;
		}
		else
		{
			start = sb->start;
		}

		Audio_MixSound(sb, start, audioplayedbuffer + AUDIO_BUFFERS);

		sb = sb->next;
	}

	i = 0;
	while (audiowrittenbuffer > audioplayedbuffer && !i)
	{
		WAVEHDR *whdr = &(audio_outheaders[audioplayedbuffer % AUDIO_BUFFERS]);

		if (whdr->dwFlags & WHDR_DONE)
		{
			audioplayedbuffer++;
		}
		else
		{
			i = 1;
		}
	}

	i = 0;
	while (((soundstoplay && audiotowritebuffer - audiowrittenbuffer > AUDIO_MIXINGBUFFERS)
		|| (!soundstoplay && audiotowritebuffer > audiowrittenbuffer)) && i < 2)
	{
		WAVEHDR *whdr = &(audio_outheaders[audiowrittenbuffer % AUDIO_BUFFERS]);
			
		hr = waveOutWrite(hwo, whdr, sizeof(*whdr));
		if (hr != MMSYSERR_NOERROR)
		{
			Log_Write(0, "AUDIO: error %d in waveOutWrite\n", hr);
			return;
		}

		i++;
		audiowrittenbuffer++;
	}

	if (audiorecording)
	{
		/*FILE *fp;*/
		i = 0;
		while (audiorecordedbuffer + AUDIO_BUFFERS > audiotoreadbuffer && i < 2)
		{
			WAVEHDR *whdr = &(audio_inheaders[audiotoreadbuffer % AUDIO_BUFFERS]);
			
			hr = waveInAddBuffer(hwi, whdr, sizeof(*whdr));
			if (hr != MMSYSERR_NOERROR)
			{
				Log_Write(0, "AUDIO: error %d in waveInAddBuffer\n", hr);
				return;
			}

			i++;
			audiotoreadbuffer++;
		}

		i = 0;
		while (audiotoreadbuffer > audioreadbuffer && !i)
		{
			WAVEHDR *whdr = &(audio_inheaders[audioreadbuffer % AUDIO_BUFFERS]);

			if (whdr->dwFlags & WHDR_DONE)
			{
				audioreadbuffer++;
			}
			else
			{
				i = 1;
			}
		}

		/*fp = fopen("wavein", "ab");*/

		i = 0;
		while (audioreadbuffer > audiorecordedbuffer && i < 2)
		{
			WAVEHDR *whdr = &(audio_inheaders[audiorecordedbuffer % AUDIO_BUFFERS]);

			/*fwrite(whdr->lpData, whdr->dwBufferLength, 1, fp);*/
			i++;
			audiorecordedbuffer++;
		}
		/*fclose(fp);*/
	}

}

void Audio_Play(unsigned char *buffer, int len)
{
	if (!audioinit)
	{
		return;
	}
/*
	soundbuffer = buffer;
	soundbufferlen = len;
	soundbufferpos = 0;
	*/
}

struct riffheader_s
{
	char id[4];
	unsigned int size;
	char type[4];
};


struct wavchunkheader_s
{
	char id[4];
	unsigned int size;
};

void Audio_PlayWav(char *filename)
{
	FILE *fp;
	int len;
	struct riffheader_s rf;
	struct wavchunkheader_s wch;
	struct soundbuffer_s *sb;

	if (!audioinit || Model_GetOption(OPTION_DISABLESOUNDS))
	{
		return;
	}

	fp = fopen(filename, "rb");

	if (!fp)
	{
		return;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	fread(&rf, 1, sizeof(rf), fp);
	if (strncmp(rf.id, "RIFF", 4) != 0)
	{
		Log_Write(0, "Error: %s is not a RIFF file, id %s\n", filename, rf.id);
		fclose(fp);
		return;
	}

	if (strncmp(rf.type, "WAVE", 4) != 0)
	{
		Log_Write(0, "Error: %s is not a RIFF WAVE file\n", filename);
		fclose(fp);
		return;
	}

	sb = malloc(sizeof(*sb));
	memset(sb, 0, sizeof(*sb));

	while(fread(&wch, 1, sizeof(wch), fp))
	{
		if (strncmp(wch.id, "fmt ", 4) == 0)
		{
			WAVEFORMATEX wfex;

			memset(&wfex, 0, sizeof(wfex));

			fread(&wfex, 1, wch.size, fp);

			sb->channels   = wfex.nChannels;
			sb->samplerate = wfex.nSamplesPerSec;
			sb->bits       = wfex.wBitsPerSample;
		}
		else if (strncmp(wch.id, "data", 4) == 0)
		{
			sb->buffer = malloc(wch.size);
			fread(sb->buffer, 1, wch.size, fp);
			sb->len = wch.size;
		}
		else
		{
			/* ignore this chunk */
			fseek(fp, wch.size, SEEK_CUR);
		}
	}

	sb->next = soundstoplay;
	soundstoplay = sb;

	fclose(fp);

	/* lazy, assume a 16 bit stereo 22khz wav, so skip the first 44 bytes */
#if 0
	fseek(fp, 44, SEEK_SET);
	len -= 44;
	free(soundbuffer);
	soundbuffer = malloc(len);
	fread(soundbuffer, 1, len, fp);
	fclose(fp);
	soundbufferlen = len;
#endif
/*	soundbufferpos = 0;*/
}


void Audio_Uninit()
{
	int i;

	waveOutReset(hwo);

	for (i = 0; i < AUDIO_BUFFERS; i++)
	{
		WAVEHDR *whdr = &(audio_outheaders[i]);
		waveOutUnprepareHeader(hwo, whdr, sizeof(*whdr));
	}

	free(audiooutbuffer);

	waveOutClose(hwo);

	audioinit = 0;
}

int Audio_GetVolume()
{
	return audio_volume;
}

void Audio_SetVolume(int vol)
{
	audio_volume = vol;
}

int Audio_SetRecording(int recording)
{
	audiorecording = recording;

	if (audiorecording)
	{
		waveInStart(hwi);
	}
	else
	{
		waveInClose(hwi);
	}

}

int Audio_ToggleRecording()
{
	Audio_SetRecording(!audiorecording);
}