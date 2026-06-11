#include "SessionCommandLogChecksum.hpp"

#include "session/SessionCommandByteStream.hpp"

namespace dev {

namespace {

constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

} // namespace

void SessionCommandLogChecksum::appendTo(SessionCommandLogBytes &bytes) const
{
	SessionCommandByteWriter writer { bytes };
	writer.writeU32(compute(bytes, bytes.size()));
}

bool SessionCommandLogChecksum::hasValidTrailingChecksum(const SessionCommandLogBytes &bytes, std::size_t payloadSize) const
{
	if (bytes.size() < payloadSize + 4U)
		return false;
	return compute(bytes, payloadSize) == readTrailingU32(bytes);
}

uint32_t SessionCommandLogChecksum::compute(const SessionCommandLogBytes &bytes, std::size_t length) const
{
	uint32_t hash = FnvOffset;
	for (std::size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= FnvPrime;
	}
	return hash;
}

uint32_t SessionCommandLogChecksum::readTrailingU32(const SessionCommandLogBytes &bytes) const
{
	const std::size_t offset = bytes.size() - 4U;
	SessionCommandByteReader reader { bytes, offset };
	uint32_t value = 0;
	reader.readU32(value);
	return value;
}

} // namespace dev
