#include "SnapshotFrameCodec.hpp"

#include <cstddef>
#include <cstdint>

#include "save/SnapshotChecksum.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'G';
constexpr uint8_t Magic2 = 'G';
constexpr uint8_t Magic3 = 'Y';
constexpr uint32_t SnapshotVersion = 7;
constexpr std::size_t HeaderSize = 8;

void WriteU8(SnapshotBytes &bytes, uint8_t value)
{
	bytes.push_back(value);
}

void WriteU32(SnapshotBytes &bytes, uint32_t value)
{
	bytes.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
	bytes.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
}

uint8_t ReadU8(const SnapshotBytes &bytes, std::size_t &offset)
{
	return bytes[offset++];
}

uint32_t ReadU32(const SnapshotBytes &bytes, std::size_t &offset)
{
	const uint32_t value = static_cast<uint32_t>(bytes[offset])
	    | (static_cast<uint32_t>(bytes[offset + 1U]) << 8U)
	    | (static_cast<uint32_t>(bytes[offset + 2U]) << 16U)
	    | (static_cast<uint32_t>(bytes[offset + 3U]) << 24U);
	offset += 4U;
	return value;
}

} // namespace

SnapshotBytes SnapshotFrameCodec::encode(const SnapshotBytes &payload) const
{
	SnapshotBytes framed;
	framed.reserve(HeaderSize + payload.size() + 4U);
	WriteU8(framed, Magic0);
	WriteU8(framed, Magic1);
	WriteU8(framed, Magic2);
	WriteU8(framed, Magic3);
	WriteU32(framed, SnapshotVersion);
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

	std::size_t offset = 0;
	const uint8_t magic0 = ReadU8(bytes, offset);
	const uint8_t magic1 = ReadU8(bytes, offset);
	const uint8_t magic2 = ReadU8(bytes, offset);
	const uint8_t magic3 = ReadU8(bytes, offset);
	const uint32_t version = ReadU32(bytes, offset);
	if (magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3 || version != SnapshotVersion)
		return std::nullopt;

	return SnapshotBytes { bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.begin() + static_cast<std::ptrdiff_t>(payloadEnd) };
}

} // namespace dev
