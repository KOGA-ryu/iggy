#include "InventoryCommandPacketListCodec.hpp"

#include <cstddef>

#include "inventory/InventoryCommandByteStream.hpp"

namespace dev {

namespace {

constexpr std::size_t PacketSize = 16;

} // namespace

InventoryCommandLogBytes InventoryCommandPacketListCodec::encode(const std::vector<InventoryCommandBytes> &packets) const
{
	InventoryCommandLogBytes bytes;
	InventoryCommandByteWriter writer { bytes };
	writer.writeU32(static_cast<uint32_t>(packets.size()));
	for (const InventoryCommandBytes &packet : packets)
		bytes.insert(bytes.end(), packet.begin(), packet.end());
	return bytes;
}

std::optional<std::vector<InventoryCommandBytes>> InventoryCommandPacketListCodec::decode(const InventoryCommandLogBytes &bytes) const
{
	if (bytes.size() < 4U)
		return std::nullopt;

	InventoryCommandByteReader reader { bytes };
	uint32_t commandCount = 0;
	if (!reader.readU32(commandCount))
		return std::nullopt;
	if (bytes.size() != 4U + (static_cast<std::size_t>(commandCount) * PacketSize))
		return std::nullopt;

	std::vector<InventoryCommandBytes> packets;
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
