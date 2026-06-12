#pragma once

#include <vector>

#include "runtime/RuntimeSessionState.hpp"
#include "scene/level/LevelMutationCacheUpdateStep.hpp"
#include "scene/level/LevelTileMutation.hpp"

namespace iggy::runtime {

enum class RuntimeLevelMutationStatus {
	Applied,
	NoMutation,
	Failed,
};

struct RuntimeLevelMutationResult {
	RuntimeLevelMutationStatus status = RuntimeLevelMutationStatus::NoMutation;
	RuntimeSessionState session;
	LevelMutationCacheUpdateResult levelMutation;
};

class RuntimeLevelMutationStep {
public:
	[[nodiscard]] RuntimeLevelMutationResult apply(
		const RuntimeSessionState &session,
		const std::vector<LevelTileEdit> &edits) const;
};

} // namespace iggy::runtime
