#include "runtime/RuntimeSaveSlotDeletion.hpp"

#include <filesystem>

namespace iggy::runtime {

RuntimeSaveSlotDeletionResult RuntimeSaveSlotDeletion::remove(
	const RuntimeSaveSlotPathPolicyConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeSaveSlotDeletionResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(config, slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeSaveSlotDeletionStatus::InvalidSlotPath;
		return result;
	}

	std::error_code error;
	const bool exists = std::filesystem::exists(result.path.path, error);
	if (error) {
		result.status = RuntimeSaveSlotDeletionStatus::DeleteFailed;
		return result;
	}
	if (!exists) {
		result.status = RuntimeSaveSlotDeletionStatus::NotFound;
		return result;
	}

	result.existed = true;
	if (!std::filesystem::is_regular_file(result.path.path, error) || error) {
		result.status = RuntimeSaveSlotDeletionStatus::DeleteFailed;
		return result;
	}

	const bool removed = std::filesystem::remove(result.path.path, error);
	result.status = removed && !error ? RuntimeSaveSlotDeletionStatus::Deleted : RuntimeSaveSlotDeletionStatus::DeleteFailed;
	return result;
}

} // namespace iggy::runtime
