LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)  
LOCAL_MODULE := avformat  
LOCAL_SRC_FILES := libavformat.a  
LOCAL_CFLAGS :=-Ilibavformat  
LOCAL_EXPORT_C_INCLUDES := libavformat  
LOCAL_EXPORT_CFLAGS := -Ilibavformat  
LOCAL_EXPORT_LDLIBS := -llog  
include $(PREBUILT_STATIC_LIBRARY)  
  
include $(CLEAR_VARS)  
LOCAL_MODULE := avcodec  
LOCAL_SRC_FILES := libavcodec.a  
LOCAL_CFLAGS :=-Ilibavcodec  
LOCAL_EXPORT_C_INCLUDES := libavcodec  
LOCAL_EXPORT_CFLAGS := -Ilibavcodec  
LOCAL_EXPORT_LDLIBS := -llog  
include $(PREBUILT_STATIC_LIBRARY)  
  
include $(CLEAR_VARS)  
LOCAL_MODULE := avutil  
LOCAL_SRC_FILES := libavutil.a   
LOCAL_CFLAGS :=-Ilibavutil  
LOCAL_EXPORT_C_INCLUDES := libavutil  
LOCAL_EXPORT_CFLAGS := -Ilibavutil  
LOCAL_EXPORT_LDLIBS := -llog  
include $(PREBUILT_STATIC_LIBRARY)  
  
include $(CLEAR_VARS)  
LOCAL_MODULE := swscale  
LOCAL_SRC_FILES :=libswscale.a  
LOCAL_CFLAGS :=-Ilibavutil -Ilibswscale  
LOCAL_EXPORT_C_INCLUDES := libswscale  
LOCAL_EXPORT_CFLAGS := -Ilibswscale  
LOCAL_EXPORT_LDLIBS := -llog 
include $(PREBUILT_STATIC_LIBRARY)   
  
include $(CLEAR_VARS)  
LOCAL_MODULE := avfilter  
LOCAL_SRC_FILES :=libavfilter.a
LOCAL_CFLAGS :=-Ilibavutil -Ilibswscale  
LOCAL_EXPORT_C_INCLUDES := libavfilter 
LOCAL_EXPORT_CFLAGS := -Ilibavfilter  
LOCAL_EXPORT_LDLIBS := -llog 
include $(PREBUILT_STATIC_LIBRARY)  

include $(CLEAR_VARS)  
LOCAL_MODULE := swresample  
LOCAL_SRC_FILES :=libswresample.a
LOCAL_CFLAGS :=-Ilibavutil -Ilibswscale  
LOCAL_EXPORT_C_INCLUDES := swresample 
LOCAL_EXPORT_CFLAGS := -Ilibswresample
LOCAL_EXPORT_LDLIBS := -llog 
include $(PREBUILT_STATIC_LIBRARY) 


include $(CLEAR_VARS)

LOCAL_MODULE    := ffmpegtools
LOCAL_SRC_FILES := xchannel.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_LDLIBS    := -L$(LOCAL_PATH) -lm -lz 
LOCAL_STATIC_LIBRARIES := avformat avcodec  swscale  swresample avfilter  avutil
LOCAL_LDLIBS    := -llog -lz -lm 
include $(BUILD_SHARED_LIBRARY)
