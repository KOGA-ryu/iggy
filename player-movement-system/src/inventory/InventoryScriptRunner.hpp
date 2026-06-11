#pragma once

#include <filesystem>
#include <vector>

#include "inventory/InventoryCommandLogFileStore.hpp"
#include "inventory/InventoryCommandReplayer.hpp"

namespace dev {

enum class InventoryScriptRunStatus {
	LoadFailed,
	NoActivePlayer,
	Completed,
};

struct InventoryScriptRunResult {
	InventoryScriptRunStatus status = InventoryScriptRunStatus::LoadFailed;
	std::vector<InventoryCommandResult> commandResults;
};

class InventoryScriptRunner {
public:
	explicit InventoryScriptRunner(
	    InventoryCommandDispatcher &dispatcher,
	    InventoryCommandLogFileStore fileStore = InventoryCommandLogFileStore {});

	[[nodiscard]] InventoryScriptRunResult run(const std::filesystem::path &path) const;

private:
	InventoryCommandLogFileStore fileStore_;
	InventoryCommandReplayer replayer_;
};

} // namespace dev
