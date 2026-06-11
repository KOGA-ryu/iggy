#pragma once

#include "interaction/DestinationAction.hpp"
#include "interaction/InteractionIntent.hpp"

namespace dev {

class DestinationActionBuilder {
public:
	[[nodiscard]] DestinationAction build(const InteractionIntent &intent) const;
};

} // namespace dev
