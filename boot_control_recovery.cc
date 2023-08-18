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

#include "update_engine/boot_control_recovery.h"

#include <base/bind.h>
#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_util.h>
#include <brillo/make_unique_ptr.h>
#include <brillo/message_loops/message_loop.h>

#include "update_engine/common/utils.h"
#include "update_engine/utils_android.h"
#ifdef USE_LE_MODE
#include <mtdutils/mounts.h>
#include <mtdutils/mtdutils.h>
#include "nad-ab-al.h"
#define MAX_SLOTS (2)
#endif
using std::string;

#ifdef USE_MTD
#define FLASH_ACCESS 1
#endif

#ifndef _UE_SIDELOAD
#error "BootControlRecovery should only be used for update_engine_sideload."
#endif

// When called from update_engine_sideload, we don't attempt to dynamically load
// the right boot_control HAL, instead we use the only HAL statically linked in
// via the PRODUCT_STATIC_BOOT_CONTROL_HAL make variable and access the module
// struct directly.
#ifndef USE_LE_MODE
extern const hw_module_t HAL_MODULE_INFO_SYM;
#endif

namespace chromeos_update_engine {

namespace boot_control {
#ifdef USE_LE_MODE
  static int MAX_NUM_SLOTS = 2;
  int boot_slot, inactive_slot;
  const char* slot_suffix_arr[] = {"_a", "_b", NULL};
#endif
// Factory defined in boot_control.h.
#ifdef USE_LE_MODE
std::unique_ptr<BootControlInterface> CreateBootControl() {
  std::unique_ptr<BootControlRecovery> boot_control(new BootControlRecovery());
  if (!boot_control->Init()) {
    return nullptr;
  }
  return std::move(boot_control);
}
#endif

}  // namespace boot_control

bool BootControlRecovery::Init() {
#ifndef USE_LE_MODE
  const hw_module_t* hw_module;
  int ret;

  // For update_engine_sideload, we simulate the hw_get_module() by accessing it
  // from the current process directly.
  hw_module = &HAL_MODULE_INFO_SYM;
  ret = 0;
  if (!hw_module ||
      strcmp(BOOT_CONTROL_HARDWARE_MODULE_ID, hw_module->id) != 0) {
    ret = -EINVAL;
  }
  if (ret != 0) {
    LOG(ERROR) << "Error loading boot_control HAL implementation.";
    return false;
  }

  module_ = reinterpret_cast<boot_control_module_t*>(
      const_cast<hw_module_t*>(hw_module));
  module_->init(module_);

  LOG(INFO) << "Loaded boot_control HAL "
            << "'" << hw_module->name << "' "
            << "version " << (hw_module->module_api_version >> 8) << "."
            << (hw_module->module_api_version & 0xff) << " "
            << "authored by '" << hw_module->author << "'.";
#endif
  LOG(INFO) << " enable bootcontrol TBD ";
  return true;
}

unsigned int BootControlRecovery::GetNumSlots() const {
#ifndef USE_LE_MODE
  return module_->getNumberSlots(module_);
#else
  LOG(INFO) << " GetNumSlots ";
  return MAX_SLOTS;
#endif
}

BootControlInterface::Slot BootControlRecovery::GetCurrentSlot() const {
#ifndef USE_LE_MODE
  return module_->getCurrentSlot(module_);
#else
  int current_slot = libnadab_get_boot_slot();
  if(current_slot == 0){
    LOG(INFO) << " current MTD slot is _a";
    return 0;
  }
  else if(current_slot == 1){
    LOG(INFO) << " current MTD slot is _b";
    return 1;
  }else{
    LOG(INFO) << " device doesnt support DUAL PARTITION";
    return -1;
  }
#endif
}

bool BootControlRecovery::GetPartitionDevice(const string& partition_name,
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

  const char* suffix = module_->getSuffix(module_, slot);
  LOG(INFO) << "boot_control current slot suffix  " << suffix;
  if (suffix == nullptr) {
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
  LOG(INFO) << "boot_control slot partition_name: " << partition_name;
  chromeos_update_engine::boot_control::boot_slot = libnadab_get_boot_slot();
  if (chromeos_update_engine::boot_control::boot_slot == -1) {
      printf(" libnadab error aborting!\n");
     return false;
  }
  // Set the inactive slot to the non-boot slot (1->0, 0->1)
  chromeos_update_engine::boot_control::inactive_slot = (chromeos_update_engine::boot_control::boot_slot + 1)%2;
  LOG(INFO) << "boot_control current inactive slot  " << chromeos_update_engine::boot_control::inactive_slot;
  char *inactive_mtd_block;
  char inactive_partition[PATH_MAX];
  std::string str;
  LOG(INFO) << "boot_control partition_name:  " << partition_name.c_str();
  if ( (partition_name == "rootfs") || (partition_name == "telaf") || (partition_name == "firmware") || (partition_name == "vm-bootsys") ){
      snprintf(inactive_partition, sizeof(inactive_partition), "%s%s", partition_name.c_str(),
          chromeos_update_engine::boot_control::slot_suffix_arr[chromeos_update_engine::boot_control::inactive_slot]);
      LOG(INFO) << "boot_control partition_name for volumes:  " << inactive_partition;
  } else {
      if(chromeos_update_engine::boot_control::boot_slot == 0)
          snprintf(inactive_partition, sizeof(inactive_partition), "%s%s", partition_name.c_str(),
              chromeos_update_engine::boot_control::slot_suffix_arr[1]);
      else
          snprintf(inactive_partition, sizeof(inactive_partition), "%s", partition_name.c_str());
      LOG(INFO) << "boot_control partition_name for non volumes:  " << inactive_partition;
  }
#ifdef FLASH_ACCESS
  str.assign(inactive_partition);
#else
  LOG(INFO) << " boot_control inactive_partition  " << inactive_partition;
  inactive_mtd_block = BootControlRecovery::getMtdBlock(inactive_partition);
  LOG(INFO) << "boot_control inactive_mtd_block: " << inactive_mtd_block;
  str.assign(inactive_mtd_block);
#endif
  *device = str;
  return true;
}

#ifndef FLASH_ACCESS
char* BootControlRecovery::getMtdBlock(char* rootfs_volume) const {
    int err = mtd_scan_partitions();
    if (err == -1){
        printf("mtd scan partition failed\n");
        return strdup("");
    }
    const MtdPartition* mtd = mtd_find_partition_by_name(rootfs_volume);
    if (mtd == NULL) {
        printf("no mtd partition named \"%s\"\n", rootfs_volume);
        return strdup("");
    }
    char mtd_devname[PATH_MAX];
    snprintf(mtd_devname, sizeof(mtd_devname), "/dev/mtdblock%d", mtd->device_index);
    return strdup(mtd_devname);
}
#endif

bool BootControlRecovery::IsSlotBootable(Slot slot) const {
#ifndef USE_LE_MODE
  int ret = module_->isSlotBootable(module_, slot);
  if (ret < 0) {
    LOG(ERROR) << "Unable to determine if slot " << SlotName(slot)
               << " is bootable: " << strerror(-ret);
    return false;
  }
#endif
  return true;
}

bool BootControlRecovery::MarkSlotUnbootable(Slot slot) {
#ifndef USE_LE_MODE
  int ret = module_->setSlotAsUnbootable(module_, slot);
  if (ret < 0) {
    LOG(ERROR) << "Unable to mark slot " << SlotName(slot)
               << " as bootable: " << strerror(-ret);
    return false;
  }
  return ret == 0;
#else
  LOG(INFO) << "MarkSlotUnbootable  " << SlotName(slot);
  int ret = libnadab_set_unbootable(slot);
  if(ret == 0) {
    return true;
  } else {
    return false;
  }
#endif
}

bool BootControlRecovery::SetActiveBootSlot(Slot slot) {
#ifndef USE_LE_MODE
  int ret = module_->setActiveBootSlot(module_, slot);
  if (ret < 0) {
    LOG(ERROR) << "Unable to set the active slot to slot " << SlotName(slot)
               << ": " << strerror(-ret);
  }
  return ret == 0;
#else
  LOG(INFO) << "SetActiveBootSlot   " << SlotName(slot);
  int ret = libnadab_set_active(slot);
  if(ret == 0) {
    LOG(INFO) << "SetActiveBootSlot  " << SlotName(slot) << " is success";
    return true;
  } else {
    LOG(INFO) << "SetActiveBootSlot  " << SlotName(slot) << " failed";
    return false;
  }
#endif
}

bool BootControlRecovery::MarkBootSuccessfulAsync(
    base::Callback<void(bool)> callback) {
#ifndef USE_LE_MODE
  int ret = module_->markBootSuccessful(module_);
  if (ret < 0) {
    LOG(ERROR) << "Unable to mark boot successful: " << strerror(-ret);
  }
  return brillo::MessageLoop::current()->PostTask(
             FROM_HERE, base::Bind(callback, ret == 0)) !=
         brillo::MessageLoop::kTaskIdNull;
#else
  LOG(INFO) << "MarkBootSuccessfulAsync";
  return true;
#endif
}

}  // namespace chromeos_update_engine
