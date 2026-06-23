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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <libgen.h>

static void test_info(openpart_t *op)
{
  printf("=== INFO ===\n");

  uint64_t size = openpart_get_size(op);
  if (size != UINT64_MAX)
    printf("Size:        %" PRIu64 " bytes\n", size);
  else
    printf("Size:        FAILED (%s)\n", openpart_strerror(op));

  uint64_t sector_size = openpart_get_sector_size(op);
  if (sector_size != UINT64_MAX)
    printf("Sector size: %" PRIu64 " bytes\n", sector_size);
  else
    printf("Sector size: FAILED (%s)\n", openpart_strerror(op));

  int is_blkdev = openpart_get_is_blkdev(op);
  if (is_blkdev != -1)
    printf("Block dev:   %s\n", is_blkdev ? "yes" : "no");
  else
    printf("Block dev:   FAILED (%s)\n", openpart_strerror(op));

  const char* uuid = openpart_get_uuid(op);
  if (uuid != NULL)
    printf("UUID:        %s\n", uuid);
  else
    printf("UUID:        FAILED (%s)\n", openpart_strerror(op));

  const char* label = openpart_get_label(op);
  if (label != NULL)
    printf("Label:       %s\n", label);
  else
    printf("Label:       FAILED (%s)\n", openpart_strerror(op));

  const char* disk_path = openpart_get_disk_path(op);
  if (disk_path != NULL)
    printf("Disk path:   %s\n", disk_path);
  else
    printf("Disk path:   FAILED (%s)\n", openpart_strerror(op));

  const char* part_path = openpart_get_part_path(op);
  if (part_path != NULL)
    printf("Partition path: %s\n", part_path);
  else
    printf("Partition path: FAILED (%s)\n", openpart_strerror(op));

  const char* part_name = openpart_get_part_name(op);
  if (part_name != NULL)
    printf("Partition name: %s (readed from %s)\n", part_name, openpart_get_disk_path(op));
  else
    printf("Partition name: FAILED (%s)\n", openpart_strerror(op));

  const char* fstype = openpart_get_fstype(op);
  if (fstype != NULL)
    printf("Filesystem:  %s\n", fstype);
  else
    printf("Filesystem:  FAILED (%s)\n", openpart_strerror(op));
}

static void test_io(openpart_t *op)
{
  uint8_t buf[openpart_get_sector_size(op)];
  ssize_t ret;

  printf("\n=== I/O ===\n");

  ret = openpart_read(op, buf, sizeof(buf), 0);
  if (ret < 0)
    printf("Read:        FAILED (%s)\n", openpart_strerror(op));
  else
    printf("Read:        OK (%zd bytes)\n", ret);

  ret = openpart_read_sector(op, buf, 0, 1);
  if (ret < 0)
    printf("Read sector: FAILED (%s)\n", openpart_strerror(op));
  else
    printf("Read sector: OK (%zd bytes)\n", ret);
}

static void test_write(openpart_t *op)
{
  const char test_data[] = "OPENPART_WRITE_TEST";
  char read_back[sizeof(test_data)];
  uint64_t size = openpart_get_size(op);
  uint64_t offset;
  ssize_t ret;

  printf("\n=== WRITE ===\n");

  offset = size - sizeof(test_data);
  ret = openpart_write(op, test_data, sizeof(test_data), offset);
  if (ret < 0) {
    printf("Write:        FAILED (%s)\n", openpart_strerror(op));
    return;
  }
  printf("Write:        OK (%zd bytes)\n", ret);
  openpart_sync(op);

  ret = openpart_read(op, read_back, sizeof(read_back), offset);
  if (ret < 0) {
    printf("Read back:    FAILED (%s)\n", openpart_strerror(op));
    return;
  }

  if (memcmp(test_data, read_back, sizeof(test_data)) == 0)
    printf("Verify:       OK\n");
  else
    printf("Verify:       FAILED (data mismatch)\n");

  void *sector_buf = openpart_alloc_sector_buf(op);
  if (!sector_buf) {
    printf("Write sector: FAILED (OOM)\n");
    return;
  }
  memcpy(sector_buf, test_data, sizeof(test_data));
  uint64_t last_sector = (size / openpart_get_sector_size(op)) - 1;

  ret = openpart_write_sector(op, sector_buf, last_sector, 1);
  openpart_free(sector_buf);
  if (ret < 0)
    printf("Write sector: FAILED (%s)\n", openpart_strerror(op));
  else
    printf("Write sector: OK (%zd bytes)\n", ret);
  openpart_sync(op);
}

