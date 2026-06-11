#include "InventoryCommandDispatcher.hpp"

namespace dev {

namespace {

InventoryCommandResult Applied(const InventoryCommand &command, EquipmentResult equipmentResult)
{
	return {
		.type = InventoryCommandResultType::Applied,
		.command = command,
		.equipmentResult = equipmentResult,
	};
}

InventoryCommandResult Rejected(const InventoryCommand &command, EquipmentResult equipmentResult = {})
{
	return {
		.type = InventoryCommandResultType::Rejected,
		.command = command,
		.equipmentResult = equipmentResult,
	};
}

bool AppliedEquipmentResult(EquipmentResultType type)
{
	return type == EquipmentResultType::Equipped || type == EquipmentResultType::Unequipped;
}

} // namespace

InventoryCommandDispatcher::InventoryCommandDispatcher(Player &player, InventoryEventSink *eventSink)
    : player_(player)
    , events_(eventSink)
{
}

InventoryCommandResult InventoryCommandDispatcher::dispatch(const InventoryCommand &command) const
{
	switch (command.type) {
	case InventoryCommandType::EquipItem: {
		if (!command.itemId.has_value()) {
			InventoryCommandResult result = Rejected(command);
			events_.emit(result);
			return result;
		}
		EquipmentResult result = equipment_.equip(player_.inventory, *command.itemId);
		InventoryCommandResult commandResult = AppliedEquipmentResult(result.type) ? Applied(command, result) : Rejected(command, result);
		events_.emit(commandResult);
		return commandResult;
	}
	case InventoryCommandType::UnequipSlot: {
		if (!command.slot.has_value()) {
			InventoryCommandResult result = Rejected(command);
			events_.emit(result);
			return result;
		}
		EquipmentResult result = equipment_.unequip(player_.inventory, *command.slot);
		InventoryCommandResult commandResult = AppliedEquipmentResult(result.type) ? Applied(command, result) : Rejected(command, result);
		events_.emit(commandResult);
		return commandResult;
	}
	}

	InventoryCommandResult result = Rejected(command);
	events_.emit(result);
	return result;
}

} // namespace dev
