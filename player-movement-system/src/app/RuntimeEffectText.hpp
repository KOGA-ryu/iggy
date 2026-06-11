#pragma once

#include <string>
#include <string_view>

#include "effects/EffectRequest.hpp"

namespace dev {

class RuntimeEffectText {
public:
	[[nodiscard]] std::string formatRequest(std::string_view label, const EffectRequest &request) const;
};

} // namespace dev
