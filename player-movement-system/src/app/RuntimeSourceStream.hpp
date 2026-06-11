#pragma once

#include <vector>

namespace dev {

template <typename Item, typename Source>
class RuntimeSourceStream {
public:
	[[nodiscard]] std::vector<Item> drain(const std::vector<Source *> &sources) const
	{
		std::vector<Item> items;
		appendSources(items, sources);
		return items;
	}

	[[nodiscard]] std::vector<Item> drain(Source &firstSource, const std::vector<Source *> &sources) const
	{
		std::vector<Item> items;
		appendSource(items, firstSource);
		appendSources(items, sources);
		return items;
	}

private:
	void appendSources(std::vector<Item> &items, const std::vector<Source *> &sources) const
	{
		for (Source *source : sources) {
			if (source == nullptr)
				continue;
			appendSource(items, *source);
		}
	}

	void appendSource(std::vector<Item> &items, Source &source) const
	{
		std::vector<Item> drained = source.drain();
		items.reserve(items.size() + drained.size());
		items.insert(items.end(), drained.begin(), drained.end());
	}
};

} // namespace dev
