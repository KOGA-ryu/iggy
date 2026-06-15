#include "runtime/RuntimeGameplaySaveSlotStore.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>

#include "runtime/RuntimeSaveFileIO.hpp"

namespace iggy::runtime {
namespace {

struct SlotDirectory {
	RuntimeSaveSlotKind kind = RuntimeSaveSlotKind::Manual;
	const char *name = "manual";
};

constexpr SlotDirectory SlotDirectories[] {
	{ RuntimeSaveSlotKind::Manual, "manual" },
	{ RuntimeSaveSlotKind::Auto, "auto" },
	{ RuntimeSaveSlotKind::Quick, "quick" },
};

RuntimeSaveSlotPathPolicyConfig pathConfigFor(const RuntimeGameplaySaveSlotStoreConfig &config)
{
	RuntimeSaveSlotPathPolicyConfig pathConfig;
	pathConfig.baseDirectory = config.baseDirectory;
	pathConfig.extension = config.extension;
	return pathConfig;
}

[[nodiscard]] bool hasListedExtension(const std::filesystem::path &path, const std::string &extension)
{
	if (extension.empty())
		return path.extension().empty();
	return path.extension() == extension;
}

[[nodiscard]] std::vector<std::filesystem::path> sortedRegularFiles(const std::filesystem::path &directory)
{
	std::vector<std::filesystem::path> files;
	std::error_code ignored;
	for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(directory, ignored)) {
		if (entry.is_regular_file(ignored))
			files.push_back(entry.path());
	}

	std::sort(files.begin(), files.end(), [](const std::filesystem::path &left, const std::filesystem::path &right) {
		return left.filename().string() < right.filename().string();
	});
	return files;
}

[[nodiscard]] RuntimeGameplaySaveSlotSummary summaryFrom(const RuntimeGameplaySnapshotLoadResult &load)
{
	RuntimeGameplaySaveSlotSummary summary;
	summary.tickIndex = load.state.session.tickIndex;
	summary.hasPlayer = load.state.session.hasPlayer;
	summary.commandFrameCount = load.state.commandQueue.frames.size();
	summary.interactionTargetCount = load.state.interaction.targets.targets().size();
	summary.interactionEffectCount = load.state.interaction.effects.entries().size();
	summary.inventoryStackCount = load.state.inventory.inventory.stacks.size();
	summary.inventoryDropCount = load.state.inventory.drops.drops.size();
	summary.npcActorCount = load.state.npcActors.actors.size();
	summary.npcControlCount = load.state.npcControls.entries.size();
	return summary;
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

RuntimeGameplaySaveSlotInspectResult RuntimeGameplaySaveSlotStore::inspect(
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeGameplaySaveSlotInspectResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfigFor(config), slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
		return result;
	}

	std::error_code error;
	result.exists = std::filesystem::exists(result.path.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::ReadFailed;
		return result;
	}
	if (!result.exists) {
		result.status = RuntimeGameplaySaveSlotManageStatus::Ok;
		return result;
	}

	result.regularFile = std::filesystem::is_regular_file(result.path.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::ReadFailed;
		return result;
	}
	if (!result.regularFile) {
		result.status = RuntimeGameplaySaveSlotManageStatus::ReadFailed;
		return result;
	}

	const RuntimeSaveFileReadResult read = RuntimeSaveFileReader {}.readBytes(result.path.path);
	result.readable = read.status == RuntimeSaveFileIOStatus::Ok;
	result.status = result.readable ? RuntimeGameplaySaveSlotManageStatus::Ok : RuntimeGameplaySaveSlotManageStatus::ReadFailed;
	return result;
}

RuntimeGameplaySaveSlotListResult RuntimeGameplaySaveSlotStore::list(const RuntimeGameplaySaveSlotStoreConfig &config) const
{
	RuntimeGameplaySaveSlotListResult result;
	if (config.baseDirectory.empty()) {
		result.status = RuntimeGameplaySaveSlotManageStatus::BaseDirectoryUnavailable;
		return result;
	}

	result.listed = true;
	std::error_code error;
	if (!std::filesystem::exists(config.baseDirectory, error) || error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::BaseDirectoryUnavailable;
		return result;
	}

	const RuntimeSaveSlotPathPolicyConfig pathConfig = pathConfigFor(config);
	for (const SlotDirectory &slotDirectory : SlotDirectories) {
		const std::filesystem::path directory = config.baseDirectory / slotDirectory.name;
		if (!std::filesystem::exists(directory, error) || error)
			continue;

		for (const std::filesystem::path &path : sortedRegularFiles(directory)) {
			if (!hasListedExtension(path, config.extension))
				continue;

			RuntimeSaveSlotId slot { slotDirectory.kind, config.extension.empty() ? path.filename().string() : path.stem().string() };
			const RuntimeSaveSlotPathResult slotPath = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfig, slot);
			if (slotPath.status != RuntimeSaveSlotPathStatus::Valid || slotPath.path != path)
				continue;

			result.entries.push_back({ slot, path });
		}
	}

	result.status = RuntimeGameplaySaveSlotManageStatus::Ok;
	return result;
}

