/*
 * Copyright (C) 2026 Yağız Zengin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LIB_OPENPART__INTERNAL_H
#define LIB_OPENPART__INTERNAL_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <uuid.h>

#define EROFS_SUPER_OFFSET 1024
#define QUICK_GET_CONTROLS(op, ret)                                                                                                   \
  do {                                                                                                                                \
    if (!op) {                                                                                                                        \
      errno = EINVAL;                                                                                                                 \
      return ret;                                                                                                                     \
    }                                                                                                                                 \
    if (!op->info_loaded) {                                                                                                           \
      if (load_info(op) < 0) {                                                                                                        \
        op->err = errno;                                                                                                              \
        return ret;                                                                                                                   \
      }                                                                                                                               \
      op->info_loaded = 1;                                                                                                            \
    }                                                                                                                                 \
  } while (0)

__BEGIN_DECLS

struct __attribute__((packed)) openpart {
  char openpart_magic[8];
  uint32_t fd;
  uint32_t flags;
  uint32_t err;
  uint64_t size;
  uint64_t sector_size;
  uint32_t info_loaded;
  char uuid[37];
  char label[256];
  char fstype[32];
};

// FROM: https://android.googlesource.com/platform/external/erofs-utils/+/refs/heads/main/include/erofs_fs.h
/* erofs on-disk super block (currently 128 bytes) */
struct erofs_super_block {
  __le32 magic;    /* file system magic number */
  __le32 checksum; /* crc32c(super_block) */
  __le32 feature_compat;
  __u8 blkszbits;   /* filesystem block size in bit shift */
  __u8 sb_extslots; /* superblock size = 128 + sb_extslots * 16 */

  __le16 root_nid; /* nid of root directory */
  __le64 inos;     /* total valid ino # (== f_files - f_favail) */

  __le64 build_time;      /* compact inode time derivation */
  __le32 build_time_nsec; /* compact inode time derivation in ns scale */
  __le32 blocks;          /* used for statfs */
  __le32 meta_blkaddr;    /* start block address of metadata area */
  __le32 xattr_blkaddr;   /* start block address of shared xattr area */
  __u8 uuid[16];          /* 128-bit uuid for volume */
  __u8 volume_name[16];   /* volume name */
  __le32 feature_incompat;
  union {
    /* bitmap for available compression algorithms */
    __le16 available_compr_algs;
    /* customized sliding window size instead of 64k by default */
    __le16 lz4_max_distance;
  } __packed u1;
  __le16 extra_devices;       /* # of devices besides the primary device */
  __le16 devt_slotoff;        /* startoff = devt_slotoff * devt_slotsize */
  __u8 dirblkbits;            /* directory block size in bit shift */
  __u8 xattr_prefix_count;    /* # of long xattr name prefixes */
  __le32 xattr_prefix_start;  /* start of long xattr prefixes */
  __le64 packed_nid;          /* nid of the special packed inode */
  __u8 xattr_filter_reserved; /* reserved for xattr name filter */
  __u8 reserved2[23];
};

typedef struct __attribute__((packed)) {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t header_crc32;
  uint32_t reserved;
  uint64_t my_lba;
  uint64_t alternate_lba;
  uint64_t first_usable_lba;
  uint64_t last_usable_lba;
  uuid_t disk_guid;
  uint64_t partition_entry_lba;
  uint32_t num_partition_entries;
  uint32_t partition_entry_size;
  uint32_t partition_entry_crc32;
} gpt_header_t;

typedef struct __attribute__((packed)) {
  uuid_t type_guid;
  uuid_t unique_guid;
  uint64_t first_lba;
  uint64_t last_lba;
  uint64_t attributes;
  uint16_t name[36];
} gpt_entry_t;

int load_info(openpart_t *op);
const char *path_from_fd(int fd);
int find_partition_name(openpart_t *op, const char *part_path, char *name_out);

__END_DECLS
#endif // #ifndef LIB_OPENPART__INTERNAL_H
