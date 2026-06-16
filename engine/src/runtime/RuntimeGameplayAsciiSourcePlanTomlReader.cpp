#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <cctype>

namespace iggy::runtime {
namespace {

bool IsWhitespaceOnly(const std::string &text)
{
	for (const unsigned char character : text) {
		if (!std::isspace(character)) {
			return false;
		}
	}
	return true;
}

void AddIssue(
	RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError:
		++result.syntaxIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType:
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString:
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnknownEnumValue:
		++result.typeIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape:
		++result.unsupportedIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid:
		++result.sourcePlanIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingTable:
	case RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingRequiredKey:
		++result.syntaxIssueCount;
		break;
	}

	result.issues.push_back(issue);
	result.issueCount = result.issues.size();
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanTomlReadResult::ok() const
{
	return status == RuntimeGameplayAsciiSourcePlanTomlReadStatus::Parsed;
}

RuntimeGameplayAsciiSourcePlanTomlReadResult RuntimeGameplayAsciiSourcePlanTomlReader::read(
	const std::string &text) const
{
	RuntimeGameplayAsciiSourcePlanTomlReadResult result;
	result.input = text;
	result.inputSize = text.size();

	if (IsWhitespaceOnly(text)) {
		RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
		issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError;
		issue.detail = "empty TOML source-plan input";
		AddIssue(result, issue);
		result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid;
		return result;
	}

	RuntimeGameplayAsciiSourcePlanTomlReadIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape;
	issue.line = 1;
	issue.column = 1;
	issue.detail = "TOML source-plan parsing is not implemented for this input yet";
	AddIssue(result, issue);
	result.status = RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax;
	return result;
}

} // namespace iggy::runtime
