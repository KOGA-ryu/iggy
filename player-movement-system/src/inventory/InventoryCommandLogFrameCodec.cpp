#include "InventoryCommandLogFrameCodec.hpp"

#include <cstddef>

#include "inventory/InventoryCommandLogChecksum.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'I';
constexpr uint8_t Magic2 = 'C';
constexpr uint8_t Magic3 = 'L';
constexpr uint32_t Version = 1;
constexpr std::size_t HeaderSize = 12;
constexpr std::size_t PacketSize = 16;

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

} // namespace

InventoryCommandLogBytes InventoryCommandLogFrameCodec::encode(const std::vector<InventoryCommandBytes> &packets) const
{
	InventoryCommandLogBytes payload;
	WriteU8(payload, Magic0);
	WriteU8(payload, Magic1);
	WriteU8(payload, Magic2);
	WriteU8(payload, Magic3);
	WriteU32(payload, Version);
	WriteU32(payload, static_cast<uint32_t>(packets.size()));

	for (const InventoryCommandBytes &packet : packets) {
		payload.insert(payload.end(), packet.begin(), packet.end());
	}

	InventoryCommandLogChecksum {}.appendTo(payload);
	return payload;
}

std::optional<std::vector<InventoryCommandBytes>> InventoryCommandLogFrameCodec::decode(const InventoryCommandLogBytes &bytes) const
{
	if (bytes.size() < HeaderSize + 4U)
		return std::nullopt;

	const std::size_t payloadSize = bytes.size() - 4U;
	if (!InventoryCommandLogChecksum {}.hasValidTrailingChecksum(bytes, payloadSize))
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

	std::vector<InventoryCommandBytes> packets;
	packets.reserve(commandCount);
	for (uint32_t index = 0; index < commandCount; ++index) {
		packets.push_back({
		    bytes.begin() + static_cast<std::ptrdiff_t>(offset),
		    bytes.begin() + static_cast<std::ptrdiff_t>(offset + PacketSize),
		});
		offset += PacketSize;
	}

	return packets;
}

} // namespace dev
