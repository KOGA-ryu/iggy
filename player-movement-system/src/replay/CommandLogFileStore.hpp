#pragma once

#include <filesystem>
#include <optional>

#include "replay/CommandLogCodec.hpp"

namespace dev {

class CommandLogFileStore {
public:
	explicit CommandLogFileStore(CommandLogCodec codec = CommandLogCodec {});

	[[nodiscard]] bool save(const std::filesystem::path &path, const CommandLog &log) const;
	[[nodiscard]] std::optional<CommandLog> load(const std::filesystem::path &path) const;

private:
	CommandLogCodec codec_;
};

} // namespace dev
