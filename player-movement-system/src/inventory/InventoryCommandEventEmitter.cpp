#include "InventoryCommandEventEmitter.hpp"

namespace dev {

namespace {

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

InventoryCommandEventEmitter::InventoryCommandEventEmitter(InventoryEventSink *eventSink)
    : eventSink_(eventSink)
{
}

void InventoryCommandEventEmitter::emit(const InventoryCommandResult &result) const
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
