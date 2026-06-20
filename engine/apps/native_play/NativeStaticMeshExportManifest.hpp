#pragma once

#include "NativeStaticMeshExportPolicy.hpp"
#include "NativeStaticMeshExportReport.hpp"

#include <cstddef>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshExportManifestStatus {
	Built,
	InvalidPolicy,
	WriterFailed,
};

struct NativeStaticMeshExportManifestResult {
	NativeStaticMeshExportManifestStatus status =
		NativeStaticMeshExportManifestStatus::InvalidPolicy;
	std::string text;
	std::size_t issueCount = 0;

	[[nodiscard]] bool written() const
	{
		return status == NativeStaticMeshExportManifestStatus::Built &&
			!text.empty();
	}
};

struct NativeStaticMeshExportManifestAssetRow {
	std::string name;
	std::string filename;
	std::size_t vertexCount = 0;
	std::size_t indexCount = 0;
	std::size_t byteCount = 0;
};

struct NativeStaticMeshExportManifestDocument {
	int version = 0;
	std::size_t assetCount = 0;
	std::size_t byteCount = 0;
	std::vector<NativeStaticMeshExportManifestAssetRow> assets;
};

enum class NativeStaticMeshExportManifestReadIssueCode {
	FileOpenFailed,
	EmptyInput,
	MalformedHeader,
	UnsupportedVersion,
	MalformedAssetCount,
	MalformedByteCount,
	MissingField,
	MalformedAssetRow,
	AssetCountMismatch,
	ByteCountMismatch,
	DuplicateAssetName,
	DuplicateAssetFilename,
	ExtraToken,
	UnexpectedLine,
};

[[nodiscard]] inline const char *NativeStaticMeshExportManifestReadIssueCodeText(
	NativeStaticMeshExportManifestReadIssueCode code)
{
	switch (code) {
	case NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed:
		return "FileOpenFailed";
	case NativeStaticMeshExportManifestReadIssueCode::EmptyInput:
		return "EmptyInput";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedHeader:
		return "MalformedHeader";
	case NativeStaticMeshExportManifestReadIssueCode::UnsupportedVersion:
		return "UnsupportedVersion";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedAssetCount:
		return "MalformedAssetCount";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedByteCount:
		return "MalformedByteCount";
	case NativeStaticMeshExportManifestReadIssueCode::MissingField:
		return "MissingField";
	case NativeStaticMeshExportManifestReadIssueCode::MalformedAssetRow:
		return "MalformedAssetRow";
	case NativeStaticMeshExportManifestReadIssueCode::AssetCountMismatch:
		return "AssetCountMismatch";
	case NativeStaticMeshExportManifestReadIssueCode::ByteCountMismatch:
		return "ByteCountMismatch";
	case NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetName:
		return "DuplicateAssetName";
	case NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetFilename:
		return "DuplicateAssetFilename";
	case NativeStaticMeshExportManifestReadIssueCode::ExtraToken:
		return "ExtraToken";
	case NativeStaticMeshExportManifestReadIssueCode::UnexpectedLine:
		return "UnexpectedLine";
	}
	return "Unknown";
}

struct NativeStaticMeshExportManifestReadIssue {
	NativeStaticMeshExportManifestReadIssueCode code =
		NativeStaticMeshExportManifestReadIssueCode::EmptyInput;
	std::size_t line = 0;
	std::string token;
};

struct NativeStaticMeshExportManifestReadResult {
	NativeStaticMeshExportManifestDocument document;
	std::vector<NativeStaticMeshExportManifestReadIssue> issues;

	[[nodiscard]] bool read() const
	{
		return issues.empty();
	}
};

inline void AddNativeStaticMeshExportManifestReadIssue(
	NativeStaticMeshExportManifestReadResult &result,
	NativeStaticMeshExportManifestReadIssueCode code,
	std::size_t line,
	std::string token = {})
{
	result.issues.push_back({ code, line, std::move(token) });
}

