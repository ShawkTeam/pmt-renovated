/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <fstream>
#include <iostream>
#include <functional>
#include "diskio.h"
#include "mbr.h"

#include <fuzzer/FuzzedDataProvider.h>

const std::string kTempFile = "/dev/tempfile";
const std::string kNull = "/dev/null";

std::ofstream silence(kNull);

class BasicMBRFuzzer {
public:
  BasicMBRFuzzer(const uint8_t *data, size_t size) : mFdp(data, size) {
    mDisk.OpenForRead(static_cast<const unsigned char *>(data), size);
  }

  ~BasicMBRFuzzer() { mDisk.Close(); }

  void process();

private:
  DiskIO mDisk;
  FuzzedDataProvider mFdp;
};

void BasicMBRFuzzer::process() {
  BasicMBRData mbrData;
  if (mFdp.ConsumeBool()) {
    BasicMBRData mbrDataFile(kTempFile);
    mbrData = mbrDataFile;
  }

  bool isLegal = false;

  while (mFdp.remaining_bytes()) {
    auto invokeMBRAPI = mFdp.PickValueInArray<const std::function<void()>>({
        [&]() {
          mbrData.SetDisk(&mDisk);
        },
        [&]() {
          if (mDisk.OpenForWrite(kTempFile)) {
            mbrData.WriteMBRData(kTempFile);
          }
          mbrData.ReadMBRData(&mDisk);
        },
        [&]() {
          uint32_t low, high;
          mbrData.GetPartRange(&low, &high);
        },
        [&]() {
          mbrData.MakeBiggestPart(mFdp.ConsumeIntegral<uint8_t>() /* index */,
                                  mFdp.ConsumeIntegral<uint8_t>() /* type */);
        },
        [&]() {
          mbrData.SetPartType(mFdp.ConsumeIntegral<uint8_t>() /* num */,
                              mFdp.ConsumeIntegral<uint8_t>() /* type */);
        },
        [&]() {
          mbrData.FindFirstInFree(mFdp.ConsumeIntegral<uint64_t>() /* start */);
        },
        [&]() {
          mbrData.GetFirstSector(mFdp.ConsumeIntegral<uint8_t>() /* index */);
        },
        [&]() {
          if (!isLegal) {
            mbrData.MakeItLegal();
            isLegal = true;
          }
        },
    });
    invokeMBRAPI();
  }
  mbrData.BlankGPTData();
}

extern "C" int LLVMFuzzerInitialize(int *, char ***) {
  std::cout.rdbuf(silence.rdbuf());
  std::cerr.rdbuf(silence.rdbuf());
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  BasicMBRFuzzer basicMBRFuzzer(data, size);
  basicMBRFuzzer.process();
  return 0;
}
