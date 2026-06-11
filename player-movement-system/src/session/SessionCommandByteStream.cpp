#include "SessionCommandByteStream.hpp"

namespace dev {

SessionCommandByteWriter::SessionCommandByteWriter(std::vector<uint8_t> &bytes)
    : bytes_(bytes)
{
}

void SessionCommandByteWriter::writeU8(uint8_t value)
{
	bytes_.push_back(value);
}

void SessionCommandByteWriter::writeU16(uint16_t value)
{
	bytes_.push_back(static_cast<uint8_t>(value & 0xFFU));
	bytes_.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
}

void SessionCommandByteWriter::writeU32(uint32_t value)
{
	writeU16(static_cast<uint16_t>(value & 0xFFFFU));
	writeU16(static_cast<uint16_t>((value >> 16U) & 0xFFFFU));
}

SessionCommandByteReader::SessionCommandByteReader(const std::vector<uint8_t> &bytes)
    : bytes_(bytes)
{
}

SessionCommandByteReader::SessionCommandByteReader(const std::vector<uint8_t> &bytes, std::size_t offset)
    : bytes_(bytes)
    , offset_(offset)
{
}

bool SessionCommandByteReader::readU8(uint8_t &value)
{
	if (offset_ + 1U > bytes_.size())
		return false;
	value = bytes_[offset_++];
	return true;
}

bool SessionCommandByteReader::readU16(uint16_t &value)
{
	if (offset_ + 2U > bytes_.size())
		return false;
	value = static_cast<uint16_t>(bytes_[offset_])
	    | (static_cast<uint16_t>(bytes_[offset_ + 1U]) << 8U);
	offset_ += 2U;
	return true;
}

bool SessionCommandByteReader::readU32(uint32_t &value)
{
	if (offset_ + 4U > bytes_.size())
		return false;
	uint16_t low = 0;
	uint16_t high = 0;
	(void)readU16(low);
	(void)readU16(high);
	value = static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 16U);
	return true;
}

std::size_t SessionCommandByteReader::offset() const
{
	return offset_;
}

bool SessionCommandByteReader::consumed() const
{
	return offset_ == bytes_.size();
}

} // namespace dev