[[nodiscard]] inline bool NativeStaticMeshExportManifestTokenValue(
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

[[nodiscard]] inline bool NativeStaticMeshExportManifestParseUnsigned(
	const std::string &value,
	std::size_t &parsed)
{
	if (value.empty())
		return false;
	std::size_t result = 0;
	for (const char c : value) {
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
		const std::size_t digit = static_cast<std::size_t>(c - '0');
		if (result > (std::numeric_limits<std::size_t>::max() - digit) / 10)
			return false;
		result = result * 10 + digit;
	}
	parsed = result;
	return true;
}

[[nodiscard]] inline bool NativeStaticMeshExportManifestParseInt(
	const std::string &value,
	int &parsed)
{
	std::size_t unsignedValue = 0;
	if (!NativeStaticMeshExportManifestParseUnsigned(value, unsignedValue))
		return false;
	if (unsignedValue > static_cast<std::size_t>(std::numeric_limits<int>::max()))
		return false;
	parsed = static_cast<int>(unsignedValue);
	return true;
}

[[nodiscard]] inline bool NativeStaticMeshExportManifestHasExtraToken(
	std::istringstream &stream)
{
	std::string extra;
	return static_cast<bool>(stream >> extra);
}

[[nodiscard]] inline bool NativeStaticMeshExportManifestHasDuplicateAssetName(
	const std::vector<NativeStaticMeshExportManifestAssetRow> &assets,
	const std::string &name)
{
	for (const NativeStaticMeshExportManifestAssetRow &asset : assets) {
		if (asset.name == name)
			return true;
	}
	return false;
}

[[nodiscard]] inline bool NativeStaticMeshExportManifestHasDuplicateAssetFilename(
	const std::vector<NativeStaticMeshExportManifestAssetRow> &assets,
	const std::string &filename)
{
	for (const NativeStaticMeshExportManifestAssetRow &asset : assets) {
		if (asset.filename == filename)
			return true;
	}
	return false;
}

[[nodiscard]] inline NativeStaticMeshExportManifestResult
BuildNativeStaticMeshExportManifestText(
	const NativeStaticMeshExportPolicy &policy)
{
	NativeStaticMeshExportManifestResult result;
	const NativeStaticMeshExportPolicyValidationResult validation =
		ValidateNativeStaticMeshExportPolicy(policy);
	if (!validation.valid()) {
		result.status = NativeStaticMeshExportManifestStatus::InvalidPolicy;
		result.issueCount = validation.issues.size();
		return result;
	}

	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);
	if (report.issueCount != 0 || report.writableCount != report.assetCount) {
		result.status = NativeStaticMeshExportManifestStatus::WriterFailed;
		result.issueCount = report.issueCount;
		return result;
	}

	std::ostringstream stream;
	stream
		<< "static-mesh-export-manifest"
		<< " version=1"
		<< " assets=" << report.assetCount
		<< " bytes=" << report.byteCount
		<< "\n";
	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		stream
			<< "asset=" << entry.name
			<< " filename=" << entry.defaultFilename
			<< " vertices=" << entry.vertexCount
			<< " indices=" << entry.indexCount
			<< " bytes=" << entry.byteCount
			<< "\n";
	}

	result.status = NativeStaticMeshExportManifestStatus::Built;
	result.text = stream.str();
	return result;
}

