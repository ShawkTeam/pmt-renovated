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

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <zlib.h>

__BEGIN_DECLS

int openpart_wipe(openpart_t *op)
{
  uint8_t buf[4096];
  uint64_t remaining;
  uint64_t offset = 0;
  ssize_t ret;

  if (!op) {
    errno = EINVAL;
    return -1;
  }

  if (!(op->flags & OP_RDWR)) {
    op->err = EACCES;
    return -1;
  }

  memset(buf, 0, sizeof(buf));
  remaining = op->size;

  while (remaining > 0) {
    size_t count = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    ret = pwrite(op->fd, buf, count, (off_t)offset);
    if (ret < 0) {
      op->err = errno;
      return -1;
    }
    offset += ret;
    remaining -= ret;
  }

  return openpart_sync(op);
}

int openpart_checksum(openpart_t *op, int algo, uint8_t *out, size_t len)
{
  uint8_t buf[4096];
  uint64_t remaining;
  uint64_t offset = 0;
  ssize_t ret;

  if (!op || !out) {
    errno = EINVAL;
    return -1;
  }

  switch (algo) {
    case OP_CKSUM_CRC32: {
      if (len < sizeof(uint32_t)) {
        op->err = EINVAL;
        return -1;
      }
      uint32_t crc = crc32(0L, Z_NULL, 0);
      remaining = op->size;
      while (remaining > 0) {
        size_t count = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        ret = pread(op->fd, buf, count, (off_t)offset);
        if (ret < 0) { op->err = errno; return -1; }
        crc = crc32(crc, buf, ret);
        offset += ret;
        remaining -= ret;
      }
      memcpy(out, &crc, sizeof(uint32_t));
      return 0;
    }
    case OP_CKSUM_SHA256: {
      if (len < SHA256_DIGEST_LENGTH) {
        op->err = EINVAL;
        return -1;
      }
      SHA256_CTX ctx;
      SHA256_Init(&ctx);
      remaining = op->size;
      while (remaining > 0) {
        size_t count = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        ret = pread(op->fd, buf, count, (off_t)offset);
        if (ret < 0) { op->err = errno; return -1; }
        SHA256_Update(&ctx, buf, ret);
        offset += ret;
        remaining -= ret;
      }
      SHA256_Final(out, &ctx);
      return 0;
    }
    case OP_CKSUM_MD5: {
      if (len < MD5_DIGEST_LENGTH) {
        op->err = EINVAL;
        return -1;
      }
      MD5_CTX ctx;
      MD5_Init(&ctx);
      remaining = op->size;
      while (remaining > 0) {
        size_t count = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        ret = pread(op->fd, buf, count, (off_t)offset);
        if (ret < 0) { op->err = errno; return -1; }
        MD5_Update(&ctx, buf, ret);
        offset += ret;
        remaining -= ret;
      }
      MD5_Final(out, &ctx);
      return 0;
    }
    default:
      op->err = EINVAL;
      return -1;
  }
}

int openpart_hexdump(openpart_t *op, uint64_t offset, size_t size, int fd)
{
  uint8_t buf[16];
  size_t remaining = size;
  uint64_t cur_offset = offset;
  ssize_t ret;

  if (!op) {
    errno = EINVAL;
    return -1;
  }

  while (remaining > 0) {
    size_t count = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    ret = pread(op->fd, buf, count, (off_t)cur_offset);
    if (ret < 0) {
      op->err = errno;
      return -1;
    }

    /* offset */
    dprintf(fd, "%08" PRIx64 "  ", cur_offset);

    /* hex */
    size_t i;
    for (i = 0; i < 16; i++) {
      if (i < (size_t)ret)
        dprintf(fd, "%02x ", buf[i]);
      else
        dprintf(fd, "   ");
      if (i == 7)
        dprintf(fd, " ");
    }

    /* ascii */
    dprintf(fd, " |");
    for (i = 0; i < (size_t)ret; i++)
      dprintf(fd, "%c", (buf[i] >= 0x20 && buf[i] < 0x7f) ? buf[i] : '.');
    dprintf(fd, "|\n");

    cur_offset += ret;
    remaining -= ret;
  }

  return 0;
}

const char* openpart_whatis_magic() { return "OPENPART"; }

__END_DECLS
