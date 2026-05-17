LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE     := module
LOCAL_SRC_FILES  := module.cpp
LOCAL_LDLIBS     := -llog
LOCAL_CPPFLAGS   := -std=c++23 -fvisibility=hidden -fvisibility-inlines-hidden
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../build/generated/jni
include $(BUILD_SHARED_LIBRARY)
