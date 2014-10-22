
# Copyright (C) 2010 The Android Open Source Project

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

SAMPLE_PATH := $(call my-dir)/../../
GAMEPLAY_LOCAL_PATH = ../../../GamePlay/

GAMEPLAY_PATH := $(SAMPLE_PATH)$(GAMEPLAY_LOCAL_PATH)

LIBPNG_PATH := $(GAMEPLAY_PATH)external-deps/png/lib/android/arm
ZLIB_PATH := $(GAMEPLAY_PATH)external-deps/zlib/lib/android/arm
LUA_PATH := $(GAMEPLAY_PATH)external-deps/lua/lib/android/arm
BULLET_PATH := $(GAMEPLAY_PATH)external-deps/bullet/lib/android/arm
VORBIS_PATH := $(GAMEPLAY_PATH)external-deps/vorbis/lib/android/arm
OGG_PATH := $(GAMEPLAY_PATH)external-deps/ogg/lib/android/arm
OPENAL_PATH := $(GAMEPLAY_PATH)external-deps/openal/lib/android/arm
LIBYAML_PATH := $(GAMEPLAY_PATH)external-deps/yaml/lib/android/arm
LIBSIGCPP_PATH := $(SAMPLE_PATH)../../ThirdParty/libsigc++/lib/android/arm
LIBCURL_PATH := $(SAMPLE_PATH)../../ThirdParty/libcurl/lib/android/arm
LIBICU_PATH := $(SAMPLE_PATH)../../ThirdParty/icu/lib/android/arm
LIBZIP_PATH := $(SAMPLE_PATH)../../ThirdParty/zip/lib/android/arm

# gameplay
LOCAL_PATH := $(GAMEPLAY_PATH)gameplay/android/obj/local/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE    := libgameplay
LOCAL_SRC_FILES := libgameplay.a
include $(PREBUILT_STATIC_LIBRARY)

