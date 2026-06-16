#include "runtime/RuntimeGameplayAsciiSourcePlanTomlReader.hpp"

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

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId(value);
}

iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult Read(
	const std::string &text)
{
	return iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReader {}.read(text);
}

std::string RootGridToml()
{
	return R"toml(
format_id = "iggy:ascii-source-plan"
version = 1
source_id = "scenario:guard-room"
source_ref = "authoring:manual-fixture"

[grid]
width = 7
height = 4
background = "."
rows = [
  "#######",
  "#.....#",
  "#.....#",
  "#######",
]
)toml";
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
	const std::string text = "unsupported = true\n[grid]\nwidth = 1\nheight = 1\nrows = [\".\"]\n";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "unsupported input should not parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::UnsupportedSyntax, "unsupported input should report unsupported status");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::UnsupportedNestedShape), "unsupported input should report unsupported issue");
	Expect(result.issueCount == 1 && result.unsupportedIssueCount == 1, "unsupported input should mirror unsupported counts");
	Expect(result.input == text && result.inputSize == text.size(), "unsupported input should be copied for diagnostics");
}

void TestRootAndGridParse()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read(RootGridToml());

	Expect(result.ok(), "valid root/grid TOML should parse");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::Parsed, "valid root/grid TOML should report Parsed");
	Expect(result.sourceValidation.ok(), "valid root/grid TOML should pass nested source validation");
	Expect(result.plan.formatId == Id("iggy:ascii-source-plan"), "root format id should parse");
	Expect(result.plan.version == 1, "root version should parse");
	Expect(result.plan.hasSourceId && result.plan.sourceId == Id("scenario:guard-room"), "root source id should parse");
	Expect(result.plan.hasSourceRef && result.plan.sourceRef == Id("authoring:manual-fixture"), "root source ref should parse");
	Expect(result.plan.grid.width == 7 && result.plan.grid.height == 4, "grid dimensions should parse");
	Expect(result.plan.grid.backgroundGlyph == '.', "grid background glyph should parse");
	Expect(result.plan.grid.rows.size() == 4, "grid rows should parse");
	Expect(result.plan.grid.rows[1] == "#.....#", "grid row contents should be preserved");
}

void TestCommentsAndInlineRowsParse()
{
	const std::string text = R"toml(
# leading comment
format_id = "iggy:ascii-source-plan" # trailing comment
[grid]
width = 3
height = 2
background = "."
rows = ["###", "#.#"] # comment outside quoted strings
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(result.ok(), "comments and inline rows should parse");
	Expect(result.plan.grid.rows.size() == 2, "inline row array should parse");
	Expect(result.plan.grid.rows[0] == "###" && result.plan.grid.rows[1] == "#.#", "inline row values should be preserved");
}

void TestMissingGridFailsAsSyntax()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result =
		Read("format_id = \"iggy:ascii-source-plan\"\nversion = 1\n");

	Expect(!result.ok(), "missing grid should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SyntaxInvalid, "missing grid should report syntax invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::MissingTable), "missing grid should report missing table");
}

void TestWrongTypeFailsAsTypeInvalid()
{
	const std::string text = R"toml(
format_id = "iggy:ascii-source-plan"
[grid]
width = "7"
height = 1
background = "."
rows = ["."]
)toml";
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "wrong typed width should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "wrong typed width should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::WrongType), "wrong typed width should report wrong type");
}

void TestRaggedRowsSurfaceSourcePlanValidation()
{
	std::string text = RootGridToml();
	const std::size_t start = text.find("\"#.....#\"");
	text.replace(start, std::string("\"#.....#\"").size(), "\"#....#\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "ragged parsed rows should fail source validation");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid, "ragged parsed rows should report source plan invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::SourcePlanInvalid), "ragged parsed rows should add source-plan issue");
	Expect(!result.sourceValidation.ok(), "ragged parsed rows should preserve nested validation failure");
	Expect(result.sourcePlanIssueCount > 0, "ragged parsed rows should mirror nested source validation issue count");
}

void TestInvalidBackgroundGlyphFailsAsTypeInvalid()
{
	std::string text = RootGridToml();
	const std::size_t start = text.find("background = \".\"");
	text.replace(start, std::string("background = \".\"").size(), "background = \"..\"");
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadResult result = Read(text);

	Expect(!result.ok(), "multi-character background glyph should fail");
	Expect(result.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::TypeInvalid, "multi-character background glyph should report type invalid");
	Expect(HasIssue(result, iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssueCode::InvalidGlyphString), "multi-character background glyph should report invalid glyph");
}

} // namespace

int main()
{
	TestEmptyInputFailsDeterministically();
	TestUnsupportedInputFailsDeterministically();
	TestRootAndGridParse();
	TestCommentsAndInlineRowsParse();
	TestMissingGridFailsAsSyntax();
	TestWrongTypeFailsAsTypeInvalid();
	TestRaggedRowsSurfaceSourcePlanValidation();
	TestInvalidBackgroundGlyphFailsAsTypeInvalid();
	return Failures;
}
