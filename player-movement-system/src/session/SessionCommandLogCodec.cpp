#include "SessionCommandLogCodec.hpp"

namespace dev {

SessionCommandLogBytes SessionCommandLogCodec::encode(const SessionCommandLog &log) const
{
	std::vector<SessionCommandBytes> packets;
	packets.reserve(log.commands().size());
	for (const SessionCommand &command : log.commands()) {
		packets.push_back(commandCodec_.encode(commandCodec_.toPacket(command)));
	}

	return frameCodec_.encode(packets);
}

std::optional<SessionCommandLog> SessionCommandLogCodec::decode(const SessionCommandLogBytes &bytes) const
{
	std::optional<std::vector<SessionCommandBytes>> packetBytes = frameCodec_.decode(bytes);
	if (!packetBytes.has_value())
		return std::nullopt;

	SessionCommandLog log;
	for (const SessionCommandBytes &bytes : *packetBytes) {
		std::optional<SessionCommandPacket> packet = commandCodec_.decode(bytes);
		if (!packet.has_value())
			return std::nullopt;
		std::optional<SessionCommand> command = commandCodec_.fromPacket(*packet);
		if (!command.has_value())
			return std::nullopt;
		log.record(*command);
	}

	return log;
}

} // namespace dev
