#pragma once

#include <filesystem>
#include <vector>

#include "inventory/InventoryEventSink.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "player/Player.hpp"

namespace dev {

class RuntimeInventoryScriptBatchRunner {
public:
	[[nodiscard]] std::vector<InventoryScriptRunResult> run(
	    std::vector<std::filesystem::path> paths,
	    Player *player,
	    InventoryEventSink *eventSink) const;
};

} // namespace dev
