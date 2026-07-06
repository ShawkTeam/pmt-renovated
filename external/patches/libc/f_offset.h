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

#ifndef _F_OFFSET_H
#define _F_OFFSET_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <android/api-level.h>

#if defined(__ANDROID__) && defined(__ANDROID_API__) && !defined(__LP64__) && __ANDROID_API__ <= 23 && _FILE_OFFSET_BITS == 64
__BEGIN_DECLS

inline int __attribute__((always_inline)) fseeko(FILE* __fp, off_t __offset, int __whence) {
  int fd = fileno(__fp);
  if (fd == -1) return -1;
  fflush(__fp);
  off64_t res = lseek64(fd, static_cast<off64_t>(__offset), __whence);
  return (res == -1) ? -1 : 0;
}

inline off_t __attribute__((always_inline)) ftello(FILE* __fp) {
  int fd = fileno(__fp);
  if (fd == -1) return -1;
  off64_t res = lseek64(fd, 0, SEEK_CUR);
  return static_cast<off_t>(res);
}

__END_DECLS
#if defined(__cplusplus)
namespace std {
  using ::fseeko;
  using ::ftello;
}
#endif

#endif // __ANDROID_API__ <= 23
#endif // _F_OFFSET_H
