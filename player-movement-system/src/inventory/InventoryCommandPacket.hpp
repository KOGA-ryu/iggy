#pragma once

#include <cstdint>
#include <vector>

namespace dev {

struct InventoryCommandPacket {
	uint8_t commandType = 0;
	uint8_t hasItemId = 0;
	uint32_t itemId = 0;
	uint8_t hasSlot = 0;
	uint8_t slot = 0;
};

using InventoryCommandBytes = std::vector<uint8_t>;

} // namespace dev
