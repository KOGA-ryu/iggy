#include "runtime/RuntimeAutosaveRotationExecutor.hpp"

namespace iggy::runtime {

RuntimeAutosaveRotationExecutionResult RuntimeAutosaveRotationExecutor::saveAutosave(
	const RuntimeSessionState &session,
	const RuntimeSaveSlotPathPolicyConfig &pathConfig,
	std::size_t maxSlots) const
{
	RuntimeAutosaveRotationExecutionResult result;
	result.plan = RuntimeAutosaveRotationPolicy {}.plan(maxSlots);
	if (!result.plan.plan.valid) {
		result.status = RuntimeAutosaveRotationExecutionStatus::InvalidPlan;
		return result;
	}

	for (const RuntimeSaveSlotId &slot : result.plan.plan.deleteSlots) {
		RuntimeSaveSlotDeletionResult deletion = RuntimeSaveSlotDeletion {}.remove(pathConfig, slot);
		const RuntimeSaveSlotDeletionStatus status = deletion.status;
		result.deletions.push_back(deletion);
		if (status != RuntimeSaveSlotDeletionStatus::Deleted && status != RuntimeSaveSlotDeletionStatus::NotFound) {
			result.status = RuntimeAutosaveRotationExecutionStatus::DeleteFailed;
			return result;
		}
	}

	for (const RuntimeAutosaveRotationRename &rename : result.plan.plan.renames) {
		RuntimeSaveSlotRenameResult renameResult = RuntimeSaveSlotRename {}.rename(pathConfig, rename.from, rename.to);
		const RuntimeSaveSlotRenameStatus status = renameResult.status;
		result.renames.push_back(renameResult);
		if (status != RuntimeSaveSlotRenameStatus::Renamed && status != RuntimeSaveSlotRenameStatus::SourceNotFound) {
			result.status = RuntimeAutosaveRotationExecutionStatus::RenameFailed;
			return result;
		}
	}

	result.save = RuntimeSaveSlotStore {}.save(session, pathConfig, result.plan.plan.writeSlot);
	result.status = result.save.status == RuntimeSaveSlotStoreStatus::Saved ? RuntimeAutosaveRotationExecutionStatus::Saved : RuntimeAutosaveRotationExecutionStatus::SaveFailed;
	return result;
}

} // namespace iggy::runtime
