#pragma once

#include "NativeStaticMeshExportPackagePolicy.hpp"

#include <cstddef>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy::native_play {

inline constexpr const char *NativeStaticMeshExportPackageManifestSidecarFilename =
	"static-mesh-export-package-manifest.txt";

enum class NativeStaticMeshExportPackageManifestStatus {
	Built,
	InvalidPolicy,
};

[[nodiscard]] inline const char *NativeStaticMeshExportPackageManifestStatusText(
	NativeStaticMeshExportPackageManifestStatus status)
{
	switch (status) {
	case NativeStaticMeshExportPackageManifestStatus::Built:
		return "Built";
	case NativeStaticMeshExportPackageManifestStatus::InvalidPolicy:
		return "InvalidPolicy";
	}
	return "Unknown";
}

struct NativeStaticMeshExportPackageManifestResult {
	NativeStaticMeshExportPackageManifestStatus status =
		NativeStaticMeshExportPackageManifestStatus::InvalidPolicy;
	std::string text;
	std::size_t issueCount = 0;

	[[nodiscard]] bool written() const
	{
		return status == NativeStaticMeshExportPackageManifestStatus::Built &&
			!text.empty();
	}
};

[[nodiscard]] inline std::string BuildNativeStaticMeshExportPackageManifestFailureText(
	const NativeStaticMeshExportPackageManifestResult &result)
{
	std::ostringstream stream;
	stream
		<< "static mesh export package manifest failed: "
		<< NativeStaticMeshExportPackageManifestStatusText(result.status)
		<< " issues=" << result.issueCount;
	return stream.str();
}

struct NativeStaticMeshExportPackageManifestAssetRow {
	std::string name;
	std::string filename;
};

struct NativeStaticMeshExportPackageManifestDocument {
	std::string formatId;
	int version = 0;
	std::string manifestFilename;
	std::vector<NativeStaticMeshExportPackageManifestAssetRow> assets;
};

enum class NativeStaticMeshExportPackageManifestReadIssueCode {
	FileOpenFailed,
	EmptyInput,
	MalformedHeader,
	UnsupportedFormatId,
	UnsupportedVersion,
	MalformedAssetCount,
	MissingField,
	MalformedAssetRow,
	AssetCountMismatch,
	DuplicateAssetName,
	DuplicateAssetFilename,
	ExtraToken,
	UnexpectedLine,
};

[[nodiscard]] inline const char *NativeStaticMeshExportPackageManifestReadIssueCodeText(
	NativeStaticMeshExportPackageManifestReadIssueCode code)
{
	switch (code) {
	case NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed:
		return "FileOpenFailed";
	case NativeStaticMeshExportPackageManifestReadIssueCode::EmptyInput:
		return "EmptyInput";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader:
		return "MalformedHeader";
	case NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedFormatId:
		return "UnsupportedFormatId";
	case NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedVersion:
		return "UnsupportedVersion";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetCount:
		return "MalformedAssetCount";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MissingField:
		return "MissingField";
	case NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetRow:
		return "MalformedAssetRow";
	case NativeStaticMeshExportPackageManifestReadIssueCode::AssetCountMismatch:
		return "AssetCountMismatch";
	case NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetName:
		return "DuplicateAssetName";
	case NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetFilename:
		return "DuplicateAssetFilename";
	case NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken:
		return "ExtraToken";
	case NativeStaticMeshExportPackageManifestReadIssueCode::UnexpectedLine:
		return "UnexpectedLine";
	}
	return "Unknown";
}

struct NativeStaticMeshExportPackageManifestReadIssue {
	NativeStaticMeshExportPackageManifestReadIssueCode code =
		NativeStaticMeshExportPackageManifestReadIssueCode::EmptyInput;
	std::size_t line = 0;
	std::string token;
};

struct NativeStaticMeshExportPackageManifestReadResult {
	NativeStaticMeshExportPackageManifestDocument document;
	std::vector<NativeStaticMeshExportPackageManifestReadIssue> issues;

	[[nodiscard]] bool read() const
	{
		return issues.empty();
	}
};

