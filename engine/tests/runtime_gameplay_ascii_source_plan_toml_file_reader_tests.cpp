#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_gameplay_ascii_source_plan_toml_file_reader_tests_tmp";
}

std::filesystem::path TempPath(const char *name)
{
	return TempRoot() / name;
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void CleanupTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void WriteText(const std::filesystem::path &path, const std::string &text)
{
	std::ofstream stream(path, std::ios::binary | std::ios::trunc);
	stream << text;
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult &result,
	iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssue &issue :
		result.issues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

std::string ValidToml()
{
	return R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:file-reader"

[grid]
width = 3
height = 3
background = "."
rows = [
  "###",
  "#.#",
  "###",
]

[no_claims]
runtime_truth = false
gameplay_execution = false
file_parsing = false
profile_scenario_conversion = false

[promotion]
ready = false
runtime_execution = false
file_parsing = false
profile_scenario_conversion = false
)toml";
}

void TestEmptyPathDoesNotCallTextReader()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read({});

	Expect(!result.ok(), "empty path file read should not be ok");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::EmptyPath, "empty path should report EmptyPath");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::EmptyPath), "empty path should add empty path issue");
	Expect(result.text.input.empty(), "empty path should not call nested text reader");
	Expect(result.bytesRead == 0, "empty path should read zero bytes");
}

void TestMissingPathReportsMissingFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("missing.toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.path == path, "missing file result should preserve path");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::MissingFile, "missing path should report MissingFile");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::MissingFile), "missing path should add missing file issue");
	Expect(result.text.input.empty(), "missing path should not call nested text reader");
}

void TestDirectoryReportsNonRegularFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("directory");
	std::filesystem::create_directories(path);

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::NonRegularFile, "directory path should report NonRegularFile");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::NonRegularFile), "directory path should add non-regular issue");
	Expect(result.text.input.empty(), "directory path should not call nested text reader");
}

void TestValidFileReadsAndParses()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("valid.toml");
	const std::string text = ValidToml();
	WriteText(path, text);

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.ok(), "valid TOML file should parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::Parsed, "valid TOML file should report Parsed");
	Expect(result.path == path, "valid TOML file result should preserve path");
	Expect(result.bytesRead == text.size(), "valid TOML file should report bytes read");
	Expect(result.text.input == text, "nested text reader should own file contents");
	Expect(result.text.ok(), "nested text reader should parse valid file contents");
	Expect(result.text.plan.sourceId == iggy::ResourceId("scenario:file-reader"), "nested plan should preserve source id");
}

void TestInvalidTomlFilePreservesNestedDiagnostics()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("invalid.toml");
	WriteText(path, "not toml");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadResult result =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(path);

	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::TomlReadFailed, "invalid TOML file should report TomlReadFailed");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::TomlReadFailed), "invalid TOML file should add wrapper failure issue");
	Expect(result.bytesRead > 0, "invalid TOML file should report bytes read");
	Expect(!result.text.ok(), "nested text reader should preserve parse failure");
	Expect(!result.text.issues.empty(), "nested text reader should preserve diagnostics");
}

} // namespace

int main()
{
	TestEmptyPathDoesNotCallTextReader();
	TestMissingPathReportsMissingFile();
	TestDirectoryReportsNonRegularFile();
	TestValidFileReadsAndParses();
	TestInvalidTomlFilePreservesNestedDiagnostics();

	CleanupTempRoot();

	if (Failures != 0) {
		std::cerr << Failures << " runtime gameplay ASCII source plan TOML file reader test(s) failed\n";
		return 1;
	}

	return 0;
}
