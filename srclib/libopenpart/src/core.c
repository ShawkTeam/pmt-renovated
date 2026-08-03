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
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/fs.h>

__BEGIN_DECLS

const char *path_from_fd(int fd)
{
  static char buf[PATH_MAX];
  char link[64];
  snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
  ssize_t ret = readlink(link, buf, sizeof(buf) - 1);
  if (ret < 0)
    return NULL;
  buf[ret] = '\0';
  return buf;
}

openpart_t *openpart_open(const char *path, int flags, uint32_t extra_oflags)
{
  int fd;
  int oflags = 0;
  openpart_t *op;
  char *real_path;

  if (!path)
    return NULL;

  real_path = realpath(path, NULL);
  if (!real_path)
    return NULL;

  if (!(flags & OP_IGNTYPE)) {
    struct stat st;
    if (stat(real_path, &st) < 0) {
      free(real_path);
      return NULL;
    }

    if (!S_ISBLK(st.st_mode)) {
      free(real_path);
      errno = ENOTBLK;
      return NULL;
    }
  }

  if (extra_oflags != 0) oflags = extra_oflags;
  if (flags & OP_RDONLY) oflags |= O_RDONLY;
  else if (flags & OP_WRONLY) oflags |= O_WRONLY;
  else oflags |= O_RDWR;
  fd = open(real_path, oflags);
  free(real_path);
  if (fd < 0)
    return NULL;

  op = malloc(sizeof(struct openpart));
  if (!op) {
    close(fd);
    return NULL;
  }

  memcpy(op->openpart_magic, "OPENPART", 8);
  op->fd     = fd;
  op->flags  = flags;
  op->err    = 0;
  op->size   = 0;
  op->sector_size = 0;
  op->info_loaded = 0;
  op->uuid[0]   = '\0';
  op->label[0]  = '\0';
  op->fstype[0] = '\0';

  if (flags & OP_IGNTYPE) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
      op->err = errno;
      free(op);
      close(fd);
      return NULL;
    }
    op->size = st.st_size;
    op->sector_size = 4096; // DEFAULT
  } else {
    uint64_t tmp_size = 0;
    if (ioctl(fd, BLKGETSIZE64, &tmp_size) < 0) {
      op->err = errno;
      free(op);
      close(fd);
      return NULL;
    }
    op->size = tmp_size;

    uint64_t tmp_sector_size = 0;
    if (ioctl(fd, BLKSSZGET, &tmp_sector_size) < 0) {
      op->err = errno;
      free(op);
      close(fd);
      return NULL;
    }
    op->sector_size = tmp_sector_size;
  }

  return op;
}

void openpart_close(openpart_t **op)
{
  if (!op || !*op)
    return;
  if ((*op)->fd > 0) {
    close((*op)->fd);
    (*op)->fd = -1;
  }
  free(*op);
  *op = NULL;
}

int openpart_errno(openpart_t *op)
{
  if (!op)
    return -1;

  return op->err;
}

const char *openpart_strerror(openpart_t *op)
{
  if (!op)
    return "invalid handle";

  return strerror(op->err);
}

__END_DECLS
