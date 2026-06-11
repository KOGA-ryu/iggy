#pragma once

#include <filesystem>

#include "inventory/InventoryEventSink.hpp"
#include "inventory/InventoryScriptRunner.hpp"
#include "player/Player.hpp"

namespace dev {

class RuntimeInventoryScriptIntake {
public:
	[[nodiscard]] InventoryScriptRunResult run(
	    const std::filesystem::path &path,
	    Player *player,
	    InventoryEventSink *eventSink) const;
};

} // namespace dev
