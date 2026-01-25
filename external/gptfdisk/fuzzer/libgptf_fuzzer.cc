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

#include <fstream>
#include <iostream>
#include <functional>
#include "diskio.h"
#include "gpt.h"
#include "parttypes.h"

#include <fuzzer/FuzzedDataProvider.h>

const int8_t kQuiet = 1;
const int8_t kMinRuns = 1;
const int8_t kGPTMaxRuns = 24;
const int16_t kMaxByte = 256;
const std::string kShowCommand = "show";
const std::string kGetCommand = "get";
const std::string kTempFile = "/dev/tempfile";
const std::string kNull = "/dev/null";
const std::string kBackup = "/dev/gptbackup";
const std::string kDoesNotExist = "/dev/does_not_exist";

std::ofstream silence(kNull);

class GptfFuzzer {
public:
  GptfFuzzer(const uint8_t *data, size_t size) : mFdp(data, size) {
    mDisk.OpenForRead(static_cast<const unsigned char *>(data), size);
  }

  ~GptfFuzzer() { mDisk.Close(); }

  void process();

private:
  void init();
  FuzzedDataProvider mFdp;
  DiskIO mDisk;
  GPTData mGptData;
};

void GptfFuzzer::init() {
  if (mFdp.ConsumeBool()) {
    mGptData.SetDisk(mDisk);
  } else {
    mGptData.SetDisk(kTempFile);
  }

  uint64_t startSector = mFdp.ConsumeIntegral<uint64_t>();
  uint64_t endSector =
      mFdp.ConsumeIntegralInRange<uint64_t>(startSector, UINT64_MAX);
  mGptData.CreatePartition(mFdp.ConsumeIntegral<uint8_t>() /* partNum */,
                           startSector, endSector);

  const UnicodeString name = mFdp.ConsumeRandomLengthString(NAME_SIZE);
  uint8_t partNum = mFdp.ConsumeIntegral<uint8_t>();
  if (mGptData.SetName(partNum, name)) {
    PartType pType;
    mGptData.ChangePartType(partNum, pType);
  }

  if (mFdp.ConsumeBool()) {
    mGptData.SetAlignment(mFdp.ConsumeIntegral<uint32_t>() /* n */);
  }

  if (mFdp.ConsumeBool()) {
    GUIDData gData(mFdp.ConsumeRandomLengthString(kMaxByte));
    gData.Randomize();
    mGptData.SetDiskGUID(gData);
    mGptData.SaveGPTBackup(kBackup);
    mGptData.SetPartitionGUID(mFdp.ConsumeIntegral<uint8_t>() /* pn */, gData);
  }

  if (mFdp.ConsumeBool()) {
    mGptData.RandomizeGUIDs();
  }

  if (mFdp.ConsumeBool()) {
    mGptData.LoadGPTBackup(kBackup);
  }

  if (mFdp.ConsumeBool()) {
    mGptData.SaveGPTData(kQuiet);
  }

  if (mFdp.ConsumeBool()) {
    mGptData.SaveMBR();
  }
}

void GptfFuzzer::process() {
  init();
  int8_t runs = mFdp.ConsumeIntegralInRange<int32_t>(kMinRuns, kGPTMaxRuns);

  while (--runs && mFdp.remaining_bytes()) {
    auto invokeGPTAPI = mFdp.PickValueInArray<const std::function<void()>>({
        [&]() {
          mGptData.XFormDisklabel(
              mFdp.ConsumeIntegral<uint8_t>() /* partNum */);
        },
        [&]() {
          mGptData.OnePartToMBR(mFdp.ConsumeIntegral<uint8_t>() /* gptPart */,
                                mFdp.ConsumeIntegral<uint8_t>() /* mbrPart */);
        },
        [&]() {
          uint32_t numSegments;
          uint64_t largestSegment;
          mGptData.FindFreeBlocks(&numSegments, &largestSegment);
        },
        [&]() {
          mGptData.FindFirstInLargest();
        },
        [&]() {
          mGptData.FindLastAvailable();
        },
        [&]() {
          mGptData.FindFirstFreePart();
        },
        [&]() {
          mGptData.MoveMainTable(
              mFdp.ConsumeIntegral<uint64_t>() /* pteSector */);
        },
        [&]() {
          mGptData.Verify();
        },
        [&]() {
          mGptData.SortGPT();
        },
        [&]() {
          std::string command = mFdp.ConsumeBool() ? kShowCommand : kGetCommand;
          std::string randomCommand = mFdp.ConsumeRandomLengthString(kMaxByte);
          mGptData.ManageAttributes(
              mFdp.ConsumeIntegral<uint8_t>() /* partNum */,
              mFdp.ConsumeBool() ? command : randomCommand,
              mFdp.ConsumeRandomLengthString(kMaxByte) /* bits */);
        },
    });
    invokeGPTAPI();
  }
  if (mFdp.ConsumeBool()) {
    mGptData.LoadPartitions(kDoesNotExist);
  }
  if (mFdp.ConsumeBool()) {
    mGptData.SwapPartitions(mFdp.ConsumeIntegral<uint8_t>() /* partNum1 */,
                            mFdp.ConsumeIntegral<uint8_t>() /* partNum2 */);
  }
  mGptData.DeletePartition(mFdp.ConsumeIntegral<uint8_t>() /* partNum */);
  mGptData.DestroyMBR();
  mGptData.DestroyGPT();
}

extern "C" int LLVMFuzzerInitialize(int *, char ***) {
  std::cout.rdbuf(silence.rdbuf());
  std::cerr.rdbuf(silence.rdbuf());
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  GptfFuzzer gptfFuzzer(data, size);
  gptfFuzzer.process();
  return 0;
}
