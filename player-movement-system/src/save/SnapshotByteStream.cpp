#include "SnapshotByteStream.hpp"

#include <cstring>

namespace dev {

SnapshotByteWriter::SnapshotByteWriter(SnapshotBytes &bytes)
    : bytes_(bytes)
{
}

void SnapshotByteWriter::writeU8(uint8_t value)
{
	bytes_.push_back(value);
}

void SnapshotByteWriter::writeU32(uint32_t value)
{
	bytes_.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes_.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
	bytes_.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
	bytes_.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
}

void SnapshotByteWriter::writeI32(int value)
{
	writeU32(static_cast<uint32_t>(value));
}

void SnapshotByteWriter::writeFloat(float value)
{
	uint32_t bits = 0;
	static_assert(sizeof(bits) == sizeof(value));
	std::memcpy(&bits, &value, sizeof(bits));
	writeU32(bits);
}

SnapshotByteReader::SnapshotByteReader(const SnapshotBytes &bytes)
    : bytes_(bytes)
{
}

SnapshotByteReader::SnapshotByteReader(const SnapshotBytes &bytes, std::size_t offset)
    : bytes_(bytes)
    , offset_(offset)
{
}

bool SnapshotByteReader::readU8(uint8_t &value)
{
	if (offset_ + 1U > bytes_.size())
		return false;
	value = bytes_[offset_++];
	return true;
}

bool SnapshotByteReader::readU32(uint32_t &value)
{
	if (offset_ + 4U > bytes_.size())
		return false;
	value = static_cast<uint32_t>(bytes_[offset_])
	    | (static_cast<uint32_t>(bytes_[offset_ + 1U]) << 8U)
	    | (static_cast<uint32_t>(bytes_[offset_ + 2U]) << 16U)
	    | (static_cast<uint32_t>(bytes_[offset_ + 3U]) << 24U);
	offset_ += 4U;
	return true;
}

bool SnapshotByteReader::readI32(int &value)
{
	uint32_t raw = 0;
	if (!readU32(raw))
		return false;
	value = static_cast<int>(raw);
	return true;
}

bool SnapshotByteReader::readFloat(float &value)
{
	uint32_t bits = 0;
	if (!readU32(bits))
		return false;
	static_assert(sizeof(bits) == sizeof(value));
	std::memcpy(&value, &bits, sizeof(value));
	return true;
}

std::size_t SnapshotByteReader::offset() const
{
	return offset_;
}

bool SnapshotByteReader::consumed() const
{
	return offset_ == bytes_.size();
}

} // namespace dev
