LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LIBSOURCES := $(wildcard $(LOCAL_PATH)/common/*.c) \
                            $(wildcard $(LOCAL_PATH)/dec/*.c) \
                            $(wildcard $(LOCAL_PATH)/enc/*.c)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SRC_FILES := $(LIBSOURCES:$(LOCAL_PATH)/%=%) \
                BrotliUtils.cpp
LOCAL_LDLIBS := -llog
LOCAL_MODULE := BrotliUtils

include $(BUILD_SHARED_LIBRARY)