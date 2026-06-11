#pragma once

#include <cstdint>
#include <filesystem>

#include "world/Point.hpp"

namespace dev {

using SaveSlotId = uint32_t;

struct SaveSlotMetadata {
	SaveSlotId slotId = 0;
	std::filesystem::path path;
	bool occupied = false;
	bool valid = false;
	Point playerTile;
	int playerHitPoints = 0;
	std::size_t enemyCount = 0;
};

} // namespace dev
