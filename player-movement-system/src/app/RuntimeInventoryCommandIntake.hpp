#pragma once

#include <vector>

#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryEventSink.hpp"
#include "player/Player.hpp"

namespace dev {

class RuntimeInventoryCommandIntake {
public:
	[[nodiscard]] std::vector<InventoryCommandResult> dispatch(
	    std::vector<InventoryCommand> commands,
	    Player *player,
	    InventoryEventSink *eventSink) const;
};

} // namespace dev
