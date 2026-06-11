#include "InventoryService.hpp"

#include "simulation/WorldEntityService.hpp"

namespace dev {

std::vector<InventoryTransferResult> InventoryService::applyPickupEvents(SimulationWorld &world, const std::vector<MovementEvent> &events) const
{
	WorldEntityService entities;
	std::vector<InventoryTransferResult> results;
	for (const MovementEvent &event : events) {
		if (!isPickupEvent(event)) {
			results.push_back({ .type = InventoryTransferResultType::InvalidEvent });
			continue;
		}

		InventoryTransferResult result {
			.type = InventoryTransferResultType::InvalidEvent,
			.playerId = event.playerId,
			.target = *event.target,
		};

		if (event.playerId >= world.players.size()) {
			results.push_back(result);
			continue;
		}

		const Item *item = findWorldItem(world, event.target->id);
		if (item == nullptr) {
			result.type = InventoryTransferResultType::MissingItem;
			results.push_back(result);
			continue;
		}

		if (world.players[event.playerId].inventory.full()) {
			result.type = InventoryTransferResultType::RejectedFull;
			results.push_back(result);
			continue;
		}

		world.players[event.playerId].inventory.items.push_back(*item);
		(void)entities.despawnItem(world, event.target->id);
		result.type = InventoryTransferResultType::Transferred;
		results.push_back(result);
	}

	return results;
}

bool InventoryService::isPickupEvent(const MovementEvent &event) const
{
	return event.type == MovementEventType::ActionExecuted
	    && event.actionType == DestinationActionType::Pickup
	    && event.target.has_value()
	    && event.target->type == TargetType::Item;
}

const Item *InventoryService::findWorldItem(const SimulationWorld &world, TargetId id) const
{
	for (const Item &item : world.items) {
		if (item.id == id)
			return &item;
	}
	return nullptr;
}

} // namespace dev