[[nodiscard]] inline NativeStaticMeshExportManifestReadResult
ReadNativeStaticMeshExportManifestText(std::string_view text)
{
	NativeStaticMeshExportManifestReadResult result;
	if (text.empty()) {
		AddNativeStaticMeshExportManifestReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::EmptyInput,
			0);
		return result;
	}

	std::istringstream lines { std::string { text } };
	std::string line;
	std::size_t lineNumber = 0;
	std::size_t expectedAssetCount = 0;
	std::size_t expectedByteCount = 0;
	bool hasHeader = false;

	while (std::getline(lines, line)) {
		++lineNumber;
		std::istringstream stream { line };
		std::string directive;
		if (!(stream >> directive)) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::UnexpectedLine,
				lineNumber);
			continue;
		}

		if (!hasHeader) {
			hasHeader = true;
			if (directive != "static-mesh-export-manifest") {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::MalformedHeader,
					lineNumber,
					directive);
				continue;
			}

			std::string versionToken;
			std::string assetsToken;
			std::string bytesToken;
			if (!(stream >> versionToken >> assetsToken >> bytesToken)) {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::MissingField,
					lineNumber);
				continue;
			}
			if (NativeStaticMeshExportManifestHasExtraToken(stream)) {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::ExtraToken,
					lineNumber);
				continue;
			}

			std::string versionValue;
			std::string assetsValue;
			std::string bytesValue;
			if (!NativeStaticMeshExportManifestTokenValue(
					versionToken,
					"version",
					versionValue) ||
					!NativeStaticMeshExportManifestTokenValue(
						assetsToken,
						"assets",
						assetsValue) ||
					!NativeStaticMeshExportManifestTokenValue(
						bytesToken,
						"bytes",
						bytesValue)) {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::MissingField,
					lineNumber);
				continue;
			}

			if (!NativeStaticMeshExportManifestParseInt(
					versionValue,
					result.document.version) ||
					result.document.version != 1) {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::UnsupportedVersion,
					lineNumber,
					versionValue);
			}
			if (!NativeStaticMeshExportManifestParseUnsigned(
					assetsValue,
					expectedAssetCount)) {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::MalformedAssetCount,
					lineNumber,
					assetsValue);
			}
			if (!NativeStaticMeshExportManifestParseUnsigned(
					bytesValue,
					expectedByteCount)) {
				AddNativeStaticMeshExportManifestReadIssue(
					result,
					NativeStaticMeshExportManifestReadIssueCode::MalformedByteCount,
					lineNumber,
					bytesValue);
			}
			result.document.assetCount = expectedAssetCount;
			result.document.byteCount = expectedByteCount;
			continue;
		}

		if (directive.rfind("asset=", 0) != 0) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::UnexpectedLine,
				lineNumber,
				directive);
			continue;
		}

		NativeStaticMeshExportManifestAssetRow asset;
		asset.name = directive.substr(std::string { "asset=" }.size());
		std::string filenameToken;
		std::string verticesToken;
		std::string indicesToken;
		std::string bytesToken;
		if (!(stream >> filenameToken >> verticesToken >> indicesToken >> bytesToken)) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::MissingField,
				lineNumber,
				directive);
			continue;
		}
		if (NativeStaticMeshExportManifestHasExtraToken(stream)) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::ExtraToken,
				lineNumber,
				directive);
			continue;
		}

		std::string vertexCountValue;
		std::string indexCountValue;
		std::string byteCountValue;
		if (!NativeStaticMeshExportManifestTokenValue(
				filenameToken,
				"filename",
				asset.filename) ||
				!NativeStaticMeshExportManifestTokenValue(
					verticesToken,
					"vertices",
					vertexCountValue) ||
				!NativeStaticMeshExportManifestTokenValue(
					indicesToken,
					"indices",
					indexCountValue) ||
				!NativeStaticMeshExportManifestTokenValue(
					bytesToken,
					"bytes",
					byteCountValue) ||
				asset.name.empty() ||
				asset.filename.empty() ||
				NativeStaticMeshExportFilenameContainsSeparator(asset.filename) ||
				!NativeStaticMeshExportManifestParseUnsigned(
					vertexCountValue,
					asset.vertexCount) ||
				!NativeStaticMeshExportManifestParseUnsigned(
					indexCountValue,
					asset.indexCount) ||
				!NativeStaticMeshExportManifestParseUnsigned(
					byteCountValue,
					asset.byteCount)) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::MalformedAssetRow,
				lineNumber,
				directive);
			continue;
		}
		if (NativeStaticMeshExportManifestHasDuplicateAssetName(
				result.document.assets,
				asset.name)) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetName,
				lineNumber,
				asset.name);
		}
		if (NativeStaticMeshExportManifestHasDuplicateAssetFilename(
				result.document.assets,
				asset.filename)) {
			AddNativeStaticMeshExportManifestReadIssue(
				result,
				NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetFilename,
				lineNumber,
				asset.filename);
		}
		result.document.assets.push_back(asset);
	}

	if (!hasHeader) {
		AddNativeStaticMeshExportManifestReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::MalformedHeader,
			0);
		return result;
	}
	if (result.document.assets.size() != expectedAssetCount) {
		AddNativeStaticMeshExportManifestReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::AssetCountMismatch,
			0,
			std::to_string(expectedAssetCount));
	}

	std::size_t actualByteCount = 0;
	bool byteCountOverflow = false;
	for (const NativeStaticMeshExportManifestAssetRow &asset :
			result.document.assets) {
		if (actualByteCount >
				std::numeric_limits<std::size_t>::max() - asset.byteCount) {
			byteCountOverflow = true;
			break;
		}
		actualByteCount += asset.byteCount;
	}
	if (byteCountOverflow || actualByteCount != expectedByteCount) {
		AddNativeStaticMeshExportManifestReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::ByteCountMismatch,
			0,
			std::to_string(expectedByteCount));
	}

	return result;
}

[[nodiscard]] inline NativeStaticMeshExportManifestReadResult
ReadNativeStaticMeshExportManifestFile(const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		NativeStaticMeshExportManifestReadResult result;
		AddNativeStaticMeshExportManifestReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed,
			0,
			path.string());
		return result;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return ReadNativeStaticMeshExportManifestText(buffer.str());
}

} // namespace iggy::native_play
