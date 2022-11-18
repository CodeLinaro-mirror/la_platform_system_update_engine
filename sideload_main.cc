//
// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Changes from Qualcomm Innovation Center are provided under the following license:
//
// Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above
//      copyright notice, this list of conditions and the following
//      disclaimer in the documentation and/or other materials provided
//      with the distribution.
//
//    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
//      contributors may be used to endorse or promote products derived
//      from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <linux/xz.h>

#include <string>
#include <vector>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <base/command_line.h>
#include <base/logging.h>
#include <base/strings/string_split.h>
#include <base/strings/stringprintf.h>
#include <brillo/asynchronous_signal_handler.h>
#include <brillo/flag_helper.h>
#include <brillo/make_unique_ptr.h>
#include <brillo/message_loops/base_message_loop.h>
#ifdef USE_LE_MODE
#include <brillo/message_loops/fake_message_loop.h>
#include <brillo/message_loops/message_loop.h>

#include <base/message_loop/message_loop.h>
#include <stdlib.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <base/run_loop.h>
#include <base/threading/thread.h>
#include "update_engine/apply_payload.h"
#endif

#include <brillo/streams/file_stream.h>
#include <brillo/streams/stream.h>

#include "update_engine/common/boot_control.h"
#include "update_engine/common/error_code_utils.h"
#include "update_engine/common/hardware.h"
#include "update_engine/common/prefs.h"
#include "update_engine/common/subprocess.h"
#include "update_engine/common/terminator.h"
#include "update_engine/common/utils.h"
#include "update_engine/update_attempter_android.h"
#include "update_engine/update_engine_service.h"

#ifdef USE_GLIB
#include <glib.h>
#endif

#if USE_MTD
extern "C" {
#include "telaf-flash-access.h"
}
#endif

using std::string;
using std::vector;
using update_engine::UpdateStatus;

#ifdef USE_LE_MODE
using namespace android;
#endif


namespace {
// The root directory used for temporary files in update_engine_sideload.
const char kSideloadRootTempDir[] = "/tmp/update_engine_sideload";
}  // namespace

namespace chromeos_update_engine {

void SetupLogging() {
  string log_file;
  logging::LoggingSettings log_settings;
  log_settings.lock_log = logging::DONT_LOCK_LOG_FILE;
  log_settings.delete_old = logging::APPEND_TO_OLD_LOG_FILE;
  log_settings.log_file = nullptr;
  log_settings.logging_dest = logging::LOG_TO_SYSTEM_DEBUG_LOG;

  logging::InitLogging(log_settings);
}

#ifdef USE_LE_MODE
class SideloadDaemonState : public DaemonStateInterface,
                            public ServiceObserverInterface {
 public:
  explicit SideloadDaemonState(brillo::StreamPtr status_stream)
      : status_stream_(std::move(status_stream)) {
    // Add this class as the only observer.
    observers_.insert(this);
  }
  ~SideloadDaemonState() override = default;

  // DaemonStateInterface overrides.
  bool StartUpdater() override { return true; }
  void AddObserver(ServiceObserverInterface* observer) override {}
  void RemoveObserver(ServiceObserverInterface* observer) override {}
  const std::set<ServiceObserverInterface*>& service_observers() override {
    return observers_;
  }

  // ServiceObserverInterface overrides.
  void SendStatusUpdate(int64_t last_checked_time,
                        double progress,
                        UpdateStatus status,
                        const string& new_version,
                        int64_t new_size) override {
    if (status_ != status && (status == UpdateStatus::DOWNLOADING ||
                              status == UpdateStatus::FINALIZING)) {
      // Split the progress bar in two parts for the two stages DOWNLOADING and
      // FINALIZING.
      ReportStatus(base::StringPrintf(
          "ui_print Step %d/2", status == UpdateStatus::DOWNLOADING ? 1 : 2));
      ReportStatus(base::StringPrintf("progress 0.5 0"));
    }
    if (status_ != status || fabs(progress - progress_) > 0.005) {
      ReportStatus(base::StringPrintf("set_progress %.lf", progress));
    }
    progress_ = progress;
    status_ = status;
  }

  void SendPayloadApplicationComplete(ErrorCode error_code) override {
    if (error_code != ErrorCode::kSuccess) {
      ReportStatus(
          base::StringPrintf("ui_print Error applying update: %d (%s)",
                             error_code,
                             utils::ErrorCodeToString(error_code).c_str()));
    }
    error_code_ = error_code;
    brillo::MessageLoop::current()->BreakLoop();
  }

  // Getters.
  UpdateStatus status() { return status_; }
  ErrorCode error_code() { return error_code_; }

 private:
  // Report a status message in the status_stream_, if any. These messages
  // should conform to the specification defined in the Android recovery.
  void ReportStatus(const string& message) {
    if (!status_stream_)
      return;
    string status_line = message + "\n";
    status_stream_->WriteAllBlocking(
        status_line.data(), status_line.size(), nullptr);
  }

  std::set<ServiceObserverInterface*> observers_;
  brillo::StreamPtr status_stream_;

  // The last status and error code reported.
  UpdateStatus status_{UpdateStatus::IDLE};
  ErrorCode error_code_{ErrorCode::kSuccess};
  double progress_{-1.};
};
#endif

void SlotSwitch(){
  LOG(INFO) << " test cb from updat_engine_service";
}


chromeos_update_engine::ApplyPayload::ApplyPayload() { LOG(INFO) << " ApplyPayload()"; }
chromeos_update_engine::ApplyPayload::~ApplyPayload() { LOG(INFO) <<" ~ApplyPayload()"; }

// Apply an update payload directly from the given payload URI.
bool chromeos_update_engine::ApplyPayload::ApplyUpdatePayload(const string& payload,
                        int64_t payload_offset,
                        int64_t payload_size,
                        const vector<string>& headers,
                        int64_t status_fd) {
  printf("\n  Update Engine Service , payload %s \n", payload.c_str());
  printf("\n Update Engine Service , payloadoffset %ld \n",payload_offset);
  printf("\n Update Engine Service , payload size %ld \n",payload_size);
  printf("\n Update Engine Service , status_fd  %ld \n",status_fd);

  LOG(INFO) << "ApplyUpdatePayload  Engine Sideloading set path :";
  LOG(INFO) << "ApplyUpdatePayload  0 ";

#ifdef USE_LE_MODE
//  brillo::BaseMessageLoop loop;
  brillo::FakeMessageLoop loop(nullptr);
  loop.SetAsCurrent();

  // Setup the subprocess handler.
  brillo::AsynchronousSignalHandler handler;
  handler.Init();
  Subprocess subprocess;
  subprocess.Init(&handler);

  SideloadDaemonState sideload_daemon_state(
      brillo::FileStream::FromFileDescriptor(status_fd, true, nullptr));

  // During the sideload we don't access the prefs persisted on disk but instead
  // use a temporary memory storage.
  MemoryPrefs prefs;

  std::unique_ptr<BootControlInterface> boot_control =
      boot_control::CreateBootControl();
  if (!boot_control) {
    LOG(ERROR) << "Error initializing the BootControlInterface.";
    return false;
  }
#ifndef USE_LE_MODE
  std::unique_ptr<HardwareInterface> hardware = hardware::CreateHardware();
  if (!hardware) {
    LOG(ERROR) << "Error initializing the HardwareInterface.";
    return false;
  }
#endif
#ifndef USE_LE_MODE
  UpdateAttempterAndroid update_attempter(
      &sideload_daemon_state, &prefs, boot_control.get(), hardware.get());
#else
  UpdateAttempterAndroid update_attempter(
      &sideload_daemon_state, &prefs, boot_control.get(), NULL);
#endif
  update_attempter.Init();

  TEST_AND_RETURN_FALSE(update_attempter.ApplyPayload(
      payload, payload_offset, payload_size, headers, nullptr));

  loop.Run();
  return sideload_daemon_state.status() == UpdateStatus::UPDATED_NEED_REBOOT;
#endif
}

}  // namespace chromeos_update_engine


