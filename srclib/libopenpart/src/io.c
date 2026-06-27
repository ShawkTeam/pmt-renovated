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

#include <libopenpart/openpart.h>
#include "internal.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ext2fs/ext2fs.h>
#include <ext2fs/ext2_fs.h>
#include <f2fs_fs.h>
#include <linux/magic.h>
#include <linux/msdos_fs.h>
#include <uuid/uuid.h>

__BEGIN_DECLS

static uint32_t read_le32(const uint8_t *buf)
{
  return (uint32_t)buf[0]        |
         ((uint32_t)buf[1] << 8) |
         ((uint32_t)buf[2] << 16)|
         ((uint32_t)buf[3] << 24);
}

static uint16_t read_le16(const uint8_t *buf)
{
  return (uint16_t)buf[0] |
         ((uint16_t)buf[1] << 8);
}

static int read_ext4_info(openpart_t *op)
{
  struct ext2_super_block sb;

  if (pread(op->fd, &sb, sizeof(sb), 1024) < 0)
    return -1;

  /* UUID */
  uuid_unparse(sb.s_uuid, op->uuid);

  /* label */
  strncpy(op->label, (char *)sb.s_volume_name, sizeof(op->label) - 1);
  op->label[sizeof(op->label) - 1] = '\0';

  return 0;
}

static int read_f2fs_info(openpart_t *op)
{
  struct f2fs_super_block sb;

  if (pread(op->fd, &sb, sizeof(sb), F2FS_SUPER_OFFSET) < 0)
    return -1;

  /* UUID */
  uuid_unparse(sb.uuid, op->uuid);

  /* label - UTF-16LE, convert to UTF-8 */
  /* f2fs volume_name UTF-16LE encoded */
  size_t i;
  for (i = 0; i < sizeof(op->label) - 1 && sb.volume_name[i]; i++)
    op->label[i] = (char)(sb.volume_name[i] & 0xFF);
  op->label[i] = '\0';

  return 0;
}

static int read_erofs_info(openpart_t *op)
{
  struct erofs_super_block sb;

  if (pread(op->fd, &sb, sizeof(sb), EROFS_SUPER_OFFSET) < 0)
    return -1;

  /* UUID */
  uuid_unparse(sb.uuid, op->uuid);

  /* label */
  strncpy(op->label, (char *)sb.volume_name, sizeof(op->label) - 1);
  op->label[sizeof(op->label) - 1] = '\0';

  return 0;
}

static int read_vfat_info(openpart_t *op)
{
  struct fat_boot_sector bs;

  if (pread(op->fd, &bs, sizeof(bs), 0) < 0)
    return -1;

  __u8 *vol_id;
  __u8 *vol_label;

  if (memcmp(bs.fat32.fs_type, "FAT32   ", 8) == 0) {
    vol_id    = bs.fat32.vol_id;
    vol_label = bs.fat32.vol_label;
  } else {
    vol_id    = bs.fat16.vol_id;
    vol_label = bs.fat16.vol_label;
  }

  snprintf(op->uuid, sizeof(op->uuid), "%02X%02X-%02X%02X",
           vol_id[3], vol_id[2], vol_id[1], vol_id[0]);

  memcpy(op->label, vol_label, MSDOS_NAME);
  op->label[MSDOS_NAME] = '\0';

  /* clean trailing space */
  for (int i = MSDOS_NAME - 1; i >= 0 && op->label[i] == ' '; i--)
    op->label[i] = '\0';

  return 0;
}

