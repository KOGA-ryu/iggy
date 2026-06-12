#include <cstdlib>
#include <vector>

#include "servers/render/TextureResource.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::ResourceId WallAsset { "asset:texture_wall" };
const iggy::ResourceId FloorAsset { "asset:texture_floor" };

iggy::render::TextureResource Texture(const char *id, const iggy::ResourceId &assetId, int width, int height)
{
	return { iggy::ResourceId { id }, assetId, width, height };
}

void ExpectTexture(const iggy::render::TextureResource &texture, const iggy::ResourceId &id, const iggy::ResourceId &assetId, int width, int height, const char *message)
{
	Expect(texture.id == id && texture.assetId == assetId && texture.width == width && texture.height == height, message);
}

void TestEmptyInputBuildsValidEmptyCatalog()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({});

	Expect(result.built, "empty input should build a valid texture catalog");
	Expect(result.issues.empty(), "empty input should produce no issues");
	Expect(result.catalog.resources().empty(), "empty input should produce empty texture resources");
}

void TestSingleValidTextureBuildsAndFindsExactResource()
{
	const iggy::ResourceId id { "texture:wall" };
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		{ id, WallAsset, 64, 32 },
	});

	Expect(result.built, "single valid texture should build");
	Expect(result.issues.empty(), "single valid texture should produce no issues");
	Expect(result.catalog.contains(id), "texture catalog should contain valid id");
	const iggy::render::TextureResource *found = result.catalog.find(id);
	Expect(found != nullptr, "find should return valid texture resource");
	if (found != nullptr)
		ExpectTexture(*found, id, WallAsset, 64, 32, "found texture should preserve id, asset id, width, and height");
}

void TestMissingLookupReturnsFalseAndNull()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		Texture("texture:wall", WallAsset, 64, 64),
	});

	Expect(result.built, "lookup test setup should build");
	Expect(!result.catalog.contains(iggy::ResourceId { "texture:floor" }), "missing texture id should not be contained");
	Expect(result.catalog.find(iggy::ResourceId { "texture:floor" }) == nullptr, "missing texture id should return null from find");
}

void TestMultipleValidTexturesPreserveInputOrder()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		Texture("texture:wall", WallAsset, 64, 64),
		Texture("texture:floor", FloorAsset, 32, 32),
		Texture("texture:ui", iggy::ResourceId { "asset:texture_ui" }, 128, 16),
	});

	Expect(result.built, "multiple valid textures should build");
	Expect(result.catalog.resources().size() == 3, "multiple valid textures should be preserved");
	if (result.catalog.resources().size() == 3) {
		Expect(result.catalog.resources()[0].id == iggy::ResourceId { "texture:wall" }, "first texture should preserve input order");
		Expect(result.catalog.resources()[1].id == iggy::ResourceId { "texture:floor" }, "second texture should preserve input order");
		Expect(result.catalog.resources()[2].id == iggy::ResourceId { "texture:ui" }, "third texture should preserve input order");
	}
}

void TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		{ {}, WallAsset, 64, 64 },
	});

	Expect(!result.built, "empty texture id should fail catalog build");
	Expect(result.catalog.resources().empty(), "empty texture id should return an empty catalog");
	Expect(result.issues.size() == 1, "empty texture id should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::render::TextureResourceIssueCode::EmptyId, "empty texture id issue should use EmptyId code");
		Expect(result.issues[0].id.empty(), "empty texture id issue should preserve empty id");
		Expect(result.issues[0].width == 64 && result.issues[0].height == 64, "empty texture id issue should preserve dimensions");
	}
}

void TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		Texture("texture:wall", WallAsset, 64, 64),
		Texture("texture:wall", WallAsset, 128, 32),
	});

	Expect(!result.built, "duplicate texture ids should fail catalog build");
	Expect(result.catalog.resources().empty(), "duplicate texture ids should return an empty catalog");
	Expect(result.issues.size() == 1, "duplicate texture ids should produce one duplicate issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::render::TextureResourceIssueCode::DuplicateId, "duplicate texture issue should use DuplicateId code");
		Expect(result.issues[0].id == iggy::ResourceId { "texture:wall" }, "duplicate texture issue should preserve duplicate id");
		Expect(result.issues[0].width == 128 && result.issues[0].height == 32, "duplicate texture issue should preserve duplicate dimensions");
	}
}

