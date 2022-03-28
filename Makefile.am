ACLOCAL_AMFLAGS := -I m4


AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS := -Wundef \
             -Wstrict-prototypes \
             -Wno-trigraphs \
             -g -O0 \
             -fno-inline \
             -fno-short-enums \
             -fpic \
             -fpermissive \
             -D__reboot=reboot \
             $(LIBSELINUX_CFLAGS) \
             -D_GNU_SOURCE \
             -D_LARGEFILE64_SOURCE \
             -Dstrlcpy=g_strlcpy \
             -Dstrlcat=g_strlcat \
             -D_UE_SIDELOAD \
             -DUSE_BINDER \
             -D_ANDROID_HOST_ \
             -D_aarch64_ \
             -D_ANDROID_

UE_CFLAGS := -DBASE_VER=576279 \
             -g -O0 \
             -DUSE_BINDER=1 \
             -DUSE_CHROME_NETWORK_PROXY=0 \
             -DUSE_CHROME_KIOSK_APP=0 \
             -DUSE_HWID_OVERRIDE=0 \
             -DUSE_MTD=0 \
             -DUSE_OMAHA=0 \
             -D_FILE_OFFSET_BITS=64 \
             -D_POSIX_C_SOURCE=199309L \
             -Wa,--noexecstack \
             -Wall \
             -Werror \
             -Wextra \
             -Wformat=2 \
             -Wno-psabi \
             -Wno-unused-parameter \
             -ffunction-sections \
             -fstack-protector-strong \
             -fvisibility=hidden \
             -DUSE_FEC=0

AM_CPPFLAGS := \
              -DUSE_LE_MODE \
              -I. \
              -I.. \
              -I$(WORKSPACE)/OTA/recovery \
              -I$(WORKSPACE)/OTA/streaming/system/update_engine/ \
              -I$(WORKSPACE)/OTA/streaming/external/libchrome \
              -I$(WORKSPACE)/OTA/streaming/external/libbrillo \
              -I$(WORKSPACE)/system/core/include \
              -I$(WORKSPACE)/system/core/base/include \
              -I$(WORKSPACE)/system/core/adb \
              $(LIBSELINUX_CFLAGS) \
              -I$(WORKSPACE)/OTA/streaming/frameworks/native/libs/binder/include \
              -I$(WORKSPACE)/OTA/system/core/base/include \
              -I$(WORKSPACE)/OTA/system/extras/perfprofd \
              -I$(WORKSPACE)/OTA/streaming/system/update_engine/client_library/include \
              -I$(WORKSPACE)/OTA/streaming/external/lzma \
              -I$(WORKSPACE)/OTA/streaming/external/xz-embedded/linux/include/linux \
              -I$(WORKSPACE)/OTA/streaming/external/lzma/C \
              -I$(WORKSPACE)/OTA/streaming/external/bsdiff/include \
              -I$(WORKSPACE)/OTA/streaming/external/libbinderwrapper/include \
              -I$(WORKSPACE)/OTA/system/core/include \
              -I$(WORKSPACE)/OTA/streaming/external/libevent/include \
              -std=c++11 \
              -DENABLE_LEGACY_BOOTLOADER_MSG_UTILS \
              -Dstrlcpy=g_strlcpy \
              -Dstrlcat=g_strlcat \
              -D_UE_SIDELOAD \
              -DUSE_BINDER \
              -D_ANDROID_HOST_ \
              -D_aarch64_ \
              -D_ANDROID_



library_includedir := $(pkgincludedir)

AM_LDFLAGS := -shared -version-info 1:0:0

lib_LTLIBRARIES := libpayload_generator.la
libpayload_generator_la_SOURCES := \
    ../update_engine/payload_generator/ab_generator.cc \
    ../update_engine/payload_generator/annotated_operation.cc \
    ../update_engine/payload_generator/blob_file_writer.cc \
    ../update_engine/payload_generator/block_mapping.cc \
    ../update_engine/payload_generator/bzip.cc \
    ../update_engine/payload_generator/cycle_breaker.cc \
    ../update_engine/payload_generator/delta_diff_generator.cc \
    ../update_engine/payload_generator/delta_diff_utils.cc \
    ../update_engine/payload_generator/ext2_filesystem.cc \
    ../update_engine/payload_generator/extent_ranges.cc \
    ../update_engine/payload_generator/extent_utils.cc \
    ../update_engine/payload_generator/full_update_generator.cc \
    ../update_engine/payload_generator/graph_types.cc \
    ../update_engine/payload_generator/graph_utils.cc \
    ../update_engine/payload_generator/inplace_generator.cc \
    ../update_engine/payload_generator/mapfile_filesystem.cc \
    ../update_engine/payload_generator/payload_file.cc \
    ../update_engine/payload_generator/payload_generation_config.cc \
    ../update_engine/payload_generator/payload_signer.cc \
    ../update_engine/payload_generator/raw_filesystem.cc \
    ../update_engine/payload_generator/tarjan.cc \
    ../update_engine/payload_generator/topological_sort.cc \
    ../update_engine/payload_generator/xz_android.cc \
    ../update_engine/update_metadata.pb.cc

