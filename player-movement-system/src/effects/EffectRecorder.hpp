#pragma once

#include <vector>

#include "effects/EffectSink.hpp"

namespace dev {

class EffectRecorder : public EffectSink {
public:
	void emit(const EffectRequest &request) override;
	void clear();

	[[nodiscard]] const std::vector<EffectRequest> &requests() const;

private:
	std::vector<EffectRequest> requests_;
};

} // namespace dev
