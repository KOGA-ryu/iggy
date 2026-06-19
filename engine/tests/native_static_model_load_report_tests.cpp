#include "../apps/native_play/NativeStaticModelLoadReport.hpp"

#include <cstdlib>
#include <filesystem>

#include "support/TestHarness.hpp"

#ifndef IGGY_NATIVE_PLAY_TEST_ASSET_DIR
#error "IGGY_NATIVE_PLAY_TEST_ASSET_DIR must point at apps/native_play/assets"
#endif

namespace {

using iggy::native_play::BuildNativeStaticModelLoadReport;
using iggy::native_play::DefaultNativeStaticModelPolicy;
using iggy::native_play::NativeStaticModelFallbackKind;
using iggy::native_play::NativeStaticModelLoadEntry;
using iggy::native_play::NativeStaticModelLoadReport;
using iggy::native_play::NativeStaticModelLoadStatus;
using iggy::native_play::NativeStaticModelPolicy;
using iggy::native_play::NativeStaticModelSlot;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path AssetRoot()
{
	return std::filesystem::path(IGGY_NATIVE_PLAY_TEST_ASSET_DIR);
}

const NativeStaticModelLoadEntry *FindEntry(
	const NativeStaticModelLoadReport &report,
	NativeStaticModelSlot slot)
{
	for (const NativeStaticModelLoadEntry &entry : report.entries) {
		if (entry.slot == slot)
			return &entry;
	}
	return nullptr;
}

void ExpectLoadedEntry(
	const NativeStaticModelLoadReport &report,
	NativeStaticModelSlot slot,
	const char *filename,
	NativeStaticModelFallbackKind fallback,
	std::size_t vertexCount,
	std::size_t indexCount)
{
	const NativeStaticModelLoadEntry *entry = FindEntry(report, slot);
	Expect(entry != nullptr, "expected load report entry should exist");
	if (entry == nullptr)
		return;
	Expect(entry->meshFilename == filename, "loaded entry should preserve filename");
	Expect(entry->status == NativeStaticModelLoadStatus::Loaded, "entry should be loaded");
	Expect(entry->fallback == fallback, "entry should report fallback kind");
	Expect(entry->issueCount == 0, "loaded entry should have no issues");
	Expect(entry->vertexCount == vertexCount, "loaded entry should report vertex count");
	Expect(entry->indexCount == indexCount, "loaded entry should report index count");
}

void TestDefaultPolicyReportsAllCheckedInAssetsLoaded()
{
	const NativeStaticModelLoadReport report =
		BuildNativeStaticModelLoadReport(DefaultNativeStaticModelPolicy(), AssetRoot());

	Expect(report.entries.size() == 4, "default report should contain four fixed slots");
	Expect(report.loadedCount == 4, "default report should load all checked-in assets");
	Expect(report.failedCount == 0, "default report should not fail any checked-in assets");
	Expect(report.missingCount == 0, "default report should not miss policy refs");
	ExpectLoadedEntry(
		report,
		NativeStaticModelSlot::Floor,
		"floor.igmesh",
		NativeStaticModelFallbackKind::Cube,
		4,
		6);
	ExpectLoadedEntry(
		report,
		NativeStaticModelSlot::Wall,
		"wall.igmesh",
		NativeStaticModelFallbackKind::Cube,
		8,
		36);
	ExpectLoadedEntry(
		report,
		NativeStaticModelSlot::NpcActor,
		"npc.igmesh",
		NativeStaticModelFallbackKind::ProceduralNpcMarker,
		7,
		30);
	ExpectLoadedEntry(
		report,
		NativeStaticModelSlot::Player,
		"player.igmesh",
		NativeStaticModelFallbackKind::ProceduralBean,
		6,
		24);
}

void TestMissingPolicyRefReportsFallbackKind()
{
	const NativeStaticModelPolicy policy {
		{
			{ NativeStaticModelSlot::Floor, "floor.igmesh" },
			{ NativeStaticModelSlot::Wall, "wall.igmesh" },
			{ NativeStaticModelSlot::NpcActor, "npc.igmesh" },
		},
	};

	const NativeStaticModelLoadReport report =
		BuildNativeStaticModelLoadReport(policy, AssetRoot());

	const NativeStaticModelLoadEntry *player =
		FindEntry(report, NativeStaticModelSlot::Player);
	Expect(report.entries.size() == 4, "missing-ref report should still cover all fixed slots");
	Expect(report.loadedCount == 3, "missing-ref report should load listed valid assets");
	Expect(report.failedCount == 0, "missing-ref report should not mark missing refs as load failures");
	Expect(report.missingCount == 1, "missing-ref report should count missing refs");
	Expect(player != nullptr, "missing player entry should exist");
	if (player != nullptr) {
		Expect(player->meshFilename.empty(), "missing policy ref should have no filename");
		Expect(
			player->status == NativeStaticModelLoadStatus::MissingPolicyRef,
			"missing policy ref should report missing status");
		Expect(
			player->fallback == NativeStaticModelFallbackKind::ProceduralBean,
			"missing player should report procedural bean fallback");
	}
}

void TestBadFilenameReportsLoadFailedWithIssueCount()
{
	NativeStaticModelPolicy policy = DefaultNativeStaticModelPolicy();
	policy.models[0].meshFilename = "missing-floor.igmesh";

	const NativeStaticModelLoadReport report =
		BuildNativeStaticModelLoadReport(policy, AssetRoot());

	const NativeStaticModelLoadEntry *floor =
		FindEntry(report, NativeStaticModelSlot::Floor);
	Expect(report.loadedCount == 3, "bad filename report should load remaining assets");
	Expect(report.failedCount == 1, "bad filename report should count load failure");
	Expect(report.missingCount == 0, "bad filename report should not count missing refs");
	Expect(floor != nullptr, "failed floor entry should exist");
	if (floor != nullptr) {
		Expect(floor->meshFilename == "missing-floor.igmesh", "failed entry should preserve filename");
		Expect(floor->status == NativeStaticModelLoadStatus::LoadFailed, "bad filename should fail load");
		Expect(floor->issueCount > 0, "bad filename should report loader issues");
		Expect(floor->vertexCount == 0, "failed load should not report vertices");
		Expect(floor->indexCount == 0, "failed load should not report indices");
		Expect(
			floor->fallback == NativeStaticModelFallbackKind::Cube,
			"failed floor should report cube fallback");
	}
}

void TestReportDoesNotInferUnlistedExistingAssets()
{
	const NativeStaticModelPolicy policy {
		{
			{ NativeStaticModelSlot::Floor, "floor.igmesh" },
		},
	};

	const NativeStaticModelLoadReport report =
		BuildNativeStaticModelLoadReport(policy, AssetRoot());

	const NativeStaticModelLoadEntry *wall =
		FindEntry(report, NativeStaticModelSlot::Wall);
	const NativeStaticModelLoadEntry *npc =
		FindEntry(report, NativeStaticModelSlot::NpcActor);
	const NativeStaticModelLoadEntry *player =
		FindEntry(report, NativeStaticModelSlot::Player);
	Expect(report.entries.size() == 4, "report should always cover fixed slots");
	Expect(report.loadedCount == 1, "report should load only listed assets");
	Expect(report.failedCount == 0, "unlisted assets should not produce load failures");
	Expect(report.missingCount == 3, "unlisted fixed slots should be missing refs");
	Expect(wall != nullptr && wall->status == NativeStaticModelLoadStatus::MissingPolicyRef, "unlisted wall should stay missing");
	Expect(npc != nullptr && npc->status == NativeStaticModelLoadStatus::MissingPolicyRef, "unlisted NPC should stay missing");
	Expect(player != nullptr && player->status == NativeStaticModelLoadStatus::MissingPolicyRef, "unlisted player should stay missing");
}

} // namespace

int main()
{
	TestDefaultPolicyReportsAllCheckedInAssetsLoaded();
	TestMissingPolicyRefReportsFallbackKind();
	TestBadFilenameReportsLoadFailedWithIssueCount();
	TestReportDoesNotInferUnlistedExistingAssets();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
