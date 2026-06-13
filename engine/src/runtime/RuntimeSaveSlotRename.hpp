#pragma once

#include "runtime/RuntimeSaveSlotPathPolicy.hpp"

namespace iggy::runtime {

enum class RuntimeSaveSlotRenameStatus {
	Renamed,
	SourceNotFound,
	InvalidSourcePath,
	InvalidDestinationPath,
	DestinationNotRegularFile,
	RenameFailed,
};

struct RuntimeSaveSlotRenameResult {
	RuntimeSaveSlotRenameStatus status = RuntimeSaveSlotRenameStatus::InvalidSourcePath;
	RuntimeSaveSlotPathResult sourcePath;
	RuntimeSaveSlotPathResult destinationPath;
	bool sourceExisted = false;
	bool destinationExisted = false;
};

class RuntimeSaveSlotRename {
public:
	[[nodiscard]] RuntimeSaveSlotRenameResult rename(
		const RuntimeSaveSlotPathPolicyConfig &config,
		RuntimeSaveSlotId source,
		RuntimeSaveSlotId destination) const;
};

} // namespace iggy::runtime
