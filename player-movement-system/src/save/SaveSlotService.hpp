#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "save/SaveGameService.hpp"
#include "save/SaveSlot.hpp"

namespace dev {

class SaveSlotService {
public:
	explicit SaveSlotService(std::filesystem::path rootDirectory, SaveGameService saveGameService = SaveGameService {});

	[[nodiscard]] std::filesystem::path pathForSlot(SaveSlotId slotId) const;
	[[nodiscard]] SaveSlotMetadata metadataForSlot(SaveSlotId slotId) const;
	[[nodiscard]] std::vector<SaveSlotMetadata> listSlots(SaveSlotId firstSlotId, std::size_t count) const;
	[[nodiscard]] bool saveSlot(SaveSlotId slotId, const SimulationWorld &world) const;
	[[nodiscard]] bool loadSlot(SaveSlotId slotId, SimulationWorld &world) const;

private:
	[[nodiscard]] std::optional<SimulationSnapshot> loadSnapshotForMetadata(SaveSlotId slotId) const;

	std::filesystem::path rootDirectory_;
	SaveGameService saveGameService_;
};

} // namespace dev
