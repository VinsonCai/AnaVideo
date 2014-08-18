#include "PlayerManager.h"

pthread_mutex_t frame_mutex;
pthread_cond_t frame_cond;
PlayerManager *PlayerManager::m_pInstance = NULL;

#define DEBUG
#ifdef DEBUG
#define LOG_TAG "play-jni"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)

#endif

PlayerManager::PlayerManager(void) {
	audioPlay = AudioPlay::GetInstance();
	first = true;
}

PlayerManager::~PlayerManager(void) {

}

int PlayerManager::init_player() {
	mute = false;
	pthread_mutex_init(&frame_mutex, NULL);
	pthread_cond_init(&frame_cond, NULL);
	return 1;
}

int PlayerManager::init_audio(int sample_rate, int channel) {
	audioPlay->init_audio();
	audioPlay->createEngine();
	first = true;
	mute = false;
	audioPlay->createBufferQueueAudioPlayer(sample_rate, channel);
}

int PlayerManager::init_video(int video_width, int video_height) {
	rgb_width = video_width;
	rgb_height = video_height;
}

int PlayerManager::insert_video(AVFrame* pFrame) {
	pthread_mutex_lock(&frame_mutex);
	payLoadList.push_back(pFrame);
	pthread_cond_signal(&frame_cond);
	pthread_mutex_unlock(&frame_mutex);
}

int PlayerManager::insert_audio(unsigned char* buffer, int data_size, int64_t pts) {
	if (first) {
		audioPlay->audio_write(buffer, data_size);
		audioPlay->audio_pts = pts;
		first = false;
	} else if (!mute){
		audioPlay->audio_play(buffer, data_size, pts);
	}
}

void PlayerManager::volume_on() {
	mute = false;
}

void PlayerManager::volume_off() {
	mute = true;
}

int PlayerManager::translate_color(jbyte* out) {
	pthread_mutex_lock(&frame_mutex);
	while(payLoadList.empty()) {
		pthread_cond_wait(&frame_cond, &frame_mutex);
	}
	AVFrame *avFrame = payLoadList.front();
	payLoadList.pop_front();
	pthread_mutex_unlock(&frame_mutex);
	if(avFrame){
		for(int y=0; y<rgb_height; y++) {
			memcpy(out+rgb_width*2*y,avFrame->data[0]+y*avFrame->linesize[0], rgb_width*2);
		}
		av_free(avFrame);
		avFrame = NULL;
		return 1;
	}
	return -1;
}
