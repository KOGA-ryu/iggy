#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dev {

class SessionCommandByteWriter {
public:
	explicit SessionCommandByteWriter(std::vector<uint8_t> &bytes);

	void writeU8(uint8_t value);
	void writeU16(uint16_t value);
	void writeU32(uint32_t value);

private:
	std::vector<uint8_t> &bytes_;
};

class SessionCommandByteReader {
public:
	explicit SessionCommandByteReader(const std::vector<uint8_t> &bytes);
	SessionCommandByteReader(const std::vector<uint8_t> &bytes, std::size_t offset);

	bool readU8(uint8_t &value);
	bool readU16(uint16_t &value);
	bool readU32(uint32_t &value);

	[[nodiscard]] std::size_t offset() const;
	[[nodiscard]] bool consumed() const;

private:
	const std::vector<uint8_t> &bytes_;
	std::size_t offset_ = 0;
};

} // namespace dev
