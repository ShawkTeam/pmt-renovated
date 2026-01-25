/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "diskio.h"

using namespace std;

int DiskIO::OpenForRead(const unsigned char* data, size_t size) {
    this->data = data;
    this->size = size;
    this->off = 0;
    this->isOpen = 1;
    this->openForWrite = 0;
    return 1;
}

void DiskIO::MakeRealName(void) { this->realFilename = this->userFilename; }

int DiskIO::OpenForRead(void) {
  struct stat64 st;

  if (this->isOpen) {
    if (this->openForWrite) {
      Close();
    } else {
      return 1;
    }
  }

  this->fd = open(realFilename.c_str(), O_RDONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
  if (this->fd == -1) {
    this->realFilename = this->userFilename = "";
  } else {
    if (fstat64(fd, &st) == 0) {
      if (!(S_ISDIR(st.st_mode) || S_ISFIFO(st.st_mode) ||
            S_ISSOCK(st.st_mode))) {
        this->isOpen = 1;
      }
    }
  }
  return this->isOpen;
}

int DiskIO::OpenForWrite(void) {
  if ((this->isOpen) && (this->openForWrite)) {
    return 1;
  }

  Close();
  this->fd = open(realFilename.c_str(), O_WRONLY | O_CREAT,
                  S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (fd >= 0) {
    this->isOpen = 1;
    this->openForWrite = 1;
  }
  return this->isOpen;
}

void DiskIO::Close(void) {
  if (this->isOpen) {
    close(this->fd);
  }
  this->isOpen = 0;
  this->openForWrite = 0;
}

int DiskIO::GetBlockSize(void) {
    return 512;
}

int DiskIO::GetPhysBlockSize(void) {
    return 512;
}

uint32_t DiskIO::GetNumHeads(void) {
    return 255;
}

uint32_t DiskIO::GetNumSecsPerTrack(void) {
    return 63;
}

int DiskIO::DiskSync(void) {
    return 1;
}

int DiskIO::Seek(uint64_t sector) {
  int retval = 1;
  off_t seekTo = sector * static_cast<uint64_t>(GetBlockSize());

  if (!isOpen) {
    if (OpenForRead() != 1) {
      retval = 0;
    }
  }

  if (isOpen && seekTo < this->size) {
    off_t sought = lseek64(fd, seekTo, SEEK_SET);
    if (sought != seekTo) {
      retval = 0;
    }
  }

  if (retval) {
    this->off = seekTo;
  }

  return retval;
}

int DiskIO::Read(void* buffer, int numBytes) {
  int actualBytes = 0;
  if (this->size > this->off) {
    actualBytes = std::min(static_cast<int>(this->size - this->off), numBytes);
    memcpy(buffer, this->data + this->off, actualBytes);
  }
    return actualBytes;
}

int DiskIO::Write(void *buffer, int numBytes) {
  int blockSize, i, numBlocks, retval = 0;
  char *tempSpace;

  if ((!this->isOpen) || (!this->openForWrite)) {
    OpenForWrite();
  }

  if (this->isOpen) {
    blockSize = GetBlockSize();
    if (numBytes <= blockSize) {
      numBlocks = 1;
      tempSpace = new char[blockSize];
    } else {
      numBlocks = numBytes / blockSize;
      if ((numBytes % blockSize) != 0)
        numBlocks++;
      tempSpace = new char[numBlocks * blockSize];
    }
    if (tempSpace == NULL) {
      return 0;
    }

    memcpy(tempSpace, buffer, numBytes);
    for (i = numBytes; i < numBlocks * blockSize; i++) {
      tempSpace[i] = 0;
    }
    retval = write(fd, tempSpace, numBlocks * blockSize);

    if (((numBlocks * blockSize) != numBytes) && (retval > 0))
      retval = numBytes;

    delete[] tempSpace;
  }
  return retval;
}

uint64_t DiskIO::DiskSize(int *) {
    return this->size / GetBlockSize();
}
