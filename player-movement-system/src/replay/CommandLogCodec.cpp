#include "CommandLogCodec.hpp"

namespace dev {

CommandLogBytes CommandLogCodec::encode(const CommandLog &log) const
{
	std::vector<PacketBytes> packets;
	packets.reserve(log.commands().size());
	for (const MovementCommand &command : log.commands()) {
		packets.push_back(commandCodec_.encode(commandCodec_.toPacket(command)));
	}

	return frameCodec_.encode(packets);
}

std::optional<CommandLog> CommandLogCodec::decode(const CommandLogBytes &bytes) const
{
	std::optional<std::vector<PacketBytes>> packetBytes = frameCodec_.decode(bytes);
	if (!packetBytes.has_value())
		return std::nullopt;

	CommandLog log;
	for (const PacketBytes &bytes : *packetBytes) {
		std::optional<MovementPacket> packet = commandCodec_.decode(bytes);
		if (!packet.has_value())
			return std::nullopt;
		std::optional<MovementCommand> command = commandCodec_.fromPacket(*packet);
		if (!command.has_value())
			return std::nullopt;
		log.record(*command);
	}

	return log;
}

} // namespace dev
