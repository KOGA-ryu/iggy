#pragma once

#include <cstddef>
#include <vector>

#include "input/RawInput.hpp"

namespace dev {

class RawInputSource {
public:
	virtual ~RawInputSource() = default;

	[[nodiscard]] virtual std::vector<RawInputEvent> drain() = 0;
};

class QueuedRawInputSource : public RawInputSource {
public:
	void enqueue(const RawInputEvent &event);
	void clear();

	[[nodiscard]] std::vector<RawInputEvent> drain() override;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t size() const;

private:
	std::vector<RawInputEvent> events_;
};

} // namespace dev
