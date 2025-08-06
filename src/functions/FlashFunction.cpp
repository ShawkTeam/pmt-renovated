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

#define FFUN "flashFunction"

namespace PartitionManager {

bool flashFunction::init(CLI::App &_app)
{
    LOGN(FFUN, INFO) << "Initializing variables of flash function." << std::endl;
    cmd = _app.add_subcommand("flash", "Flash image(s) to partition(s)");
    cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
    cmd->add_option("imageFile(s)", rawImageNames, "Name(s) of image file(s)")->required()->check([&](const std::string& val) {
        const std::vector<std::string> inputs = splitIfHasDelim(val, ',');
        for (const auto& input : inputs) {
            if (!Helper::fileIsExists(input)) return std::string("Couldn't find image file: " + input);
        }
        return std::string();
    });
    cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading image(s) and writing to partition(s)");

    return true;
}

bool flashFunction::run()
{
    processCommandLine(partitions, imageNames, rawPartitions, rawImageNames, ',', true);
    if (partitions.size() != imageNames.size())
        throw CLI::ValidationError("You must provide an image file(s) as long as the partition name(s)");

    for (size_t i = 0; i < partitions.size(); i++) {
        std::string& partitionName = partitions[i];
        std::string& imageName = imageNames[i];

        LOGN(FFUN, INFO) << "flashing " << imageName << " to " << partitionName << std::endl;

        if (!Variables->PartMap->hasPartition(partitionName))
            throw Error("Couldn't find partition: %s", partitionName.data());

        if (Variables->onLogical && !Variables->PartMap->isLogical(partitionName)) {
            if (Variables->forceProcess) LOGN(FFUN, WARNING) << "Partition " << partitionName << " is exists but not logical. Ignoring (from --force, -f)." << std::endl;
            else throw Error("Used --logical (-l) flag but is not logical partition: %s", partitionName.data());
        }

        bufferSize = (Helper::fileSize(imageName) % bufferSize == 0) ? bufferSize : 1;
        LOGN(FFUN, INFO) << "Using buffer size: " << bufferSize;

        const int ffd = open(imageName.data(), O_RDONLY);
        if (ffd < 0) throw Error("Can't open image file %s: %s", imageName.data(), strerror(errno));

        const int pfd = open(Variables->PartMap->getRealPathOf(partitionName).data(), O_RDWR | O_TRUNC);
        if (pfd < 0) {
            close(ffd);
            throw Error("Can't open partition: %s: %s", partitionName.data(), strerror(errno));
        }

        LOGN(FFUN, INFO) << "Writing image " << imageName << " to partition: " << partitionName << std::endl;
        auto* buffer = new char[bufferSize];
        memset(buffer, 0x00, bufferSize);
        ssize_t bytesRead;
        while ((bytesRead = read(ffd, buffer, bufferSize)) > 0) {
            if (const ssize_t bytesWritten = write(pfd, buffer, bytesRead); bytesWritten != bytesRead) {
                close(pfd);
                close(ffd);
                delete[] buffer;
                throw Error("Can't write partition to output file %s: %s", imageName.data(), strerror(errno));
            }
        }
        
        close(pfd);
        close(ffd);
        delete[] buffer;
    }

    LOGN(FFUN, INFO) << "Operation successfully completed." << std::endl;
    return true;
}

bool flashFunction::isUsed() const { return cmd->parsed(); }

const char* flashFunction::name() const { return FFUN; }

} // namespace PartitionManager

