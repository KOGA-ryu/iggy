#include <cstdlib>
#include <vector>

#include "servers/render/MaterialResource.hpp"
#include "servers/render/RenderCommand2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::ResourceId CanvasShader { "shader:canvas" };
const iggy::ResourceId DebugShader { "shader:debug" };
const iggy::ResourceId WallTexture { "texture:wall" };
const iggy::ResourceId FloorTexture { "texture:floor" };

iggy::render::MaterialResource Material(const char *id, const iggy::ResourceId &shaderId, const iggy::ResourceId &textureId)
{
	return { iggy::ResourceId { id }, shaderId, textureId };
}

void ExpectMaterial(const iggy::render::MaterialResource &material, const iggy::ResourceId &id, const iggy::ResourceId &shaderId, const iggy::ResourceId &textureId, const char *message)
{
	Expect(material.id == id && material.shaderId == shaderId && material.textureId == textureId, message);
}

void TestEmptyInputBuildsValidEmptyCatalog()
{
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({});

	Expect(result.built, "empty input should build a valid material catalog");
	Expect(result.issues.empty(), "empty input should produce no issues");
	Expect(result.catalog.resources().empty(), "empty input should produce empty material resources");
}

void TestSingleValidMaterialBuildsAndFindsExactResource()
{
	const iggy::ResourceId id { "material:wall" };
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		{ id, CanvasShader, WallTexture },
	});

	Expect(result.built, "single valid material should build");
	Expect(result.issues.empty(), "single valid material should produce no issues");
	Expect(result.catalog.contains(id), "material catalog should contain valid id");
	const iggy::render::MaterialResource *found = result.catalog.find(id);
	Expect(found != nullptr, "find should return valid material resource");
	if (found != nullptr)
		ExpectMaterial(*found, id, CanvasShader, WallTexture, "found material should preserve id, shader id, and texture id");
}

void TestMissingLookupReturnsFalseAndNull()
{
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		Material("material:wall", CanvasShader, WallTexture),
	});

	Expect(result.built, "lookup test setup should build");
	Expect(!result.catalog.contains(iggy::ResourceId { "material:floor" }), "missing material id should not be contained");
	Expect(result.catalog.find(iggy::ResourceId { "material:floor" }) == nullptr, "missing material id should return null from find");
}

void TestMultipleValidMaterialsPreserveInputOrder()
{
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		Material("material:wall", CanvasShader, WallTexture),
		Material("material:floor", CanvasShader, FloorTexture),
		Material("material:debug", DebugShader, WallTexture),
	});

	Expect(result.built, "multiple valid materials should build");
	Expect(result.catalog.resources().size() == 3, "multiple valid materials should be preserved");
	if (result.catalog.resources().size() == 3) {
		Expect(result.catalog.resources()[0].id == iggy::ResourceId { "material:wall" }, "first material should preserve input order");
		Expect(result.catalog.resources()[1].id == iggy::ResourceId { "material:floor" }, "second material should preserve input order");
		Expect(result.catalog.resources()[2].id == iggy::ResourceId { "material:debug" }, "third material should preserve input order");
	}
}

void TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog()
{
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		{ {}, CanvasShader, WallTexture },
	});

	Expect(!result.built, "empty material id should fail catalog build");
	Expect(result.catalog.resources().empty(), "empty material id should return an empty catalog");
	Expect(result.issues.size() == 1, "empty material id should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::render::MaterialResourceIssueCode::EmptyId, "empty material id issue should use EmptyId code");
		Expect(result.issues[0].id.empty(), "empty material id issue should preserve empty id");
		Expect(result.issues[0].shaderId == CanvasShader && result.issues[0].textureId == WallTexture, "empty material id issue should preserve shader and texture ids");
	}
}

void TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog()
{
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		Material("material:wall", CanvasShader, WallTexture),
		Material("material:wall", DebugShader, FloorTexture),
	});

	Expect(!result.built, "duplicate material ids should fail catalog build");
	Expect(result.catalog.resources().empty(), "duplicate material ids should return an empty catalog");
	Expect(result.issues.size() == 1, "duplicate material ids should produce one duplicate issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::render::MaterialResourceIssueCode::DuplicateId, "duplicate material issue should use DuplicateId code");
		Expect(result.issues[0].id == iggy::ResourceId { "material:wall" }, "duplicate material issue should preserve duplicate id");
		Expect(result.issues[0].shaderId == DebugShader && result.issues[0].textureId == FloorTexture, "duplicate material issue should preserve duplicate shader and texture ids");
	}
}