# libpng
LOCAL_PATH := $(LIBPNG_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libpng 
LOCAL_SRC_FILES := libpng.a
include $(PREBUILT_STATIC_LIBRARY)

# libzlib
LOCAL_PATH := $(ZLIB_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libz
LOCAL_SRC_FILES := libz.a
include $(PREBUILT_STATIC_LIBRARY)

# liblua
LOCAL_PATH := $(LUA_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := liblua
LOCAL_SRC_FILES := liblua.a
include $(PREBUILT_STATIC_LIBRARY)

# libLinearMath
LOCAL_PATH := $(BULLET_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libLinearMath
LOCAL_SRC_FILES := libLinearMath.a
include $(PREBUILT_STATIC_LIBRARY)

# libBulletCollision
LOCAL_PATH := $(BULLET_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libBulletCollision
LOCAL_SRC_FILES := libBulletCollision.a
include $(PREBUILT_STATIC_LIBRARY)

# libBulletDynamics
LOCAL_PATH := $(BULLET_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libBulletDynamics
LOCAL_SRC_FILES := libBulletDynamics.a
include $(PREBUILT_STATIC_LIBRARY)

# libvorbis
LOCAL_PATH := $(VORBIS_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libvorbis
LOCAL_SRC_FILES := libvorbis.a
include $(PREBUILT_STATIC_LIBRARY)

# libogg
LOCAL_PATH := $(OGG_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libogg
LOCAL_SRC_FILES := libogg.a
include $(PREBUILT_STATIC_LIBRARY)

# libOpenAL
LOCAL_PATH := $(OPENAL_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libOpenAL
LOCAL_SRC_FILES := libOpenAL.a
include $(PREBUILT_STATIC_LIBRARY)

# libyaml
LOCAL_PATH := $(LIBYAML_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libyaml
LOCAL_SRC_FILES := libyaml.a
include $(PREBUILT_STATIC_LIBRARY)

# libsigc++
LOCAL_PATH := $(LIBSIGCPP_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libsigc++
LOCAL_SRC_FILES := libsigc++.a
include $(PREBUILT_STATIC_LIBRARY)

# libcurl
LOCAL_PATH := $(LIBCURL_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libcurl
LOCAL_SRC_FILES := libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

# libicu
LOCAL_PATH := $(LIBICU_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libicuuc
LOCAL_SRC_FILES := libicuuc.a
include $(PREBUILT_STATIC_LIBRARY)

# libicu
LOCAL_PATH := $(LIBICU_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libicui18n
LOCAL_SRC_FILES := libicui18n.a
include $(PREBUILT_STATIC_LIBRARY)

# libicu
LOCAL_PATH := $(LIBICU_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libicudata
LOCAL_SRC_FILES := libicudata.a
include $(PREBUILT_STATIC_LIBRARY)

# libzip
LOCAL_PATH := $(LIBZIP_PATH)
include $(CLEAR_VARS)
LOCAL_MODULE    := libzip
LOCAL_SRC_FILES := libzip.a
include $(PREBUILT_STATIC_LIBRARY)

# armor inspector
LOCAL_PATH := $(SAMPLE_PATH)
include $(CLEAR_VARS)

LOCAL_MODULE    := wot-armor-inspector
LOCAL_SRC_FILES := \
    $(GAMEPLAY_LOCAL_PATH)gameplay/src/gameplay-main-android.cpp \
    pch.cpp \
    uuidlib/clear.c \
    uuidlib/compare.c \
    uuidlib/copy.c \
    uuidlib/gen_uuid.c \
    uuidlib/isnull.c \
    uuidlib/pack.c \
    uuidlib/parse.c \
    uuidlib/unpack.c \
    uuidlib/unparse.c \
    uuidlib/uuid_time.c \
    base/main.cpp \
    base/main/asset.cpp \
    base/main/gameplay_assets.cpp \
    base/main/dictionary.cpp \
    base/math/transformable.cpp \
    base/render/particle_system.cpp \
    base/services/debug_service.cpp \
    base/services/input_service.cpp \
    base/services/render_service.cpp \
    base/services/service.cpp \
    base/services/service_manager.cpp \
    base/services/social_service.cpp \
    base/services/storefront_service.cpp \
    base/services/tracker_service.cpp \
    base/utils/singleton.cpp \
    base/utils/profiler.cpp \
    base/utils/utils.cpp \
    game/game.cpp \
    game/memory_stream.cpp \
    game/view_settings.cpp \
    game/wot_model.cpp \
    game/wot_text.cpp \
    game/wot_vehicle.cpp \
    game/wot_vehicle_info.cpp \
    game/wot_vehicles_db.cpp \
    game/wot_xml.cpp \
    game/services/scene_service.cpp \
    game/services/ui_service.cpp \

LOCAL_LDLIBS    := -llog -landroid -lEGL -lGLESv2 -lOpenSLES
LOCAL_CFLAGS    := -O3 -D__ANDROID__ -Wno-psabi -fpermissive -fexceptions \
    -I"$(GAMEPLAY_PATH)external-deps/lua/include" \
    -I"$(GAMEPLAY_PATH)external-deps/bullet/include" \
    -I"$(GAMEPLAY_PATH)external-deps/png/include" \
    -I"$(GAMEPLAY_PATH)external-deps/ogg/include" \
    -I"$(GAMEPLAY_PATH)external-deps/vorbis/include" \
    -I"$(GAMEPLAY_PATH)external-deps/openal/include" \
    -I"$(GAMEPLAY_PATH)external-deps/yaml/include" \
    -I"$(SAMPLE_PATH)../../ThirdParty/libsigc++/include" \
    -I"$(SAMPLE_PATH)../../ThirdParty/libcurl/include" \
    -I"$(SAMPLE_PATH)../../ThirdParty/icu/include" \
    -I"$(SAMPLE_PATH)../../ThirdParty/zip/include" \
    -I"$(SAMPLE_PATH)base" \
    -I"$(GAMEPLAY_PATH)gameplay/src" \
    -DHAVE_INTTYPES_H \
    -DHAVE_UNISTD_H \
    -DHAVE_ERRNO_H \
    -DHAVE_NETINET_IN_H \
    -DHAVE_SYS_IOCTL_H \
    -DHAVE_SYS_MMAN_H \
    -DHAVE_SYS_MOUNT_H \
    -DHAVE_SYS_PRCTL_H \
    -DHAVE_SYS_RESOURCE_H \
    -DHAVE_SYS_SELECT_H \
    -DHAVE_SYS_STAT_H \
    -DHAVE_SYS_TYPES_H \
    -DHAVE_STDLIB_H \
    -DHAVE_STRDUP \
    -DHAVE_MMAP \
    -DHAVE_UTIME_H \
    -DHAVE_GETPAGESIZE \
    -DHAVE_LSEEK64 \
    -DHAVE_LSEEK64_PROTOTYPE \
    -DHAVE_EXT2_IOCTLS \
    -DHAVE_LINUX_FD_H \
    -DHAVE_TYPE_SSIZE_T \
    -DHAVE_SYS_TIME_H \
    -DHAVE_SYS_PARAM_H \
    -DHAVE_SYSCONF \
    -D_GLIBCXX_USE_C99_STDINT_TR1 \
    -DU_DISABLE_RENAMING=1 \
    -DGP_USE_SOCIAL \
    -DGP_USE_STOREFRONT \

LOCAL_CPPFLAGS := -std=gnu++0x  -Wno-switch-enum -Wno-switch
LOCAL_STATIC_LIBRARIES := android_native_app_glue libgameplay libpng libz liblua libBulletDynamics libBulletCollision libLinearMath libvorbis libogg libOpenAL libyaml libsigc++ libcurl libicui18n libicuuc libicudata libzip
LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
