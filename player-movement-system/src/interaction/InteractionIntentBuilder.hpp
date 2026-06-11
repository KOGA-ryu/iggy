#pragma once

#include "interaction/InteractionIntent.hpp"
#include "targeting/Target.hpp"

namespace dev {

class InteractionIntentBuilder {
public:
	InteractionIntent build(Target target, bool standGround) const;
};

} // namespace dev

