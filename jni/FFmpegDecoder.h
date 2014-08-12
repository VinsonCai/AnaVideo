extern "C" {
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavutil/time.h"
#include "include/libavutil/avstring.h"
#include "include/libavutil/mem.h"
#include "include/libswresample/swresample.h"
#include "include/libswscale/swscale.h"

}
#include <pthread.h>
#include <list>
using namespace std;
typedef struct VideoState {
	pthread_t read_tid;
	pthread_t audio_tid;
	pthread_t video_tid;
	pthread_mutex_t video_mutex;
	pthread_cond_t video_cond;
	pthread_mutex_t audio_mutex;
	pthread_cond_t audio_cond;
	AVFormatContext *ic;
	int audio_stream;
	AVStream *audio_st;
	uint8_t *audio_buf;
	unsigned int audio_buf_size; /* in bytes */
	AVPacket audio_pkt;
	struct SwrContext *swr_ctx;
	int video_stream;
	AVStream *video_st;
	char filename[1024];
	SwrContext* swrContext;
	int rgb_width;
	int rgb_height;
}VideoState;

class FFmpegDecoder {
public:
	static FFmpegDecoder *m_pInstance;
	VideoState *is;
	bool first;
	bool mute;
	typedef struct VIDEOINFO {
		unsigned char* buffer;
		int pts;
	}VIDEOINFO;
	typedef list<VIDEOINFO*> LIST;
	LIST payLoadList;
public:
	FFmpegDecoder(void);
	~FFmpegDecoder(void);
	static FFmpegDecoder *GetInstance()
	{
		if (m_pInstance == NULL) {
			m_pInstance = new FFmpegDecoder;
		}
		return m_pInstance;
	}


	int init_translate(int width, int height);
	int open_video(const char * path);


	int init_player(int width, int height);
	int init_video(int video_width, int video_height);
	int insert_video(unsigned char* buffer, int64_t pts);
	int insert_audio(unsigned char* buffer, int data_size, int64_t pts);
	int start_audio(const void*buffer, int size, int64_t pts);
	int update_video();
	void delay();
	void volume_on();
	void volume_off();
};
