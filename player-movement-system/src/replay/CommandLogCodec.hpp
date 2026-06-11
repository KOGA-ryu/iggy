#pragma once

#include <optional>

#include "network/MovementCodec.hpp"
#include "replay/CommandLog.hpp"
#include "replay/CommandLogBytes.hpp"
#include "replay/CommandLogFrameCodec.hpp"

namespace dev {

class CommandLogCodec {
public:
	[[nodiscard]] CommandLogBytes encode(const CommandLog &log) const;
	[[nodiscard]] std::optional<CommandLog> decode(const CommandLogBytes &bytes) const;

private:
	MovementCodec commandCodec_;
	CommandLogFrameCodec frameCodec_;
};

} // namespace dev
