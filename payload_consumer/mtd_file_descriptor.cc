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

#include "update_engine/payload_consumer/mtd_file_descriptor.h"

#include <fcntl.h>
#include <mtd/ubi-user.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include <base/files/file_path.h>
#include <base/strings/string_number_conversions.h>
#include <base/strings/string_util.h>
#include <base/strings/stringprintf.h>

#include "update_engine/common/subprocess.h"
#include "update_engine/common/utils.h"

#if USE_TELAF
extern "C" {
#include "telaf-flash-access.h"
}
#endif

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

using std::string;
using std::vector;

namespace {

static const char kSysfsClassUbi[] = "/sys/class/ubi/";
static const char kUsableEbSize[] = "/usable_eb_size";
static const char kReservedEbs[] = "/reserved_ebs";

using chromeos_update_engine::UbiVolumeInfo;
using chromeos_update_engine::utils::ReadFile;

// Return a UbiVolumeInfo pointer if |path| is a UBI volume. Otherwise, return
// a null unique pointer.
std::unique_ptr<UbiVolumeInfo> GetUbiVolumeInfo(const string& path) {
  base::FilePath device_node(path);
  base::FilePath ubi_name(device_node.BaseName());

  string sysfs_node(kSysfsClassUbi);
  sysfs_node.append(ubi_name.MaybeAsASCII());

  std::unique_ptr<UbiVolumeInfo> ret;

  // Obtain volume info from sysfs.
  string s_reserved_ebs;
  if (!ReadFile(sysfs_node + kReservedEbs, &s_reserved_ebs)) {
    LOG(ERROR) << "Cannot read " << sysfs_node + kReservedEbs;
    return ret;
  }
  string s_eb_size;
  if (!ReadFile(sysfs_node + kUsableEbSize, &s_eb_size)) {
    LOG(ERROR) << "Cannot read " << sysfs_node + kUsableEbSize;
    return ret;
  }

  base::TrimWhitespaceASCII(s_reserved_ebs,
                            base::TRIM_TRAILING,
                            &s_reserved_ebs);
  base::TrimWhitespaceASCII(s_eb_size, base::TRIM_TRAILING, &s_eb_size);

  uint64_t reserved_ebs, eb_size;
  if (!base::StringToUint64(s_reserved_ebs, &reserved_ebs)) {
    LOG(ERROR) << "Cannot parse reserved_ebs: " << s_reserved_ebs;
    return ret;
  }
  if (!base::StringToUint64(s_eb_size, &eb_size)) {
    LOG(ERROR) << "Cannot parse usable_eb_size: " << s_eb_size;
    return ret;
  }

#if !USE_MTD
  ret.reset(new UbiVolumeInfo);
#endif
  ret->reserved_ebs = reserved_ebs;
  ret->eraseblock_size = eb_size;
  return ret;
}

}  // namespace

