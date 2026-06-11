#pragma once

#include <cstddef>
#include <cstdint>

#include "save/SnapshotBytes.hpp"

namespace dev {

class SnapshotByteWriter {
public:
	explicit SnapshotByteWriter(SnapshotBytes &bytes);

	void writeU8(uint8_t value);
	void writeU32(uint32_t value);
	void writeI32(int value);
	void writeFloat(float value);

private:
	SnapshotBytes &bytes_;
};

class SnapshotByteReader {
public:
	explicit SnapshotByteReader(const SnapshotBytes &bytes);
	SnapshotByteReader(const SnapshotBytes &bytes, std::size_t offset);

	bool readU8(uint8_t &value);
	bool readU32(uint32_t &value);
	bool readI32(int &value);
	bool readFloat(float &value);

	[[nodiscard]] std::size_t offset() const;
	[[nodiscard]] bool consumed() const;

private:
	const SnapshotBytes &bytes_;
	std::size_t offset_ = 0;
};

} // namespace dev
