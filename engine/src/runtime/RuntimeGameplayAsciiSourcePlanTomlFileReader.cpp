#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"

#include <fstream>
#include <iterator>

namespace iggy::runtime {
namespace {

void addIssue(
	RuntimeGameplayAsciiSourcePlanTomlFileReadResult &result,
	RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode code,
	std::string detail)
{
	RuntimeGameplayAsciiSourcePlanTomlFileReadIssue issue;
	issue.code = code;
	issue.path = result.path;
	issue.detail = std::move(detail);
	result.issues.push_back(std::move(issue));
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanTomlFileReadResult::ok() const
{
	return status == RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::Parsed;
}

RuntimeGameplayAsciiSourcePlanTomlFileReadResult
RuntimeGameplayAsciiSourcePlanTomlFileReader::read(const std::filesystem::path &path) const
{
	RuntimeGameplayAsciiSourcePlanTomlFileReadResult result;
	result.path = path;

	if (path.empty()) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::EmptyPath;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::EmptyPath,
			"empty TOML source-plan path");
		return result;
	}

	std::error_code error;
	if (!std::filesystem::exists(path, error)) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::MissingFile;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::MissingFile,
			"TOML source-plan file does not exist");
		return result;
	}
	if (error) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::OpenFailed;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::OpenFailed,
			"could not inspect TOML source-plan path");
		return result;
	}

	if (!std::filesystem::is_regular_file(path, error)) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::NonRegularFile;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::NonRegularFile,
			"TOML source-plan path is not a regular file");
		return result;
	}
	if (error) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::OpenFailed;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::OpenFailed,
			"could not classify TOML source-plan path");
		return result;
	}

	std::ifstream stream(path, std::ios::binary);
	if (!stream.is_open()) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::OpenFailed;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::OpenFailed,
			"could not open TOML source-plan file");
		return result;
	}

	const std::string text(
		(std::istreambuf_iterator<char>(stream)),
		std::istreambuf_iterator<char>());
	if (stream.bad()) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::ReadFailed;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::ReadFailed,
			"could not read TOML source-plan file");
		return result;
	}

	result.bytesRead = text.size();
	result.text = RuntimeGameplayAsciiSourcePlanTomlReader {}.read(text);
	if (!result.text.ok()) {
		result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::TomlReadFailed;
		addIssue(result, RuntimeGameplayAsciiSourcePlanTomlFileReadIssueCode::TomlReadFailed,
			"TOML source-plan text reader failed");
		return result;
	}

	result.status = RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::Parsed;
	return result;
}

} // namespace iggy::runtime
