#include "InventoryCommandLogCodec.hpp"

namespace dev {

InventoryCommandLogBytes InventoryCommandLogCodec::encode(const InventoryCommandLog &log) const
{
	std::vector<InventoryCommandBytes> packets;
	packets.reserve(log.commands().size());
	for (const InventoryCommand &command : log.commands()) {
		packets.push_back(commandCodec_.encode(commandCodec_.toPacket(command)));
	}

	return frameCodec_.encode(packets);
}

std::optional<InventoryCommandLog> InventoryCommandLogCodec::decode(const InventoryCommandLogBytes &bytes) const
{
	std::optional<std::vector<InventoryCommandBytes>> packetBytes = frameCodec_.decode(bytes);
	if (!packetBytes.has_value())
		return std::nullopt;

	InventoryCommandLog log;
	for (const InventoryCommandBytes &bytes : *packetBytes) {
		std::optional<InventoryCommandPacket> packet = commandCodec_.decode(bytes);
		if (!packet.has_value())
			return std::nullopt;
		std::optional<InventoryCommand> command = commandCodec_.fromPacket(*packet);
		if (!command.has_value())
			return std::nullopt;
		log.record(*command);
	}

	return log;
}

} // namespace dev
