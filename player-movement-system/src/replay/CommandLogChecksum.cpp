#include "CommandLogChecksum.hpp"

namespace dev {

namespace {

constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

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

uint16_t ReadU16(const CommandLogBytes &bytes, std::size_t &offset)
{
	const uint16_t value = static_cast<uint16_t>(bytes[offset])
	    | (static_cast<uint16_t>(bytes[offset + 1U]) << 8U);
	offset += 2U;
	return value;
}

uint32_t ReadU32(const CommandLogBytes &bytes, std::size_t &offset)
{
	const uint32_t low = ReadU16(bytes, offset);
	const uint32_t high = ReadU16(bytes, offset);
	return low | (high << 16U);
}

} // namespace

void CommandLogChecksum::appendTo(CommandLogBytes &bytes) const
{
	WriteU32(bytes, compute(bytes, bytes.size()));
}

bool CommandLogChecksum::hasValidTrailingChecksum(const CommandLogBytes &bytes, std::size_t payloadSize) const
{
	if (bytes.size() < payloadSize + 4U)
		return false;
	return compute(bytes, payloadSize) == readTrailingU32(bytes);
}

uint32_t CommandLogChecksum::compute(const CommandLogBytes &bytes, std::size_t length) const
{
	uint32_t hash = FnvOffset;
	for (std::size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= FnvPrime;
	}
	return hash;
}

uint32_t CommandLogChecksum::readTrailingU32(const CommandLogBytes &bytes) const
{
	std::size_t offset = bytes.size() - 4U;
	return ReadU32(bytes, offset);
}

} // namespace dev