namespace chromeos_update_engine {

MtdFileDescriptor::MtdFileDescriptor()
#if USE_MTD
#if USE_TELAF
    {}
#else
    : nad_mtd_ctx_(nullptr, &nad_mtd_close) {}
#endif
#else
    : read_ctx_(nullptr, &mtd_read_close),
      write_ctx_(nullptr, &mtd_write_close) {}
#endif

#if USE_MTD
UbiFileDescriptor::UbiFileDescriptor()
#if USE_TELAF
    {}
#else
    : nad_ubi_ctx_(nullptr, &nad_ubi_close) {}
#endif
#endif

bool MtdFileDescriptor::IsMtd(const char* path) {
#if !USE_MTD
  uint64_t size;
  return mtd_node_info(path, &size, nullptr, nullptr) == 0;
#else
  char mtd_no[32] = {"/dev/"};
  int ret = MtdFileDescriptor::GetMtdno(path,(mtd_no+5));
  if (ret == -1){
    return false;
  }
  return true;
#endif
}

#if USE_MTD && !USE_TELAF
nad_mtd_hndl_t *MtdFileDescriptor::NadMtdHandler(int fd, const char *dev_node_name) {
  nad_mtd_hndl_t *mtd_hndl;
  int ret = -1;
  LOG(INFO) << " MtdFileDescriptor::NadMtdHandler 1.1 mtd node name " << dev_node_name;
  mtd_hndl = (nad_mtd_hndl_t *)malloc(sizeof(nad_mtd_hndl_t));
  if (mtd_hndl == NULL) {
      LOG(ERROR) << " MtdFileDescriptor::NadMtdHandler malloc failed " << ret;
      ret = -1;
  }
  ret = nad_mtd_open(dev_node_name, mtd_hndl);
  if(0 != ret)
  {
      LOG(ERROR) << " MtdFileDescriptor::NadMtdHandler open nad_mtd_open failed, ret " << ret;
      ret = -1;
  }
  return mtd_hndl;
}
#endif

bool MtdFileDescriptor::Open(const char* path, int flags, mode_t mode) {
  // This File Descriptor does not support read and write.
  TEST_AND_RETURN_FALSE((flags & O_ACCMODE) != O_RDWR);
  // But we need to open the underlying file descriptor in O_RDWR mode because
  // during write, we need to read back to verify the write actually sticks or
  // we have to skip the block. That job is done by mtdutils library.
  if ((flags & O_ACCMODE) == O_WRONLY) {
    flags &= ~O_ACCMODE;
    flags |= O_RDWR;
  }
#if !USE_MTD
  TEST_AND_RETURN_FALSE(
      EintrSafeFileDescriptor::Open(path, flags | O_CLOEXEC, mode));
#endif

  int ret = -1;
  if ((flags & O_ACCMODE) == O_RDWR) {
#if !USE_MTD
    write_ctx_.reset(mtd_write_descriptor(fd_, path));
#else
#if USE_TELAF
    //telaf_connect_to_flash_access();
    ret = telaf_mtd_open (path);
    if (ret != 0){
       LOG(ERROR) << " MtdFileDescriptor open failed ";
       return false;
    }
#else
    LOG(INFO) << " MtdFileDescriptor::Open 1.4 write ";
    nad_mtd_hndl_t *hndl = MtdFileDescriptor::NadMtdHandler(fd_, path);
    if (hndl == NULL) {
        LOG(INFO) << " MtdFileDescriptor::Open 1.6 ";
        Close();
        return false;
    }
    nad_mtd_ctx_.reset(hndl);
#endif
#endif
    nr_written_ = 0;

    total_blocks_number_ = 0;
    bad_blocks_number_ = 0;
    erase_size_ = 0;
    write_size_ = 0;
  } else {
#if !USE_MTD
    read_ctx_.reset(mtd_read_descriptor(fd_, path));
#endif
  }

#if !USE_MTD
  if (!read_ctx_ && !write_ctx_) {
    Close();
    return false;
  }
  return true;
#else
#if USE_TELAF
  mtd_info_t *mtd_info;
  mtd_info = (mtd_info_t *) malloc(sizeof(mtd_info_t));
  if( mtd_info == NULL ) {
    printf("memory allocationed failed \n");
    return false;
  }


  ret = telaf_mtd_information(mtd_info);
  if (ret != 0){
    LOG(ERROR) << " telaf_mtd_information failed ";
    free(mtd_info);
    return false;
  }
  total_blocks_number_ = mtd_info->blk_num;
  bad_blocks_number_ = mtd_info->bad_blk_num;
  erase_size_ = mtd_info->erase_size;
  write_size_ = mtd_info->write_size;

  for (int blk_num = 0 ; blk_num < total_blocks_number_ ; blk_num++) {
    LOG(INFO) << " MtdFileDescriptor::Open erase block: "<< blk_num;
    ret = telaf_mtd_erase_block (blk_num);
    if( ret != 0) {
      LOG(ERROR) << " MtdFileDescriptor::Open erase failed ";
      free(mtd_info);
      return false;
    }
  }
  return true;
#else
  nad_mtd_hndl_t *hndl1 = nad_mtd_ctx_.get();
  if (hndl1 == NULL) {
    LOG(ERROR) << " mtd hndl context error MtdFileDescriptor::Open ";
    Close();
    return false;
  }
  total_blocks_number_ = hndl1->info.size / hndl1->info.erasesize;
  erase_size_ = hndl1->info.erasesize;
  write_size_ = hndl1->info.writesize;

  for (int blk_num = 0 ; blk_num < total_blocks_number_ ; blk_num++) {
    LOG(INFO) << " MtdFileDescriptor::Open erase block: "<< blk_num;
    ret = nad_mtd_erase_block(hndl1, blk_num);
    if( ret != 0) {
      LOG(ERROR) << " MtdFileDescriptor::Open erase failed ";
      return false;
    }
  }

  return true;
#endif
#endif
}

bool MtdFileDescriptor::Open(const char* path, int flags) {
  mode_t cur = umask(022);
  umask(cur);
  return Open(path, flags, 0777 & ~cur);
}

ssize_t MtdFileDescriptor::Read(void* buf, size_t count) {
#if !USE_MTD
  CHECK(read_ctx_);
  return mtd_read_data(read_ctx_.get(), static_cast<char*>(buf), count);
#else
#if !USE_TELAF
  CHECK(nad_mtd_ctx_);
  LOG(INFO) << "MtdFileDescriptor::Read 1.8 ";
#endif
  return -1; // read is not performed here, its done in libbrillo file_stream.cc
#endif
}

ssize_t MtdFileDescriptor::Write(const void* buf, size_t count) {
#if !USE_MTD
  CHECK(write_ctx_);
  ssize_t result = mtd_write_data(write_ctx_.get(),
                                  static_cast<const char*>(buf),
                                  count);
  if (result > 0) {
    nr_written_ += result;
  }
  return result;
#else
#if USE_TELAF
  ssize_t ret = -1;
  LOG(INFO) << " MtdFileDescriptor::Write mtd count:" << count;
  // assume count is always multiple of erase_size_ for block based flash apis
  // for last write operation count <= erase_size_ depending on image bondary
  unsigned char* source = reinterpret_cast<unsigned char*>(const_cast<void*>(buf));
  int pages_written = nr_written_/erase_size_;
  if(count >= erase_size_) {
    int iter = count/erase_size_;
    unsigned char* source = reinterpret_cast<unsigned char*>(const_cast<void*>(buf));
    unsigned char dest[erase_size_];
    int pages_written = nr_written_/erase_size_;
    for (int i=0 ; i < iter ; i++) {
      memset(dest, 0, sizeof(dest));
      memcpy(dest, source + i*erase_size_, erase_size_);
      ret = telaf_mtd_write_block (dest ,  pages_written + i, erase_size_);
      if (ret != 0) {
        LOG(ERROR) << "MtdFileDescriptor::Write Failed";
        return -1;
      }
    }
  } else {
    ret = telaf_mtd_write_block (reinterpret_cast<unsigned char*>(const_cast<void*>(buf)) , pages_written, count);
    if (ret != 0) {
      LOG(ERROR) << "MtdFileDescriptor::Write Failed";
      return -1;
    }
  }
  nr_written_ += count;
  return count;
#else
  nad_mtd_hndl_t *hndl1 = nad_mtd_ctx_.get();
  if(nullptr == hndl1){
      LOG(ERROR) << "MtdFileDescriptor::Write Failed, nad_mtd_ctx_.get() returned null ";
      return -1;
  }
  int iter = ((count > nr_written_) ? (count - nr_written_) : (count))/hndl1->info.writesize;
  ssize_t ret = -1;
  LOG(INFO) << " MtdFileDescriptor::Write mtd count:" << count;
  // assume count is always multiple of erase_size_ for block based flash apis
  unsigned char* source = reinterpret_cast<unsigned char*>(const_cast<void*>(buf));

  unsigned char dest[hndl1->info.writesize];
  memset(dest, 0, sizeof(dest));
  int pages_written = nr_written_/hndl1->info.writesize;
  for (int i = 1 ; i <= iter ; i++) {
    memset(dest, 0, sizeof(dest));
    memcpy(dest, source + (i-1)*(hndl1->info.writesize), hndl1->info.writesize);
    ret = nad_mtd_write_page(hndl1, dest, pages_written + i-1, hndl1->info.writesize);
    if(0 != ret) {
      LOG(ERROR) << "MtdFileDescriptor::Write Failed";
      return -1;
    } else {
      LOG(INFO) << "MtdFileDescriptor::Write success ";
    }
  }

  nr_written_ += count;
  return count;
#endif
#endif
}

off64_t MtdFileDescriptor::Seek(off64_t offset, int whence) {
#if USE_MTD
    LOG(INFO) << "MtdFileDescriptor::Seek  write_ctx_ return nr_written_ " << nr_written_;
    return nr_written_;
#else
  if (write_ctx_) {
    return nr_written_;
  }
#endif
  return EintrSafeFileDescriptor::Seek(offset, whence);
}

bool MtdFileDescriptor::Close() {
#if !USE_MTD
    read_ctx_.reset();
    write_ctx_.reset();
  return EintrSafeFileDescriptor::Close();
#else
#if USE_TELAF
  telaf_mtd_close();
#else
  LOG(INFO) << "MtdFileDescriptor::Close ";
  nad_mtd_ctx_.reset();
#endif
  return true;
#endif
}

#if !USE_TELAF
nad_ubi_hndl_t *UbiFileDescriptor::NadUbiHandler(int fd, const char *dev_node_name, int read_only) {
  nad_ubi_hndl_t *ubi_hndl;
  int ret = -1;
  LOG(INFO) << " UbiFileDescriptor::NadUbiHandler ubi dev_node_name: " << dev_node_name;
  ubi_hndl = (nad_ubi_hndl_t *)malloc(sizeof(nad_ubi_hndl_t));
  if (ubi_hndl == NULL)
  {
      LOG(ERROR) << " UbiFileDescriptor::NadUbiHandler malloc failed " << ret;
      ret = -1;
      return NULL;
  }
  ret = nad_ubi_open(dev_node_name, ubi_hndl, read_only);
  if(0 != ret)
  {
      LOG(ERROR) << " UbiFileDescriptor::NadUbiHandler open nad_ubi_open failed, ret " << ret;
      ret = -1;
  }
  fd_ = ubi_hndl->fd;
  ret = nad_ubi_ioctl(ubi_hndl, ubi_hndl->vol_info.data_bytes);
  if(0 != ret)
  {
      LOG(ERROR) << " UbiFileDescriptor::NadUbiHandler open nad_ubi_open failed, ret " << ret;
      ret = -1;
  }
 return ubi_hndl;
}
#endif

bool UbiFileDescriptor::IsUbi(const char* path) {
  base::FilePath device_node(path);
  base::FilePath ubi_name(device_node.BaseName());
  TEST_AND_RETURN_FALSE(base::StartsWith(ubi_name.MaybeAsASCII(), "ubi",
                                         base::CompareCase::SENSITIVE));

  return static_cast<bool>(GetUbiVolumeInfo(path));
}

bool UbiFileDescriptor::Open(const char* path, int flags, mode_t mode) {
  LOG(INFO) << "UbiFileDescriptor::Open 1.2 "<< path;
#if !USE_MTD
  std::unique_ptr<UbiVolumeInfo> info = GetUbiVolumeInfo(path);
  if (!info) {
    return false;
  }
#endif

  // This File Descriptor does not support read and write.
  TEST_AND_RETURN_FALSE((flags & O_ACCMODE) != O_RDWR);
#if !USE_MTD
  TEST_AND_RETURN_FALSE(
      EintrSafeFileDescriptor::Open(path, flags | O_CLOEXEC, mode));
#else
#if USE_TELAF
  /* read mode second arg as 1, for write mode set it as 0 */
  int ret = telaf_ubi_open (path, 0);
  if (ret != 0){
    LOG(ERROR) << " UbiFileDescriptor open failed ";
    return false;
  }
  LOG(INFO) << "UbiFileDescriptor::Open ";
  ret = telaf_ubi_info( &free_leb_number_, &leb_number_, &volume_size_);
  if (ret != 0){
    LOG(ERROR) << " UbiFileDescriptor open failed ";
    return false;
  }

  ret = telaf_ubi_ioctl(volume_size_);
  if (ret != 0){
    LOG(ERROR) << " UbiFileDescriptor ioctl ";
    return false;
  }
  LOG(INFO) << " UbiFileDescriptor ioctl ok ";
#else
  nad_ubi_hndl_t *hndl;
#endif
#endif

#if !USE_MTD
  usable_eb_blocks_ = info->reserved_ebs;
  eraseblock_size_ = info->eraseblock_size;
  volume_size_ = usable_eb_blocks_ * eraseblock_size_;
#endif

  if ((flags & O_ACCMODE) == O_WRONLY) {
    // It's best to use volume update ioctl so that UBI layer will mark the
    // volume as being updated, and only clear that mark if the update is
    // successful. We will need to pad to the whole volume size at close.
#if !USE_MTD
    uint64_t vsize = volume_size_;
    if (ioctl(fd_, UBI_IOCVOLUP, &vsize) != 0) {
      PLOG(ERROR) << "Cannot issue volume update ioctl";
      EintrSafeFileDescriptor::Close();
      return false;
    }
#else
  LOG(INFO) << " UbiFileDescriptor:: ioctl is handled in nad_ubi_open skipped here ";
#endif
    mode_ = kWriteOnly;
    nr_written_ = 0;
#if USE_MTD && !USE_TELAF
    hndl = UbiFileDescriptor::NadUbiHandler(fd_, path, 0);
    if(hndl == NULL) {
      LOG(ERROR) << " NadUbiHandler handler init failed ";
      return false;
    }
    nad_ubi_ctx_.reset(hndl);
    LOG(INFO) << " UbiFileDescriptor::Open write only mode, leb size :" << hndl->vol_info.leb_size;
#endif
  } else {
#if USE_MTD && !USE_TELAF
    hndl = UbiFileDescriptor::NadUbiHandler(fd_, path, 1);
    if(hndl == NULL) {
      LOG(ERROR) << " NadUbiHandler handler init failed ";
      return false;
    }
    nad_ubi_ctx_.reset(hndl);
    LOG(INFO) << " UbiFileDescriptor::Open read only mode, leb size :" << hndl->vol_info.leb_size;
#endif
    mode_ = kReadOnly;
  }

#if USE_MTD && !USE_TELAF
  nad_ubi_hndl_t *hndl1 = nad_ubi_ctx_.get();
  if (!nad_ubi_ctx_) {
    Close();
    return false;
  }
#endif
  return true;
}

bool UbiFileDescriptor::Open(const char* path, int flags) {
  mode_t cur = umask(022);
  umask(cur);
  return Open(path, flags, 0777 & ~cur);
}

ssize_t UbiFileDescriptor::Read(void* buf, size_t count) {
  CHECK(mode_ == kReadOnly);
  return EintrSafeFileDescriptor::Read(buf, count);
}

ssize_t UbiFileDescriptor::Write(const void* buf, size_t count) {
  CHECK(mode_ == kWriteOnly);
  LOG(INFO) << "UbiFileDescriptor::Write " << count;
  ssize_t nr_chunk = 0;
#if !USE_MTD
  nr_chunk = EintrSafeFileDescriptor::Write(buf, count);
#else
#if USE_TELAF
  int iter = count/16384;
  char dest[16384];
  char *source = reinterpret_cast<char*>(const_cast<void*>(buf));
  for (int i=0 ; i < iter ; i++) {
    LOG(INFO) << "UbiFileDescriptor::Write iter: " << i;
    memset(dest, 0, sizeof(dest));
    memcpy(dest, source + i*16384, 16384);
    int ret =  telaf_ubi_write (dest, 16384);
    if(ret != 0) {
      LOG(ERROR) << "UbiFileDescriptor::Write  error";
      return -1;
    } else {
      LOG(INFO) << "UbiFileDescriptor::Write  ok";
    }
  }
  if(count%16384){
    // in this case we have some more data present , we need to write one more cycle for extra data
    LOG(INFO) << "UbiFileDescriptor::Write  count is not multiple of 16384 addition write cycle added " << iter;
    memset(dest, 0, sizeof(dest));
    memcpy(dest, source + iter*16384, 16384);
    int ret =  telaf_ubi_write (dest, count - iter*16384);
    if(ret != 0) {
      LOG(ERROR) << "UbiFileDescriptor::Write  error";
      return -1;
    } else {
      LOG(INFO) << "UbiFileDescriptor::Write  ok";
    }
  } else {
    LOG(ERROR) << "UbiFileDescriptor::Write  count is multiple of 16384 ";
  }
  nr_chunk = count;
#else
  nad_ubi_hndl_t *hndl1 = nad_ubi_ctx_.get();
  LOG(INFO) << " UbiFileDescriptor:: get legsize before write operation " << hndl1->vol_info.leb_size;
  int ret = nad_ubi_write (hndl1, reinterpret_cast<char*>(const_cast<void*>(buf)), count);
  if(ret == 0) {
    nr_chunk = count;
    LOG(INFO) << "UbiFileDescriptor::Write  ok";
  }else {
    LOG(ERROR) << "UbiFileDescriptor::Write  error";
  }
#endif
#endif
  if (nr_chunk >= 0) {
    nr_written_ += nr_chunk;
  }
  LOG(INFO) << "UbiFileDescriptor::Write " << nr_written_;
  return nr_chunk;
}

off64_t UbiFileDescriptor::Seek(off64_t offset, int whence) {
  if (mode_ == kWriteOnly) {
    // Ignore seek in write mode.
    return nr_written_;
  }
  //LOG(INFO) << "UbiFileDescriptor::Seek ";
  return EintrSafeFileDescriptor::Seek(offset, whence);
}

bool UbiFileDescriptor::Close() {
  bool pad_ok = true;
#if !USE_MTD
  if (IsOpen() && mode_ == kWriteOnly) {
    while (nr_written_ < volume_size_) {
      // We have written less than the whole volume. In order for us to clear
      // the update marker, we need to fill the rest. It is recommended to fill
      // UBI writes with 0xFF.
      if (to_write > sizeof(buf)) {
        to_write = sizeof(buf);
      }
      ssize_t nr_chunk;
      nr_chunk = EintrSafeFileDescriptor::Write(buf, to_write);
      if (nr_chunk < 0) {
        //LOG(ERROR) << "Cannot 0xFF-pad before closing.";
        // There is an error, but we can't really do any meaningful thing here.
        pad_ok = false;
        break;
      }
      nr_written_ += nr_chunk;
    }
  }
  return EintrSafeFileDescriptor::Close() && pad_ok;
#else
  if (mode_ == kWriteOnly) {
#if USE_TELAF
    char dest[16384];
    memset(dest, 0xFF, sizeof(dest));
    uint64_t to_write = volume_size_ - nr_written_;
    if( to_write%16384 != 0)
      LOG(INFO) << "UbiFileDescriptor::Close need handling writing for left over bytes: ";
    int iter = to_write/16384;
    LOG(INFO) << "UbiFileDescriptor::Close need : " << to_write << " more bytes to write";
    ssize_t pad_bytes = 0;
    for (int i = 0; i < iter; i++){
      int ret = telaf_ubi_write (dest, 16384);
      if(ret == 0) {
        LOG(INFO) << "UbiFileDescriptor::Close Write pad bytes ok";
        pad_bytes += 16384;
      } else {
        LOG(ERROR) << "UbiFileDescriptor::Close Write pad bytes error";
      }
    }
    if ((nr_written_ + pad_bytes) == volume_size_)
        LOG(INFO) << "UbiFileDescriptor::Close all bytes written, close can be called";
    else {
        LOG(ERROR) << "UbiFileDescriptor::Close pad bytes: " << pad_bytes;
        LOG(ERROR) << "UbiFileDescriptor::Close nr_written_: " << nr_written_;
        LOG(ERROR) << "UbiFileDescriptor::Close volume_size_: " << volume_size_;
        nr_written_ += pad_bytes;
        to_write = volume_size_ - nr_written_;
        int ret = telaf_ubi_write (dest, 16384);
        if(ret == 0) {
          LOG(INFO) << "UbiFileDescriptor::Close Write pad bytes ok";
          pad_bytes += 16384;
        }else {
          LOG(ERROR) << "UbiFileDescriptor::Close Write pad bytes error";
        }
    }
  telaf_ubi_close();
  LOG(INFO) << "UbiFileDescriptor::Close ,  pad_ok" << pad_ok;
#else
  nad_ubi_hndl_t *hndl1 = nad_ubi_ctx_.get();
  LOG(INFO) << "UbiFileDescriptor::Close mode_ " << mode_;
  LOG(INFO) << "UbiFileDescriptor::Close kWriteOnly " << kWriteOnly;
  volume_size_ = hndl1->vol_info.data_bytes;
  if (mode_ == kWriteOnly) {
    char buf[1024];
    uint64_t to_write = volume_size_ - nr_written_;
    memset(buf, 0xFF, sizeof(buf));
    while (nr_written_ < volume_size_) {
      // We have written less than the whole volume. In order for us to clear
      // the update marker, we need to fill the rest. It is recommended to fill
      // UBI writes with 0xFF.
      if (to_write > sizeof(buf)) {
        to_write = sizeof(buf);
      }
      ssize_t nr_chunk;
      int ret = nad_ubi_write(hndl1, buf, to_write);
      if(ret == 0) {
        nr_chunk = to_write;
        LOG(INFO) << "UbiFileDescriptor::Close Write pad bytes ok" << nr_chunk ;
      } else {
        LOG(ERROR) << "UbiFileDescriptor::Close Write pad bytes error" << nr_chunk;
      }
      if (nr_chunk < 0) {
        //LOG(ERROR) << "Cannot 0xFF-pad before closing.";
        // There is an error, but we can't really do any meaningful thing here.
        pad_ok = false;
        break;
      }
      nr_written_ += nr_chunk;
    }
  }
  LOG(INFO) << " UbiFileDescriptor:: Close ";
  nad_ubi_close(hndl1);
#endif
  }
#endif
  return pad_ok;
}

#if USE_MTD
int MtdFileDescriptor::GetMtdno (const char* parti_name, char* mtdno)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t nread;
    char* val;
    char* saveptr = NULL;
    char part_no[10]={0};
    int part_found  = 0;

