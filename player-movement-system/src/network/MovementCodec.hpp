#pragma once

#include <optional>

#include "commands/MovementCommand.hpp"
#include "network/MovementPacket.hpp"

namespace dev {

class MovementCodec {
public:
	MovementPacket toPacket(const MovementCommand &command) const;
	std::optional<MovementCommand> fromPacket(const MovementPacket &packet) const;

	PacketBytes encode(const MovementPacket &packet) const;
	std::optional<MovementPacket> decode(const PacketBytes &bytes) const;
};

} // namespace dev