static int detect_fstype(openpart_t *op)
{
  uint8_t buf[4096];
  uint32_t magic32;
  uint16_t magic16;

  if (pread(op->fd, buf, sizeof(buf), 1024) < 0)
    return -1;
  magic32 = read_le32(buf);

  if (magic32 == EXT2_SUPER_MAGIC) {
    struct ext2_super_block sb;
    if (pread(op->fd, &sb, sizeof(sb), 1024) < 0) {
      snprintf(op->fstype, sizeof(op->fstype), "ext4");
      return 0;
    }

    if ((sb.s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS) ||
        (sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT) ||
        (sb.s_feature_incompat & EXT4_FEATURE_INCOMPAT_FLEX_BG)) {
      snprintf(op->fstype, sizeof(op->fstype), "ext4");
    } else if ((sb.s_feature_incompat & EXT3_FEATURE_INCOMPAT_RECOVER) ||
               (sb.s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL)) {
      snprintf(op->fstype, sizeof(op->fstype), "ext3");
    } else {
      snprintf(op->fstype, sizeof(op->fstype), "ext2");
    }

    return 0;
  }
  if (magic32 == F2FS_SUPER_MAGIC) {
    snprintf(op->fstype, sizeof(op->fstype), "f2fs");
    return 0;
  }
  if (magic32 == EROFS_SUPER_MAGIC_V1) {
    snprintf(op->fstype, sizeof(op->fstype), "erofs");
    return 0;
  }

  if (pread(op->fd, buf, 2, 510) < 0)
    return -1;

  magic16 = read_le16(buf);
  if (magic16 == 0xAA55) {
    snprintf(op->fstype, sizeof(op->fstype), "vfat");
    return 0;
  }

  snprintf(op->fstype, sizeof(op->fstype), "unknown");
  return 0;
}

int load_info(openpart_t *op)
{
  if (detect_fstype(op) < 0)
    return -1;

  if (strcmp(op->fstype, "ext4") == 0)
    return read_ext4_info(op);
  else if (strcmp(op->fstype, "f2fs") == 0)
    return read_f2fs_info(op);
  else if (strcmp(op->fstype, "erofs") == 0)
    return read_erofs_info(op);
  else if (strcmp(op->fstype, "vfat") == 0)
    return read_vfat_info(op);

  return 0;
}

ssize_t openpart_read(openpart_t *op, void *buf, size_t count, uint64_t offset)
{
  ssize_t ret;

  if (!op || !buf) {
    errno = EINVAL;
    return -1;
  }

  ret = pread(op->fd, buf, count, (off_t)offset);
  if (ret < 0)
    op->err = errno;

  return ret;
}

ssize_t openpart_write(openpart_t *op, const void *buf, size_t count, uint64_t offset)
{
  ssize_t ret;

  if (!op || !buf) {
    errno = EINVAL;
    return -1;
  }

  if (!(op->flags & OP_RDWR)) {
    op->err = EACCES;
    return -1;
  }

  ret = pwrite(op->fd, buf, count, (off_t)offset);
  if (ret < 0)
    op->err = errno;

  return ret;
}

ssize_t openpart_read_sector(openpart_t *op, void *buf, uint64_t sector, size_t count)
{
  if (!op || !buf) {
    errno = EINVAL;
    return -1;
  }

  uint64_t offset = sector * op->sector_size;
  return openpart_read(op, buf, count * op->sector_size, offset);
}

ssize_t openpart_write_sector(openpart_t *op, const void *buf, uint64_t sector, size_t count)
{
  if (!op || !buf) {
    errno = EINVAL;
    return -1;
  }

  if (!(op->flags & OP_RDWR)) {
    op->err = EACCES;
    return -1;
  }

  uint64_t sector_size = op->sector_size;
  uint64_t offset = sector * sector_size;
  return openpart_write(op, buf, count * sector_size, offset);
}

