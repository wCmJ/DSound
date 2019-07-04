#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<Windows.h>
#include<dsound.h>
#pragma comment(lib, "dsound.lib")//for DirectSoundCreate8()
#pragma comment(lib, "dxguid.lib")//for IID_...
//#pragma comment(lib, "Winmm.lib")
#define MAX_AUDIO_BUF 4
#define BUFFERNOTIFYSIZE 192000

int sample_rate = 44100;
int channels = 2;
int bits_per_sample = 16;

BOOL main(int argc, char* argv[])
{
	int i;
	FILE *fp;
	if ((fp = fopen("..\\simplest_media_play\\NocturneNo2inEflat_44.1k_s16le.pcm", "rb")) == NULL)
	{
		printf("fopen error\n");
		exit(0);
	}
	IDirectSound8 *m_pDS = NULL;
	IDirectSoundBuffer8 *m_pDSBuffer8 = NULL;
	IDirectSoundBuffer *m_pDSBuffer = NULL;
	IDirectSoundNotify8 *m_pDSNotify = NULL;
	DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
	HANDLE m_event[MAX_AUDIO_BUF];

	SetConsoleTitle(TEXT("Simplest Audio Play DirectSound"));

	//init directsound
	if (FAILED(DirectSoundCreate8(NULL, &m_pDS, NULL)))//create directsound object
	{
		return FALSE;
	}

	if (FAILED(m_pDS->SetCooperativeLevel(FindWindow(NULL, TEXT("Simplest Audio Play DirectSound")), DSSCL_NORMAL)))//set cooperation level, otherwise there is no audio
	{
		return FALSE;
	}

	DSBUFFERDESC dsbd;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
	dsbd.dwBufferBytes = MAX_AUDIO_BUF * BUFFERNOTIFYSIZE;
	//WAVE HEADER
	dsbd.lpwfxFormat = (WAVEFORMATEX*)malloc(sizeof(WAVEFORMATEX));
	dsbd.lpwfxFormat->wFormatTag = WAVE_FORMAT_PCM;
	dsbd.lpwfxFormat->nChannels = channels;
	dsbd.lpwfxFormat->nSamplesPerSec = sample_rate;
	dsbd.lpwfxFormat->nAvgBytesPerSec = sample_rate * (bits_per_sample / 8)*channels;
	dsbd.lpwfxFormat->nBlockAlign = (bits_per_sample / 8)*channels;
	dsbd.lpwfxFormat->wBitsPerSample = bits_per_sample;
	dsbd.lpwfxFormat->cbSize = 0;

	//create a sound buffer object to manage audio samples.
	HRESULT hrl;
	if (FAILED(m_pDS->CreateSoundBuffer(&dsbd, &m_pDSBuffer, NULL)))//create main cache buffer
	{
		return FALSE;
	}
	if (FAILED(m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8,(LPVOID*)&m_pDSBuffer8)))//create a secondary cache buffer, to store voice data file that will be played
	{
		return FALSE;
	}
	//get IDirectSoundNotify8
	if (FAILED(m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify)))//create notify object, to notify application that the specify position arrived
	{
		return FALSE;
	}
	for (int i = 0; i < MAX_AUDIO_BUF; ++i)
	{
		m_pDSPosNotify[i].dwOffset = i * BUFFERNOTIFYSIZE;
		m_event[i] = ::CreateEvent(NULL,false,false,NULL);
		m_pDSPosNotify[i].hEventNotify = m_event[i];
	}

	m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF,m_pDSPosNotify);//set notify position
	m_pDSNotify->Release();

	//start playing
	BOOL isPlaying = TRUE;
	LPVOID buf = NULL;
	DWORD buf_len = 0;
	DWORD res = WAIT_OBJECT_0;
	DWORD offset = BUFFERNOTIFYSIZE;
	m_pDSBuffer8->SetCurrentPosition(0);//set play origin
	m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);//start play

	//loop for play
	while (isPlaying)
	{
		if ((res >= WAIT_OBJECT_0) && (res <= WAIT_OBJECT_0 + 3))
		{
			m_pDSBuffer8->Lock(offset, BUFFERNOTIFYSIZE, &buf, &buf_len, NULL, NULL, 0);//lock secondary cache buffer, ready to write data
			if (fread(buf, 1, buf_len, fp) != buf_len)//read data
			{
				fseek(fp, 0, SEEK_SET);
				fread(buf, 1, buf_len, fp);
				//close:
				//isPlaying = 0
			}
			m_pDSBuffer8->Unlock(buf, buf_len, NULL, 0);//unlock
			offset += buf_len;
			offset %= (BUFFERNOTIFYSIZE * MAX_AUDIO_BUF);
			printf("this is %7d of buffer\n", offset);
		}
		res = WaitForMultipleObjects(MAX_AUDIO_BUF, m_event, FALSE, INFINITE);//wait for notify that play position has arrived
	}
	return 0;
}
