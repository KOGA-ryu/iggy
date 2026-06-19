#include "../apps/native_play/NativeStaticModelPolicy.hpp"

#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::DefaultNativeStaticModelPolicy;
using iggy::native_play::FindNativeStaticModelAsset;
using iggy::native_play::NativeStaticModelAssetRef;
using iggy::native_play::NativeStaticModelPolicy;
using iggy::native_play::NativeStaticModelSlot;
using iggy::test::Expect;
using iggy::test::Failures;

void TestDefaultPolicyContainsStableSlotsAndFilenames()
{
	const NativeStaticModelPolicy policy = DefaultNativeStaticModelPolicy();

	Expect(policy.models.size() == 4, "default policy should contain exactly four model refs");
	if (policy.models.size() != 4)
		return;

	Expect(policy.models[0].slot == NativeStaticModelSlot::Floor, "floor should be first");
	Expect(policy.models[0].meshFilename == "floor.igmesh", "floor filename should be exact");
	Expect(policy.models[1].slot == NativeStaticModelSlot::Wall, "wall should be second");
	Expect(policy.models[1].meshFilename == "wall.igmesh", "wall filename should be exact");
	Expect(policy.models[2].slot == NativeStaticModelSlot::NpcActor, "NPC actor should be third");
	Expect(policy.models[2].meshFilename == "npc.igmesh", "NPC filename should be exact");
	Expect(policy.models[3].slot == NativeStaticModelSlot::Player, "player should be fourth");
	Expect(policy.models[3].meshFilename == "player.igmesh", "player filename should be exact");
}

void TestLookupReturnsMatchingDefaultEntries()
{
	const NativeStaticModelPolicy policy = DefaultNativeStaticModelPolicy();

	const NativeStaticModelAssetRef *floor =
		FindNativeStaticModelAsset(policy, NativeStaticModelSlot::Floor);
	const NativeStaticModelAssetRef *wall =
		FindNativeStaticModelAsset(policy, NativeStaticModelSlot::Wall);
	const NativeStaticModelAssetRef *npc =
		FindNativeStaticModelAsset(policy, NativeStaticModelSlot::NpcActor);
	const NativeStaticModelAssetRef *player =
		FindNativeStaticModelAsset(policy, NativeStaticModelSlot::Player);

	Expect(floor != nullptr && floor->meshFilename == "floor.igmesh", "floor lookup should return floor ref");
	Expect(wall != nullptr && wall->meshFilename == "wall.igmesh", "wall lookup should return wall ref");
	Expect(npc != nullptr && npc->meshFilename == "npc.igmesh", "NPC lookup should return NPC ref");
	Expect(player != nullptr && player->meshFilename == "player.igmesh", "player lookup should return player ref");
}

void TestMissingSlotReturnsNull()
{
	const NativeStaticModelPolicy policy {
		{
			{ NativeStaticModelSlot::Floor, "floor.igmesh" },
		},
	};

	Expect(
		FindNativeStaticModelAsset(policy, NativeStaticModelSlot::Player) == nullptr,
		"missing slot should return null");
}

void TestDuplicateSlotReturnsFirstMatch()
{
	const NativeStaticModelPolicy policy {
		{
			{ NativeStaticModelSlot::Wall, "first-wall.igmesh" },
			{ NativeStaticModelSlot::Wall, "second-wall.igmesh" },
		},
	};

	const NativeStaticModelAssetRef *wall =
		FindNativeStaticModelAsset(policy, NativeStaticModelSlot::Wall);

	Expect(wall != nullptr, "duplicate lookup should still return a result");
	if (wall != nullptr)
		Expect(wall->meshFilename == "first-wall.igmesh", "duplicate lookup should return first match");
}

void TestPolicyHelpersAreValueOnly()
{
	const NativeStaticModelPolicy policy = DefaultNativeStaticModelPolicy();

	for (const NativeStaticModelAssetRef &model : policy.models) {
		Expect(!model.meshFilename.empty(), "default filenames should be value strings");
		Expect(
			model.meshFilename.find('/') == std::string::npos,
			"default policy should not contain filesystem paths");
		Expect(
			model.meshFilename.find('\\') == std::string::npos,
			"default policy should not contain platform path separators");
	}
}

} // namespace

int main()
{
	TestDefaultPolicyContainsStableSlotsAndFilenames();
	TestLookupReturnsMatchingDefaultEntries();
	TestMissingSlotReturnsNull();
	TestDuplicateSlotReturnsFirstMatch();
	TestPolicyHelpersAreValueOnly();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
