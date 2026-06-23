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

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

__BEGIN_DECLS

static int get_parent_disk(const char *part_path, char *disk_path)
{
  char sysfs[PATH_MAX];
  char resolved[PATH_MAX];
  const char *part_name;

  part_name = strrchr(part_path, '/');
  if (!part_name)
    return -1;
  part_name++;

  /* /sys/class/block/sda3/.. -> /sys/class/block/sda */
  snprintf(sysfs, sizeof(sysfs), "/sys/class/block/%s/..", part_name);

  if (realpath(sysfs, resolved) == NULL)
    return -1;

  const char *disk_name = strrchr(resolved, '/');
  if (!disk_name)
    return -1;
  disk_name++;

  snprintf(disk_path, PATH_MAX, "/dev/block/%s", disk_name);
  return 0;
}

static int get_partition_start_lba(const char *part_name, uint64_t *start_lba)
{
  char sysfs[PATH_MAX];
  FILE *f;

  snprintf(sysfs, sizeof(sysfs), "/sys/class/block/%s/start", part_name);

  f = fopen(sysfs, "r");
  if (!f)
    return -1;

  if (fscanf(f, "%" SCNu64, start_lba) != 1) {
    fclose(f);
    return -1;
  }

  fclose(f);
  return 0;
}

int find_partition_name(openpart_t* op, const char *part_path, char *name_out)
{
  char disk_path[PATH_MAX];
  char part_name[64];
  gpt_header_t header;
  gpt_entry_t entry;
  int disk_fd;
  uint64_t offset;
  uint64_t start_lba;
  uint32_t i;
  const char *pname;

  pname = strrchr(part_path, '/');
  if (!pname)
    return -1;
  pname++;

  strncpy(part_name, pname, sizeof(part_name) - 1);
  part_name[sizeof(part_name) - 1] = '\0';

  if (get_partition_start_lba(part_name, &start_lba) < 0)
    return -1;
  start_lba = start_lba / (op->sector_size / 512);

  if (get_parent_disk(part_path, disk_path) < 0)
    return -1;

  printf("Looking for LBA: %" PRIu64 "\n", start_lba);
  disk_fd = open(disk_path, O_RDONLY);
  if (disk_fd < 0)
    return -1;

  if (pread(disk_fd, &header, sizeof(header), op->sector_size) < 0) {
    close(disk_fd);
    return -1;
  }

  if (header.signature != 0x5452415020494645ULL) {
    close(disk_fd);
    return -1;
  }

  offset = header.partition_entry_lba * op->sector_size;

  for (i = 0; i < header.num_partition_entries; i++) {
    size_t j;

    if (pread(disk_fd, &entry, header.partition_entry_size, (off_t)offset) < 0)
      break;

    offset += header.partition_entry_size;

    if (entry.first_lba == 0 && entry.last_lba == 0)
      continue;

    if (entry.first_lba == start_lba) {
      for (j = 0; j < PATH_MAX - 1 && j < 36 && entry.name[j]; j++)
        name_out[j] = (char)(entry.name[j] & 0xFF);
      name_out[j] = '\0';
      close(disk_fd);
      return 0;
    }
  }

  close(disk_fd);
  return -1;
}

const char *openpart_get_part_name(openpart_t *op)
{
  static char name[64];
  const char *path;
  if (!op)
    return NULL;

  path = path_from_fd(op->fd);
  if (!path)
    return NULL;

  if (find_partition_name(op, path, name) < 0)
    return NULL;

  return name;
}

const char* openpart_get_disk_path(openpart_t* op)
{
  if (!op)
    return NULL;

  static char disk_path[PATH_MAX];
  if (get_parent_disk(path_from_fd(op->fd), disk_path) < 0)
    return NULL;

  return disk_path;
}

__END_DECLS
