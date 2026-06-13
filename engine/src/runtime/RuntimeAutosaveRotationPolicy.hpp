#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeSaveSlotPathPolicy.hpp"

namespace iggy::runtime {

struct RuntimeAutosaveRotationRename {
	RuntimeSaveSlotId from;
	RuntimeSaveSlotId to;
};

struct RuntimeAutosaveRotationPlan {
	bool valid = false;
	RuntimeSaveSlotId writeSlot;
	std::vector<RuntimeSaveSlotId> deleteSlots;
	std::vector<RuntimeAutosaveRotationRename> renames;
};

enum class RuntimeAutosaveRotationIssueCode {
	InvalidMaxSlots,
};

struct RuntimeAutosaveRotationIssue {
	RuntimeAutosaveRotationIssueCode code = RuntimeAutosaveRotationIssueCode::InvalidMaxSlots;
	std::size_t maxSlots = 0;
};

struct RuntimeAutosaveRotationResult {
	RuntimeAutosaveRotationPlan plan;
	std::vector<RuntimeAutosaveRotationIssue> issues;
};

class RuntimeAutosaveRotationPolicy {
public:
	[[nodiscard]] RuntimeAutosaveRotationResult plan(std::size_t maxSlots) const;
};

} // namespace iggy::runtime
