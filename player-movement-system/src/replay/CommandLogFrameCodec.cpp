#include "CommandLogFrameCodec.hpp"

#include <cstddef>

#include "replay/CommandLogChecksum.hpp"
#include "replay/CommandPacketListCodec.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'M';
constexpr uint8_t Magic2 = 'C';
constexpr uint8_t Magic3 = 'L';
constexpr uint32_t Version = 1;
constexpr std::size_t HeaderSize = 8;

void WriteU8(CommandLogBytes &bytes, uint8_t value)
{
	bytes.push_back(value);
}

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

bool ReadU8(const CommandLogBytes &bytes, std::size_t &offset, uint8_t &value)
{
	if (offset + 1U > bytes.size())
		return false;
	value = bytes[offset++];
	return true;
}

bool ReadU32(const CommandLogBytes &bytes, std::size_t &offset, uint32_t &value)
{
	if (offset + 4U > bytes.size())
		return false;
	value = static_cast<uint32_t>(bytes[offset])
	    | (static_cast<uint32_t>(bytes[offset + 1U]) << 8U)
	    | (static_cast<uint32_t>(bytes[offset + 2U]) << 16U)
	    | (static_cast<uint32_t>(bytes[offset + 3U]) << 24U);
	offset += 4U;
	return true;
}

} // namespace

CommandLogBytes CommandLogFrameCodec::encode(const std::vector<PacketBytes> &packets) const
{
	CommandLogBytes payload;
	WriteU8(payload, Magic0);
	WriteU8(payload, Magic1);
	WriteU8(payload, Magic2);
	WriteU8(payload, Magic3);
	WriteU32(payload, Version);

	const CommandLogBytes packetList = CommandPacketListCodec {}.encode(packets);
	payload.insert(payload.end(), packetList.begin(), packetList.end());

	CommandLogChecksum {}.appendTo(payload);
	return payload;
}

std::optional<std::vector<PacketBytes>> CommandLogFrameCodec::decode(const CommandLogBytes &bytes) const
{
	if (bytes.size() < HeaderSize + 4U)
		return std::nullopt;

	const std::size_t payloadSize = bytes.size() - 4U;
	if (!CommandLogChecksum {}.hasValidTrailingChecksum(bytes, payloadSize))
		return std::nullopt;

	std::size_t offset = 0;
	uint8_t magic0 = 0;
	uint8_t magic1 = 0;
	uint8_t magic2 = 0;
	uint8_t magic3 = 0;
	uint32_t version = 0;
	if (!ReadU8(bytes, offset, magic0)
	    || !ReadU8(bytes, offset, magic1)
	    || !ReadU8(bytes, offset, magic2)
	    || !ReadU8(bytes, offset, magic3)
	    || !ReadU32(bytes, offset, version))
		return std::nullopt;

	if (magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3 || version != Version)
		return std::nullopt;

	return CommandPacketListCodec {}.decode({
	    bytes.begin() + static_cast<std::ptrdiff_t>(HeaderSize),
	    bytes.begin() + static_cast<std::ptrdiff_t>(payloadSize),
	});
}

} // namespace dev
