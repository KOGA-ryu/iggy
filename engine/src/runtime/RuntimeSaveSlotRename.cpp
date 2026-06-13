#include "runtime/RuntimeSaveSlotRename.hpp"

#include <filesystem>

namespace iggy::runtime {

RuntimeSaveSlotRenameResult RuntimeSaveSlotRename::rename(
	const RuntimeSaveSlotPathPolicyConfig &config,
	RuntimeSaveSlotId source,
	RuntimeSaveSlotId destination) const
{
	RuntimeSaveSlotRenameResult result;
	const RuntimeSaveSlotPathPolicy pathPolicy;
	result.sourcePath = pathPolicy.pathFor(config, source);
	result.destinationPath = pathPolicy.pathFor(config, destination);

	if (result.sourcePath.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeSaveSlotRenameStatus::InvalidSourcePath;
		return result;
	}
	if (result.destinationPath.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeSaveSlotRenameStatus::InvalidDestinationPath;
		return result;
	}

	std::error_code error;
	const bool sourceExists = std::filesystem::exists(result.sourcePath.path, error);
	if (error) {
		result.status = RuntimeSaveSlotRenameStatus::RenameFailed;
		return result;
	}
	if (!sourceExists) {
		result.status = RuntimeSaveSlotRenameStatus::SourceNotFound;
		return result;
	}

	result.sourceExisted = true;
	if (!std::filesystem::is_regular_file(result.sourcePath.path, error) || error) {
		result.status = RuntimeSaveSlotRenameStatus::RenameFailed;
		return result;
	}

	if (result.sourcePath.path == result.destinationPath.path) {
		result.destinationExisted = true;
		result.status = RuntimeSaveSlotRenameStatus::Renamed;
		return result;
	}

	const bool destinationExists = std::filesystem::exists(result.destinationPath.path, error);
	if (error) {
		result.status = RuntimeSaveSlotRenameStatus::RenameFailed;
		return result;
	}
	result.destinationExisted = destinationExists;

	if (destinationExists) {
		if (!std::filesystem::is_regular_file(result.destinationPath.path, error) || error) {
			result.status = RuntimeSaveSlotRenameStatus::DestinationNotRegularFile;
			return result;
		}

		const bool removed = std::filesystem::remove(result.destinationPath.path, error);
		if (!removed || error) {
			result.status = RuntimeSaveSlotRenameStatus::RenameFailed;
			return result;
		}
	}

	std::filesystem::rename(result.sourcePath.path, result.destinationPath.path, error);
	result.status = error ? RuntimeSaveSlotRenameStatus::RenameFailed : RuntimeSaveSlotRenameStatus::Renamed;
	return result;
}

} // namespace iggy::runtime
