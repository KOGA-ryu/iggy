#pragma once

#include <vector>

#include "runtime/RuntimeAutosaveRotationPolicy.hpp"
#include "runtime/RuntimeSaveSlotDeletion.hpp"
#include "runtime/RuntimeSaveSlotRename.hpp"
#include "runtime/RuntimeSaveSlotStore.hpp"
#include "runtime/RuntimeSessionState.hpp"

namespace iggy::runtime {

enum class RuntimeAutosaveRotationExecutionStatus {
	Saved,
	InvalidPlan,
	DeleteFailed,
	RenameFailed,
	SaveFailed,
};

struct RuntimeAutosaveRotationExecutionResult {
	RuntimeAutosaveRotationExecutionStatus status = RuntimeAutosaveRotationExecutionStatus::InvalidPlan;
	RuntimeAutosaveRotationResult plan;
	std::vector<RuntimeSaveSlotDeletionResult> deletions;
	std::vector<RuntimeSaveSlotRenameResult> renames;
	RuntimeSaveSlotSaveResult save;
};

class RuntimeAutosaveRotationExecutor {
public:
	[[nodiscard]] RuntimeAutosaveRotationExecutionResult saveAutosave(
		const RuntimeSessionState &session,
		const RuntimeSaveSlotPathPolicyConfig &pathConfig,
		std::size_t maxSlots) const;
};

} // namespace iggy::runtime
