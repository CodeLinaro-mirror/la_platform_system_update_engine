ACLOCAL_AMFLAGS := -I m4


AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS := -Wundef \
             -Wstrict-prototypes \
             -Wno-trigraphs \
             -g -O0 \
             -fno-inline \
             -fno-short-enums \
             -fpic \
             -D__reboot=reboot \
             -D_GNU_SOURCE \
             -D_LARGEFILE64_SOURCE \
             -Dstrlcpy=g_strlcpy \
             -Dstrlcat=g_strlcat \
             -D_UE_SIDELOAD \
             -DUSE_BINDER

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
              -I$(WORKSPACE)/OTA/streaming/frameworks/native/libs/binder/include \
              -I$(WORKSPACE)/OTA/system/core/base/include \
              -I$(WORKSPACE)/OTA/system/extras/perfprofd \
              -I$(WORKSPACE)/OTA/streaming/system/update_engine/client_library/include \
              -I$(WORKSPACE)/OTA/streaming/external/xz-embedded/linux/include/linux \
              -I$(WORKSPACE)/OTA/streaming/external/xz-embedded/linux/include \
              -I$(WORKSPACE)/OTA/streaming/external/lzma \
              -I$(WORKSPACE)/OTA/streaming/external/lzma/C \
              -I$(WORKSPACE)/OTA/streaming/external/bsdiff/include \
              -I$(WORKSPACE)/OTA/streaming/external/libbinderwrapper/include \
              -I$(WORKSPACE)/OTA/system/core/include \
              -I$(WORKSPACE)/OTA/streaming/external/libevent/include \
              -std=c++17 \
              -D_UE_SIDELOAD \
              -DUSE_BINDER

if USE_G_STRLCPY_STRLCAT
AM_CPPFLAGS += -Dstrlcpy=g_strlcpy -Dstrlcat=g_strlcat
endif

if BUILD_HOST
AM_CPPFLAGS += -DUSE_NATIVE \
              -DBUILD_HOST
else
AM_CPPFLAGS += $(LIBSELINUX_CFLAGS) \
              -D_ANDROID_HOST_ \
              -D_aarch64_ \
              -D_ANDROID_
endif

if TELAF
AM_CPPFLAGS += -DUSE_TELAF
endif

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
libpayload_generator_la_LIBADD := -lm -lpthread -levent -lprotobuf-lite @GLIB_LIBS@
libpayload_generator_la_LDFLAGS := -all-static
libpayload_generator_la_LIBADD += -labsl_raw_logging_internal \
                                  -labsl_str_format_internal \
                                  -labsl_log_internal_message \
                                  -labsl_log_internal_globals \
                                  -labsl_log_internal_format \
                                  -labsl_log_internal_check_op
else
libpayload_generator_la_CFLAGS := $(AM_CFLAGS)
libpayload_generator_la_CPPFLAGS := $(AM_CPPFLAGS)
libpayload_generator_la_LIBADD := -lm -lpthread -levent -lprotobuf-lite
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
libpayload_consumer_la_CFLAGS := $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libpayload_consumer_la_CPPFLAGS := $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libpayload_consumer_la_LIBADD := -lm -lpthread -levent -lprotobuf-lite -lbspatch ./libpayload_generator.la -lbrillo -lchrome -lxz @GLIB_LIBS@
libpayload_consumer_la_LDFLAGS := -all-static
else
libpayload_consumer_la_CFLAGS := $(AM_CFLAGS)
libpayload_consumer_la_CPPFLAGS := $(AM_CPPFLAGS)
libpayload_consumer_la_LIBADD := -lm -lpthread -levent  -lprotobuf-lite -lbspatch -lxz
libpayload_consumer_la_LDFLAGS := -all-static
endif

if TARGET_NAND_BOOT
AM_CFLAGS += -DUSE_MTD=1
AM_CPPFLAGS += -DUSE_MTD=1
libpayload_consumer_la_SOURCES += \
        ../update_engine/payload_consumer/mtd_file_descriptor.cc
if TELAF
libpayload_consumer_la_LIBADD += -ltelaf_flash_access
else
libpayload_consumer_la_LIBADD += -lnad_ab_al \
                                  -lnad_mtd_al \
                                  -lnad_ubi_al
endif
endif

