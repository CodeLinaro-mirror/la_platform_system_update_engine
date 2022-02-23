//
// Copyright (C) 2015 The Android Open Source Project
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

#include "update_engine/boot_control_android.h"

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <brillo/make_unique_ptr.h>
#include <brillo/message_loops/message_loop.h>

#include "update_engine/common/utils.h"
#include "update_engine/utils_android.h"
#ifdef USE_LE_MODE
#include "libabctl.h"

#define MAX_SLOTS (2)
#endif
using std::string;

#ifndef USE_LE_MODE
using android::hardware::Return;
using android::hardware::boot::V1_0::BoolResult;
using android::hardware::boot::V1_0::CommandResult;
using android::hardware::boot::V1_0::IBootControl;
using android::hardware::hidl_string;

namespace {
auto StoreResultCallback(CommandResult* dest) {
  return [dest](const CommandResult& result) { *dest = result; };
}
}  // namespace
#endif
namespace chromeos_update_engine {

namespace boot_control {
#ifdef USE_LE_MODE
  static int MAX_NUM_SLOTS = 2;
#endif
// Factory defined in boot_control.h.
#ifndef USE_LE_MODE
std::unique_ptr<BootControlInterface> CreateBootControl() {
  std::unique_ptr<BootControlAndroid> boot_control(new BootControlAndroid());
  if (!boot_control->Init()) {
    return nullptr;
  }
  return std::move(boot_control);
}
#endif

}  // namespace boot_control

bool BootControlAndroid::Init() {
#ifndef USE_LE_MODE
  module_ = IBootControl::getService();
  if (module_ == nullptr) {
    LOG(ERROR) << "Error getting bootctrl HIDL module.";
    return false;
  }

  LOG(INFO) << "Loaded boot control hidl hal.";
#endif
  return true;
}

unsigned int BootControlAndroid::GetNumSlots() const {
#ifndef USE_LE_MODE
  return module_->getNumberSlots();
#else
  return MAX_SLOTS;
#endif
}

BootControlInterface::Slot BootControlAndroid::GetCurrentSlot() const {
#ifndef USE_LE_MODE
  return module_->getCurrentSlot();
#else
  if(libabctl_getSuccessStatus(0) == 1)
	 return 0;
  else if(libabctl_getSuccessStatus(1) == 1)
	 return 1;
#endif
}

bool BootControlAndroid::GetPartitionDevice(const string& partition_name,
                                            Slot slot,
                                            string* device) const {
  // We can't use fs_mgr to look up |partition_name| because fstab
  // doesn't list every slot partition (it uses the slotselect option
  // to mask the suffix).
  //
  // We can however assume that there's an entry for the /misc mount
  // point and use that to get the device file for the misc
  // partition. This helps us locate the disk that |partition_name|
  // resides on. From there we'll assume that a by-name scheme is used
  // so we can just replace the trailing "misc" by the given
  // |partition_name| and suffix corresponding to |slot|, e.g.
  //
  //   /dev/block/platform/soc.0/7824900.sdhci/by-name/misc ->
  //   /dev/block/platform/soc.0/7824900.sdhci/by-name/boot_a
  //
  // If needed, it's possible to relax the by-name assumption in the
  // future by trawling /sys/block looking for the appropriate sibling
  // of misc and then finding an entry in /dev matching the sysfs
  // entry.

#ifndef USE_LE_MODE
  base::FilePath misc_device;
  if (!utils::DeviceForMountPoint("/misc", &misc_device))
    return false;

  if (!utils::IsSymlink(misc_device.value().c_str())) {
    LOG(ERROR) << "Device file " << misc_device.value() << " for /misc "
               << "is not a symlink.";
    return false;
  }

  string suffix;
  auto store_suffix_cb = [&suffix](hidl_string cb_suffix) {
    suffix = cb_suffix.c_str();
  };
  Return<void> ret = module_->getSuffix(slot, store_suffix_cb);

  if (!ret.isOk()) {
    LOG(ERROR) << "boot_control impl returned no suffix for slot "
               << SlotName(slot);
    return false;
  }

  base::FilePath path = misc_device.DirName().Append(partition_name + suffix);
  if (!base::PathExists(path)) {
    LOG(ERROR) << "Device file " << path.value() << " does not exist.";
    return false;
  }

  *device = path.value();
#endif
  return true;
}

bool BootControlAndroid::IsSlotBootable(Slot slot) const {
#ifndef USE_LE_MODE
  Return<BoolResult> ret = module_->isSlotBootable(slot);
  if (!ret.isOk()) {
    LOG(ERROR) << "Unable to determine if slot " << SlotName(slot)
               << " is bootable: "
               << ret.description();
    return false;
  }
  if (ret == BoolResult::INVALID_SLOT) {
    LOG(ERROR) << "Invalid slot: " << SlotName(slot);
    return false;
  }
  return ret == BoolResult::TRUE;
#endif
  return true;
}

bool BootControlAndroid::MarkSlotUnbootable(Slot slot) {
#ifndef USE_LE_MODE
  CommandResult result;
  auto ret = module_->setSlotAsUnbootable(slot, StoreResultCallback(&result));
  if (!ret.isOk()) {
    LOG(ERROR) << "Unable to call MarkSlotUnbootable for slot "
               << SlotName(slot) << ": "
               << ret.description();
    return false;
  }
  if (!result.success) {
    LOG(ERROR) << "Unable to mark slot " << SlotName(slot)
               << " as unbootable: " << result.errMsg.c_str();
  }
  return result.success;
#else
  int ret = libabctl_setUnbootable(slot);
  if(ret == 0) {
    return true;
  } else {
    return false;
  }
#endif
}

bool BootControlAndroid::SetActiveBootSlot(Slot slot) {
#ifndef USE_LE_MODE
  CommandResult result;
  auto ret = module_->setActiveBootSlot(slot, StoreResultCallback(&result));
  if (!ret.isOk()) {
    LOG(ERROR) << "Unable to call SetActiveBootSlot for slot " << SlotName(slot)
               << ": " << ret.description();
    return false;
  }
  if (!result.success) {
    LOG(ERROR) << "Unable to set the active slot to slot " << SlotName(slot)
               << ": " << result.errMsg.c_str();
  }
  return result.success;
#else
  int ret = libabctl_setActive(slot);
  if(ret == 0) {
    return true;
  } else {
    return false;
  }
#endif
}

bool BootControlAndroid::MarkBootSuccessfulAsync(
    base::Callback<void(bool)> callback) {
#ifndef USE_LE_MODE
  CommandResult result;
  auto ret = module_->markBootSuccessful(StoreResultCallback(&result));
  if (!ret.isOk()) {
    LOG(ERROR) << "Unable to call MarkBootSuccessful: "
               << ret.description();
    return false;
  }
  if (!result.success) {
    LOG(ERROR) << "Unable to mark boot successful: " << result.errMsg.c_str();
  }
  return brillo::MessageLoop::current()->PostTask(
             FROM_HERE, base::Bind(callback, result.success)) !=
         brillo::MessageLoop::kTaskIdNull;
#else
  int ret = libabctl_SetBootSuccess();
  if (ret != 0) {
    LOG(ERROR) << "Unable to call MarkBootSuccessful: ";
    return false;
  }
  return true;
#endif
}

}  // namespace chromeos_update_engine
