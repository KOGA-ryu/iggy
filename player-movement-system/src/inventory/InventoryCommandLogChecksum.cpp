#include "InventoryCommandLogChecksum.hpp"

#include "inventory/InventoryCommandByteStream.hpp"

namespace dev {

namespace {

constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

} // namespace

void InventoryCommandLogChecksum::appendTo(InventoryCommandLogBytes &bytes) const
{
	InventoryCommandByteWriter { bytes }.writeU32(compute(bytes, bytes.size()));
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
	uint32_t value = 0;
	(void)InventoryCommandByteReader { bytes, offset }.readU32(value);
	return value;
}

} // namespace dev