inline void AddNativeStaticMeshExportPackageManifestReadIssue(
	NativeStaticMeshExportPackageManifestReadResult &result,
	NativeStaticMeshExportPackageManifestReadIssueCode code,
	std::size_t line,
	std::string token = {})
{
	result.issues.push_back({ code, line, std::move(token) });
}

[[nodiscard]] inline bool NativeStaticMeshExportPackageManifestTokenValue(
	const std::string &token,
	const char *field,
	std::string &value)
{
	const std::string prefix = std::string { field } + "=";
	if (token.rfind(prefix, 0) != 0)
		return false;
	value = token.substr(prefix.size());
	return !value.empty();
}

[[nodiscard]] inline bool NativeStaticMeshExportPackageManifestParseUnsigned(
	const std::string &value,
	std::size_t &parsed)
{
	if (value.empty())
		return false;
	std::size_t result = 0;
	for (const char c : value) {
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
		result = result * 10 + static_cast<std::size_t>(c - '0');
	}
	parsed = result;
	return true;
}

[[nodiscard]] inline bool NativeStaticMeshExportPackageManifestParseInt(
	const std::string &value,
	int &parsed)
{
	std::size_t unsignedValue = 0;
	if (!NativeStaticMeshExportPackageManifestParseUnsigned(value, unsignedValue))
		return false;
	parsed = static_cast<int>(unsignedValue);
	return static_cast<std::size_t>(parsed) == unsignedValue;
}

[[nodiscard]] inline bool NativeStaticMeshExportPackageManifestHasExtraToken(
	std::istringstream &stream)
{
	std::string extra;
	return static_cast<bool>(stream >> extra);
}

[[nodiscard]] inline bool NativeStaticMeshExportPackageManifestHasDuplicateAssetName(
	const std::vector<NativeStaticMeshExportPackageManifestAssetRow> &assets,
	const std::string &name)
{
	for (const NativeStaticMeshExportPackageManifestAssetRow &asset : assets) {
		if (asset.name == name)
			return true;
	}
	return false;
}

[[nodiscard]] inline bool NativeStaticMeshExportPackageManifestHasDuplicateAssetFilename(
	const std::vector<NativeStaticMeshExportPackageManifestAssetRow> &assets,
	const std::string &filename)
{
	for (const NativeStaticMeshExportPackageManifestAssetRow &asset : assets) {
		if (asset.filename == filename)
			return true;
	}
	return false;
}

[[nodiscard]] inline NativeStaticMeshExportPackageManifestResult
BuildNativeStaticMeshExportPackageManifestText(
	const NativeStaticMeshExportPackagePolicy &policy)
{
	NativeStaticMeshExportPackageManifestResult result;
	const NativeStaticMeshExportPackagePolicyValidationResult validation =
		ValidateNativeStaticMeshExportPackagePolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshExportPackageManifestStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	std::ostringstream stream;
	stream
		<< "static-mesh-export-package-manifest"
		<< " format=" << policy.formatId
		<< " version=" << policy.version
		<< " manifest=" << policy.manifestFilename
		<< " assets=" << policy.meshPolicy.assets.size()
		<< "\n";
	for (const NativeStaticMeshExportAssetRef &asset : policy.meshPolicy.assets) {
		stream
			<< "asset=" << asset.name
			<< " filename=" << asset.defaultFilename
			<< "\n";
	}

	result.status = NativeStaticMeshExportPackageManifestStatus::Built;
	result.text = stream.str();
	return result;
}