static void test_mount(openpart_t *op)
{
  int ret;
  printf("\n=== MOUNT ===\n");

  ret = openpart_is_mounted(op);
  if (ret < 0)
    printf("Is mounted:  FAILED (%s)\n", openpart_strerror(op));
  else {
    printf("Is mounted:  %s\n", ret ? "yes" : "no");
    if (ret) printf("Mount path:  %s\n", openpart_mntpath(op));
  }
}

static void test_checksum(openpart_t *op)
{
  uint8_t out[32];
  int ret;

  printf("\n=== CHECKSUM ===\n");

  ret = openpart_checksum(op, OP_CKSUM_CRC32, out, sizeof(out));
  if (ret < 0)
    printf("CRC32:       FAILED (%s)\n", openpart_strerror(op));
  else
    printf("CRC32:       %02x%02x%02x%02x\n", out[0], out[1], out[2], out[3]);

  ret = openpart_checksum(op, OP_CKSUM_MD5, out, sizeof(out));
  if (ret < 0)
    printf("MD5:         FAILED (%s)\n", openpart_strerror(op));
  else {
    printf("MD5:         ");
    for (int i = 0; i < 16; i++) printf("%02x", out[i]);
    printf("\n");
  }

  ret = openpart_checksum(op, OP_CKSUM_SHA256, out, sizeof(out));
  if (ret < 0)
    printf("SHA256:      FAILED (%s)\n", openpart_strerror(op));
  else {
    printf("SHA256:      ");
    for (int i = 0; i < 32; i++) printf("%02x", out[i]);
    printf("\n");
  }
}

static void test_hexdump(openpart_t *op)
{
  printf("\n=== HEXDUMP (first 64 byte) ===\n");
  if (openpart_hexdump(op, 0, 64, STDOUT_FILENO) < 0)
    printf("Hexdump:     FAILED (%s)\n", openpart_strerror(op));
}

int main(int argc, char** argv)
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <partition>\n", argv[0]);
    return 1;
  }

  printf("Opening: %s\n\n", argv[1]);

  openpart_t *op = openpart_open(argv[1], OP_RDONLY, 0);
  if (!op) {
    fprintf(stderr, "Failed to open: %s\n", strerror(errno));
    return 1;
  }

  test_info(op);
  test_io(op);
  test_mount(op);
  test_checksum(op);
  test_hexdump(op);

  printf("\n=== SYNC ===\n");
  if (openpart_sync(op) < 0)
    printf("Sync:        FAILED (%s)\n", openpart_strerror(op));
  else
    printf("Sync:        OK\n");

  char save_path[256];
  snprintf(save_path, sizeof(save_path), "%s", basename(argv[1]));
  printf("\n=== SAVE ===\n");
  if (openpart_save_to_file(save_path, op))
    printf("Save:        OK (%s)\n", save_path);
  else
    printf("Save:        FAILED (%s)\n", openpart_strerror(op));

  printf("\n=== SAVE VALIDATION ===\n");
  if (openpart_is_valid_save(save_path))
    printf("Validation:  OK\n");
  else
    printf("Validation:  FAILED (%s)\n", openpart_strerror(op));

  printf("\n=== READ FROM FILE ===\n");
  openpart_t* op2 = openpart_read_data_from_file(save_path, OP_RDONLY, 0);
  if (op2)
    printf("Read from file: OK\n");
  else
    printf("Read from file: FAILED (%s)\n", openpart_strerror(op2));

  printf("Opening: test.img\n");
  openpart_t *op3 = openpart_open("test.img", OP_RDWR | OP_IGNTYPE, 0);
  if (!op3) {
    fprintf(stderr, "Failed to open: %s\n", strerror(errno));
    openpart_close(op);
    openpart_close(op2);
    return 1;
  }
  test_write(op3);

  openpart_close(op);
  openpart_close(op2);
  openpart_close(op3);
  return 0;
}
