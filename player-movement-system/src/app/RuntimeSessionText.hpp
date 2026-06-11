#pragma once

#include <string>
#include <string_view>

#include "session/SessionCommand.hpp"
#include "session/SessionEvent.hpp"

namespace dev {

class RuntimeSessionText {
public:
	[[nodiscard]] std::string formatResult(std::string_view label, const SessionCommandResult &result) const;
	[[nodiscard]] std::string formatEvent(std::string_view label, const SessionEvent &event) const;
};

} // namespace dev
