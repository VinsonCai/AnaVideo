#include <jni.h>
#include <list>
extern "C"{
jint Java_com_lisa_testplayer_jni_LocalPlayer_open(JNIEnv* env, jobject thiz, jstring pFileName);
void Java_com_lisa_testplayer_jni_LocalPlayer_initTranslate(JNIEnv *env, jobject thiz, jint pWidth, jint pHeight);
jint Java_com_lisa_testplayer_jni_LocalPlayer_translateColor(JNIEnv *env,jobject thiz,jbyteArray out);

}
#include "PacketQueue.h"
#include "PlayerManager.h"
#include "FFmpegDecoder.h"


jint Java_com_lisa_testplayer_jni_LocalPlayer_open(JNIEnv* env, jobject thiz, jstring pFileName) {
	FFmpegDecoder *ffmpegDecoder = FFmpegDecoder::GetInstance();
	const char *_filename = env->GetStringUTFChars(pFileName, NULL);
	ffmpegDecoder->open_video(_filename);
	PlayerManager *playerManager = PlayerManager::GetInstance();
	playerManager->init_player();
	env->ReleaseStringUTFChars(pFileName, _filename);
}

void Java_com_lisa_testplayer_jni_LocalPlayer_initTranslate(JNIEnv *env, jobject thiz, jint pWidth, jint pHeight) {
	FFmpegDecoder *ffmpegDecoder = FFmpegDecoder::GetInstance();
	ffmpegDecoder->init_translate(pWidth, pHeight);
	PlayerManager *playerManager = PlayerManager::GetInstance();
	playerManager->init_video(pWidth, pHeight);
}


jint Java_com_lisa_testplayer_jni_LocalPlayer_translateColor(JNIEnv *env,jobject thiz,jbyteArray out){
	PlayerManager *playerManager = PlayerManager::GetInstance();
	jbyte *Picture= env->GetByteArrayElements(out,0);
	playerManager->translate_color(Picture);
	env->ReleaseByteArrayElements(out, Picture, 0);

}

