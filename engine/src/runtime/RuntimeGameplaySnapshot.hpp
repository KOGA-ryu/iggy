#pragma once

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeSessionSnapshot.hpp"

namespace iggy::runtime {

struct RuntimeGameplaySnapshot {
	RuntimeSessionSnapshot session;
	RuntimeCommandQueueState commandQueue;
	RuntimeInteractionState interaction;
	RuntimeInventoryState inventory;
	NpcActorState2DRegistry npcActors;
	NpcActorControlState2DRegistry npcControls;
};

struct RuntimeGameplaySnapshotRestoreConfig {
	RuntimeSessionSnapshotRestoreConfig session;
};

struct RuntimeGameplaySnapshotRestoreResult {
	bool restored = false;
	RuntimeGameplayState state;
	RuntimeSessionSnapshotRestoreResult session;
};

class RuntimeGameplaySnapshotBuilder {
public:
	[[nodiscard]] RuntimeGameplaySnapshot capture(const RuntimeGameplayState &state) const;
};

class RuntimeGameplaySnapshotRestorer {
public:
	[[nodiscard]] RuntimeGameplaySnapshotRestoreResult restore(
		const RuntimeGameplaySnapshot &snapshot,
		const RuntimeGameplaySnapshotRestoreConfig &config) const;
};

} // namespace iggy::runtime
