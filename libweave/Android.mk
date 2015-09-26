# Copyright (C) 2015 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# TODO(avakulenko): Remove this condition when libchromeos can be built on
# non-Linux host.
ifeq ($(HOST_OS),linux)

# Common variables
# ========================================================

libweaveCommonCppExtension := .cc
libweaveCommonCFlags := -Wall -Werror \
	-Wno-char-subscripts -Wno-missing-field-initializers \
	-Wno-unused-function -Wno-unused-parameter

libweaveCommonCppFlags := \
	-Wno-deprecated-register \
	-Wno-sign-compare \
	-Wno-sign-promo \
	-Wno-non-virtual-dtor \

libweaveCommonCIncludes := \
	$(LOCAL_PATH)/.. \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/third_party/modp_b64/modp_b64 \
	external/gtest/include \

libweaveSharedLibraries := \
	libchrome \
	libexpat \
	libcrypto \

# libweave-external
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave-external
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := $(libweaveCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_STATIC_LIBRARIES :=
LOCAL_RTTI_FLAG := -frtti
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/external

LOCAL_SRC_FILES := \
	external/crypto/p224.cc \
	external/crypto/p224_spake.cc \
	external/crypto/sha2.cc \
	third_party/modp_b64/modp_b64.cc \

include $(BUILD_STATIC_LIBRARY)

# libweave-common
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave-common
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := $(libweaveCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_STATIC_LIBRARIES := libweave-external
LOCAL_RTTI_FLAG := -frtti
LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
	src/backoff_entry.cc \
	src/base_api_handler.cc \
	src/commands/cloud_command_proxy.cc \
	src/commands/command_definition.cc \
	src/commands/command_dictionary.cc \
	src/commands/command_instance.cc \
	src/commands/command_manager.cc \
	src/commands/command_queue.cc \
	src/commands/object_schema.cc \
	src/commands/prop_constraints.cc \
	src/commands/prop_types.cc \
	src/commands/prop_values.cc \
	src/commands/schema_constants.cc \
	src/commands/schema_utils.cc \
	src/commands/user_role.cc \
	src/config.cc \
	src/data_encoding.cc \
	src/device_manager.cc \
	src/device_registration_info.cc \
	src/error.cc \
	src/http_constants.cc \
	src/json_error_codes.cc \
	src/notification/notification_parser.cc \
	src/notification/pull_channel.cc \
	src/notification/xml_node.cc \
	src/notification/xmpp_channel.cc \
	src/notification/xmpp_iq_stanza_handler.cc \
	src/notification/xmpp_stream_parser.cc \
	src/privet/cloud_delegate.cc \
	src/privet/constants.cc \
	src/privet/device_delegate.cc \
	src/privet/openssl_utils.cc \
	src/privet/privet_handler.cc \
	src/privet/privet_manager.cc \
	src/privet/privet_types.cc \
	src/privet/publisher.cc \
	src/privet/security_manager.cc \
	src/privet/wifi_bootstrap_manager.cc \
	src/privet/wifi_ssid_generator.cc \
	src/registration_status.cc \
	src/states/error_codes.cc \
	src/states/state_change_queue.cc \
	src/states/state_manager.cc \
	src/states/state_package.cc \
	src/string_utils.cc \
	src/utils.cc \

include $(BUILD_STATIC_LIBRARY)

# libweave-test
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave-test
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := \
	$(libweaveCommonCIncludes) \
	external/gmock/include \

LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_STATIC_LIBRARIES := libgtest libgmock
LOCAL_CLANG := true
LOCAL_RTTI_FLAG := -frtti
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/test/fake_stream.cc \
	src/test/fake_task_runner.cc \
	src/test/mock_command.cc \
	src/test/mock_http_client.cc \
	src/test/unittest_utils.cc \

include $(BUILD_STATIC_LIBRARY)

# libweave
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := $(libweaveCommonCIncludes)
LOCAL_SHARED_LIBRARIES := $(libweaveSharedLibraries)
LOCAL_WHOLE_STATIC_LIBRARIES := libweave-external libweave-common
LOCAL_CLANG := true
LOCAL_RTTI_FLAG := -frtti
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/empty.cc

include $(BUILD_SHARED_LIBRARY)

# libweave_testrunner
# ========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libweave_testrunner
LOCAL_CPP_EXTENSION := $(libweaveCommonCppExtension)
LOCAL_CFLAGS := $(libweaveCommonCFlags)
LOCAL_CPPFLAGS := $(libweaveCommonCppFlags)
LOCAL_C_INCLUDES := \
	$(libweaveCommonCIncludes) \
	external/gmock/include \

LOCAL_SHARED_LIBRARIES := \
	$(libweaveSharedLibraries) \

LOCAL_STATIC_LIBRARIES := \
	libweave-external \
	libweave-common \
	libweave-test \
	libchromeos-test-helpers \
	libgtest libgmock \
	libchrome_test_helpers \

LOCAL_CLANG := true
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)

LOCAL_SRC_FILES := \
	external/crypto/p224_spake_unittest.cc \
	external/crypto/p224_unittest.cc \
	external/crypto/sha2_unittest.cc \
	src/backoff_entry_unittest.cc \
	src/base_api_handler_unittest.cc \
	src/commands/cloud_command_proxy_unittest.cc \
	src/commands/command_definition_unittest.cc \
	src/commands/command_dictionary_unittest.cc \
	src/commands/command_instance_unittest.cc \
	src/commands/command_manager_unittest.cc \
	src/commands/command_queue_unittest.cc \
	src/commands/object_schema_unittest.cc \
	src/commands/schema_utils_unittest.cc \
	src/config_unittest.cc \
	src/data_encoding_unittest.cc \
	src/device_registration_info_unittest.cc \
	src/error_unittest.cc \
	src/notification/notification_parser_unittest.cc \
	src/notification/xml_node_unittest.cc \
	src/notification/xmpp_channel_unittest.cc \
	src/notification/xmpp_iq_stanza_handler_unittest.cc \
	src/notification/xmpp_stream_parser_unittest.cc \
	src/privet/privet_handler_unittest.cc \
	src/privet/security_manager_unittest.cc \
	src/privet/wifi_ssid_generator_unittest.cc \
	src/states/state_change_queue_unittest.cc \
	src/states/state_manager_unittest.cc \
	src/states/state_package_unittest.cc \
	src/string_utils_unittest.cc \
	src/test/weave_testrunner.cc \
	src/weave_unittest.cc \

include $(BUILD_NATIVE_TEST)

endif # HOST_OS == linux
