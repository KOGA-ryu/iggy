#pragma once

#include "inventory/EquipmentService.hpp"
#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryCommandEventEmitter.hpp"
#include "player/Player.hpp"

namespace dev {

class InventoryCommandDispatcher {
public:
	explicit InventoryCommandDispatcher(Player &player, InventoryEventSink *eventSink = nullptr);

	[[nodiscard]] InventoryCommandResult dispatch(const InventoryCommand &command) const;

private:
	Player &player_;
	EquipmentService equipment_;
	InventoryCommandEventEmitter events_;
};

} // namespace dev