void TestInvalidDimensionsFailWithInvalidSizeIssue()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		Texture("texture:zero_width", WallAsset, 0, 32),
		Texture("texture:negative_height", FloorAsset, 32, -1),
	});

	Expect(!result.built, "invalid texture dimensions should fail catalog build");
	Expect(result.catalog.resources().empty(), "invalid texture dimensions should return an empty catalog");
	Expect(result.issues.size() == 2, "invalid texture dimensions should produce one issue per invalid resource");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::render::TextureResourceIssueCode::InvalidSize && result.issues[0].width == 0 && result.issues[0].height == 32, "zero width should produce InvalidSize with dimensions preserved");
		Expect(result.issues[1].code == iggy::render::TextureResourceIssueCode::InvalidSize && result.issues[1].width == 32 && result.issues[1].height == -1, "negative height should produce InvalidSize with dimensions preserved");
	}
}

void TestMultipleIssuesReportedInDeterministicOrder()
{
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		Texture("texture:wall", WallAsset, 64, 64),
		{ {}, WallAsset, 0, 64 },
		Texture("texture:wall", FloorAsset, -1, -2),
	});

	Expect(!result.built, "multiple texture issues should fail catalog build");
	Expect(result.catalog.resources().empty(), "multiple texture issues should return an empty catalog");
	Expect(result.issues.size() == 4, "multiple texture issues should all be reported");
	if (result.issues.size() == 4) {
		Expect(result.issues[0].code == iggy::render::TextureResourceIssueCode::EmptyId, "empty id issue should be reported first for empty-id resource");
		Expect(result.issues[1].code == iggy::render::TextureResourceIssueCode::InvalidSize && result.issues[1].id.empty(), "invalid size should follow empty id for the same resource");
		Expect(result.issues[2].code == iggy::render::TextureResourceIssueCode::DuplicateId && result.issues[2].id == iggy::ResourceId { "texture:wall" }, "duplicate issue should appear when duplicate resource is encountered");
		Expect(result.issues[3].code == iggy::render::TextureResourceIssueCode::InvalidSize && result.issues[3].id == iggy::ResourceId { "texture:wall" }, "invalid size should follow duplicate issue for the same resource");
	}
}

void TestEmptyAssetIdIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "texture:generated" };
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		{ id, {}, 16, 16 },
	});

	Expect(result.built, "empty asset id should be allowed");
	const iggy::render::TextureResource *found = result.catalog.find(id);
	Expect(found != nullptr, "empty asset id texture should be findable");
	if (found != nullptr)
		Expect(found->assetId.empty(), "empty asset id should be preserved");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::ResourceId namespaced { "texture:wall" };
	const iggy::ResourceId unqualified { "wall" };
	const iggy::render::TextureResourceBuildResult result = iggy::render::TextureResourceCatalogBuilder {}.build({
		{ namespaced, WallAsset, 64, 64 },
		{ unqualified, FloorAsset, 32, 32 },
	});

	Expect(result.built, "namespaced and unqualified texture ids should build as distinct resources");
	Expect(result.catalog.contains(namespaced), "texture catalog should contain namespaced id");
	Expect(result.catalog.contains(unqualified), "texture catalog should contain unqualified id");
	if (const iggy::render::TextureResource *found = result.catalog.find(unqualified))
		ExpectTexture(*found, unqualified, FloorAsset, 32, 32, "unqualified lookup should find exact unqualified resource");
	else
		Expect(false, "unqualified lookup should return a texture resource");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyCatalog();
	TestSingleValidTextureBuildsAndFindsExactResource();
	TestMissingLookupReturnsFalseAndNull();
	TestMultipleValidTexturesPreserveInputOrder();
	TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog();
	TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog();
	TestInvalidDimensionsFailWithInvalidSizeIssue();
	TestMultipleIssuesReportedInDeterministicOrder();
	TestEmptyAssetIdIsAllowedAndPreserved();
	TestNamespacedAndUnqualifiedIdsAreDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
