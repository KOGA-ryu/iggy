#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"
#include "../apps/native_play/NativeStaticMeshExportPolicy.hpp"

#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuiltInNativeStaticMeshExportAsset;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::FindNativeStaticMeshExportAsset;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportAssetRef;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshAssetWriteResult;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;

void ExpectBasenameOnly(const std::string &filename, const char *label)
{
	Expect(!filename.empty(), std::string { label } + " filename should be non-empty");
	Expect(
		filename.find('/') == std::string::npos,
		std::string { label } + " filename should not contain slash separators");
	Expect(
		filename.find('\\') == std::string::npos,
		std::string { label } + " filename should not contain backslash separators");
}

void TestDefaultPolicyContainsStableRefs()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();

	Expect(policy.assets.size() == 3, "default export policy should contain exactly three refs");
	if (policy.assets.size() != 3)
		return;

	Expect(policy.assets[0].id == NativeStaticMeshBuiltInExportId::Cube, "cube should be first");
	Expect(policy.assets[0].name == "cube", "cube name should be exact");
	Expect(policy.assets[0].defaultFilename == "cube.igmesh", "cube filename should be exact");
	ExpectBasenameOnly(policy.assets[0].defaultFilename, "cube");

	Expect(policy.assets[1].id == NativeStaticMeshBuiltInExportId::Bean, "bean should be second");
	Expect(policy.assets[1].name == "bean", "bean name should be exact");
	Expect(policy.assets[1].defaultFilename == "bean.igmesh", "bean filename should be exact");
	ExpectBasenameOnly(policy.assets[1].defaultFilename, "bean");

	Expect(policy.assets[2].id == NativeStaticMeshBuiltInExportId::NpcMarker, "NPC marker should be third");
	Expect(policy.assets[2].name == "npc-marker", "NPC marker name should be exact");
	Expect(policy.assets[2].defaultFilename == "npc-marker.igmesh", "NPC marker filename should be exact");
	ExpectBasenameOnly(policy.assets[2].defaultFilename, "npc marker");
}

void TestLookupReturnsMatchingRefs()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();

	const NativeStaticMeshExportAssetRef *cube =
		FindNativeStaticMeshExportAsset(policy, "cube");
	const NativeStaticMeshExportAssetRef *bean =
		FindNativeStaticMeshExportAsset(policy, "bean");
	const NativeStaticMeshExportAssetRef *npc =
		FindNativeStaticMeshExportAsset(policy, "npc-marker");

	Expect(cube != nullptr && cube->id == NativeStaticMeshBuiltInExportId::Cube, "cube lookup should return cube ref");
	Expect(bean != nullptr && bean->id == NativeStaticMeshBuiltInExportId::Bean, "bean lookup should return bean ref");
	Expect(npc != nullptr && npc->id == NativeStaticMeshBuiltInExportId::NpcMarker, "NPC marker lookup should return NPC ref");
}

void TestMissingNameReturnsNull()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();

	Expect(
		FindNativeStaticMeshExportAsset(policy, "floor") == nullptr,
		"missing export name should return null");
}

void TestDuplicateNameReturnsFirstMatch()
{
	const NativeStaticMeshExportPolicy policy {
		{
			{ NativeStaticMeshBuiltInExportId::Bean, "duplicate", "first.igmesh" },
			{ NativeStaticMeshBuiltInExportId::NpcMarker, "duplicate", "second.igmesh" },
		},
	};

	const NativeStaticMeshExportAssetRef *asset =
		FindNativeStaticMeshExportAsset(policy, "duplicate");

	Expect(asset != nullptr, "duplicate lookup should still return a ref");
	if (asset != nullptr) {
		Expect(
			asset->id == NativeStaticMeshBuiltInExportId::Bean,
			"duplicate lookup should return first matching id");
		Expect(
			asset->defaultFilename == "first.igmesh",
			"duplicate lookup should return first matching filename");
	}
}

void TestBuiltInIdHelperProducesWriterValidMeshes()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();

	for (const NativeStaticMeshExportAssetRef &asset : policy.assets) {
		const NativeStaticMeshAssetWriteResult write =
			WriteNativeStaticMeshAssetText(BuiltInNativeStaticMeshExportAsset(asset.id));
		Expect(write.written(), asset.name + " built-in mesh should write");
	}
}

} // namespace

int main()
{
	TestDefaultPolicyContainsStableRefs();
	TestLookupReturnsMatchingRefs();
	TestMissingNameReturnsNull();
	TestDuplicateNameReturnsFirstMatch();
	TestBuiltInIdHelperProducesWriterValidMeshes();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
