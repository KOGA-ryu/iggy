#include "InventoryCommandLogCodec.hpp"

#include <cstddef>

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'I';
constexpr uint8_t Magic2 = 'C';
constexpr uint8_t Magic3 = 'L';
constexpr uint32_t Version = 1;
constexpr std::size_t HeaderSize = 12;
constexpr std::size_t PacketSize = 16;
constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

void WriteU8(InventoryCommandLogBytes &bytes, uint8_t value)
{
	bytes.push_back(value);
}

void WriteU16(InventoryCommandLogBytes &bytes, uint16_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void WriteU32(InventoryCommandLogBytes &bytes, uint32_t value)
{
	WriteU16(bytes, static_cast<uint16_t>(value & 0xFFFFU));
	WriteU16(bytes, static_cast<uint16_t>((value >> 16U) & 0xFFFFU));
}

uint8_t ReadU8(const InventoryCommandLogBytes &bytes, std::size_t &offset)
{
	return bytes[offset++];
}

uint16_t ReadU16(const InventoryCommandLogBytes &bytes, std::size_t &offset)
{
	const uint16_t value = static_cast<uint16_t>(bytes[offset])
	    | (static_cast<uint16_t>(bytes[offset + 1U]) << 8U);
	offset += 2U;
	return value;
}

uint32_t ReadU32(const InventoryCommandLogBytes &bytes, std::size_t &offset)
{
	const uint32_t low = ReadU16(bytes, offset);
	const uint32_t high = ReadU16(bytes, offset);
	return low | (high << 16U);
}

uint32_t ChecksumOf(const InventoryCommandLogBytes &bytes, std::size_t length)
{
	uint32_t hash = FnvOffset;
	for (std::size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= FnvPrime;
	}
	return hash;
}

uint32_t ReadTrailingU32(const InventoryCommandLogBytes &bytes)
{
	const std::size_t offset = bytes.size() - 4U;
	return static_cast<uint32_t>(bytes[offset])
	    | (static_cast<uint32_t>(bytes[offset + 1U]) << 8U)
	    | (static_cast<uint32_t>(bytes[offset + 2U]) << 16U)
	    | (static_cast<uint32_t>(bytes[offset + 3U]) << 24U);
}

} // namespace

InventoryCommandLogBytes InventoryCommandLogCodec::encode(const InventoryCommandLog &log) const
{
	InventoryCommandLogBytes payload;
	WriteU8(payload, Magic0);
	WriteU8(payload, Magic1);
	WriteU8(payload, Magic2);
	WriteU8(payload, Magic3);
	WriteU32(payload, Version);
	WriteU32(payload, static_cast<uint32_t>(log.commands().size()));

	for (const InventoryCommand &command : log.commands()) {
		const InventoryCommandBytes packetBytes = commandCodec_.encode(commandCodec_.toPacket(command));
		payload.insert(payload.end(), packetBytes.begin(), packetBytes.end());
	}

	const uint32_t checksum = ChecksumOf(payload, payload.size());
	WriteU32(payload, checksum);
	return payload;
}

std::optional<InventoryCommandLog> InventoryCommandLogCodec::decode(const InventoryCommandLogBytes &bytes) const
{
	if (bytes.size() < HeaderSize + 4U)
		return std::nullopt;

	const std::size_t payloadSize = bytes.size() - 4U;
	if (ChecksumOf(bytes, payloadSize) != ReadTrailingU32(bytes))
		return std::nullopt;

	std::size_t offset = 0;
	const uint8_t magic0 = ReadU8(bytes, offset);
	const uint8_t magic1 = ReadU8(bytes, offset);
	const uint8_t magic2 = ReadU8(bytes, offset);
	const uint8_t magic3 = ReadU8(bytes, offset);
	const uint32_t version = ReadU32(bytes, offset);
	const uint32_t commandCount = ReadU32(bytes, offset);

	if (magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3 || version != Version)
		return std::nullopt;

	if (payloadSize != HeaderSize + (static_cast<std::size_t>(commandCount) * PacketSize))
		return std::nullopt;

	InventoryCommandLog log;
	for (uint32_t index = 0; index < commandCount; ++index) {
		InventoryCommandBytes packetBytes {
			bytes.begin() + static_cast<std::ptrdiff_t>(offset),
			bytes.begin() + static_cast<std::ptrdiff_t>(offset + PacketSize),
		};
		offset += PacketSize;

		std::optional<InventoryCommandPacket> packet = commandCodec_.decode(packetBytes);
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
