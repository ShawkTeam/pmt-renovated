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

#define BFUN "backupFunction"

namespace PartitionManager {

bool backupFunction::init(CLI::App &_app)
{
    LOGN(BFUN, INFO) << "Initializing variables of backup function." << std::endl;
    cmd = _app.add_subcommand("backup", "Backup partition(s) to file(s)");
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("output(s)", rawOutputNames, "File name(s) (or path(s)) to save the partition image(s)");
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading partition(s) and writing to file(s)");

    return true;
}

bool backupFunction::run()
{
    processCommandLine(partitions, outputNames, rawPartitions, rawOutputNames, ',', true);
    if (!outputNames.empty() && partitions.size() != outputNames.size())
        throw CLI::ValidationError("You must provide an output name(s) as long as the partition name(s)");

    for (size_t i = 0; i < partitions.size(); i++) {
        std::string partitionName = partitions[i];
        std::string outputName = outputNames.empty() ? partitionName + ".img" : outputNames[i];

        LOGN(BFUN, INFO) << "backupping " << partitionName << " as " << outputName << std::endl;

        if (!Variables->PartMap->hasPartition(partitionName))
            throw Error("Couldn't find partition: %s", partitionName.data());

        if (Variables->onLogical && !Variables->PartMap->isLogical(partitionName)) {
            if (Variables->forceProcess) LOGN(BFUN, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)." << std::endl;
            else throw Error("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
        }

        if (Helper::fileIsExists(outputName) && !Variables->forceProcess) throw Error("%s is exists. Remove it, or use --force (-f) flag.", outputName.data());
        else LOGN(BFUN, INFO) << outputName << " is exists but ignoring (from --force, -f). Re-creating." << std::endl;

        bufferSize = (Variables->PartMap->sizeOf(partitionName) % bufferSize == 0) ? bufferSize : 1;
        LOGN(BFUN, INFO) << "Using buffer size: " << bufferSize << std::endl;

        const int pfd = open(Variables->PartMap->getRealPathOf(partitionName).data(), O_RDONLY);
        if (pfd < 0)
            throw Error("Can't open partition: %s: %s", partitionName.data(), strerror(errno));

        const int ffd = open(outputName.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (ffd < 0) {
            close(pfd);
            throw Error("Can't create/open output file %s: %s", outputName.data(), strerror(errno));
        }

        LOGN(BFUN, INFO) << "Writing partition " << partitionName << " to file: " << outputName << std::endl;
        auto* buffer = new char[bufferSize];
        memset(buffer, 0x00, bufferSize);
        ssize_t bytesRead;
        while ((bytesRead = read(pfd, buffer, bufferSize)) > 0) {
            if (const ssize_t bytesWritten = write(ffd, buffer, bytesRead); bytesWritten != bytesRead) {
                close(pfd);
                close(ffd);
                delete[] buffer;
                throw Error("Can't write partition to output file %s: %s", outputName.data(), strerror(errno));
            }
        }

        close(pfd);
        close(ffd);
        delete[] buffer;
    }

    LOGN(BFUN, INFO) << "Operation successfully completed." << std::endl;
    return true;
}

bool backupFunction::isUsed() const { return cmd->parsed(); }

const char* backupFunction::name() const { return BFUN; }

} // namespace PartitionManager
