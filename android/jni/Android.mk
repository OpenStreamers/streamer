LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := android
LOCAL_SRC_FILES := android.cpp

include $(BUILD_SHARED_LIBRARY)
