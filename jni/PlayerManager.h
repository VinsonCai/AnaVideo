#include <jni.h>
#include <list>
#include "AudioPlay.h"
#include <android/log.h>
extern "C" {
#include "include/libavcodec/avcodec.h"

}

class PlayerManager {
public:
	static PlayerManager *m_pInstance;
//	VideoDisplay *videoDisplay;
	AudioPlay *audioPlay;
	bool first;
	bool mute;
	typedef struct VIDEOINFO {
		unsigned char* buffer;
		int pts;
	}VIDEOINFO;
	typedef list<AVFrame*> LIST;
	LIST payLoadList;
	int rgb_width;
	int rgb_height;
public:
	PlayerManager(void);
	~PlayerManager(void);
	static PlayerManager *GetInstance()
	{
		if (m_pInstance == NULL) {
			m_pInstance = new PlayerManager;
		}
		return m_pInstance;
	}

	int init_player();
	int init_audio(int sample_rate, int channel);
	int init_video(int video_width, int video_height);
	int insert_video(AVFrame* pFrame);
	int insert_audio(unsigned char* buffer, int data_size, int64_t pts);
	int start_audio(const void*buffer, int size, int64_t pts);
	int update_video();
	void delay();
	void volume_on();
	void volume_off();

	int translate_color(jbyte* out);

};
