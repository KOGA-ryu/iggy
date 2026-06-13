#pragma once

#include "runtime/RuntimeSaveSlotPathPolicy.hpp"

namespace iggy::runtime {

enum class RuntimeSaveSlotDeletionStatus {
	Deleted,
	NotFound,
	InvalidSlotPath,
	DeleteFailed,
};

struct RuntimeSaveSlotDeletionResult {
	RuntimeSaveSlotDeletionStatus status = RuntimeSaveSlotDeletionStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	bool existed = false;
};

class RuntimeSaveSlotDeletion {
public:
	[[nodiscard]] RuntimeSaveSlotDeletionResult remove(
		const RuntimeSaveSlotPathPolicyConfig &config,
		RuntimeSaveSlotId slot) const;
};

} // namespace iggy::runtime
