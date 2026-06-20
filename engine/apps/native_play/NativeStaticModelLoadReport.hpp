#pragma once

#include "NativeStaticMeshAssetLoader.hpp"
#include "NativeStaticModelPolicy.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace iggy::native_play {

enum class NativeStaticModelLoadStatus {
	MissingPolicyRef,
	Loaded,
	LoadFailed,
};

[[nodiscard]] inline const char *NativeStaticModelLoadStatusText(
	NativeStaticModelLoadStatus status)
{
	switch (status) {
	case NativeStaticModelLoadStatus::MissingPolicyRef:
		return "MissingPolicyRef";
	case NativeStaticModelLoadStatus::Loaded:
		return "Loaded";
	case NativeStaticModelLoadStatus::LoadFailed:
		return "LoadFailed";
	}
	return "Unknown";
}

enum class NativeStaticModelFallbackKind {
	Cube,
	ProceduralBean,
	ProceduralNpcMarker,
};

[[nodiscard]] inline const char *NativeStaticModelFallbackKindText(
	NativeStaticModelFallbackKind fallback)
{
	switch (fallback) {
	case NativeStaticModelFallbackKind::Cube:
		return "Cube";
	case NativeStaticModelFallbackKind::ProceduralBean:
		return "ProceduralBean";
	case NativeStaticModelFallbackKind::ProceduralNpcMarker:
		return "ProceduralNpcMarker";
	}
	return "Unknown";
}

struct NativeStaticModelLoadEntry {
	NativeStaticModelSlot slot = NativeStaticModelSlot::Player;
	std::string meshFilename;
	NativeStaticModelLoadStatus status =
		NativeStaticModelLoadStatus::MissingPolicyRef;
	NativeStaticModelFallbackKind fallback =
		NativeStaticModelFallbackKind::ProceduralBean;
	std::size_t issueCount = 0;
	std::size_t vertexCount = 0;
	std::size_t indexCount = 0;
};

struct NativeStaticModelLoadReport {
	std::vector<NativeStaticModelLoadEntry> entries;
	std::size_t loadedCount = 0;
	std::size_t failedCount = 0;
	std::size_t missingCount = 0;
};

[[nodiscard]] inline std::array<NativeStaticModelSlot, 4>
NativeStaticModelReportSlots()
{
	return {
		NativeStaticModelSlot::Floor,
		NativeStaticModelSlot::Wall,
		NativeStaticModelSlot::NpcActor,
		NativeStaticModelSlot::Player,
	};
}

[[nodiscard]] inline NativeStaticModelFallbackKind
NativeStaticModelFallbackForSlot(NativeStaticModelSlot slot)
{
	switch (slot) {
	case NativeStaticModelSlot::Floor:
	case NativeStaticModelSlot::Wall:
		return NativeStaticModelFallbackKind::Cube;
	case NativeStaticModelSlot::NpcActor:
		return NativeStaticModelFallbackKind::ProceduralNpcMarker;
	case NativeStaticModelSlot::Player:
		return NativeStaticModelFallbackKind::ProceduralBean;
	}
	return NativeStaticModelFallbackKind::ProceduralBean;
}

[[nodiscard]] inline NativeStaticModelLoadReport BuildNativeStaticModelLoadReport(
	const NativeStaticModelPolicy &policy,
	const std::filesystem::path &assetRoot)
{
	NativeStaticModelLoadReport report;
	for (const NativeStaticModelSlot slot : NativeStaticModelReportSlots()) {
		NativeStaticModelLoadEntry entry;
		entry.slot = slot;
		entry.fallback = NativeStaticModelFallbackForSlot(slot);

		const NativeStaticModelAssetRef *asset =
			FindNativeStaticModelAsset(policy, slot);
		if (asset == nullptr) {
			entry.status = NativeStaticModelLoadStatus::MissingPolicyRef;
			++report.missingCount;
			report.entries.push_back(entry);
			continue;
		}

		entry.meshFilename = asset->meshFilename;
		const NativeStaticMeshAssetLoadResult load =
			LoadNativeStaticMeshAssetFile(assetRoot / asset->meshFilename);
		entry.issueCount = load.issues.size();
		if (load.loaded()) {
			entry.status = NativeStaticModelLoadStatus::Loaded;
			entry.vertexCount = load.asset.vertices.size();
			entry.indexCount = load.asset.indices.size();
			++report.loadedCount;
		} else {
			entry.status = NativeStaticModelLoadStatus::LoadFailed;
			++report.failedCount;
		}
		report.entries.push_back(entry);
	}
	return report;
}

[[nodiscard]] inline std::string BuildNativeStaticModelLoadReportText(
	const NativeStaticModelLoadReport &report)
{
	std::ostringstream stream;
	stream
		<< "static-model-load-report"
		<< " loaded=" << report.loadedCount
		<< " failed=" << report.failedCount
		<< " missing=" << report.missingCount
		<< "\n";
	for (const NativeStaticModelLoadEntry &entry : report.entries) {
		stream
			<< "slot=" << NativeStaticModelSlotText(entry.slot)
			<< " filename=" << (entry.meshFilename.empty() ? "<missing>" : entry.meshFilename)
			<< " status=" << NativeStaticModelLoadStatusText(entry.status)
			<< " fallback=" << NativeStaticModelFallbackKindText(entry.fallback)
			<< " vertices=" << entry.vertexCount
			<< " indices=" << entry.indexCount
			<< " issues=" << entry.issueCount
			<< "\n";
	}
	return stream.str();
}

} // namespace iggy::native_play
