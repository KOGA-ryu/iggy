#include "SessionCommandLogFrameCodec.hpp"

#include <cstddef>

#include "session/SessionCommandByteStream.hpp"
#include "session/SessionCommandLogChecksum.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'S';
constexpr uint8_t Magic2 = 'C';
constexpr uint8_t Magic3 = 'L';
constexpr uint32_t Version = 1;
constexpr std::size_t HeaderSize = 12;
constexpr std::size_t PacketSize = 25;

} // namespace

SessionCommandLogBytes SessionCommandLogFrameCodec::encode(const std::vector<SessionCommandBytes> &packets) const
{
	SessionCommandLogBytes payload;
	SessionCommandByteWriter writer { payload };
	writer.writeU8(Magic0);
	writer.writeU8(Magic1);
	writer.writeU8(Magic2);
	writer.writeU8(Magic3);
	writer.writeU32(Version);
	writer.writeU32(static_cast<uint32_t>(packets.size()));

	for (const SessionCommandBytes &packet : packets) {
		payload.insert(payload.end(), packet.begin(), packet.end());
	}

	SessionCommandLogChecksum {}.appendTo(payload);
	return payload;
}

std::optional<std::vector<SessionCommandBytes>> SessionCommandLogFrameCodec::decode(const SessionCommandLogBytes &bytes) const
{
	if (bytes.size() < HeaderSize + 4U)
		return std::nullopt;

	const std::size_t payloadSize = bytes.size() - 4U;
	if (!SessionCommandLogChecksum {}.hasValidTrailingChecksum(bytes, payloadSize))
		return std::nullopt;

	SessionCommandByteReader reader { bytes };
	uint8_t magic0 = 0;
	uint8_t magic1 = 0;
	uint8_t magic2 = 0;
	uint8_t magic3 = 0;
	uint32_t version = 0;
	uint32_t commandCount = 0;
	if (!reader.readU8(magic0)
	    || !reader.readU8(magic1)
	    || !reader.readU8(magic2)
	    || !reader.readU8(magic3)
	    || !reader.readU32(version)
	    || !reader.readU32(commandCount))
		return std::nullopt;

	if (magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3 || version != Version)
		return std::nullopt;

	if (payloadSize != HeaderSize + (static_cast<std::size_t>(commandCount) * PacketSize))
		return std::nullopt;

	std::vector<SessionCommandBytes> packets;
	packets.reserve(commandCount);
	std::size_t packetOffset = reader.offset();
	for (uint32_t index = 0; index < commandCount; ++index) {
		packets.push_back({
		    bytes.begin() + static_cast<std::ptrdiff_t>(packetOffset),
		    bytes.begin() + static_cast<std::ptrdiff_t>(packetOffset + PacketSize),
		});
		packetOffset += PacketSize;
	}

	return packets;
}

} // namespace dev
