#include "SnapshotChecksum.hpp"

#include "save/SnapshotByteStream.hpp"

namespace dev {

namespace {

constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

} // namespace

void SnapshotChecksum::appendTo(SnapshotBytes &bytes) const
{
	SnapshotByteWriter writer { bytes };
	writer.writeU32(compute(bytes, bytes.size()));
}

bool SnapshotChecksum::hasValidTrailingChecksum(const SnapshotBytes &bytes, std::size_t payloadSize) const
{
	if (bytes.size() < payloadSize + 4U)
		return false;
	return compute(bytes, payloadSize) == readTrailingU32(bytes);
}

uint32_t SnapshotChecksum::compute(const SnapshotBytes &bytes, std::size_t length) const
{
	uint32_t hash = FnvOffset;
	for (std::size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= FnvPrime;
	}
	return hash;
}

uint32_t SnapshotChecksum::readTrailingU32(const SnapshotBytes &bytes) const
{
	const std::size_t offset = bytes.size() - 4U;
	SnapshotByteReader reader { bytes, offset };
	uint32_t value = 0;
	reader.readU32(value);
	return value;
}

} // namespace dev
