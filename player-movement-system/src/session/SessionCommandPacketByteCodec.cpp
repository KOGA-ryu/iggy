#include "SessionCommandPacketByteCodec.hpp"

#include <cstddef>

#include "session/SessionCommandByteStream.hpp"
#include "session/SessionCommandPacketValidator.hpp"

namespace dev {

namespace {

constexpr std::size_t PacketSize = 25;

} // namespace

SessionCommandBytes SessionCommandPacketByteCodec::encode(const SessionCommandPacket &packet) const
{
	SessionCommandBytes bytes;
	bytes.reserve(PacketSize);
	SessionCommandByteWriter writer { bytes };
	writer.writeU8(packet.commandType);
	writer.writeU8(packet.hasNewGameSettings);
	writer.writeU16(static_cast<uint16_t>(packet.playerStartX));
	writer.writeU16(static_cast<uint16_t>(packet.playerStartY));
	writer.writeU16(static_cast<uint16_t>(packet.playerHitPoints));
	writer.writeU8(packet.hasSlotId);
	writer.writeU32(packet.slotId);
	writer.writeU8(packet.hasMode);
	writer.writeU8(packet.mode);
	writer.writeU16(0); // reserved
	writer.writeU32(0); // reserved
	writer.writeU32(0); // reserved
	return bytes;
}

std::optional<SessionCommandPacket> SessionCommandPacketByteCodec::decode(const SessionCommandBytes &bytes) const
{
	if (bytes.size() != PacketSize)
		return std::nullopt;

	SessionCommandByteReader reader { bytes };
	SessionCommandPacket packet;
	uint16_t playerStartX = 0;
	uint16_t playerStartY = 0;
	uint16_t playerHitPoints = 0;
	uint16_t reserved16 = 0;
	uint32_t reserved32 = 0;
	if (!reader.readU8(packet.commandType)
	    || !reader.readU8(packet.hasNewGameSettings)
	    || !reader.readU16(playerStartX)
	    || !reader.readU16(playerStartY)
	    || !reader.readU16(playerHitPoints)
	    || !reader.readU8(packet.hasSlotId)
	    || !reader.readU32(packet.slotId)
	    || !reader.readU8(packet.hasMode)
	    || !reader.readU8(packet.mode)
	    || !reader.readU16(reserved16)
	    || !reader.readU32(reserved32)
	    || !reader.readU32(reserved32)
	    || !reader.consumed())
		return std::nullopt;
	packet.playerStartX = static_cast<int16_t>(playerStartX);
	packet.playerStartY = static_cast<int16_t>(playerStartY);
	packet.playerHitPoints = static_cast<int16_t>(playerHitPoints);

	if (!SessionCommandPacketValidator {}.isValid(packet))
		return std::nullopt;
	return packet;
}

} // namespace dev
