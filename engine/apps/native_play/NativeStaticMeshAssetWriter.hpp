#pragma once

#include "NativeStaticMeshAsset.hpp"

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticMeshAssetWriteIssueCode {
	InvalidMesh,
	NonTriangleIndexCount,
};

[[nodiscard]] inline const char *NativeStaticMeshAssetWriteIssueCodeText(
	NativeStaticMeshAssetWriteIssueCode code)
{
	switch (code) {
	case NativeStaticMeshAssetWriteIssueCode::InvalidMesh:
		return "InvalidMesh";
	case NativeStaticMeshAssetWriteIssueCode::NonTriangleIndexCount:
		return "NonTriangleIndexCount";
	}
	return "Unknown";
}

struct NativeStaticMeshAssetWriteIssue {
	NativeStaticMeshAssetWriteIssueCode code =
		NativeStaticMeshAssetWriteIssueCode::InvalidMesh;
};

struct NativeStaticMeshAssetWriteResult {
	std::string text;
	std::vector<NativeStaticMeshAssetWriteIssue> issues;

	[[nodiscard]] bool written() const
	{
		return issues.empty() && !text.empty();
	}
};

[[nodiscard]] inline std::string BuildNativeStaticMeshAssetWriteFailureText(
	std::string_view name,
	const NativeStaticMeshAssetWriteResult &result)
{
	std::ostringstream stream;
	stream
		<< "failed to write static mesh asset: "
		<< name
		<< " issues=" << result.issues.size();
	return stream.str();
}

inline void AddNativeStaticMeshAssetWriteIssue(
	NativeStaticMeshAssetWriteResult &result,
	NativeStaticMeshAssetWriteIssueCode code)
{
	result.issues.push_back({ code });
}

[[nodiscard]] inline NativeStaticMeshAssetWriteResult WriteNativeStaticMeshAssetText(
	const NativeStaticMeshAsset &asset)
{
	NativeStaticMeshAssetWriteResult result;
	if (!IsNativeStaticMeshAssetValid(asset))
		AddNativeStaticMeshAssetWriteIssue(
			result,
			NativeStaticMeshAssetWriteIssueCode::InvalidMesh);
	if (asset.indices.size() % 3U != 0U)
		AddNativeStaticMeshAssetWriteIssue(
			result,
			NativeStaticMeshAssetWriteIssueCode::NonTriangleIndexCount);
	if (!result.issues.empty())
		return result;

	std::ostringstream stream;
	stream << std::setprecision(std::numeric_limits<float>::max_digits10);
	stream << "# Native static mesh asset\n";
	for (const NativeStaticMeshVertex &vertex : asset.vertices) {
		stream
			<< "v "
			<< vertex.position.x << ' '
			<< vertex.position.y << ' '
			<< vertex.position.z << ' '
			<< vertex.color[0] << ' '
			<< vertex.color[1] << ' '
			<< vertex.color[2] << '\n';
	}
	for (std::size_t index = 0; index < asset.indices.size(); index += 3U) {
		stream
			<< "tri "
			<< asset.indices[index] << ' '
			<< asset.indices[index + 1U] << ' '
			<< asset.indices[index + 2U] << '\n';
	}
	result.text = stream.str();
	return result;
}

} // namespace iggy::native_play