int main(int argc, char** argv) {
  DEFINE_string(payload,
                "file:///data/payload.bin",
                "The URI to the update payload to use.");
  DEFINE_int64(
      offset, 0, "The offset in the payload where the CrAU update starts. ");
  DEFINE_int64(size,
               0,
               "The size of the CrAU part of the payload. If 0 is passed, it "
               "will be autodetected.");
  DEFINE_string(headers,
                "",
                "A list of key-value pairs, one element of the list per line.");
  DEFINE_int64(status_fd, -1, "A file descriptor to notify the update status.");

  chromeos_update_engine::Terminator::Init();
  chromeos_update_engine::SetupLogging();
  brillo::FlagHelper::Init(argc, argv, "Update Engine Sideload");

  LOG(INFO) << "Update Engine Sideloading starting";

  // xz-embedded requires to initialize its CRC-32 table once on startup.
  xz_crc32_init();

  // When called from recovery, /data is not accessible, so we need to use
  // /tmp for temporary files.
  chromeos_update_engine::utils::SetRootTempDir(kSideloadRootTempDir);

  vector<string> headers = base::SplitString(
      FLAGS_headers, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
#ifdef USE_LE_MODE
  int64_t update_status_fd = -1;
  for(size_t i = 0; i < headers.size(); ++i) {
        LOG(INFO) << " headers strings split :"<< headers[i];
  }
#endif

#ifndef USE_LE_MODE
  if (!chromeos_update_engine::ApplyUpdatePayload(
          FLAGS_payload, FLAGS_offset, FLAGS_size, headers, FLAGS_status_fd))
    return 1;
#else

    if (argc == 1) {
        telaf_connect_to_flash_access();
        stream_update_service_init();
    } else {
        std::ifstream payload_prop;
        size_t index = 0;
        int64_t payload_offset = 0;
        int64_t payload_size;
        vector<string> payload_header;
        payload_prop.open("/data/stream_update/properties.txt");
        for(std::string metadata; std::getline(payload_prop, metadata); ) {
           LOG(INFO) << "\n" << metadata;
           payload_header.push_back(metadata);
        }
        sp<android::ProcessState> proc(android::ProcessState::self());
        android::ProcessState::self()->startThreadPool();
        std::string::size_type strtype;
        std::string size = payload_header[1].substr(10, 8);
        payload_size = std::stoi( size,&strtype ); // "The size of the CrAU part of the payload. If 0 is passed, it "
                                                  // "will be autodetected."
        LOG(INFO) << " update_engine_client payload size :" << payload_size;
        for(size_t i = 0; i < payload_header.size(); ++i) {
          LOG(INFO) << " update_engine_client headers strings split :" << payload_header[i];
        }
        sp<IStreamUpdateService> stream_update_service = getStreamUpdateService();
        LOG(INFO) << "Client init ";

        sp<UpdateNotifyService> callback(new UpdateNotifyService);
        sp<IBinder> binder(callback.get());
        stream_update_service->registerCallback(binder);

        stream_update_service->applyUpdatePayload(FLAGS_payload, payload_offset, payload_size, payload_header, update_status_fd);
        {
        Mutex::Autolock _l(callback->mNotifyLock);
        LOG(INFO) << "wait for call to be executed ";
        callback->mNotifyCond.wait(callback->mNotifyLock);
        }
        LOG(INFO) << "exit from client";
    }
#endif
  return 0;
}
