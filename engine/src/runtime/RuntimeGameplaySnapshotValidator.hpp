#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplaySnapshot.hpp"
#include "runtime/RuntimeSessionSnapshotValidator.hpp"
#include "scene/npc/NpcActorFrameState2D.hpp"

namespace iggy::runtime {

enum class RuntimeGameplaySnapshotIssueCode {
	SessionInvalid,
	NpcActorOutOfBounds,
	NpcActorMissingControl,
	NpcControlMissingActor,
};

struct RuntimeGameplaySnapshotIssue {
	RuntimeGameplaySnapshotIssueCode code = RuntimeGameplaySnapshotIssueCode::SessionInvalid;
	std::size_t index = 0;
	RuntimeSessionSnapshotIssue sessionIssue;
	NpcActorFrameState2DIssue npcFrameIssue;
};

struct RuntimeGameplaySnapshotValidationResult {
	bool valid = false;
	RuntimeSessionSnapshotValidationResult session;
	NpcActorFrameState2DProjectionResult npcFrameState;
	std::vector<RuntimeGameplaySnapshotIssue> issues;
};

class RuntimeGameplaySnapshotValidator {
public:
	[[nodiscard]] RuntimeGameplaySnapshotValidationResult validate(
		const RuntimeGameplaySnapshot &snapshot) const;
};

} // namespace iggy::runtime
