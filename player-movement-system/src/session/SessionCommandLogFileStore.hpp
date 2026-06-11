#pragma once

#include <filesystem>
#include <optional>

#include "session/SessionCommandLogCodec.hpp"

namespace dev {

class SessionCommandLogFileStore {
public:
	explicit SessionCommandLogFileStore(SessionCommandLogCodec codec = SessionCommandLogCodec {});

	[[nodiscard]] bool save(const std::filesystem::path &path, const SessionCommandLog &log) const;
	[[nodiscard]] std::optional<SessionCommandLog> load(const std::filesystem::path &path) const;

private:
	SessionCommandLogCodec codec_;
};

} // namespace dev
