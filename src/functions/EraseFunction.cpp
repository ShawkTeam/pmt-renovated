/*
Copyright 2025 Yağız Zengin

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

#include <cstdlib>
#include <fcntl.h>
#include <cerrno>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include "functions.hpp"

#define EFUN "eraseFunction"

namespace PartitionManager {

bool eraseFunction::init(CLI::App &_app)
{
    LOGN(EFUN, INFO) << "Initializing variables of erase function." << std::endl;
    cmd = _app.add_subcommand("erase", "Writes zero bytes to partition(s)");
    cmd->add_option("partition(s)", partitions, "Partition name(s)")->required()->delimiter(',');
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for writing zero bytes to partition(s)");
    return true;
}

bool eraseFunction::run()
{
    for (const auto& partitionName : partitions) {
        if (!Variables->PartMap->hasPartition(partitionName))
            throw Error("Couldn't find partition: %s", partitionName.data());

        if (Variables->onLogical && !Variables->PartMap->isLogical(partitionName)) {
            if (Variables->forceProcess) LOGN(EFUN, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)." << std::endl;
            else throw Error("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
        }

        bufferSize = (Variables->PartMap->sizeOf(partitionName) % bufferSize == 0) ? bufferSize : 1;
        LOGN(EFUN, INFO) << "Using buffer size: " << bufferSize;

        const int pfd = open(Variables->PartMap->getRealPathOf(partitionName).data(), O_WRONLY);
        if (pfd < 0)
            throw Error("Can't open partition: %s: %s", partitionName.data(), strerror(errno));

        if (!Variables->forceProcess) Helper::confirmPropt("Are you sure you want to continue? This could render your device unusable! Do not continue if you do not know what you are doing!");

        LOGN(EFUN, INFO) << "Writing zero bytes to partition: " << partitionName << std::endl;
        auto* buffer = new char[bufferSize];
        memset(buffer, 0x00, bufferSize);
        ssize_t bytesWritten = 0;
        const uint64_t partitionSize = Variables->PartMap->sizeOf(partitionName);

        while (bytesWritten < partitionSize) {
            size_t toWrite = sizeof(buffer);
            if (partitionSize - bytesWritten < sizeof(buffer)) toWrite = partitionSize - bytesWritten;

            if (const ssize_t result = write(pfd, buffer, toWrite); result == -1) {
                close(pfd);
                delete[] buffer;
                throw Error("Can't write zero bytes to partition: %s: %s", partitionName.data(), strerror(errno));
            } else bytesWritten += result;
        }

        close(pfd);
        delete[] buffer;
    }

    LOGN(EFUN, INFO) << "Operation successfully completed." << std::endl;
    return true;
}

bool eraseFunction::isUsed() const { return cmd->parsed(); }

const char* eraseFunction::name() const { return EFUN; }

} // namespace PartitionManager
