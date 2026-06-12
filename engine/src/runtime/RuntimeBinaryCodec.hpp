#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace iggy::runtime {

class RuntimeBinaryWriter {
public:
	void writeU8(std::uint8_t value);
	void writeBool(bool value);
	void writeU16LE(std::uint16_t value);
	void writeU32LE(std::uint32_t value);
	void writeU64LE(std::uint64_t value);
	void writeI32LE(std::int32_t value);
	void writeF32LE(float value);
	void writeBytes(const std::vector<std::uint8_t> &bytes);

	[[nodiscard]] const std::vector<std::uint8_t> &bytes() const;
	[[nodiscard]] std::vector<std::uint8_t> takeBytes() const;

private:
	std::vector<std::uint8_t> bytes_;
};

enum class RuntimeBinaryReadStatus {
	Ok,
	OutOfBytes,
};

class RuntimeBinaryReader {
public:
	explicit RuntimeBinaryReader(const std::vector<std::uint8_t> &bytes);

	RuntimeBinaryReadStatus readU8(std::uint8_t &value);
	RuntimeBinaryReadStatus readBool(bool &value);
	RuntimeBinaryReadStatus readU16LE(std::uint16_t &value);
	RuntimeBinaryReadStatus readU32LE(std::uint32_t &value);
	RuntimeBinaryReadStatus readU64LE(std::uint64_t &value);
	RuntimeBinaryReadStatus readI32LE(std::int32_t &value);
	RuntimeBinaryReadStatus readF32LE(float &value);
	RuntimeBinaryReadStatus readBytes(std::size_t count, std::vector<std::uint8_t> &out);

	[[nodiscard]] std::size_t offset() const;
	[[nodiscard]] std::size_t remaining() const;

private:
	[[nodiscard]] bool canRead(std::size_t count) const;

	const std::vector<std::uint8_t> &bytes_;
	std::size_t offset_ = 0;
};

} // namespace iggy::runtime
