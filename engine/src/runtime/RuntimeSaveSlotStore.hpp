#pragma once

#include "runtime/RuntimeSaveSlotPathPolicy.hpp"
#include "runtime/RuntimeSessionSaveLoad.hpp"

namespace iggy::runtime {

enum class RuntimeSaveSlotStoreStatus {
	Saved,
	Loaded,
	InvalidSlotPath,
	SaveFailed,
	LoadFailed,
};

struct RuntimeSaveSlotSaveResult {
	RuntimeSaveSlotStoreStatus status = RuntimeSaveSlotStoreStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	RuntimeSessionSaveResult save;
};

struct RuntimeSaveSlotLoadResult {
	RuntimeSaveSlotStoreStatus status = RuntimeSaveSlotStoreStatus::InvalidSlotPath;
	RuntimeSaveSlotPathResult path;
	RuntimeSessionLoadResult load;
};

class RuntimeSaveSlotStore {
public:
	[[nodiscard]] RuntimeSaveSlotSaveResult save(
		const RuntimeSessionState &session,
		const RuntimeSaveSlotPathPolicyConfig &pathConfig,
		RuntimeSaveSlotId slot) const;

	[[nodiscard]] RuntimeSaveSlotLoadResult load(
		const RuntimeSaveSlotPathPolicyConfig &pathConfig,
		RuntimeSaveSlotId slot,
		const RuntimeSessionSnapshotRestoreConfig &restoreConfig) const;
};

} // namespace iggy::runtime
