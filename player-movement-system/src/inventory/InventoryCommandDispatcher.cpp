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

InventoryEventType EventTypeFor(const InventoryCommandResult &result)
{
	if (result.type == InventoryCommandResultType::Rejected)
		return InventoryEventType::Rejected;
	if (result.equipmentResult.type == EquipmentResultType::Equipped)
		return InventoryEventType::Equipped;
	if (result.equipmentResult.type == EquipmentResultType::Unequipped)
		return InventoryEventType::Unequipped;
	return InventoryEventType::Rejected;
}

} // namespace

InventoryCommandDispatcher::InventoryCommandDispatcher(Player &player, InventoryEventSink *eventSink)
    : player_(player)
    , eventSink_(eventSink)
{
}

InventoryCommandResult InventoryCommandDispatcher::dispatch(const InventoryCommand &command) const
{
	switch (command.type) {
	case InventoryCommandType::EquipItem: {
		if (!command.itemId.has_value()) {
			InventoryCommandResult result = Rejected(command);
			emit(result);
			return result;
		}
		EquipmentResult result = equipment_.equip(player_.inventory, *command.itemId);
		InventoryCommandResult commandResult = AppliedEquipmentResult(result.type) ? Applied(command, result) : Rejected(command, result);
		emit(commandResult);
		return commandResult;
	}
	case InventoryCommandType::UnequipSlot: {
		if (!command.slot.has_value()) {
			InventoryCommandResult result = Rejected(command);
			emit(result);
			return result;
		}
		EquipmentResult result = equipment_.unequip(player_.inventory, *command.slot);
		InventoryCommandResult commandResult = AppliedEquipmentResult(result.type) ? Applied(command, result) : Rejected(command, result);
		emit(commandResult);
		return commandResult;
	}
	}

	InventoryCommandResult result = Rejected(command);
	emit(result);
	return result;
}

void InventoryCommandDispatcher::emit(const InventoryCommandResult &result) const
{
	if (eventSink_ == nullptr)
		return;
	eventSink_->emit({
	    .type = EventTypeFor(result),
	    .commandType = result.command.type,
	    .commandResult = result.type,
	    .equipmentResult = result.equipmentResult.type,
	    .itemId = result.equipmentResult.itemId,
	    .slot = result.equipmentResult.slot,
	});
}

} // namespace dev
