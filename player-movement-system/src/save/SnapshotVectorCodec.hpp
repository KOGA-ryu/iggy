#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "save/SnapshotByteStream.hpp"

namespace dev {

class SnapshotVectorCodec {
public:
	template <typename T, typename WriteFn>
	void writeVector(SnapshotByteWriter &writer, const std::vector<T> &items, WriteFn writeItem) const
	{
		writer.writeU32(countOf(items.size()));
		for (const T &item : items)
			writeItem(writer, item);
	}

	template <typename T, typename ReadFn>
	[[nodiscard]] bool readVector(SnapshotByteReader &reader, std::vector<T> &items, ReadFn readItem) const
	{
		uint32_t count = 0;
		if (!reader.readU32(count))
			return false;

		items.clear();
		items.reserve(count);
		for (uint32_t i = 0; i < count; ++i) {
			T item;
			if (!readItem(reader, item))
				return false;
			items.push_back(std::move(item));
		}
		return true;
	}

private:
	[[nodiscard]] static uint32_t countOf(std::size_t size)
	{
		return size > std::numeric_limits<uint32_t>::max()
		    ? std::numeric_limits<uint32_t>::max()
		    : static_cast<uint32_t>(size);
	}
};

} // namespace dev