    if (NULL == parti_name || NULL == mtdno)
    {
      return -1;
    }

    /* Open text file for reading with position at beginning of file */
    fp = fopen("/proc/mtd", "r");

    if (fp == NULL) {
      //LOG(ERROR) << "Failed to open /proc/mtd";
      return -1;
    }

    /* Recursively read each line till we reach the end of file */
    while ((nread = getline(&line, &len, fp)) != -1) {

        /* Tokenize the string to parse each line*/
        val = strtok_r(line, " :\"", &saveptr);
        memset(part_no, 0, sizeof(part_no));

        if (val!=NULL)
            strlcpy(part_no, val, strlen(val)+1);

        while (val!=NULL){
          if (!strncmp(val,parti_name,strlen(parti_name))) {
            strlcpy(mtdno,part_no,strlen(part_no)+1);
            part_found = 1;
            goto out;
          }
          val=strtok_r(NULL, " :\"", &saveptr);
        }
      }
      out:
      {
        free(line);
        fclose(fp);
      }
    if(part_found == 0)
    {
        LOG(ERROR) << "Failed to get partition num for " << parti_name;
        return -1;
    }
    return 0;
}

int UbiFileDescriptor::GetVolIdByName(const char *volname, int *ubi_id, int *vol_id)
{
    int i, j, ret;
    int ubinode, volcount[MAX_VOL_COUNT];

    if (volname == NULL) return -1;

    ubinode = UbiFileDescriptor::GetVolCount(volcount);

    for (i = 0; i < ubinode; i++) {
        for (j = 0; j < volcount[i]; j++) {
            char name[MAX_NAME_LEN];
            memset(name, 0, sizeof(name));
            ret = UbiFileDescriptor::GetVolName(i, j, name);
            if (ret != 0) continue;
            if (strcmp(name, volname) == 0) {
                *ubi_id = i;
                *vol_id = j;
                return 0;
            }
        }
    }
    LOG(WARNING) << " no ubi volume id found by name " << volname;
    return -1;
}

