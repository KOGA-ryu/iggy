#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dev {

class InventoryCommandByteWriter {
public:
	explicit InventoryCommandByteWriter(std::vector<uint8_t> &bytes);

	void writeU8(uint8_t value);
	void writeU16(uint16_t value);
	void writeU32(uint32_t value);

private:
	std::vector<uint8_t> &bytes_;
};

class InventoryCommandByteReader {
public:
	explicit InventoryCommandByteReader(const std::vector<uint8_t> &bytes);
	InventoryCommandByteReader(const std::vector<uint8_t> &bytes, std::size_t offset);

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
