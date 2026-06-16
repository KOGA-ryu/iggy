#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

#include <iostream>

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
		result.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult Read(
	const std::string &text)
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(text);
}

void TestEmptyInputFailsDeterministically()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read("");

	Expect(!result.ok(), "empty input should not parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid, "empty input should report syntax invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError), "empty input should report syntax issue");
	Expect(result.issueCount == 1 && result.syntaxIssueCount == 1, "empty input should mirror syntax counts");
	Expect(result.input.empty(), "empty input should be preserved by value");
}

void TestUnsupportedInputFailsDeterministically()
{
	const std::string text = "format_id = \"iggy:ascii-source-plan\"\n";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsupported input should not parse in skeleton reader");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported input should report unsupported status");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape), "unsupported input should report unsupported issue");
	Expect(result.issueCount == 1 && result.unsupportedIssueCount == 1, "unsupported input should mirror unsupported counts");
	Expect(result.input == text && result.inputSize == text.size(), "unsupported input should be copied for diagnostics");
}

} // namespace

int main()
{
	TestEmptyInputFailsDeterministically();
	TestUnsupportedInputFailsDeterministically();
	return Failures;
}
