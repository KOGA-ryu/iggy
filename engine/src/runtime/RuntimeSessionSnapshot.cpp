#include "runtime/RuntimeSessionSnapshot.hpp"

namespace iggy::runtime {

RuntimeSessionSnapshot RuntimeSessionSnapshotBuilder::capture(const RuntimeSessionState &session) const
{
	RuntimeSessionSnapshot snapshot;
	snapshot.level = session.level;
	snapshot.tickIndex = session.tickIndex;
	snapshot.player = session.player;
	snapshot.hasPlayer = session.hasPlayer;
	return snapshot;
}

RuntimeSessionSnapshotRestoreResult RuntimeSessionSnapshotRestorer::restore(
	const RuntimeSessionSnapshot &snapshot,
	const RuntimeSessionSnapshotRestoreConfig &config) const
{
	RuntimeSessionSnapshotRestoreResult result;
	RuntimeSessionBuildConfig adjustedConfig = config.buildConfig;
	adjustedConfig.hasPlayer = snapshot.hasPlayer;
	adjustedConfig.player = snapshot.player;

	result.build = RuntimeSessionBuilder {}.build(snapshot.level, adjustedConfig);
	if (!result.build.built)
		return result;

	result.restored = true;
	result.session = result.build.state;
	result.session.tickIndex = snapshot.tickIndex;
	return result;
}

} // namespace iggy::runtime
