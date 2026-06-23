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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mount.h>
#include <mntent.h>

__BEGIN_DECLS

int openpart_is_mounted(openpart_t *op)
{
  FILE *f;
  struct mntent *mnt;
  char fd_path[PATH_MAX];
  char real_device_path[PATH_MAX];
  int found = 0;

  if (!op)
    return -1;

  snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", op->fd);
  if (realpath(fd_path, real_device_path) == NULL) {
    op->err = errno;
    return -1;
  }

  f = setmntent("/proc/mounts", "r");
  if (!f) {
    op->err = errno;
    return -1;
  }

  while ((mnt = getmntent(f)) != NULL) {
    if (strcmp(mnt->mnt_fsname, real_device_path) == 0) {
      found = 1;
      break;
    }
  }

  endmntent(f);
  return found;
}

int openpart_mount(openpart_t *op, const char *target, int flags)
{
  int mflags = 0;

  if (!op || !target) {
    errno = EINVAL;
    return -1;
  }

  if (!op->info_loaded) {
    if (load_info(op) < 0) {
      op->err = errno;
      return -1;
    }
    op->info_loaded = 1;
  }

  if (flags & OP_MOUNT_RDONLY)
    mflags |= MS_RDONLY;

  if (mount(path_from_fd(op->fd), target, op->fstype, mflags, NULL) < 0) {
    op->err = errno;
    return -1;
  }

  return 0;
}

int openpart_umount(openpart_t *op)
{
  if (!op) {
    errno = EINVAL;
    return -1;
  }

  if (umount(path_from_fd(op->fd)) < 0) {
    op->err = errno;
    return -1;
  }

  return 0;
}

const char *openpart_mntpath(openpart_t *op)
{
  FILE *f;
  struct mntent *mnt;
  static char mntpath[PATH_MAX];

  if (!op)
    return NULL;

  const char* path = path_from_fd(op->fd);
  if (!path) {
    op->err = errno;
    return NULL;
  }

  f = setmntent("/proc/mounts", "r");
  if (!f) {
    op->err = errno;
    return NULL;
  }

  while ((mnt = getmntent(f)) != NULL) {
    if (strcmp(mnt->mnt_fsname, path) == 0) {
      strncpy(mntpath, mnt->mnt_dir, sizeof(mntpath) - 1);
      mntpath[sizeof(mntpath) - 1] = '\0';
      endmntent(f);
      return mntpath;
    }
  }

  endmntent(f);
  return NULL;
}

__END_DECLS
