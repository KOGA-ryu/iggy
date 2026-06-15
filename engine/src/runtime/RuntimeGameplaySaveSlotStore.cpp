#include "runtime/RuntimeGameplaySaveSlotStore.hpp"

namespace iggy::runtime {
namespace {

RuntimeSaveSlotPathPolicyConfig pathConfigFor(const RuntimeGameplaySaveSlotStoreConfig &config)
{
	RuntimeSaveSlotPathPolicyConfig pathConfig;
	pathConfig.baseDirectory = config.baseDirectory;
	pathConfig.extension = config.extension;
	return pathConfig;
}

} // namespace

RuntimeGameplaySaveSlotSaveResult RuntimeGameplaySaveSlotStore::save(
	const RuntimeGameplaySnapshot &snapshot,
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeGameplaySaveSlotSaveResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfigFor(config), slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotStatus::InvalidSlotPath;
		return result;
	}

	result.save = RuntimeGameplaySnapshotSaver {}.save(result.path.path, snapshot);
	result.status = result.save.status == RuntimeGameplaySnapshotSaveStatus::Saved
		? RuntimeGameplaySaveSlotStatus::Saved
		: RuntimeGameplaySaveSlotStatus::SaveFailed;
	return result;
}

RuntimeGameplaySaveSlotSaveResult RuntimeGameplaySaveSlotStore::saveState(
	const RuntimeGameplayState &state,
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId slot) const
{
	return save(RuntimeGameplaySnapshotBuilder {}.capture(state), config, slot);
}

RuntimeGameplaySaveSlotLoadResult RuntimeGameplaySaveSlotStore::load(
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeGameplaySaveSlotLoadResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfigFor(config), slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotStatus::InvalidSlotPath;
		return result;
	}

	result.load = RuntimeGameplaySnapshotLoader {}.load(result.path.path, config.restore);
	result.state = result.load.state;
	result.status = result.load.status == RuntimeGameplaySnapshotLoadStatus::Loaded
		? RuntimeGameplaySaveSlotStatus::Loaded
		: RuntimeGameplaySaveSlotStatus::LoadFailed;
	return result;
}

} // namespace iggy::runtime
