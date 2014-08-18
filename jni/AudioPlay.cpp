#include "AudioPlay.h"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "audio_play", __VA_ARGS__))
const SLEnvironmentalReverbSettings AudioPlay::reverbSettings = SL_I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR;
AudioPlay *AudioPlay::m_pInstance = NULL;


AudioPlay::AudioPlay(void) {
	audio_buffer = NULL;
}

AudioPlay::~AudioPlay(void) {

}
// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
	LOGD("callback.............");
	AudioPlay *mAudioPlay = AudioPlay::GetInstance();
	mAudioPlay->get_audio();
}

int64_t AudioPlay::get_audio() {
	pthread_mutex_lock(&audio_mutex);
	if (audioList.empty()) {
		pthread_cond_wait(&audio_cond, &audio_mutex);
	}
	AUDIOINFO audioInfo = audioList.front();
	audioList.pop_front();
	pthread_mutex_unlock(&audio_mutex);
	audio_pts = audioInfo.pkt_pts;
	if (audio_buffer) {
		free(audio_buffer);
		audio_buffer = NULL;
	}
	audio_buffer = audioInfo.data;
	(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, audio_buffer, audioInfo.data_size);
	return audio_pts;
}


void AudioPlay::audio_play(unsigned char* buffer, int data_size, int64_t pts) {
	AUDIOINFO audioInfo;
	audioInfo.data_size = data_size;
	audioInfo.data = buffer;
	audioInfo.pkt_pts = pts;
	pthread_mutex_lock(&audio_mutex);
	audioList.push_back(audioInfo);
	pthread_cond_signal(&audio_cond);
	pthread_mutex_unlock(&audio_mutex);
}

void AudioPlay::init_audio() {
	pthread_mutex_init (&audio_mutex, NULL);
	pthread_cond_init (&audio_cond, NULL);
	audio_buffer = NULL;
}

void AudioPlay::createEngine() {

	SLresult result;

	// create engine
	result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the engine
	result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the engine interface, which is needed in order to create other objects
	result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[1] = {SL_BOOLEAN_FALSE};
	result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the output mix
	result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the environmental reverb interface
	// this could fail if the environmental reverb effect is not available,
	// either because the feature is not present, excessive CPU load, or
	// the required MODIFY_AUDIO_SETTINGS permission was not requested and granted
	result = (*outputMixObject)->GetInterface(outputMixObject, SL_IID_ENVIRONMENTALREVERB,
			&outputMixEnvironmentalReverb);
	if (SL_RESULT_SUCCESS == result) {
		result = (*outputMixEnvironmentalReverb)->SetEnvironmentalReverbProperties(
				outputMixEnvironmentalReverb, &reverbSettings);
		(void)result;
	}
	// ignore unsuccessful result codes for environmental reverb, as it is optional for this example

}

void AudioPlay::createBufferQueueAudioPlayer(int rate, int channel) {
	LOGD("over....");
	SLresult result;

	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataFormat_PCM format_pcm;
	format_pcm.formatType = SL_DATAFORMAT_PCM;
	format_pcm.numChannels = channel;
	format_pcm.samplesPerSec = rate*1000;
	LOGD("rate....%d,%d", rate, channel);
	format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	if( channel == 2 ) {
		format_pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
		//	} else if(channel==6) {
		//		format_pcm.channelMask = KSAUDIO_SPEAKER_5POINT1;
	} else {
		format_pcm.channelMask = SL_SPEAKER_FRONT_CENTER;
	}
	format_pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	SLDataSource audioSrc = {&loc_bufq, &format_pcm};

	// configure audio sink
	SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
	SLDataSink audioSnk = {&loc_outmix, NULL};

	// create audio player
	const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
			/*SL_IID_MUTESOLO,*/ SL_IID_VOLUME};
	const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
			/*SL_BOOLEAN_TRUE,*/ SL_BOOLEAN_TRUE};
	result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSnk,
			3, ids, req);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// realize the player
	result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the play interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the buffer queue interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
			&bqPlayerBufferQueue);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// register callback on the buffer queue
	result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// get the effect send interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_EFFECTSEND,
			&bqPlayerEffectSend);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

#if 0   // mute/solo is not supported for sources that are known to be mono, as this is
	// get the mute/solo interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_MUTESOLO, &bqPlayerMuteSolo);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;
#endif

	// get the volume interface
	result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_VOLUME, &bqPlayerVolume);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;

	// set the player's state to playing
	result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	assert(SL_RESULT_SUCCESS == result);
	(void)result;
}

void AudioPlay::audio_write(const void*buffer, int size) {
	(*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, buffer, size);
}


