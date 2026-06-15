#pragma once

#include <filesystem>
#include <string>

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
};

} // namespace iggy::runtime
