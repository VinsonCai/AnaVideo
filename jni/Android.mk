LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := avcodec
LOCAL_SRC_FILES := libavcodec.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := avfilter
LOCAL_SRC_FILES := libavfilter.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := avutil
LOCAL_SRC_FILES := libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := swscale
LOCAL_SRC_FILES := libswscale.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := swresample
LOCAL_SRC_FILES := libswresample.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS -Wno-sign-compare -Wno-switch -Wno-pointer-sign -DHAVE_NEON=1 \
      -mfpu=neon -mfloat-abi=softfp -fPIC -DANDROID 

LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/include
LOCAL_MODULE    := local-player

LOCAL_SRC_FILES :=  PacketQueue.cpp AudioPlay.cpp PlayerManager.cpp FFmpegDecoder.cpp  decode.cpp
LOCAL_LDLIBS :=-L$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib \
-L$(LOCAL_PATH)  -lavformat -lavcodec -lavdevice -lavfilter -lavutil \
-lswscale -lswresample -llog -ljnigraphics -lz -lgcc  -lGLESv2 -lGLESv1_CM  -ldl
LOCAL_LDLIBS    += -lOpenSLES
LOCAL_LDLIBS    += -landroid

include $(BUILD_SHARED_LIBRARY)
include $(all-subdir-makefiles)