if USE_GLIB
libpayload_generator_la_CFLAGS := $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libpayload_generator_la_CPPFLAGS := $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libpayload_generator_la_LIBADD := -lm -lpthread -levent -lprotobuf @GLIB_LIBS@
libpayload_generator_la_LDFLAGS := -all-static
else
libpayload_generator_la_CFLAGS := $(AM_CFLAGS)
libpayload_generator_la_CPPFLAGS := $(AM_CPPFLAGS)
libpayload_generator_la_LIBADD := -lm -lpthread -levent -lprotobuf
libpayload_generator_la_LDFLAGS := -all-static
endif

lib_LTLIBRARIES += libpayload_consumer.la
libpayload_consumer_la_SOURCES := \
        ../update_engine/common/action_processor.cc  \
        ../update_engine/common/boot_control_stub.cc  \
        ../update_engine/common/clock.cc  \
        ../update_engine/common/constants.cc  \
        ../update_engine/common/cpu_limiter.cc  \
        ../update_engine/common/error_code_utils.cc  \
        ../update_engine/common/file_fetcher.cc  \
        ../update_engine/common/hash_calculator.cc  \
        ../update_engine/common/http_common.cc  \
        ../update_engine/common/http_fetcher.cc  \
        ../update_engine/common/hwid_override.cc  \
        ../update_engine/common/multi_range_http_fetcher.cc  \
        ../update_engine/common/platform_constants_android.cc  \
        ../update_engine/common/prefs.cc  \
        ../update_engine/common/subprocess.cc  \
        ../update_engine/common/terminator.cc  \
        ../update_engine/common/utils.cc  \
        ../update_engine/payload_consumer/bzip_extent_writer.cc  \
        ../update_engine/payload_consumer/delta_performer.cc  \
        ../update_engine/payload_consumer/download_action.cc  \
        ../update_engine/payload_consumer/extent_writer.cc  \
        ../update_engine/payload_consumer/file_descriptor.cc  \
        ../update_engine/payload_consumer/file_writer.cc  \
        ../update_engine/payload_consumer/filesystem_verifier_action.cc  \
        ../update_engine/payload_consumer/install_plan.cc  \
        ../update_engine/payload_consumer/payload_constants.cc  \
        ../update_engine/payload_consumer/payload_verifier.cc  \
        ../update_engine/payload_consumer/postinstall_runner_action.cc  \
        ../update_engine/payload_consumer/xz_extent_writer.cc

