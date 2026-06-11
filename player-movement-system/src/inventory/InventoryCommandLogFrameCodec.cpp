#include "InventoryCommandLogFrameCodec.hpp"

#include <cstddef>

#include "inventory/InventoryCommandByteStream.hpp"
#include "inventory/InventoryCommandLogChecksum.hpp"
#include "inventory/InventoryCommandPacketListCodec.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'I';
constexpr uint8_t Magic2 = 'C';
constexpr uint8_t Magic3 = 'L';
constexpr uint32_t Version = 1;
constexpr std::size_t HeaderSize = 8;

} // namespace

InventoryCommandLogBytes InventoryCommandLogFrameCodec::encode(const std::vector<InventoryCommandBytes> &packets) const
{
	InventoryCommandLogBytes payload;
	InventoryCommandByteWriter writer { payload };
	writer.writeU8(Magic0);
	writer.writeU8(Magic1);
	writer.writeU8(Magic2);
	writer.writeU8(Magic3);
	writer.writeU32(Version);

	const InventoryCommandLogBytes packetList = InventoryCommandPacketListCodec {}.encode(packets);
	payload.insert(payload.end(), packetList.begin(), packetList.end());

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

	InventoryCommandByteReader reader { bytes };
	uint8_t magic0 = 0;
	uint8_t magic1 = 0;
	uint8_t magic2 = 0;
	uint8_t magic3 = 0;
	uint32_t version = 0;
	if (!reader.readU8(magic0)
	    || !reader.readU8(magic1)
	    || !reader.readU8(magic2)
	    || !reader.readU8(magic3)
	    || !reader.readU32(version))
		return std::nullopt;

	if (magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3 || version != Version)
		return std::nullopt;

	return InventoryCommandPacketListCodec {}.decode({
	    bytes.begin() + static_cast<std::ptrdiff_t>(HeaderSize),
	    bytes.begin() + static_cast<std::ptrdiff_t>(payloadSize),
	});
}

} // namespace dev
