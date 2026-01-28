/*
   Copyright 2026 Yağız Zengin

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <unistd.h>

#include <PartitionManager/PartitionManager.hpp>
#include <cstdarg>

OutUtil::OutUtil() {
  standart = makeFilePointer(stdout);
  error = makeFilePointer(stderr);

  collector.delAfterProgress(standart);
  collector.delAfterProgress(error);
}

void OutUtil::print(const char *format, ...) const {
  va_list args;
  va_start(args, format);
  vfprintf(standart, format, args);
  va_end(args);
}

void OutUtil::println(const char *format, ...) const {
  va_list args;
  va_start(args, format);
  vfprintf(standart, format, args);
  print("\n");
  va_end(args);
}

FILE *OutUtil::getStandartPointer() const { return standart; }
FILE *OutUtil::getErrorPointer() const { return error; }

FILE *OutUtil::makeFilePointer(FILE *real) { return funopen(real, nullptr, writer, nullptr, nullptr); }

int OutUtil::writer(void *cookie, const char *buf, const int size) {
  auto *real = static_cast<FILE *>(cookie);
  if (!PartitionManager::Variables->quietProcess) {
    const size_t ret = fwrite(buf, 1, static_cast<size_t>(size), real);
    fflush(real);
    return static_cast<int>(ret);
  }

  return size;
}
