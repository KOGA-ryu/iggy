#pragma once

#include "effects/EffectRequest.hpp"

namespace dev {

class EffectSink {
public:
	virtual ~EffectSink() = default;

	virtual void emit(const EffectRequest &request) = 0;
};

} // namespace dev
