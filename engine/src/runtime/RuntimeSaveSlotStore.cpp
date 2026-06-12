#include "runtime/RuntimeSaveSlotStore.hpp"

namespace iggy::runtime {

RuntimeSaveSlotSaveResult RuntimeSaveSlotStore::save(
	const RuntimeSessionState &session,
	const RuntimeSaveSlotPathPolicyConfig &pathConfig,
	RuntimeSaveSlotId slot) const
{
	RuntimeSaveSlotSaveResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfig, slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeSaveSlotStoreStatus::InvalidSlotPath;
		return result;
	}

	result.save = RuntimeSessionSaver {}.save(session, result.path.path);
	result.status = result.save.status == RuntimeSessionSaveStatus::Saved ? RuntimeSaveSlotStoreStatus::Saved : RuntimeSaveSlotStoreStatus::SaveFailed;
	return result;
}

RuntimeSaveSlotLoadResult RuntimeSaveSlotStore::load(
	const RuntimeSaveSlotPathPolicyConfig &pathConfig,
	RuntimeSaveSlotId slot,
	const RuntimeSessionSnapshotRestoreConfig &restoreConfig) const
{
	RuntimeSaveSlotLoadResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfig, slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeSaveSlotStoreStatus::InvalidSlotPath;
		return result;
	}

	result.load = RuntimeSessionLoader {}.load(result.path.path, restoreConfig);
	result.status = result.load.status == RuntimeSessionLoadStatus::Loaded ? RuntimeSaveSlotStoreStatus::Loaded : RuntimeSaveSlotStoreStatus::LoadFailed;
	return result;
}

} // namespace iggy::runtime
