#pragma once

#include <cstddef>
#include <filesystem>
#include <vector>

namespace dev {

class MovementScriptSource {
public:
	virtual ~MovementScriptSource() = default;

	[[nodiscard]] virtual std::vector<std::filesystem::path> drain() = 0;
};

class QueuedMovementScriptSource : public MovementScriptSource {
public:
	void enqueue(const std::filesystem::path &path);
	void clear();

	[[nodiscard]] std::vector<std::filesystem::path> drain() override;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;

private:
	std::vector<std::filesystem::path> paths_;
};

} // namespace dev