RuntimeGameplaySaveSlotDeleteResult RuntimeGameplaySaveSlotStore::remove(
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeGameplaySaveSlotDeleteResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfigFor(config), slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
		return result;
	}

	std::error_code error;
	const bool exists = std::filesystem::exists(result.path.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::DeleteFailed;
		return result;
	}
	if (!exists) {
		result.status = RuntimeGameplaySaveSlotManageStatus::NotFound;
		return result;
	}

	result.existed = true;
	if (!std::filesystem::is_regular_file(result.path.path, error) || error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::DeleteFailed;
		return result;
	}

	const bool removed = std::filesystem::remove(result.path.path, error);
	result.deleted = removed && !error;
	result.status = result.deleted ? RuntimeGameplaySaveSlotManageStatus::Ok : RuntimeGameplaySaveSlotManageStatus::DeleteFailed;
	return result;
}

RuntimeGameplaySaveSlotMoveResult RuntimeGameplaySaveSlotStore::rename(
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId source,
	RuntimeSaveSlotId destination) const
{
	RuntimeGameplaySaveSlotMoveResult result;
	const RuntimeSaveSlotPathPolicy pathPolicy;
	const RuntimeSaveSlotPathPolicyConfig pathConfig = pathConfigFor(config);
	result.sourcePath = pathPolicy.pathFor(pathConfig, source);
	result.destinationPath = pathPolicy.pathFor(pathConfig, destination);
	if (result.sourcePath.status != RuntimeSaveSlotPathStatus::Valid || result.destinationPath.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
		return result;
	}

	std::error_code error;
	result.sourceExisted = std::filesystem::exists(result.sourcePath.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::RenameFailed;
		return result;
	}
	if (!result.sourceExisted) {
		result.status = RuntimeGameplaySaveSlotManageStatus::NotFound;
		return result;
	}
	if (!std::filesystem::is_regular_file(result.sourcePath.path, error) || error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::RenameFailed;
		return result;
	}

	if (result.sourcePath.path == result.destinationPath.path) {
		result.destinationExisted = true;
		result.completed = true;
		result.status = RuntimeGameplaySaveSlotManageStatus::Ok;
		return result;
	}

	result.destinationExisted = std::filesystem::exists(result.destinationPath.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::RenameFailed;
		return result;
	}
	if (result.destinationExisted) {
		result.status = RuntimeGameplaySaveSlotManageStatus::AlreadyExists;
		return result;
	}

	std::filesystem::rename(result.sourcePath.path, result.destinationPath.path, error);
	result.completed = !error;
	result.status = result.completed ? RuntimeGameplaySaveSlotManageStatus::Ok : RuntimeGameplaySaveSlotManageStatus::RenameFailed;
	return result;
}

RuntimeGameplaySaveSlotMoveResult RuntimeGameplaySaveSlotStore::copy(
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId source,
	RuntimeSaveSlotId destination) const
{
	RuntimeGameplaySaveSlotMoveResult result;
	const RuntimeSaveSlotPathPolicy pathPolicy;
	const RuntimeSaveSlotPathPolicyConfig pathConfig = pathConfigFor(config);
	result.sourcePath = pathPolicy.pathFor(pathConfig, source);
	result.destinationPath = pathPolicy.pathFor(pathConfig, destination);
	if (result.sourcePath.status != RuntimeSaveSlotPathStatus::Valid || result.destinationPath.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
		return result;
	}

	std::error_code error;
	result.sourceExisted = std::filesystem::exists(result.sourcePath.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::CopyFailed;
		return result;
	}
	if (!result.sourceExisted) {
		result.status = RuntimeGameplaySaveSlotManageStatus::NotFound;
		return result;
	}
	if (!std::filesystem::is_regular_file(result.sourcePath.path, error) || error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::CopyFailed;
		return result;
	}

	result.destinationExisted = std::filesystem::exists(result.destinationPath.path, error);
	if (error) {
		result.status = RuntimeGameplaySaveSlotManageStatus::CopyFailed;
		return result;
	}
	if (result.destinationExisted) {
		result.status = RuntimeGameplaySaveSlotManageStatus::AlreadyExists;
		return result;
	}

	result.completed = std::filesystem::copy_file(result.sourcePath.path, result.destinationPath.path, std::filesystem::copy_options::none, error);
	result.status = result.completed && !error ? RuntimeGameplaySaveSlotManageStatus::Ok : RuntimeGameplaySaveSlotManageStatus::CopyFailed;
	return result;
}

RuntimeGameplaySaveSlotSummaryResult RuntimeGameplaySaveSlotStore::readSummary(
	const RuntimeGameplaySaveSlotStoreConfig &config,
	RuntimeSaveSlotId slot) const
{
	RuntimeGameplaySaveSlotSummaryResult result;
	result.path = RuntimeSaveSlotPathPolicy {}.pathFor(pathConfigFor(config), slot);
	if (result.path.status != RuntimeSaveSlotPathStatus::Valid) {
		result.status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
		return result;
	}

	result.load = RuntimeGameplaySnapshotLoader {}.load(result.path.path, config.restore);
	if (result.load.status != RuntimeGameplaySnapshotLoadStatus::Loaded) {
		result.status = RuntimeGameplaySaveSlotManageStatus::LoadFailed;
		return result;
	}

	result.summary = summaryFrom(result.load);
	result.loaded = true;
	result.status = RuntimeGameplaySaveSlotManageStatus::Ok;
	return result;
}

} // namespace iggy::runtime
