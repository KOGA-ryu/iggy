#pragma once

#include <cstddef>

#include "runtime/RuntimeSessionState.hpp"
#include "scene/level/LevelRuntimeState.hpp"
#include "scene/player/PlayerAgentState.hpp"

namespace iggy::runtime {

struct RuntimeSessionSnapshot {
	LevelRuntimeState level;
	std::size_t tickIndex = 0;
	PlayerAgentState player;
	bool hasPlayer = false;
};

struct RuntimeSessionSnapshotRestoreConfig {
	RuntimeSessionBuildConfig buildConfig;
};

struct RuntimeSessionSnapshotRestoreResult {
	bool restored = false;
	RuntimeSessionState session;
	RuntimeSessionBuildResult build;
};

class RuntimeSessionSnapshotBuilder {
public:
	[[nodiscard]] RuntimeSessionSnapshot capture(const RuntimeSessionState &session) const;
};

class RuntimeSessionSnapshotRestorer {
public:
	[[nodiscard]] RuntimeSessionSnapshotRestoreResult restore(
		const RuntimeSessionSnapshot &snapshot,
		const RuntimeSessionSnapshotRestoreConfig &config) const;
};

} // namespace iggy::runtime
