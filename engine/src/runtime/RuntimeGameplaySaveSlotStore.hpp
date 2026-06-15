#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplaySnapshotSaveLoad.hpp"
#include "runtime/RuntimeSaveSlotPathPolicy.hpp"

namespace iggy::runtime {

enum class RuntimeGameplaySaveSlotStatus {
	Saved,
	Loaded,
	InvalidSlotPath,
	SaveFailed,
	LoadFailed,
};

enum class RuntimeGameplaySaveSlotManageStatus {
	Ok,
	InvalidSlotPath,
	BaseDirectoryUnavailable,
	NotFound,
	AlreadyExists,
	ReadFailed,
	WriteFailed,
	DeleteFailed,
	RenameFailed,
	CopyFailed,
	LoadFailed,
};

struct RuntimeGameplaySaveSlotStoreConfig {
	std::filesystem::path baseDirectory;
	std::string extension = ".iggygameplay";
	RuntimeGameplaySnapshotRestoreConfig restore;
};

struct RuntimeGameplaySaveSlotSaveResult {
	RuntimeGameplaySaveSlotStatus status = RuntimeGameplaySaveSlotStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	RuntimeGameplaySnapshotSaveResult save;
};

struct RuntimeGameplaySaveSlotLoadResult {
	RuntimeGameplaySaveSlotStatus status = RuntimeGameplaySaveSlotStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	RuntimeGameplaySnapshotLoadResult load;
	RuntimeGameplayState state;
};

struct RuntimeGameplaySaveSlotInspectResult {
	RuntimeGameplaySaveSlotManageStatus status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	bool exists = false;
	bool regularFile = false;
	bool readable = false;
};

struct RuntimeGameplaySaveSlotListEntry {
	RuntimeSaveSlotId slot;
	std::filesystem::path path;
};

struct RuntimeGameplaySaveSlotListResult {
	RuntimeGameplaySaveSlotManageStatus status = RuntimeGameplaySaveSlotManageStatus::BaseDirectoryUnavailable;
	std::vector<RuntimeGameplaySaveSlotListEntry> entries;
	bool listed = false;
};

struct RuntimeGameplaySaveSlotDeleteResult {
	RuntimeGameplaySaveSlotManageStatus status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	bool existed = false;
	bool deleted = false;
};

struct RuntimeGameplaySaveSlotMoveResult {
	RuntimeGameplaySaveSlotManageStatus status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult sourcePath;
	RuntimeSaveSlotPathResult destinationPath;
	bool sourceExisted = false;
	bool destinationExisted = false;
	bool completed = false;
};

struct RuntimeGameplaySaveSlotSummary {
	std::uint64_t tickIndex = 0;
	bool hasPlayer = false;
	std::size_t commandFrameCount = 0;
	std::size_t interactionTargetCount = 0;
	std::size_t interactionEffectCount = 0;
	std::size_t inventoryStackCount = 0;
	std::size_t inventoryDropCount = 0;
	std::size_t npcActorCount = 0;
	std::size_t npcControlCount = 0;
};

struct RuntimeGameplaySaveSlotSummaryResult {
	RuntimeGameplaySaveSlotManageStatus status = RuntimeGameplaySaveSlotManageStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	RuntimeGameplaySnapshotLoadResult load;
	RuntimeGameplaySaveSlotSummary summary;
	bool loaded = false;
};

class RuntimeGameplaySaveSlotStore {
public:
	[[nodiscard]] RuntimeGameplaySaveSlotSaveResult save(
		const RuntimeGameplaySnapshot &snapshot,
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId slot) const;

	[[nodiscard]] RuntimeGameplaySaveSlotSaveResult saveState(
		const RuntimeGameplayState &state,
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId slot) const;

	[[nodiscard]] RuntimeGameplaySaveSlotLoadResult load(
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId slot) const;

	[[nodiscard]] RuntimeGameplaySaveSlotInspectResult inspect(
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId slot) const;

	[[nodiscard]] RuntimeGameplaySaveSlotListResult list(const RuntimeGameplaySaveSlotStoreConfig &config) const;

	[[nodiscard]] RuntimeGameplaySaveSlotDeleteResult remove(
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId slot) const;

	[[nodiscard]] RuntimeGameplaySaveSlotMoveResult rename(
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId source,
		RuntimeSaveSlotId destination) const;

	[[nodiscard]] RuntimeGameplaySaveSlotMoveResult copy(
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId source,
		RuntimeSaveSlotId destination) const;

	[[nodiscard]] RuntimeGameplaySaveSlotSummaryResult readSummary(
		const RuntimeGameplaySaveSlotStoreConfig &config,
		RuntimeSaveSlotId slot) const;
};

} // namespace iggy::runtime
