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
#include <future>
#include <unistd.h>
#include <PartitionManager/PartitionManager.hpp>
#include "functions.hpp"

#define FFUN "flashFunction"

namespace PartitionManager {
    pair flashFunction::runAsync(const std::string &partitionName, const std::string &imageName, int bufferSize) {
        if (!Helper::fileIsExists(imageName)) return {format("Couldn't find image file: %s", imageName.data()), false};
        if (!Variables->PartMap->hasPartition(partitionName)) return {
            format("Couldn't find partition: %s", partitionName.data()), false
        };

        LOGN(FFUN, INFO) << "flashing " << imageName << " to " << partitionName << std::endl;

        if (Variables->onLogical && !Variables->PartMap->isLogical(partitionName)) {
            if (Variables->forceProcess)
                LOGN(FFUN, WARNING) << "Partition " << partitionName <<
                        " is exists but not logical. Ignoring (from --force, -f)." << std::endl;
            else return {
                format("Used --logical (-l) flag but is not logical partition: %s", partitionName.data()), false
            };
        }

        setupBufferSize(bufferSize, imageName);
        LOGN(FFUN, INFO) << "Using buffer size: " << bufferSize;

        const int ffd = open(imageName.data(), O_RDONLY);
        if (ffd < 0) return {format("Can't open image file %s: %s", imageName.data(), strerror(errno)), false};

        const int pfd = open(Variables->PartMap->getRealPathOf(partitionName).data(), O_RDWR | O_TRUNC);
        if (pfd < 0) {
            close(ffd);
            return {format("Can't open partition: %s: %s", partitionName.data(), strerror(errno)), false};
        }

        LOGN(FFUN, INFO) << "Writing image " << imageName << " to partition: " << partitionName << std::endl;
        auto *buffer = new char[bufferSize];
        memset(buffer, 0x00, bufferSize);
        ssize_t bytesRead;
        while ((bytesRead = read(ffd, buffer, bufferSize)) > 0) {
            if (const ssize_t bytesWritten = write(pfd, buffer, bytesRead); bytesWritten != bytesRead) {
                close(pfd);
                close(ffd);
                delete[] buffer;
                return {
                    format("Can't write partition to output file %s: %s", imageName.data(), strerror(errno)), false
                };
            }
        }

        close(pfd);
        close(ffd);
        delete[] buffer;

        return {format("%s is successfully wrote to %s partition\n", imageName.data(), partitionName.data()), true};
    }

    bool flashFunction::init(CLI::App &_app) {
        LOGN(FFUN, INFO) << "Initializing variables of flash function." << std::endl;
        cmd = _app.add_subcommand("flash", "Flash image(s) to partition(s)");
        cmd->add_option("partition(s)", rawPartitions, "Partition name(s)")->required();
        cmd->add_option("imageFile(s)", rawImageNames, "Name(s) of image file(s)")->required();
        cmd->add_option("-b,--buffer-size", bufferSize, "Buffer size for reading image(s) and writing to partition(s)");
        cmd->add_option("-I,--image-directory", imageDirectory, "Directory to find image(s) and flash to partition(s)");

        return true;
    }

    bool flashFunction::run() {
        processCommandLine(partitions, imageNames, rawPartitions, rawImageNames, ',', true);
        if (partitions.size() != imageNames.size())
            throw CLI::ValidationError("You must provide an image file(s) as long as the partition name(s)");

        std::vector<std::future<pair> > futures;
        for (size_t i = 0; i < partitions.size(); i++) {
            std::string imageName = imageNames[i];
            if (!imageDirectory.empty()) imageName.insert(0, imageDirectory + '/');

            futures.push_back(std::async(std::launch::async, runAsync, partitions[i], imageName, bufferSize));
            LOGN(FFUN, INFO) << "Created thread for flashing image to " << partitions[i] << std::endl;
        }

        std::string end;
        bool endResult = true;
        for (auto &future: futures) {
            auto [fst, snd] = future.get();
            if (!snd) {
                end += fst + '\n';
                endResult = false;
            } else print("%s", fst.c_str());
        }

        if (!endResult) throw Error("%s", end.c_str());

        LOGN(FFUN, INFO) << "Operation successfully completed." << std::endl;
        return endResult;
    }

    bool flashFunction::isUsed() const { return cmd->parsed(); }

    const char *flashFunction::name() const { return FFUN; }
} // namespace PartitionManager
