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

/**
 * @file openpart.h
 * @author Yağız Zengin ([YZBruh](https://github.com/YZBruh))
 * @brief Openpart library header file.
 */

#ifndef LIB_OPENPART__OPENPART_H
#define LIB_OPENPART__OPENPART_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

/**
 * @name OpenPart library macros and type definations.
 * @brief Open flags, openpart_get() flags, mount flags, checksum algorithms.
 *
 * @{
 */

#define OP_RDONLY 0x1  ///< Open as read-only.
#define OP_WRONLY 0x2  ///< Open as write-only.
#define OP_RDWR 0x4    ///< Open as read-write.
#define OP_IGNTYPE 0x8 ///< Ignore input path type.

#define OP_INFO_SIZE 0x1        ///< Get size.
#define OP_INFO_UUID 0x2        ///< Get filesystem UUID.
#define OP_INFO_LABEL 0x3       ///< Get filesystem label.
#define OP_INFO_FSTYPE 0x4      ///< Get filesystem type.
#define OP_INFO_SECTOR_SIZE 0x5 ///< Get sector size.
#define OP_INFO_IS_BLKDEV 0x6   ///< Get whether the partition is a block device.
#define OP_INFO_PART_NAME 0x7   ///< Get partition name (finds partition name from partition table, founding with using sysfs).
#define OP_INFO_PART_PATH 0x8   ///< Get partition path.
#define OP_INFO_DISK_PATH 0x9   ///< Get partition table path (reading from sysfs).
#define OP_INFO_FD 0xA          ///< Get file descriptor.

#define OP_MOUNT_RDONLY 0x1 ///< Mount as read-only.
#define OP_MOUNT_RDWR 0x2   ///< Mount as read-write.

#define OP_CKSUM_CRC32 0x1  ///< Check CRC32.
#define OP_CKSUM_SHA256 0x2 ///< Check SHA256.
#define OP_CKSUM_MD5 0x3    ///< Check MD5.

