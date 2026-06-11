#pragma once

#include <cstdint>
#include <vector>

namespace dev {

struct SessionCommandPacket {
	uint8_t commandType = 0;
	uint8_t hasNewGameSettings = 0;
	int16_t playerStartX = 0;
	int16_t playerStartY = 0;
	int16_t playerHitPoints = 0;
	uint8_t hasSlotId = 0;
	uint32_t slotId = 0;
	uint8_t hasMode = 0;
	uint8_t mode = 0;
};

using SessionCommandBytes = std::vector<uint8_t>;

} // namespace dev