void TestMultipleIssuesReportedInDeterministicOrder()
{
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		Material("material:wall", CanvasShader, WallTexture),
		{ {}, DebugShader, FloorTexture },
		Material("material:wall", DebugShader, FloorTexture),
	});

	Expect(!result.built, "multiple material issues should fail catalog build");
	Expect(result.catalog.resources().empty(), "multiple material issues should return an empty catalog");
	Expect(result.issues.size() == 2, "multiple material issues should all be reported");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::render::MaterialResourceIssueCode::EmptyId && result.issues[0].shaderId == DebugShader && result.issues[0].textureId == FloorTexture, "empty id issue should preserve references at its input position");
		Expect(result.issues[1].code == iggy::render::MaterialResourceIssueCode::DuplicateId && result.issues[1].id == iggy::ResourceId { "material:wall" }, "duplicate issue should appear when duplicate material is encountered");
	}
}

void TestEmptyShaderIdIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "material:no_shader" };
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		{ id, {}, WallTexture },
	});

	Expect(result.built, "empty shader id should be allowed");
	const iggy::render::MaterialResource *found = result.catalog.find(id);
	Expect(found != nullptr, "empty shader id material should be findable");
	if (found != nullptr)
		Expect(found->shaderId.empty(), "empty shader id should be preserved");
}

void TestEmptyTextureIdIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "material:no_texture" };
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		{ id, CanvasShader, {} },
	});

	Expect(result.built, "empty texture id should be allowed");
	const iggy::render::MaterialResource *found = result.catalog.find(id);
	Expect(found != nullptr, "empty texture id material should be findable");
	if (found != nullptr)
		Expect(found->textureId.empty(), "empty texture id should be preserved");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::ResourceId namespaced { "material:wall" };
	const iggy::ResourceId unqualified { "wall" };
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		{ namespaced, CanvasShader, WallTexture },
		{ unqualified, DebugShader, FloorTexture },
	});

	Expect(result.built, "namespaced and unqualified material ids should build as distinct resources");
	Expect(result.catalog.contains(namespaced), "material catalog should contain namespaced id");
	Expect(result.catalog.contains(unqualified), "material catalog should contain unqualified id");
	if (const iggy::render::MaterialResource *found = result.catalog.find(unqualified))
		ExpectMaterial(*found, unqualified, DebugShader, FloorTexture, "unqualified lookup should find exact unqualified material resource");
	else
		Expect(false, "unqualified lookup should return a material resource");
}

void TestRenderCommandMaterialIdCanResolveAgainstCatalog()
{
	const iggy::ResourceId materialId { "material:wall" };
	const iggy::render::MaterialResourceBuildResult result = iggy::render::MaterialResourceCatalogBuilder {}.build({
		{ materialId, CanvasShader, WallTexture },
	});
	const iggy::render::RenderCommand2D command {
		iggy::render::RenderCommand2DType::Quad,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		materialId,
		0,
		0,
	};

	Expect(result.built, "render command material lookup setup should build");
	Expect(result.catalog.find(command.materialId) != nullptr, "render command material id should be findable in material catalog");
	if (const iggy::render::MaterialResource *found = result.catalog.find(command.materialId))
		ExpectMaterial(*found, materialId, CanvasShader, WallTexture, "render command material id should resolve to matching material descriptor in test-only composition");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyCatalog();
	TestSingleValidMaterialBuildsAndFindsExactResource();
	TestMissingLookupReturnsFalseAndNull();
	TestMultipleValidMaterialsPreserveInputOrder();
	TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog();
	TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog();
	TestMultipleIssuesReportedInDeterministicOrder();
	TestEmptyShaderIdIsAllowedAndPreserved();
	TestEmptyTextureIdIsAllowedAndPreserved();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestRenderCommandMaterialIdCanResolveAgainstCatalog();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
