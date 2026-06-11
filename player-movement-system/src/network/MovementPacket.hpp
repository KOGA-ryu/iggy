#pragma once

#include <cstdint>
#include <vector>

#include "commands/MovementCommand.hpp"
#include "interaction/DestinationAction.hpp"
#include "targeting/Target.hpp"

namespace dev {

struct MovementPacket {
	uint8_t commandType = 0;
	uint8_t playerId = 0;
	int16_t destinationX = 0;
	int16_t destinationY = 0;
	uint8_t hasDestinationAction = 0;
	uint8_t actionType = 0;
	uint8_t targetType = 0;
	uint32_t targetId = 0;
	int16_t targetX = 0;
	int16_t targetY = 0;
	int16_t rangeTiles = 0;
};

using PacketBytes = std::vector<uint8_t>;

} // namespace dev

