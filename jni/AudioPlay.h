#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <android/log.h>
#include <pthread.h>
#include <unistd.h>
#include <list>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
using namespace std;

class AudioPlay {
public:
	int64_t audio_pts;
	// engine interfaces
	SLObjectItf engineObject;
	SLEngineItf engineEngine;

	// output mix interfaces
	SLObjectItf outputMixObject;
	SLEnvironmentalReverbItf outputMixEnvironmentalReverb;

	// buffer queue player interfaces
	SLObjectItf bqPlayerObject;
	SLPlayItf bqPlayerPlay;
	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;
	SLEffectSendItf bqPlayerEffectSend;
	SLMuteSoloItf bqPlayerMuteSolo;
	SLVolumeItf bqPlayerVolume;

	// aux effect on the output mix, used by the buffer queue player
	static const SLEnvironmentalReverbSettings reverbSettings;

	pthread_mutex_t audio_mutex;
	pthread_cond_t audio_cond;

	// pointer and size of the next player buffer to enqueue, and number of remaining buffers
	unsigned char * audio_buffer;
	typedef struct AUDIOINFO {
		int data_size;
		unsigned char * data;
		int64_t pkt_pts;
	}AUDIOINFO;
	typedef list<AUDIOINFO> AUDIOLIST;
	AUDIOLIST audioList;
	static AudioPlay *m_pInstance;
public:
	AudioPlay(void);
		~AudioPlay(void);
		static AudioPlay *GetInstance()
		{
			if (m_pInstance == NULL) {
				m_pInstance = new AudioPlay;
			}
			return m_pInstance;
		}

	void createEngine();
	void createBufferQueueAudioPlayer(int rate, int channel);
	void audio_write(const void*buffer, int size);
	void audio_play(unsigned char* buffer, int data_size, int64_t pts);
	int64_t get_audio();
};
