#pragma once

#include "inventory/EquipmentService.hpp"
#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryEventSink.hpp"
#include "player/Player.hpp"

namespace dev {

class InventoryCommandDispatcher {
public:
	explicit InventoryCommandDispatcher(Player &player, InventoryEventSink *eventSink = nullptr);

	[[nodiscard]] InventoryCommandResult dispatch(const InventoryCommand &command) const;

private:
	void emit(const InventoryCommandResult &result) const;

	Player &player_;
	EquipmentService equipment_;
	InventoryEventSink *eventSink_;
};

} // namespace dev
