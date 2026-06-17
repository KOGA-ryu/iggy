#include "runtime/RuntimeGameplayAuthoringErrorCodes.hpp"

#include <iostream>
#include <string>

namespace {

int Failures = 0;

void ExpectEqual(const char *actual, const char *expected, const char *message)
{
	if (std::string(actual) != expected) {
		std::cerr << "FAIL: " << message << " expected " << expected
			<< " got " << actual << '\n';
		++Failures;
	}
}

void TestReaderAndSourcePlanCodes()
{
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::MissingFile),
		"missing_file",
		"file read issue code should be stable");
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SyntaxError),
		"syntax_error",
		"TOML read issue code should be stable");
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayAsciiSourcePlanIssueCode::AnnotatedCellGlyphMismatch),
		"annotated_cell_glyph_mismatch",
		"source-plan issue code should be stable");
}

void TestAdapterAndConverterCodes()
{
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterIssueCode::AsciiSourcePlanConversionFailed),
		"ascii_source_plan_conversion_failed",
		"adapter issue code should be stable");
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnknownAuthoredControlActor),
		"unknown_authored_control_actor",
		"conversion issue code should be stable");
}

void TestScenarioCodes()
{
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayProfileScenarioIssueCode::MissingProfileTrait),
		"missing_profile_trait",
		"profile issue code should be stable");
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayScenarioIssueCode::MissingControl),
		"missing_control",
		"scenario issue code should be stable");
	ExpectEqual(
		iggy::runtime::runtimeGameplayAuthoringCodeText(
			iggy::runtime::RuntimeGameplayProfileScenarioRunStatus::ValidationFailed),
		"validation_failed",
		"run status code should be stable");
}

} // namespace

int main()
{
	TestReaderAndSourcePlanCodes();
	TestAdapterAndConverterCodes();
	TestScenarioCodes();

	if (Failures != 0)
		return 1;
	return 0;
}