[[nodiscard]] inline NativeStaticMeshExportPackageManifestReadResult
ReadNativeStaticMeshExportPackageManifestText(std::string_view text)
{
	NativeStaticMeshExportPackageManifestReadResult result;
	if (text.empty()) {
		AddNativeStaticMeshExportPackageManifestReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::EmptyInput,
			0);
		return result;
	}

	std::istringstream lines { std::string { text } };
	std::string line;
	std::size_t lineNumber = 0;
	std::size_t expectedAssetCount = 0;
	bool hasHeader = false;

	while (std::getline(lines, line)) {
		++lineNumber;
		std::istringstream stream { line };
		std::string directive;
		if (!(stream >> directive)) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::UnexpectedLine,
				lineNumber);
			continue;
		}

		if (!hasHeader) {
			hasHeader = true;
			if (directive != "static-mesh-export-package-manifest") {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader,
					lineNumber,
					directive);
				continue;
			}

			std::string formatToken;
			std::string versionToken;
			std::string manifestToken;
			std::string assetsToken;
			if (!(stream >> formatToken >> versionToken >> manifestToken >> assetsToken)) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::MissingField,
					lineNumber);
				continue;
			}
			if (NativeStaticMeshExportPackageManifestHasExtraToken(stream)) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken,
					lineNumber);
				continue;
			}

			std::string versionValue;
			std::string assetsValue;
			if (!NativeStaticMeshExportPackageManifestTokenValue(
					formatToken,
					"format",
					result.document.formatId) ||
					!NativeStaticMeshExportPackageManifestTokenValue(
						versionToken,
						"version",
						versionValue) ||
					!NativeStaticMeshExportPackageManifestTokenValue(
						manifestToken,
						"manifest",
						result.document.manifestFilename) ||
					!NativeStaticMeshExportPackageManifestTokenValue(
						assetsToken,
						"assets",
						assetsValue)) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::MissingField,
					lineNumber);
				continue;
			}

			if (result.document.formatId != NativeStaticMeshExportPackageFormatId) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedFormatId,
					lineNumber,
					result.document.formatId);
			}
			if (!NativeStaticMeshExportPackageManifestParseInt(
					versionValue,
					result.document.version) ||
					result.document.version !=
						NativeStaticMeshExportPackageFormatVersion) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedVersion,
					lineNumber,
					versionValue);
			}
			if (NativeStaticMeshExportFilenameContainsSeparator(
					result.document.manifestFilename)) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader,
					lineNumber,
					result.document.manifestFilename);
			}
			if (!NativeStaticMeshExportPackageManifestParseUnsigned(
					assetsValue,
					expectedAssetCount)) {
				AddNativeStaticMeshExportPackageManifestReadIssue(
					result,
					NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetCount,
					lineNumber,
					assetsValue);
			}
			continue;
		}

		if (directive.rfind("asset=", 0) != 0) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::UnexpectedLine,
				lineNumber,
				directive);
			continue;
		}

		NativeStaticMeshExportPackageManifestAssetRow asset;
		asset.name = directive.substr(std::string { "asset=" }.size());
		std::string filenameToken;
		if (!(stream >> filenameToken)) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::MissingField,
				lineNumber,
				directive);
			continue;
		}
		if (NativeStaticMeshExportPackageManifestHasExtraToken(stream)) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken,
				lineNumber,
				directive);
			continue;
		}
		if (!NativeStaticMeshExportPackageManifestTokenValue(
				filenameToken,
				"filename",
				asset.filename) ||
				asset.name.empty() ||
				asset.filename.empty() ||
				NativeStaticMeshExportFilenameContainsSeparator(asset.filename)) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetRow,
				lineNumber,
				directive);
			continue;
		}
		if (NativeStaticMeshExportPackageManifestHasDuplicateAssetName(
				result.document.assets,
				asset.name)) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetName,
				lineNumber,
				asset.name);
		}
		if (NativeStaticMeshExportPackageManifestHasDuplicateAssetFilename(
				result.document.assets,
				asset.filename)) {
			AddNativeStaticMeshExportPackageManifestReadIssue(
				result,
				NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetFilename,
				lineNumber,
				asset.filename);
		}
		result.document.assets.push_back(asset);
	}

	if (!hasHeader) {
		AddNativeStaticMeshExportPackageManifestReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader,
			0);
		return result;
	}
	if (result.document.assets.size() != expectedAssetCount) {
		AddNativeStaticMeshExportPackageManifestReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::AssetCountMismatch,
			0,
			std::to_string(expectedAssetCount));
	}

	return result;
}

[[nodiscard]] inline NativeStaticMeshExportPackageManifestReadResult
ReadNativeStaticMeshExportPackageManifestFile(const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		NativeStaticMeshExportPackageManifestReadResult result;
		AddNativeStaticMeshExportPackageManifestReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed,
			0,
			path.string());
		return result;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return ReadNativeStaticMeshExportPackageManifestText(buffer.str());
}

} // namespace iggy::native_play
