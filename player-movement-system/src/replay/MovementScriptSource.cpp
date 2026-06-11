#include "MovementScriptSource.hpp"

namespace dev {

void QueuedMovementScriptSource::enqueue(const std::filesystem::path &path)
{
	paths_.push_back(path);
}

void QueuedMovementScriptSource::clear()
{
	paths_.clear();
}

std::vector<std::filesystem::path> QueuedMovementScriptSource::drain()
{
	std::vector<std::filesystem::path> drained;
	drained.swap(paths_);
	return drained;
}

bool QueuedMovementScriptSource::empty() const
{
	return paths_.empty();
}

std::size_t QueuedMovementScriptSource::size() const
{
	return paths_.size();
}

} // namespace dev
