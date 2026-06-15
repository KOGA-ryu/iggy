#include "runtime/RuntimeGameplaySnapshot.hpp"

namespace iggy::runtime {

RuntimeGameplaySnapshot RuntimeGameplaySnapshotBuilder::capture(const RuntimeGameplayState &state) const
{
	RuntimeGameplaySnapshot snapshot;
	snapshot.session = RuntimeSessionSnapshotBuilder {}.capture(state.session);
	snapshot.commandQueue = state.commandQueue;
	snapshot.interaction = state.interaction;
	snapshot.inventory = state.inventory;
	snapshot.npcActors = state.npcActors;
	snapshot.npcControls = state.npcControls;
	return snapshot;
}

RuntimeGameplaySnapshotRestoreResult RuntimeGameplaySnapshotRestorer::restore(
	const RuntimeGameplaySnapshot &snapshot,
	const RuntimeGameplaySnapshotRestoreConfig &config) const
{
	RuntimeGameplaySnapshotRestoreResult result;
	result.session = RuntimeSessionSnapshotRestorer {}.restore(snapshot.session, config.session);
	if (!result.session.restored) {
		return result;
	}

	result.restored = true;
	result.state.session = result.session.session;
	result.state.commandQueue = snapshot.commandQueue;
	result.state.interaction = snapshot.interaction;
	result.state.inventory = snapshot.inventory;
	result.state.npcActors = snapshot.npcActors;
	result.state.npcControls = snapshot.npcControls;
	return result;
}

} // namespace iggy::runtime