if !BUILD_HOST
lib_LTLIBRARIES += libupdate_engine_client.la
endif
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
libupdate_engine_client_la_CFLAGS := $(AM_CFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libupdate_engine_client_la_CPPFLAGS := $(AM_CPPFLAGS) -DUSE_GLIB @GLIB_CFLAGS@
libupdate_engine_client_la_LIBADD := -lm -lpthread -lprotobuf-lite -lcurl -lmodpb64 -lbinderstreaming -lchrome -lbrillo -levent -lutils -lbspatch ./libpayload_consumer.la @GLIB_LIBS@
libupdate_engine_client_la_LDFLAGS := -all-static
else
libupdate_engine_client_la_CFLAGS := $(AM_CFLAGS)
libupdate_engine_client_la_CPPFLAGS := $(AM_CPPFLAGS)
libupdate_engine_client_la_LIBADD := -lm -lpthread -levent  -lprotobuf-lite -lbspatch
endif


if BUILD_HOST
ue_delta_generator_src_files := \
    ../update_engine/payload_generator/generate_delta_main.cc

delta_generator_SOURCES := $(ue_delta_generator_src_files)
endif

requiredlibs := -lm \
               -lpthread \
               -lz \
               -lmincrypt \
               -lselinux \
               -lcutils \
               -llog \
               -lprotobuf-lite \
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
               -lprotobuf-lite \
               ./libpayload_generator.la \
               ./libpayload_consumer.la \
               -lbspatch \
               -lbase \
               -lbrillo \
               -lchrome \
               -lbinderstreaming \
               -levent \
               -lcutils \
               -llog \
               -lalzma \
               -lbz2 \
               -lxz \
               -lmodpb64 \
	       -lstdc++ \
               -lutils \
               -lcrypto \
               -lsparse

if USE_GLIB
delta_generator_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
delta_generator_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
delta_generator_LDADD := $(delta_generator_requiredlibs) @GLIB_LIBS@
else
delta_generator_CFLAGS := $(AM_CFLAGS)  -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
delta_generator_CPPFLAGS :=  $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
delta_generator_LDADD := $(delta_generator_requiredlibs)
endif


cpp_sources := update_engine/scripts/brillo_update_payload


RECOVERY_API_VERSION := 3

brillo_update_payload_SOURCES := $(cpp_sources)
if USE_GLIB
brillo_update_payload_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
brillo_update_payload_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
brillo_update_payload_CPPFLAGS += -I $(WORKSPACE)/system/core/include
brillo_update_payload_LDADD := $(delta_generator_requiredlibs) @GLIB_LIBS@
else
brillo_update_payload_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
brillo_update_payload_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
brillo_update_payload_CPPFLAGS += -I $(WORKSPACE)/system/core/include
brillo_update_payload_LDADD := $(delta_generator_requiredlibs)
endif


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
               -lssl \
               -lbrillo \
               -lchrome \
               -lcutils \
               -levent \
               -llog \
               -lprotobuf-lite \
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
               -lnad_ab_al

if TARGET_NAND_BOOT
if TELAF
update_engine_sideload_requiredlibs += -ltelaf_flash_access
else
update_engine_sideload_requiredlibs += -lnad_mtd_al \
               -lnad_ubi_al

endif
endif

if USE_GLIB
update_engine_sideload_CFLAGS := $(AM_CFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
update_engine_sideload_CPPFLAGS := $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION) -DUSE_GLIB @GLIB_CFLAGS@
update_engine_sideload_LDADD := $(update_engine_sideload_requiredlibs) -lext4_utils -lmincrypt  @GLIB_LIBS@
else
update_engine_sideload_CFLAGS := $(AM_CFLAGS)  -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
update_engine_sideload_CPPFLAGS :=  $(AM_CPPFLAGS) -DRECOVERY_API_VERSION=$(RECOVERY_API_VERSION)
update_engine_sideload_LDADD := $(requiredlibs)
endif

if TELAF
update_engine_sideload_LDFLAGS := -L${PKG_CONFIG_SYSROOT_DIR}/telaf/legato/legato-af/build/sa525m/framework/lib -L${PKG_CONFIG_SYSROOT_DIR}/telaf/legato/legato-af/build/sa525m/system/app/tafUpdateSvc/staging/read-only/lib
update_engine_sideload_LDADD +=  -llegato -lComponent_taf_lib_flash
endif

if BUILD_HOST
bin_PROGRAMS = brillo_update_payload
bin_PROGRAMS += delta_generator
else
bin_PROGRAMS = update_engine_sideload
endif

pkgconfigdir := $(libdir)/pkgconfig
EXTRA_DIST := $(pkgconfig_DATA)

if TARGET_SUPPORTS_AB
brillo_update_payload_CPPFLAGS += -DTARGET_SUPPORTS_AB
if !BUILD_HOST
brillo_update_payload_LDADD += -lnad_ab_al
if TELAF
brillo_update_payload_LDADD += -ltelaf_flash_access
else
brillo_update_payload_LDADD += -lnad_mtd_al \
                               -lnad_ubi_al
endif
endif
endif

if TARGET_SUPPORTS_OTA_VERIFICATION
brillo_update_payload_CPPFLAGS += -DTARGET_SUPPORTS_OTA_VERIFICATION
endif

if TARGET_NAND_AB_BOOT
brillo_update_payload_CPPFLAGS += -DTARGET_NAND_AB_BOOT
endif
