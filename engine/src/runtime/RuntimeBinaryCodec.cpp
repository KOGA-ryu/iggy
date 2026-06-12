#include "runtime/RuntimeBinaryCodec.hpp"

#include <cstring>

namespace iggy::runtime {

void RuntimeBinaryWriter::writeU8(std::uint8_t value)
{
	bytes_.push_back(value);
}

void RuntimeBinaryWriter::writeBool(bool value)
{
	writeU8(value ? 1U : 0U);
}

void RuntimeBinaryWriter::writeU16LE(std::uint16_t value)
{
	for (int shift = 0; shift <= 8; shift += 8)
		writeU8(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
}

void RuntimeBinaryWriter::writeU32LE(std::uint32_t value)
{
	for (int shift = 0; shift <= 24; shift += 8)
		writeU8(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
}

void RuntimeBinaryWriter::writeU64LE(std::uint64_t value)
{
	for (int shift = 0; shift <= 56; shift += 8)
		writeU8(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
}

void RuntimeBinaryWriter::writeI32LE(std::int32_t value)
{
	writeU32LE(static_cast<std::uint32_t>(value));
}

void RuntimeBinaryWriter::writeF32LE(float value)
{
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	writeU32LE(bits);
}

void RuntimeBinaryWriter::writeBytes(const std::vector<std::uint8_t> &bytes)
{
	bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
}

const std::vector<std::uint8_t> &RuntimeBinaryWriter::bytes() const
{
	return bytes_;
}

std::vector<std::uint8_t> RuntimeBinaryWriter::takeBytes() const
{
	return bytes_;
}

RuntimeBinaryReader::RuntimeBinaryReader(const std::vector<std::uint8_t> &bytes)
	: bytes_(bytes)
{
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readU8(std::uint8_t &value)
{
	if (!canRead(1))
		return RuntimeBinaryReadStatus::OutOfBytes;

	value = bytes_[offset_];
	++offset_;
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readBool(bool &value)
{
	std::uint8_t raw = 0;
	const RuntimeBinaryReadStatus status = readU8(raw);
	if (status != RuntimeBinaryReadStatus::Ok)
		return status;

	value = raw != 0;
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readU16LE(std::uint16_t &value)
{
	if (!canRead(2))
		return RuntimeBinaryReadStatus::OutOfBytes;

	std::uint16_t result = 0;
	for (int shift = 0; shift <= 8; shift += 8)
		result |= static_cast<std::uint16_t>(bytes_[offset_++]) << shift;
	value = result;
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readU32LE(std::uint32_t &value)
{
	if (!canRead(4))
		return RuntimeBinaryReadStatus::OutOfBytes;

	std::uint32_t result = 0;
	for (int shift = 0; shift <= 24; shift += 8)
		result |= static_cast<std::uint32_t>(bytes_[offset_++]) << shift;
	value = result;
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readU64LE(std::uint64_t &value)
{
	if (!canRead(8))
		return RuntimeBinaryReadStatus::OutOfBytes;

	std::uint64_t result = 0;
	for (int shift = 0; shift <= 56; shift += 8)
		result |= static_cast<std::uint64_t>(bytes_[offset_++]) << shift;
	value = result;
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readI32LE(std::int32_t &value)
{
	std::uint32_t bits = 0;
	const RuntimeBinaryReadStatus status = readU32LE(bits);
	if (status != RuntimeBinaryReadStatus::Ok)
		return status;

	std::memcpy(&value, &bits, sizeof(value));
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readF32LE(float &value)
{
	std::uint32_t bits = 0;
	const RuntimeBinaryReadStatus status = readU32LE(bits);
	if (status != RuntimeBinaryReadStatus::Ok)
		return status;

	std::memcpy(&value, &bits, sizeof(value));
	return RuntimeBinaryReadStatus::Ok;
}

RuntimeBinaryReadStatus RuntimeBinaryReader::readBytes(std::size_t count, std::vector<std::uint8_t> &out)
{
	if (!canRead(count))
		return RuntimeBinaryReadStatus::OutOfBytes;

	out.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(offset_), bytes_.begin() + static_cast<std::ptrdiff_t>(offset_ + count));
	offset_ += count;
	return RuntimeBinaryReadStatus::Ok;
}

std::size_t RuntimeBinaryReader::offset() const
{
	return offset_;
}

std::size_t RuntimeBinaryReader::remaining() const
{
	return bytes_.size() - offset_;
}

bool RuntimeBinaryReader::canRead(std::size_t count) const
{
	return count <= remaining();
}

} // namespace iggy::runtime
