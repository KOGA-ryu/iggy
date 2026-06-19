#pragma once

#include "NativeStaticMeshAsset.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshAssetLoadIssueCode {
	FileOpenFailed,
	UnknownDirective,
	MalformedVertex,
	MalformedTriangle,
	IndexOutOfRange,
	ExtraToken,
	InvalidMesh,
};

struct NativeStaticMeshAssetLoadIssue {
	NativeStaticMeshAssetLoadIssueCode code =
		NativeStaticMeshAssetLoadIssueCode::InvalidMesh;
	std::size_t line = 0;
	std::string token;
};

struct NativeStaticMeshAssetLoadResult {
	NativeStaticMeshAsset asset;
	std::vector<NativeStaticMeshAssetLoadIssue> issues;

	[[nodiscard]] bool loaded() const
	{
		return issues.empty() && IsNativeStaticMeshAssetValid(asset);
	}
};

inline void AddNativeStaticMeshAssetLoadIssue(
	NativeStaticMeshAssetLoadResult &result,
	NativeStaticMeshAssetLoadIssueCode code,
	std::size_t line,
	std::string token = {})
{
	result.issues.push_back({ code, line, std::move(token) });
}

[[nodiscard]] inline bool NativeStaticMeshAssetLoaderHasExtraToken(
	std::istringstream &stream)
{
	std::string extra;
	return static_cast<bool>(stream >> extra);
}

[[nodiscard]] inline NativeStaticMeshAssetLoadResult LoadNativeStaticMeshAssetText(
	std::string_view text)
{
	NativeStaticMeshAssetLoadResult result;
	std::istringstream lines { std::string { text } };
	std::string line;
	std::size_t lineNumber = 0;
	while (std::getline(lines, line)) {
		++lineNumber;
		if (const std::size_t comment = line.find('#'); comment != std::string::npos)
			line.resize(comment);

		std::istringstream stream { line };
		std::string directive;
		if (!(stream >> directive))
			continue;

		if (directive == "v") {
			NativeStaticMeshVertex vertex;
			if (!(stream >> vertex.position.x >> vertex.position.y >> vertex.position.z >>
					vertex.color[0] >> vertex.color[1] >> vertex.color[2])) {
				AddNativeStaticMeshAssetLoadIssue(
					result,
					NativeStaticMeshAssetLoadIssueCode::MalformedVertex,
					lineNumber,
					directive);
				continue;
			}
			if (NativeStaticMeshAssetLoaderHasExtraToken(stream)) {
				AddNativeStaticMeshAssetLoadIssue(
					result,
					NativeStaticMeshAssetLoadIssueCode::ExtraToken,
					lineNumber,
					directive);
				continue;
			}
			result.asset.vertices.push_back(vertex);
			continue;
		}

		if (directive == "tri") {
			int a = 0;
			int b = 0;
			int c = 0;
			if (!(stream >> a >> b >> c)) {
				AddNativeStaticMeshAssetLoadIssue(
					result,
					NativeStaticMeshAssetLoadIssueCode::MalformedTriangle,
					lineNumber,
					directive);
				continue;
			}
			if (NativeStaticMeshAssetLoaderHasExtraToken(stream)) {
				AddNativeStaticMeshAssetLoadIssue(
					result,
					NativeStaticMeshAssetLoadIssueCode::ExtraToken,
					lineNumber,
					directive);
				continue;
			}
			constexpr int MaxIndex =
				static_cast<int>(std::numeric_limits<std::uint16_t>::max());
			if (a < 0 || b < 0 || c < 0 || a > MaxIndex || b > MaxIndex || c > MaxIndex) {
				AddNativeStaticMeshAssetLoadIssue(
					result,
					NativeStaticMeshAssetLoadIssueCode::IndexOutOfRange,
					lineNumber,
					directive);
				continue;
			}
			result.asset.indices.push_back(static_cast<std::uint16_t>(a));
			result.asset.indices.push_back(static_cast<std::uint16_t>(b));
			result.asset.indices.push_back(static_cast<std::uint16_t>(c));
			continue;
		}

		AddNativeStaticMeshAssetLoadIssue(
			result,
			NativeStaticMeshAssetLoadIssueCode::UnknownDirective,
			lineNumber,
			directive);
	}

	if (result.issues.empty() && !IsNativeStaticMeshAssetValid(result.asset)) {
		AddNativeStaticMeshAssetLoadIssue(
			result,
			NativeStaticMeshAssetLoadIssueCode::InvalidMesh,
			0);
	}
	return result;
}

[[nodiscard]] inline NativeStaticMeshAssetLoadResult LoadNativeStaticMeshAssetFile(
	const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		NativeStaticMeshAssetLoadResult result;
		AddNativeStaticMeshAssetLoadIssue(
			result,
			NativeStaticMeshAssetLoadIssueCode::FileOpenFailed,
			0,
			path.string());
		return result;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return LoadNativeStaticMeshAssetText(buffer.str());
}

} // namespace iggy::native_play
