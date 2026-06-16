#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAsciiSourcePlanTomlReadStatus {
	Parsed,
	SyntaxInvalid,
	TypeInvalid,
	UnsupportedSyntax,
	SourcePlanInvalid,
};

enum class RuntimeGameplayAsciiSourcePlanTomlReadIssueCode {
	SyntaxError,
	MissingTable,
	MissingRequiredKey,
	WrongType,
	InvalidGlyphString,
	UnknownEnumValue,
	UnsupportedNestedShape,
	SourcePlanInvalid,
};

struct RuntimeGameplayAsciiSourcePlanTomlReadIssue {
	RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code =
		RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError;
	std::size_t line = 0;
	std::size_t column = 0;
	std::string key;
	std::string detail;
};

struct RuntimeGameplayAsciiSourcePlanTomlReadResult {
	std::string input;
	RuntimeGameplayAsciiSourcePlan plan;
	RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation;
	std::vector<RuntimeGameplayAsciiSourcePlanTomlReadIssue> issues;
	RuntimeGameplayAsciiSourcePlanTomlReadStatus status =
		RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid;
	std::size_t inputSize = 0;
	std::size_t issueCount = 0;
	std::size_t syntaxIssueCount = 0;
	std::size_t typeIssueCount = 0;
	std::size_t unsupportedIssueCount = 0;
	std::size_t sourcePlanIssueCount = 0;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAsciiSourcePlanTomlReader {
public:
	[[nodiscard]] RuntimeGameplayAsciiSourcePlanTomlReadResult read(
		const std::string &text) const;
};

} // namespace iggy::runtime