int UbiFileDescriptor::GetVolName(int ubi_id, int vol_id, char *volname)
{
    int ret;
    char path[64];

    if (ubi_id < 0 || vol_id < 0 || volname == NULL) return -1;

    memset(path, 0, sizeof(path));
    snprintf(path, sizeof(path)-1, SYS_CLASS_UBI_VOL_NAME_PATH, ubi_id, vol_id);
    ret = UbiFileDescriptor::GetStringFromFile(path, volname);
    if (ret < 0) return -1;

    return 0;
}

int UbiFileDescriptor::GetVolCount(int volcount[])
{
    int ubinode = 0;

    do {
        char path[64];
        int value;
        memset(path, 0, sizeof(path));
        snprintf(path, sizeof(path)-1, SYS_CLASS_UBI_VOL_COUNT_PATH, ubinode);
        value = UbiFileDescriptor::GetValueFromFile(path);
        if (value != -1) {
            volcount[ubinode++] = value;
        }
        else
             break;
    } while (1);

    return ubinode;
}


int UbiFileDescriptor::GetStringFromFile(const char *path, char *data)
{
    int rc;
    FILE *fp;

    if (path == NULL || data == NULL)
            return -1;
    fp = fopen(path, "r");
    if (fp == NULL) return -1;
    rc = fscanf(fp, "%s\n", data);
    MRC_UNUSED(rc);
    fclose(fp);
    return 0;
}

int UbiFileDescriptor::GetValueFromFile(const char *path)
{
    int value = 0, rc;
    FILE *fp;

    if (path == NULL) return -1;
    fp = fopen(path, "r");
    if (fp == NULL) return -1;
    rc = fscanf(fp, "%d\n", &value);
    MRC_UNUSED(rc);
    fclose(fp);
    return value;
}
#endif

}  // namespace chromeos_update_engine
