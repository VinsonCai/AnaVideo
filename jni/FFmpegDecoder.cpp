#include <jni.h>
#include <android/log.h>

#include "FFmpegDecoder.h"
#include "PacketQueue.h"
#include "PlayerManager.h"

PakcetQueue video_queue;
PakcetQueue audio_queue;
PlayerManager *mPlayerManager;
//VideoDisplay mVideoDisplay;
//AudioPlay mAudioPlay;
static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
uint8_t *out[]={audio_buf};
uint8_t *buffer = NULL;

#define DEBUG
#ifdef DEBUG
#define LOG_TAG "ffmpeg decoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)

#endif
FFmpegDecoder *FFmpegDecoder::m_pInstance = NULL;

FFmpegDecoder::FFmpegDecoder(void) {
	mPlayerManager = PlayerManager::GetInstance();
}

FFmpegDecoder::~FFmpegDecoder(void) {

}

int get_video_frame(VideoState *is, AVFrame *frame, AVPacket *pkt, int *serial) {
	double pts = 0;
	int got_picture;
	if (video_queue.packet_queue_get(pkt, 1, serial) <= 0)
		return -1;
	if(avcodec_decode_video2(is->video_st->codec, frame, &got_picture, pkt) < 0) {
		return -1;
	}

	return got_picture;
}

void* audio_thread(void *arg) {
	AVPacket pkt = { 0 };
	VideoState *is = (VideoState *)arg;
	int serial = 0;
	int got_audio;
	AVCodecContext *aCodecCtx = is->audio_st->codec;
	bool mIsFirst = true;
	while(1) {
		AVFrame *frame = avcodec_alloc_frame();
		avcodec_get_frame_defaults(frame);

		if (audio_queue.packet_queue_get(&pkt, 1, &serial) <= 0) {
			continue;
		}
		int ret = avcodec_decode_audio4(aCodecCtx, frame, &got_audio, &pkt);
		if (ret>0 && got_audio) {
			//重采样得到AV_SAMPLE_FMT_S16格式 (AV_SAMPLE_FMT_S16P->AV_SAMPLE_FMT_S16)
			if (aCodecCtx->sample_fmt != AV_SAMPLE_FMT_S16) {
				if (is->swrContext) {
					swr_free(&is->swrContext);
				}
				int64_t dec_channel_layout =
						(frame->channel_layout && av_frame_get_channels(frame) == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
								frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(frame));
				int64_t out_channel_layout = av_get_default_channel_layout(aCodecCtx->channels);
				is->swrContext = swr_alloc_set_opts(NULL,
						out_channel_layout, // out channel layout
						AV_SAMPLE_FMT_S16, // out sample format
						aCodecCtx->sample_rate, // out sample rate
						dec_channel_layout, // in channel layout
						aCodecCtx->sample_fmt, // in sample format
						aCodecCtx->sample_rate, // in sample rate
						0, // log offset
						NULL); // log context
				if (swr_init(is->swrContext)<0) {
					LOGD("swr_init for av sample fmt fail");
				}
				int nb_sample = frame->nb_samples;
				int out_channels = aCodecCtx->channels;
				if (is->swrContext) {
					const uint8_t **in = (const uint8_t **)frame->data;
					int ret = swr_convert(is->swrContext, out, nb_sample, in, nb_sample);
					if (ret < 0) {
						LOGD("swr_convert.....fail");
					}
					int data_size = frame->linesize[0] = ret * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * out_channels;
					frame->pts = av_frame_get_best_effort_timestamp(frame);
					frame->pts = frame->pts*av_q2d(is->audio_st->time_base)*1000;
					int len = data_size%8;
					if (len!= 0) {
						data_size = data_size-len+8;
					}
					unsigned char * buffer = (unsigned char *) malloc(data_size);
					memcpy(buffer, out[0],data_size);
					AVRational tb = {1, frame->sample_rate};
					if (frame->pts != AV_NOPTS_VALUE) {
						frame->pts = av_rescale_q(frame->pts, aCodecCtx->time_base, tb);
					}

					if (mIsFirst) {
						int channel =  aCodecCtx->channels;
						mPlayerManager->init_audio(aCodecCtx->sample_rate, channel);
						mPlayerManager->insert_audio(out[0], data_size, 0);
						mIsFirst = false;
					} else {
						mPlayerManager->insert_audio(buffer, data_size, frame->pts);
					}
					av_free(frame);
//					free(buffer);
				}
			}
		}
	}
}

