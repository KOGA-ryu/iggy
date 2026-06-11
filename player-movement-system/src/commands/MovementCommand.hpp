#pragma once

#include <cstdint>
#include <optional>

#include "interaction/DestinationAction.hpp"
#include "world/Point.hpp"

namespace dev {

using PlayerId = uint8_t;

enum class MovementCommandType : uint8_t {
	WalkTo,
	MoveThenAct,
	StandAndAct,
	Stop,
};

struct MovementCommand {
	MovementCommandType type;
	PlayerId playerId;
	Point destination;
	std::optional<DestinationAction> destinationAction;
};

} // namespace dev
