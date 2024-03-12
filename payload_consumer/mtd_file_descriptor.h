//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef UPDATE_ENGINE_PAYLOAD_CONSUMER_MTD_FILE_DESCRIPTOR_H_
#define UPDATE_ENGINE_PAYLOAD_CONSUMER_MTD_FILE_DESCRIPTOR_H_

// This module defines file descriptors that deal with NAND media. We are
// concerned with raw NAND access (as MTD device), and through UBI layer.

#define MAX_VOL_COUNT    32
#define MAX_NAME_LEN     32

#include "update_engine/payload_consumer/file_descriptor.h"

#if USE_MTD
#if !USE_TELAF
#include "nad-mtd-al.h"
#include "nad-ubi-al.h"
#endif
#define MRC_UNUSED(arg) (arg = arg)
#define SYS_CLASS_UBI_DEV_PATH       "/sys/class/ubi/ubi%d/"
#define SYS_CLASS_UBI_VOL_COUNT_PATH "/sys/class/ubi/ubi%d/volumes_count"
#define SYS_CLASS_UBI_VOL_NAME_PATH  "/sys/class/ubi/ubi%d_%d/name"
#define UBI_DEVICE_PATH              "/dev/ubi%d_%d"
#endif


namespace chromeos_update_engine {
// A class defining the file descriptor API for raw MTD device. This file
// descriptor supports either random read, or sequential write but not both at
// once.
class MtdFileDescriptor : public EintrSafeFileDescriptor {
 public:
  MtdFileDescriptor();

  static bool IsMtd(const char* path);
  static int GetMtdno (const char* parti_name, char* mtdno);

  bool Open(const char* path, int flags, mode_t mode) override;
  bool Open(const char* path, int flags) override;
  ssize_t Read(void* buf, size_t count) override;
  ssize_t Write(const void* buf, size_t count) override;
  off64_t Seek(off64_t offset, int whence) override;
  uint64_t BlockDevSize() override { return 0; }
#if USE_MTD
#if !USE_TELAF
  nad_mtd_hndl_t *NadMtdHandler(int fd, const char *dev_node_name);
#endif
#endif
  bool BlkIoctl(int request,
                uint64_t start,
                uint64_t length,
                int* result) override {
    return false;
  }
  bool Close() override;

 private:
#if USE_MTD
#if !USE_TELAF
  std::unique_ptr<nad_mtd_hndl_t, decltype(&nad_mtd_close)> nad_mtd_ctx_;
#endif
#else
  std::unique_ptr<MtdReadContext, decltype(&mtd_read_close)> read_ctx_;
  std::unique_ptr<MtdWriteContext, decltype(&mtd_write_close)> write_ctx_;
#endif
  uint64_t nr_written_;
  uint32_t total_blocks_number_;
  uint32_t bad_blocks_number_;
  uint32_t erase_size_;
  uint32_t write_size_;
};

struct UbiVolumeInfo {
  // Number of eraseblocks.
  uint64_t reserved_ebs;
  // Size of each eraseblock.
  uint64_t eraseblock_size;
};

// A file descriptor to update a UBI volume, similar to MtdFileDescriptor.
// Once the file descriptor is opened for write, the volume is marked as being
// updated. The volume will not be usable until an update is completed. See
// UBI_IOCVOLUP ioctl operation.
class UbiFileDescriptor : public EintrSafeFileDescriptor {
 public:
  UbiFileDescriptor();
  // Perform some queries about |path| to see if it is a UBI volume.
  static bool IsUbi(const char* path);
#if USE_MTD
  static int GetVolIdByName(const char *volname, int *ubi_id, int *vol_id);
  static int GetVolName(int ubi_id, int vol_id, char *volname);
  static int GetVolCount(int volcount[]);
  static int GetStringFromFile(const char *path, char *data);
  static int GetValueFromFile(const char *path);
#endif

  bool Open(const char* path, int flags, mode_t mode) override;
  bool Open(const char* path, int flags) override;
  ssize_t Read(void* buf, size_t count) override;
  ssize_t Write(const void* buf, size_t count) override;
  off64_t Seek(off64_t offset, int whence) override;
  uint64_t BlockDevSize() override { return 0; }
#if !USE_TELAF
  nad_ubi_hndl_t *NadUbiHandler(int fd, const char *dev_node_name, int read_only);
#endif
  bool BlkIoctl(int request,
                uint64_t start,
                uint64_t length,
                int* result) override {
    return false;
  }
  bool Close() override;

 private:
  enum Mode {
    kReadOnly,
    kWriteOnly
  };

  uint64_t usable_eb_blocks_;
  uint64_t eraseblock_size_;
  uint64_t volume_size_;
  uint32_t leb_number_;
  uint32_t free_leb_number_;
  uint64_t nr_written_;
#if !USE_TELAF
  std::unique_ptr<nad_ubi_hndl_t, decltype(&nad_ubi_close)> nad_ubi_ctx_;
#endif

  Mode mode_;
};

}  // namespace chromeos_update_engine

#endif  // UPDATE_ENGINE_PAYLOAD_CONSUMER_MTD_FILE_DESCRIPTOR_H_