void* video_thread(void *arg) {
	AVPacket pkt = { 0 };
	VideoState *is = (VideoState *)arg;
	int ret;
	int serial = 0;
	AVCodecContext *aCodecCtx = is->video_st->codec;
	AVRational avRational = aCodecCtx->time_base;
	double rate = av_q2d(avRational);
	int num = avRational.num;  //分子
	int den = avRational.den;  //分母
	double fps = (double) den/num;
	AVRational streamRational = is->video_st->r_frame_rate;//{1,25}
	num = streamRational.num;  //分子
	den = streamRational.den;  //分母
	fps = (double) num/den;
	int numBytes=avpicture_get_size(PIX_FMT_RGB565, is->rgb_width, is->rgb_height);
	if (buffer == NULL) {
		buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	}

	struct SwsContext *sws_ctx =
			sws_getContext(
					is->video_st->codec->width,
					is->video_st->codec->height,
					is->video_st->codec->pix_fmt,
					is->rgb_width,
					is->rgb_height,
					AV_PIX_FMT_RGB565,
					SWS_BILINEAR,
					NULL,
					NULL,
					NULL);

	while(1) {
		AVFrame *frame = avcodec_alloc_frame();
		avcodec_get_frame_defaults(frame);
		ret = get_video_frame(is, frame, &pkt, &serial);
		if (!ret) {
			av_free(frame);
			frame = NULL;
			continue;
		}
		if(ret>0) {
			AVFrame* pFrameRGB = avcodec_alloc_frame();
			//			// Assign appropriate parts of buffer to image planes in pFrameRGB
			//			  // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
			//			  // of AVPicture
			avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB565,
					is->rgb_width, is->rgb_width);
			sws_scale(sws_ctx,(uint8_t const * const *)frame->data,frame->linesize,0,
					is->video_st->codec->height,
					pFrameRGB->data,
					pFrameRGB->linesize);
			frame->pts = av_frame_get_best_effort_timestamp(frame);//获取时间戳
			frame->pts = frame->pts*av_q2d(is->video_st->time_base)*1000;//转化为实际播放时间(ms)
			pFrameRGB->pts = frame->pts;
			mPlayerManager->insert_video(pFrameRGB);
		}
		av_free(frame);
		frame = NULL;
		av_free_packet(&pkt);
	}
	the_end:
	avcodec_flush_buffers(is->video_st->codec);
	return 0;
}

int stream_component_open(VideoState *is, int stream_index) {
	AVFormatContext *ic = is->ic;
	AVCodecContext *avctx;
	AVCodec *codec;
	if (stream_index < 0 || stream_index >= ic->nb_streams)
		return -1;
	avctx = ic->streams[stream_index]->codec;
	codec = avcodec_find_decoder(avctx->codec_id);
	if (avcodec_open2(avctx, codec, NULL) < 0) {
		return -1;
	}

	switch(avctx->codec_type){
	case AVMEDIA_TYPE_AUDIO:
		LOGD("audio thread..........................");
		is->audio_stream = stream_index;
		is->audio_st = ic->streams[stream_index];
		audio_queue.packet_queue_start();
		pthread_create(&is->audio_tid, NULL, audio_thread, is);
		break;
	case AVMEDIA_TYPE_VIDEO:
		LOGD("video_thread................................");
		//mPlayerManager->setVideoParams(avctx->width, avctx->height);
		//		is->last_video_stream = stream_index;
		is->video_stream = stream_index;
		is->video_st = ic->streams[stream_index];
		video_queue.packet_queue_start();
		pthread_create(&is->video_tid, NULL, video_thread, is);
		//		pthread_create(&delay_tid, NULL, handle_delay_event, is);
		break;
	}
}

void* read_thread(void *arg) {
	VideoState *is = (VideoState*)arg;
	AVFormatContext *pFormatCtx = NULL;
	AVPacket pkt1, *pkt = &pkt1;
	int64_t stream_start_time;
	int pkt_in_play_range = 0;
	int st_index[AVMEDIA_TYPE_NB];
	memset(st_index, -1, sizeof(st_index));
	if (avformat_open_input(&pFormatCtx, is->filename, NULL, NULL)!=0) {
		LOGD("avformat_open failed...%s", is->filename);
		//		return -1;
	} else {
		LOGD("avformat_open success");
	}
	is->ic = pFormatCtx;
	if(av_find_stream_info(pFormatCtx)<0) {
		LOGD("av find stream info failed");
	} else {
		LOGD("av find stream info success");
	}

	av_dump_format(pFormatCtx, 0, is->filename, 0);//我们可以使用这个函数把获取到得参数全部输出。
	LOGD("av_dump_format success");
	int video_disable = -1;
	int audio_disable = -1;

	for(jint i=0; i<pFormatCtx->nb_streams; i++) {//区分视频流和音频流
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO && video_disable<0) {//找到视频流
			video_disable = 1;
			stream_component_open(is, i);
		}
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && audio_disable<0) {//找到音频流
			audio_disable = 1;
			stream_component_open(is, i);
		}
	}
	while (av_read_frame(pFormatCtx, pkt)>=0) {
		stream_start_time = pFormatCtx->streams[pkt->stream_index]->start_time;
		if (pkt->stream_index == is->audio_stream ) {
			audio_queue.packet_queue_put(pkt);
		} else if (pkt->stream_index == is->video_stream) {
			video_queue.packet_queue_put(pkt);
		} else {
			av_free_packet(pkt);
		}
	}
}


int FFmpegDecoder::open_video(const char* path) {
	avcodec_register_all();
	av_register_all();
	LOGD("av_register success");
	is = (VideoState*)av_mallocz(sizeof(VideoState));//分配内存并将其内容清零
	if (!is)
		return -1;
	av_strlcpy(is->filename, path, sizeof(is->filename));
	pthread_create(&is->read_tid, NULL, read_thread, is);
	return 1;
}

int FFmpegDecoder::init_translate(int width, int height) {
	if (is) {
		is->rgb_width  = width;
		is->rgb_height = height;
	}
	return 1;
}
