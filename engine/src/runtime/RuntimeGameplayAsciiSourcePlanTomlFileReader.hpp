#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiSourcePlanTomlFileReadStatus {
	Parsed,
	EmptyPath,
	MissingFile,
	NonRegularFile,
	OpenFailed,
	ReadFailed,
	TomlReadFailed,
};

enum class RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode {
	EmptyPath,
	MissingFile,
	NonRegularFile,
	OpenFailed,
	ReadFailed,
	TomlReadFailed,
};

struct RuntimeGameplayAsciiSourcePlanTomlFileReadIssue {
	RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code =
		RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::OpenFailed;
	std::filesystem::path path;
	std::string detail;
};

struct RuntimeGameplayAsciiSourcePlanTomlFileReadResult {
	std::filesystem::path path;
	RuntimeGameplayAsciiSourcePlanTomlReadResult text;
	std::vector<RuntimeGameplayAsciiSourcePlanTomlFileReadIssue> issues;
	RuntimeGameplayAsciiSourcePlanTomlFileReadStatus status =
		RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::OpenFailed;
	std::size_t bytesRead = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanTomlFileReader {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanTomlFileReadResult read(
		const std::filesystem::path &path) const;
};

} // namespace iggy::runtime
