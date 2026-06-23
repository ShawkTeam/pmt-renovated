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

#ifndef LIB_OPENPART__OPENPART_H
#define LIB_OPENPART__OPENPART_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

/* Open flags */
#define OP_RDONLY 0x1
#define OP_WRONLY 0x2
#define OP_RDWR 0x4
#define OP_IGNTYPE 0x8

/* Info flags */
#define OP_INFO_SIZE 0x1
#define OP_INFO_UUID 0x2
#define OP_INFO_LABEL 0x3
#define OP_INFO_FSTYPE 0x4
#define OP_INFO_SECTOR_SIZE 0x5
#define OP_INFO_IS_BLKDEV 0x6
#define OP_INFO_PART_NAME 0x7

/* Mount flags */
#define OP_MOUNT_RDONLY 0x1
#define OP_MOUNT_RDWR 0x2

/* Checksum algorithms */
#define OP_CKSUM_CRC32 0x1
#define OP_CKSUM_SHA256 0x2
#define OP_CKSUM_MD5 0x3

/* Opaque openpart_t type defination */
typedef struct openpart openpart_t;

/* Core */
openpart_t *openpart_open(const char *path, int flags, uint32_t extra_oflags);
openpart_t *openpart_read_data_from_file(const char *path, int flags, uint32_t extra_oflags);
void openpart_close(openpart_t *op);

/* Read, sync */
ssize_t openpart_read(openpart_t *op, void *buf, size_t count, uint64_t offset);
ssize_t openpart_write(openpart_t *op, const void *buf, size_t count, uint64_t offset);
ssize_t openpart_read_sector(openpart_t *op, void *buf, uint64_t sector, size_t count);
ssize_t openpart_write_sector(openpart_t *op, const void *buf, uint64_t sector, size_t count);
int openpart_save_to_file(const char *path, openpart_t *op);
int openpart_is_valid_save(const char *path);
int openpart_sync(openpart_t *op);
void *openpart_alloc_sector_buf(openpart_t *op);
void openpart_free(void *buf);

/* Read basic informations */
int openpart_get(openpart_t *op, int info, void **buf);
uint64_t openpart_get_size(openpart_t *op);
const char *openpart_get_part_name(openpart_t *op);
const char *openpart_get_part_path(openpart_t *op);
const char *openpart_get_disk_path(openpart_t *op);
const char *openpart_get_uuid(openpart_t *op);
const char *openpart_get_label(openpart_t *op);
const char *openpart_get_fstype(openpart_t *op);
uint64_t openpart_get_sector_size(openpart_t *op);
int openpart_get_is_blkdev(openpart_t *op);

/* Mount */
int openpart_is_mounted(openpart_t *op);
int openpart_mount(openpart_t *op, const char *target, int flags);
int openpart_umount(openpart_t *op);
const char *openpart_mntpath(openpart_t *op);

/* Utility */
int openpart_wipe(openpart_t *op);
int openpart_checksum(openpart_t *op, int algo, uint8_t *out, size_t len);
int openpart_hexdump(openpart_t *op, uint64_t offset, size_t size, int fd);
const char *openpart_whatis_magic();

/* Error */
int openpart_errno(openpart_t *op);
const char *openpart_strerror(openpart_t *op);

#define openpart_get_size2(op, out) openpart_get((op), OP_INFO_SIZE, (void **)(out))
#define openpart_get_uuid2(op, out) openpart_get((op), OP_INFO_UUID, (void **)(out))
#define openpart_get_label2(op, out) openpart_get((op), OP_INFO_LABEL, (void **)(out))
#define openpart_get_fstype2(op, out) openpart_get((op), OP_INFO_FSTYPE, (void **)(out))
#define openpart_get_sector_size2(op, out) openpart_get((op), OP_INFO_SECTOR_SIZE, (void **)(out))
#define openpart_get_is_blkdev2(op, out) openpart_get((op), OP_INFO_IS_BLKDEV, (void **)(out))
#define openpart_get_part_name2(op, out) openpart_get((op), OP_INFO_PART_NAME, (void **)(out))

__END_DECLS
#endif // #ifndef LIB_OPENPART__OPENPART_H