#define openpart_get_size2(op, out)                                                                                                   \
  openpart_get((op), OP_INFO_SIZE, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_SIZE.
#define openpart_get_uuid2(op, out)                                                                                                   \
  openpart_get((op), OP_INFO_UUID, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_UUID.
#define openpart_get_label2(op, out)                                                                                                  \
  openpart_get((op), OP_INFO_LABEL, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_LABEL.
#define openpart_get_fstype2(op, out)                                                                                                 \
  openpart_get((op), OP_INFO_FSTYPE, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_FSTYPE.
#define openpart_get_sector_size2(op, out)                                                                                            \
  openpart_get((op), OP_INFO_SECTOR_SIZE, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_SECTOR_SIZE.
#define openpart_get_is_blkdev2(op, out)                                                                                              \
  openpart_get((op), OP_INFO_IS_BLKDEV, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_IS_BLKDEV.
#define openpart_get_part_name2(op, out)                                                                                              \
  openpart_get((op), OP_INFO_PART_NAME, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_PART_NAME.
#define openpart_get_part_path2(op, out)                                                                                              \
  openpart_get((op), OP_INFO_PART_PATH, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_PART_PATH.
#define openpart_get_disk_path2(op, out)                                                                                              \
  openpart_get((op), OP_INFO_DISK_PATH, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_DISK_PATH.
#define openpart_get_fd2(op, out)                                                                                                     \
  openpart_get((op), OP_INFO_FD, (void **)(out)) ///< @c openpart_get() implementation for @c OP_INFO_FD.

typedef struct openpart
    openpart_t; ///< Opaque @c openpart_t type defination. The struct body is available in the @c src/internal.h file.
/** @} */

/**
 * @name OpenPart core functions.
 * @brief Open, close, read data from file.
 *
 * @{
 */

/**
 * @brief Open partition.
 *
 * @param path Path to the partition.
 * @param flags OpenPart open flags.
 * @param extra_oflags Extra @c open() flags.
 * @warning It is verified that the input path is a blocking device. To bypass this, use the @c OP_IGNTYPE flag.
 * @warning Allocated @c openpart_t* object is must be closed with @c openpart_close().
 * @return @c NULL on error, otherwise pointer to the openpart_t structure.
 */
openpart_t *openpart_open(const char *path, int flags, uint32_t extra_oflags);

/**
 * @brief Read data from a basic info backup file (generated by OpenPart).
 *
 * @param path File path.
 * @param flags OpenPart open flags.
 * @param extra_oflags Extra @c open() flags.
 * @warning It is verified that the input path is a blocking device. To bypass this, use the @c OP_IGNTYPE flag.
 * @return @c NULL on error, otherwise pointer to the openpart_t structure.
 */
openpart_t *openpart_read_data_from_file(const char *path, int flags, uint32_t extra_oflags);

/**
 * @brief Close partition.
 *
 * @param op Pointer to the openpart_t structure.
 */
void openpart_close(openpart_t **op);
/** @} */

/**
 * @name OpenPart I/O functions.
 * @brief Read and write data from/to partition, sync, free the allocated @c openpart_t* memory, save basic info to files.
 *
 * @{
 */

/**
 * @brief Read data from partition.
 *
 * @param op @c openpart_t* object.
 * @param buf Buffer.
 * @param count Count for reading (as bytes).
 * @param offset Target offset.
 * @return Readed byte count (0 and samllers on error).
 */
ssize_t openpart_read(openpart_t *op, void *buf, size_t count, uint64_t offset);

/**
 * @brief Write data to partition.
 *
 * @param op @c openpart_t* object.
 * @param buf Buffer.
 * @param count Count for writing (as bytes).
 * @param offset Target offset.
 * @return Writed byte count (0 and samllers on error).
 */
ssize_t openpart_write(openpart_t *op, const void *buf, size_t count, uint64_t offset);

/**
 * @brief Read sectors from partition.
 *
 * @param op @c openpart_t* object.
 * @param buf Buffer.
 * @param sector Total sectors for read.
 * @param count Count for reading (sector count, like 1).
 * @return Readed byte count (0 and samllers on error).
 */
ssize_t openpart_read_sector(openpart_t *op, void *buf, uint64_t sector, size_t count);

/**
 * @brief Write sector to partition.
 *
 * @param op @c openpart_t* object.
 * @param buf Buffer.
 * @param sector Total sectors for write.
 * @param count Count for reading (sector count, like 1).
 * @return Writed byte count (0 and samllers on error).
 */
ssize_t openpart_write_sector(openpart_t *op, const void *buf, uint64_t sector, size_t count);

/**
 * @brief Save basic info to file.
 *
 * @param path File path.
 * @param op @c openpart_t* object.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_save_to_file(const char *path, openpart_t *op);

/**
 * @brief Check input file is a valid basic info backup.
 *
 * @param path File path.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_is_valid_save(const char *path);

/**
 * @brief Sync partition with (@c fsync() ).
 *
 * @param op @c openpart_t* object.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_sync(openpart_t *op);

/**
 * @brief Allocate sector buffer.
 *
 * @param op @c openpart_t* object.
 * @warning The allocated buffer is must be freed with @c openpart_free() or @c free().
 * @return NULL on error, otherwise pointer to the allocated buffer.
 */
void *openpart_alloc_sector_buf(openpart_t *op);

/**
 * @brief Free the memory.
 *
 * @param buf Buffer.
 */
void openpart_free(void *buf);
/** @} */

/**
 * @name OpenPart basic info functions.
 * @brief Get basic info from partition.
 *
 * @{
 */

/**
 * @brief Write requested info to the input buffer.
 *
 * @param op @c openpart_t* object.
 * @param info OP_INFO*** flag.
 * @param buf Buffer.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_get(openpart_t *op, int info, void **buf);

/**
 * @brief Get size of partition.
 * @param op @c openpart_t* object.
 * @return Actual size on success, otherwise -1 or 0.
 */
uint64_t openpart_get_size(openpart_t *op);

/**
 * @brief Get partition name.
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise partition name.
 */
const char *openpart_get_part_name(openpart_t *op);

/**
 * @brief Get partition path.
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise partition path.
 */
const char *openpart_get_part_path(openpart_t *op);

/**
 * @brief Get partition table path (reading from sysfs).
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise partition table path.
 */
const char *openpart_get_disk_path(openpart_t *op);

/**
 * @brief Get filesystem UUID.
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise filesystem UUID.
 */
const char *openpart_get_uuid(openpart_t *op);

/**
 * @brief Get filesystem label.
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise filesystem label.
 */
const char *openpart_get_label(openpart_t *op);

/**
 * @brief Get filesystem type.
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise filesystem type (as name).
 */
const char *openpart_get_fstype(openpart_t *op);

/**
 * @brief Get sector size.
 *
 * @param op @c openpart_t* object.
 * @return Actual sector size on success, otherwise -1 or 0.
 */
uint64_t openpart_get_sector_size(openpart_t *op);

/**
 * @brief Get file descriptor.
 *
 * @param op @c openpart_t* object.
 * @return File descriptor.
 */
int openpart_get_fd(openpart_t *op);

/**
 * @brief Get opened path is block device or not.
 *
 * @param op @c openpart_t* object.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_get_is_blkdev(openpart_t *op);
/** @} */

/**
 * @name OpenPart mount functions.
 * @brief Mount and unmount filesystems, check mount status, get mount paths (of already mounted filesystems).
 *
 * @{
 */

/**
 * @brief Check partition is mounted or not.
 *
 * @param op @c openpart_t* object.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_is_mounted(openpart_t *op);

/**
 * @brief Mount partition.
 *
 * @param op @c openpart_t* object.
 * @param target Target mount path.
 * @param flags OpenPart mount flags.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_mount(openpart_t *op, const char *target, int flags);

/**
 * @brief Unmpunt partition.
 *
 * @param op @c openpart_t* object.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_umount(openpart_t *op);

/**
 * @brief Get mount path of partition.
 *
 * @param op @c openpart_t* object.
 * @return NULL on error, otherwise mount path.
 */
const char *openpart_mntpath(openpart_t *op);
/** @} */

/**
 * @name OpenPart utility functions.
 * @brief Utility functions.
 *
 * @{
 */

/**
 * @brief Wipe the partition (with filling 0).
 *
 * @param op @c openpart_t* object.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_wipe(openpart_t *op);

/**
 * @brief Do checksum test.
 *
 * @param op @c openpart_t* object.
 * @param algo Algorithm flag (use the OP_CKSUM*** flags).
 * @param out Output buffer for checksum result (string).
 * @param len Output buffer length.
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_checksum(openpart_t *op, int algo, uint8_t *out, size_t len);

/**
 * @brief Dump hex.
 *
 * @param op @c openpart_t* object.
 * @param offset Target offset.
 * @param size Size for read (as bytes).
 * @param fd File descriptor of output stream (like @c STDOUT_FILENO for @c stdout ).
 * @return 1 (true) on success, otherwise -1 or 0.
 */
int openpart_hexdump(openpart_t *op, uint64_t offset, size_t size, int fd);

/// @brief Get basic information file magic.
const char *openpart_whatis_magic();
/** @} */

/**
 * @name OpenPart error functions.
 * @brief Get error information.
 *
 * @{
 */

/**
 * @brief Get errno from @c openpart_t* object.
 *
 * @param op @c openpart_t* object.
 * @return @c errno value.
 */
int openpart_errno(openpart_t *op);

/**
 * @brief Get errno from @c openpart_t* object and get correct message with @c strerror().
 *
 * @param op @c openpart_t* object.
 * @return Correct message with @c strerror().
 */
const char *openpart_strerror(openpart_t *op);
/** @} */

__END_DECLS
#endif // #ifndef LIB_OPENPART__OPENPART_H