if USE_GLIB
libpayload_consumer_la_CFLAGS := $(AM_CFLAGS) $(UE_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libpayload_consumer_la_CPPFLAGS := $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libpayload_consumer_la_LIBADD := -lm -lpthread -levent -lprotobuf -lbspatch ./libpayload_generator.la -lbrillo -lchrome @GLIB_LIBS@
libpayload_consumer_la_LDFLAGS := -all-static
else
libpayload_consumer_la_CFLAGS := $(AM_CFLAGS)
libpayload_consumer_la_CPPFLAGS := $(AM_CPPFLAGS)
libpayload_consumer_la_LIBADD := -lm -lpthread -levent  -lprotobuf -lbspatch
libpayload_consumer_la_LDFLAGS := -all-static
endif


lib_LTLIBRARIES += libupdate_engine_client.la
libupdate_engine_client_la_SOURCES := \
    ../update_engine/android/os/IUpdateEngineCallback.cpp \
    ../update_engine/android/os/IUpdateEngine.cpp \
    ../update_engine/binder_service_android.cc \
    ../update_engine/certificate_checker.cc \
    ../update_engine/daemon.cc \
    ../update_engine/daemon_state_android.cc \
    ../update_engine/hardware_android.cc \
    ../update_engine/libcurl_http_fetcher.cc \
    ../update_engine/proxy_resolver.cc \
    ../update_engine/update_attempter_android.cc \
    ../update_engine/update_status_utils.cc

if USE_GLIB
libupdate_engine_client_la_CFLAGS := $(AM_CFLAGS) $(UE_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libupdate_engine_client_la_CPPFLAGS := $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libupdate_engine_client_la_LIBADD := -lm -lpthread -lprotobuf -lcurl -lmodpb64 -lbinderstreaming -lchrome -lbrillo -levent -lutils -lbspatch ./libpayload_consumer.la @GLIB_LIBS@
libupdate_engine_client_la_LDFLAGS := -all-static
else
libupdate_engine_client_la_CFLAGS := $(AM_CFLAGS)
libupdate_engine_client_la_CPPFLAGS := $(AM_CPPFLAGS)
libupdate_engine_client_la_LIBADD := -lm -lpthread -levent  -lprotobuf -lbspatch
endif



#lib_LTLIBRARIES += libupdate_engine_boot_control.la
libupdate_engine_boot_control_la_SOURCES := \
    ../update_engine/boot_control_android.cc

if USE_GLIB
libupdate_engine_boot_control_la_CFLAGS := $(AM_CFLAGS) $(UE_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libupdate_engine_boot_control_la_CPPFLAGS := $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libupdate_engine_boot_control_la_LDFLAGS := -all-static
else
libupdate_engine_client_la_CFLAGS := $(AM_CFLAGS)
libupdate_engine_client_la_CPPFLAGS := $(AM_CPPFLAGS)
endif
ue_delta_generator_src_files := \
    ../update_engine/payload_generator/generate_delta_main.cc

delta_generator_SOURCES := $(ue_delta_generator_src_files)

requiredlibs := -lm \
               -lpthread \
               -lz \
               -lmincrypt \
               -lselinux \
               -lcutils \
               -llog \
               -lprotobuf \
               ./libpayload_generator.la \
               ./libpayload_consumer.la \
               -lalzma \
               -lbz2 \
               -lxz \
               -lmodpb64 \
               -lbinderstreaming \
               -lutils \
               -lcrypto


delta_generator_requiredlibs := -lm \
               -lpthread \
               -lz \
               -lmincrypt \
               -lselinux \
               -lprotobuf \
               ./libpayload_generator.la \
               ./libpayload_consumer.la \
               -lbase \
               -lchrome \
               -levent \
               -lcutils \
               -llog \
               -lalzma \
               -lbz2 \
               -lxz \
               -lmodpb64 \
               -lutils \
               -lbinderstreaming \
               -lcrypto


if USE_GLIB
delta_generator_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
delta_generator_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
delta_generator_LDADD := $(delta_generator_requiredlibs) @GLIB_LIBS@
else
delta_generator_CFLAGS := $(AM_CFLAGS)  -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
delta_generator_CPPFLAGS :=  $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
delta_generator_LDADD := $(delta_generator_requiredlibs)
endif
bin_PROGRAMS = delta_generator

cpp_sources := update_engine/scripts/brillo_update_payload


RECOVERY_API_VERSION := 3

brillo_update_payload_SOURCES := $(cpp_sources)
if USE_GLIB
brillo_update_payload_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
brillo_update_payload_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
brillo_update_payload_CPPFLAGS += -I $(WORKSPACE)/system/core/include
brillo_update_payload_LDADD := $(requiredlibs) ./delta_generator @GLIB_LIBS@
else
brillo_update_payload_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
brillo_update_payload_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
brillo_update_payload_CPPFLAGS += -I $(WORKSPACE)/system/core/include
brillo_update_payload_LDADD := $(requiredlibs)
endif


bin_PROGRAMS += brillo_update_payload

update_engine_sideload_src_files := \
    ../update_engine/boot_control_recovery.cc \
    ../update_engine/network_selector_stub.cc \
    ../update_engine/hardware_android.cc \
    ../update_engine/proxy_resolver.cc \
    ../update_engine/sideload_main.cc \
    ../update_engine/update_attempter_android.cc \
    ../update_engine/update_status_utils.cc \
    ../update_engine/update_engine_service.cc


update_engine_sideload_SOURCES := $(update_engine_sideload_src_files)

update_engine_sideload_requiredlibs := -lm \
               -lpthread \
               -lz \
               -lmincrypt \
               -lselinux \
               -lbssl \
               -lbrillo \
               -lchrome \
               -lcutils \
               -levent \
               -llog \
               -lprotobuf \
               ./libpayload_generator.la \
               ./libpayload_consumer.la \
               ./libupdate_engine_client.la \
               -levent \
               -lmodpb64 \
               -lbinderstreaming \
               -lutils \
               -lbinderwrapper \
               -lbspatch \
               -lalzma \
               -lbz2 \
               -lxz \
               -lcrypto \
               -labctl \
               -lmtdutils


if USE_GLIB
update_engine_sideload_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
update_engine_sideload_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
update_engine_sideload_LDADD := $(update_engine_sideload_requiredlibs) -lext4_utils -lmincrypt  @GLIB_LIBS@
else
update_engine_sideload_CFLAGS := $(AM_CFLAGS)  -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
update_engine_sideload_CPPFLAGS :=  $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
update_engine_sideload_LDADD := $(requiredlibs)
endif

bin_PROGRAMS += update_engine_sideload

pkgconfigdir := $(libdir)/pkgconfig
EXTRA_DIST := $(pkgconfig_DATA)

if TARGET_SUPPORTS_AB
brillo_update_payload_CPPFLAGS += -DTARGET_SUPPORTS_AB
brillo_update_payload_LDADD += -labctl
endif

if TARGET_SUPPORTS_OTA_VERIFICATION
brillo_update_payload_CPPFLAGS += -DTARGET_SUPPORTS_OTA_VERIFICATION
endif

if TARGET_NAND_AB_BOOT
brillo_update_payload_CPPFLAGS += -DTARGET_NAND_AB_BOOT
endif
