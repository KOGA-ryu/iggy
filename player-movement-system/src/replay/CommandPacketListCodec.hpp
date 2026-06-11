#pragma once

#include <optional>
#include <vector>

#include "network/MovementPacket.hpp"
#include "replay/CommandLogBytes.hpp"

namespace dev {

class CommandPacketListCodec {
public:
	[[nodiscard]] CommandLogBytes encode(const std::vector<PacketBytes> &packets) const;
	[[nodiscard]] std::optional<std::vector<PacketBytes>> decode(const CommandLogBytes &bytes) const;
};

} // namespace dev
