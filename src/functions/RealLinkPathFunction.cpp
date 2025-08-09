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

#include <PartitionManager/PartitionManager.hpp>
#include "functions.hpp"

#define RLPFUN "realPathFunction"

namespace PartitionManager {
	bool realLinkPathFunction::init(CLI::App &_app) {
		LOGN(RLPFUN, INFO) << "Initializing variables of real link path function." << std::endl;
		cmd = _app.add_subcommand("real-linkpath", "Tell real link paths of partition(s)");
		cmd->add_option("partition(s)", partitions, "Partition name(s)")->required();
		return true;
	}

	bool realLinkPathFunction::run() {
		for (const auto &partition: partitions) {
			if (!Variables->PartMap->hasPartition(partition))
				throw Error("Couldn't find partition: %s", partition.data());

			if (Variables->onLogical && !Variables->PartMap->isLogical(partition)) {
				if (Variables->forceProcess)
					LOGN(RLPFUN, WARNING) << "Partition " << partition <<
							" is exists but not logical. Ignoring (from --force, -f)." << std::endl;
				else throw Error("Used --logical (-l) flag but is not logical partition: %s", partition.data());
			}

			println("%s", Variables->PartMap->getRealPathOf(partition).data());
		}

		return true;
	}

	bool realLinkPathFunction::isUsed() const { return cmd->parsed(); }

	const char *realLinkPathFunction::name() const { return RLPFUN; }
} // namespace PartitionManager
