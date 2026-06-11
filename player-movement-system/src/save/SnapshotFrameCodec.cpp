#include "SnapshotFrameCodec.hpp"

#include <cstddef>
#include <cstdint>

#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotChecksum.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'G';
constexpr uint8_t Magic2 = 'G';
constexpr uint8_t Magic3 = 'Y';
constexpr uint32_t SnapshotVersion = 7;
constexpr std::size_t HeaderSize = 8;

} // namespace

SnapshotBytes SnapshotFrameCodec::encode(const SnapshotBytes &payload) const
{
	SnapshotBytes framed;
	framed.reserve(HeaderSize + payload.size() + 4U);
	SnapshotByteWriter writer { framed };
	writer.writeU8(Magic0);
	writer.writeU8(Magic1);
	writer.writeU8(Magic2);
	writer.writeU8(Magic3);
	writer.writeU32(SnapshotVersion);
	framed.insert(framed.end(), payload.begin(), payload.end());
	SnapshotChecksum {}.appendTo(framed);
	return framed;
}

std::optional<SnapshotBytes> SnapshotFrameCodec::decode(const SnapshotBytes &bytes) const
{
	if (bytes.size() < HeaderSize + 4U)
		return std::nullopt;

	const std::size_t payloadEnd = bytes.size() - 4U;
	if (!SnapshotChecksum {}.hasValidTrailingChecksum(bytes, payloadEnd))
		return std::nullopt;

	SnapshotByteReader reader { bytes };
	uint8_t magic0 = 0;
	uint8_t magic1 = 0;
	uint8_t magic2 = 0;
	uint8_t magic3 = 0;
	uint32_t version = 0;
	if (!reader.readU8(magic0) || !reader.readU8(magic1) || !reader.readU8(magic2) || !reader.readU8(magic3) || !reader.readU32(version))
		return std::nullopt;
	if (magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3 || version != SnapshotVersion)
		return std::nullopt;

	return SnapshotBytes { bytes.begin() + static_cast<std::ptrdiff_t>(reader.offset()), bytes.begin() + static_cast<std::ptrdiff_t>(payloadEnd) };
}

} // namespace dev
