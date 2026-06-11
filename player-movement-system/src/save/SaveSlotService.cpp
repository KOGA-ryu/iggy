#include "SaveSlotService.hpp"

#include <string>

#include "save/SnapshotFileStore.hpp"

namespace dev {

SaveSlotService::SaveSlotService(std::filesystem::path rootDirectory, SaveGameService saveGameService)
    : rootDirectory_(std::move(rootDirectory))
    , saveGameService_(saveGameService)
{
}

std::filesystem::path SaveSlotService::pathForSlot(SaveSlotId slotId) const
{
	return rootDirectory_ / ("slot_" + std::to_string(slotId) + ".igsave");
}

SaveSlotMetadata SaveSlotService::metadataForSlot(SaveSlotId slotId) const
{
	const std::filesystem::path path = pathForSlot(slotId);
	SaveSlotMetadata metadata {
		.slotId = slotId,
		.path = path,
		.occupied = std::filesystem::exists(path),
		.valid = false,
	};

	if (!metadata.occupied)
		return metadata;

	std::optional<SimulationSnapshot> snapshot = loadSnapshotForMetadata(slotId);
	if (!snapshot.has_value())
		return metadata;

	metadata.valid = true;
	metadata.enemyCount = snapshot->enemies.size();
	if (!snapshot->players.empty()) {
		metadata.playerTile = snapshot->players.front().position.tile;
		metadata.playerHitPoints = snapshot->players.front().combatStats.hitPoints;
	}
	return metadata;
}

std::vector<SaveSlotMetadata> SaveSlotService::listSlots(SaveSlotId firstSlotId, std::size_t count) const
{
	std::vector<SaveSlotMetadata> slots;
	slots.reserve(count);
	for (std::size_t offset = 0; offset < count; ++offset) {
		slots.push_back(metadataForSlot(firstSlotId + static_cast<SaveSlotId>(offset)));
	}
	return slots;
}

bool SaveSlotService::saveSlot(SaveSlotId slotId, const SimulationWorld &world) const
{
	std::error_code error;
	std::filesystem::create_directories(rootDirectory_, error);
	if (error)
		return false;

	return saveGameService_.saveWorld(pathForSlot(slotId), world);
}

bool SaveSlotService::loadSlot(SaveSlotId slotId, SimulationWorld &world) const
{
	return saveGameService_.loadWorld(pathForSlot(slotId), world);
}

std::optional<SimulationSnapshot> SaveSlotService::loadSnapshotForMetadata(SaveSlotId slotId) const
{
	return SnapshotFileStore {}.load(pathForSlot(slotId));
}

} // namespace dev
