#include "InventoryCommandLogChecksum.hpp"

namespace dev {

namespace {

constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

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

} // namespace

void InventoryCommandLogChecksum::appendTo(InventoryCommandLogBytes &bytes) const
{
	WriteU32(bytes, compute(bytes, bytes.size()));
}

bool InventoryCommandLogChecksum::hasValidTrailingChecksum(const InventoryCommandLogBytes &bytes, std::size_t payloadSize) const
{
	if (bytes.size() < payloadSize + 4U)
		return false;
	return compute(bytes, payloadSize) == readTrailingU32(bytes);
}

uint32_t InventoryCommandLogChecksum::compute(const InventoryCommandLogBytes &bytes, std::size_t length) const
{
	uint32_t hash = FnvOffset;
	for (std::size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= FnvPrime;
	}
	return hash;
}

uint32_t InventoryCommandLogChecksum::readTrailingU32(const InventoryCommandLogBytes &bytes) const
{
	const std::size_t offset = bytes.size() - 4U;
	return static_cast<uint32_t>(bytes[offset])
	    | (static_cast<uint32_t>(bytes[offset + 1U]) << 8U)
	    | (static_cast<uint32_t>(bytes[offset + 2U]) << 16U)
	    | (static_cast<uint32_t>(bytes[offset + 3U]) << 24U);
}

} // namespace dev
