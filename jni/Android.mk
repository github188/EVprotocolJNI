# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := EVprotocol
LOCAL_SRC_FILES := com_easivend_evprotocol_EVprotocol.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/ev_driver/smart210_uart.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/ev_api/EV_com.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/ev_api/EV_timer.c
LOCAL_SRC_FILES += $(LOCAL_PATH)/ev_driver/ev_config.c

LOCAL_MODULE_FILENAME := libEVprotocol
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)