int openpart_get(openpart_t *op, int info, void **out)
{
  QUICK_GET_CONTROLS(op, -1);

  switch (info) {
    case OP_INFO_SIZE: {
      uint64_t *size = malloc(sizeof(uint64_t));
      if (!size) { op->err = errno; return -1; }
      *size = op->size;
      *out = size;
      return 0;
    }
    case OP_INFO_UUID: {
      char *uuid = malloc(37);
      if (!uuid) { op->err = errno; return -1; }
      strncpy(uuid, op->uuid, 37);
      *out = uuid;
      return 0;
    }
    case OP_INFO_LABEL: {
      char *label = malloc(sizeof(op->label));
      if (!label) { op->err = errno; return -1; }
      strncpy(label, op->label, sizeof(op->label));
      *out = label;
      return 0;
    }
    case OP_INFO_FSTYPE: {
      char *fstype = malloc(sizeof(op->fstype));
      if (!fstype) { op->err = errno; return -1; }
      strncpy(fstype, op->fstype, sizeof(op->fstype));
      *out = fstype;
      return 0;
    }
    case OP_INFO_SECTOR_SIZE: {
      uint64_t *sector_size = malloc(sizeof(uint64_t));
      if (!sector_size) { op->err = errno; return -1; }
      *sector_size = op->sector_size;
      *out = sector_size;
      return 0;
    }
    case OP_INFO_IS_BLKDEV: {
      int *is_blkdev = malloc(sizeof(int));
      if (!is_blkdev) { op->err = errno; return -1; }
      *is_blkdev = 1;
      *out = is_blkdev;
      return 0;
    }
    case OP_INFO_PART_NAME: {
      char *part_name = malloc(PATH_MAX);
      if (!part_name) { op->err = errno; return -1; }

      const char *path = path_from_fd(op->fd);
      if (!path) {
        free(part_name);
        op->err = errno;
        return -1;
      }

      if (find_partition_name(op, path, part_name) < 0) {
        free(part_name);
        op->err = ENOENT;
        return -1;
      }

      *out = part_name;
      return 0;
    }
    case OP_INFO_PART_PATH: {
      const char *path = path_from_fd(op->fd);
      if (!path) { op->err = errno; return -1; }
      char *result = malloc(strlen(path) + 1);
      if (!result) { op->err = errno; return -1; }
      strcpy(result, path);
      *out = result;
      return 0;
    }
    case OP_INFO_DISK_PATH: {
      const char* path = openpart_get_disk_path(op);
      if (!path) { op->err = errno; return -1; }
      char* result = malloc(strlen(path) + 1);
      if (!result) { op->err = errno; return -1; }
      strcpy(result, path);
      *out = result;
      return 0;
    }
    default:
      op->err = EINVAL;
      return -1;
  }
}

uint64_t openpart_get_size(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, UINT64_MAX);
  return op->size;
}

const char* openpart_get_uuid(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, NULL);
  return op->uuid;
}

const char* openpart_get_label(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, NULL);
  return op->label;
}

const char* openpart_get_fstype(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, NULL);
  return op->fstype;
}

uint64_t openpart_get_sector_size(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, UINT64_MAX);
  return op->sector_size;
}

int openpart_get_is_blkdev(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, -1);

  if (op->flags & OP_IGNTYPE) {
    struct stat st;
    if (fstat(op->fd, &st) < 0)
      return 0;
    if (!S_ISBLK(st.st_mode))
      return 0;
  }
  return 1;
}

const char* openpart_get_part_path(openpart_t* op)
{
  QUICK_GET_CONTROLS(op, NULL);
  return path_from_fd(op->fd);
}

int openpart_save_to_file(const char *path, openpart_t *op)
{
  FILE *f;

  if (!path || !op) {
    errno = EINVAL;
    return -1;
  }

  f = fopen(path, "wb");
  if (!f)
    return -1;

  const char* p_path = path_from_fd(op->fd);
  if (!p_path) {
    fclose(f);
    return -1;
  }
  size_t path_len = strlen(p_path) + 1;

  if (fwrite(op->openpart_magic, sizeof(op->openpart_magic), 1, f) != 1 ||
      fwrite(&path_len, sizeof(path_len), 1, f) != 1 ||
      fwrite(p_path, path_len, 1, f) != 1 ||
      fwrite(&op->flags, sizeof(op->flags), 1, f) != 1 ||
      fwrite(&op->err, sizeof(op->err), 1, f) != 1 ||
      fwrite(&op->size, sizeof(op->size), 1, f) != 1 ||
      fwrite(&op->sector_size, sizeof(op->sector_size), 1, f) != 1 ||
      fwrite(&op->info_loaded, sizeof(op->info_loaded), 1, f) != 1 ||
      fwrite(op->uuid, sizeof(op->uuid), 1, f) != 1 ||
      fwrite(op->label, sizeof(op->label), 1, f) != 1 ||
      fwrite(op->fstype, sizeof(op->fstype), 1, f) != 1) {
    fclose(f);
    return -1;
  }

  fclose(f);
  return 1;
}

void openpart_free(void *out)
{
  free(out);
}

void *openpart_alloc_sector_buf(openpart_t *op)
{
  if (!op)
    return NULL;
  return calloc(1, op->sector_size);
}

int openpart_sync(openpart_t *op)
{
  int ret;

  if (!op)
    return -1;

  ret = fsync(op->fd);
  if (ret < 0)
    op->err = errno;

  return ret;
}

__END_DECLS
