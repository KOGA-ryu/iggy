#pragma once

#include <string>
#include <vector>

#include "app/RuntimeDebugManifest.hpp"

namespace dev {

class RuntimeDebugManifestIndexText {
public:
	[[nodiscard]] std::vector<std::string> format(const RuntimeDebugManifestContext &context) const;
};

} // namespace dev
