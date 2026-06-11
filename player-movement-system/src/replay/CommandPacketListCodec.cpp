#include "CommandPacketListCodec.hpp"

#include <cstddef>

namespace dev {

namespace {

constexpr std::size_t PacketSize = 26;

void WriteU16(CommandLogBytes &bytes, uint16_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void WriteU32(CommandLogBytes &bytes, uint32_t value)
{
	WriteU16(bytes, static_cast<uint16_t>(value & 0xFFFFU));
	WriteU16(bytes, static_cast<uint16_t>((value >> 16U) & 0xFFFFU));
}

bool ReadU32(const CommandLogBytes &bytes, uint32_t &value)
{
	if (bytes.size() < 4U)
		return false;
	value = static_cast<uint32_t>(bytes[0])
	    | (static_cast<uint32_t>(bytes[1]) << 8U)
	    | (static_cast<uint32_t>(bytes[2]) << 16U)
	    | (static_cast<uint32_t>(bytes[3]) << 24U);
	return true;
}

} // namespace

CommandLogBytes CommandPacketListCodec::encode(const std::vector<PacketBytes> &packets) const
{
	CommandLogBytes bytes;
	WriteU32(bytes, static_cast<uint32_t>(packets.size()));
	for (const PacketBytes &packet : packets)
		bytes.insert(bytes.end(), packet.begin(), packet.end());
	return bytes;
}

std::optional<std::vector<PacketBytes>> CommandPacketListCodec::decode(const CommandLogBytes &bytes) const
{
	uint32_t commandCount = 0;
	if (!ReadU32(bytes, commandCount))
		return std::nullopt;

	if (bytes.size() != 4U + (static_cast<std::size_t>(commandCount) * PacketSize))
		return std::nullopt;

	std::vector<PacketBytes> packets;
	packets.reserve(commandCount);
	std::size_t packetOffset = 4U;
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
