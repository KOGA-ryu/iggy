#include "InventoryScriptSource.hpp"

namespace dev {

void QueuedInventoryScriptSource::enqueue(const std::filesystem::path &path)
{
	paths_.push_back(path);
}

void QueuedInventoryScriptSource::clear()
{
	paths_.clear();
}

std::vector<std::filesystem::path> QueuedInventoryScriptSource::drain()
{
	std::vector<std::filesystem::path> drained;
	drained.swap(paths_);
	return drained;
}

bool QueuedInventoryScriptSource::empty() const
{
	return paths_.empty();
}

std::size_t QueuedInventoryScriptSource::size() const
{
	return paths_.size();
}

} // namespace dev
