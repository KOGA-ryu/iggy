#include <cstdlib>
#include <vector>

#include "servers/render/ShaderResource.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::ResourceId CanvasShaderAsset { "asset:canvas_shader" };
const iggy::ResourceId DebugShaderAsset { "asset:debug_shader" };

iggy::render::ShaderResource Shader(const char *id, const iggy::ResourceId &assetId, iggy::render::ShaderResourceKind kind)
{
	return { iggy::ResourceId { id }, assetId, kind };
}

void ExpectShader(const iggy::render::ShaderResource &shader, const iggy::ResourceId &id, const iggy::ResourceId &assetId, iggy::render::ShaderResourceKind kind, const char *message)
{
	Expect(shader.id == id && shader.assetId == assetId && shader.kind == kind, message);
}

void TestEmptyInputBuildsValidEmptyCatalog()
{
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({});

	Expect(result.built, "empty input should build a valid shader catalog");
	Expect(result.issues.empty(), "empty input should produce no issues");
	Expect(result.catalog.resources().empty(), "empty input should produce empty shader resources");
}

void TestCanvas2DShaderBuildsAndFindsExactResource()
{
	const iggy::ResourceId id { "shader:canvas" };
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		{ id, CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D },
	});

	Expect(result.built, "single Canvas2D shader should build");
	Expect(result.issues.empty(), "single Canvas2D shader should produce no issues");
	Expect(result.catalog.contains(id), "shader catalog should contain valid id");
	const iggy::render::ShaderResource *found = result.catalog.find(id);
	Expect(found != nullptr, "find should return valid shader resource");
	if (found != nullptr)
		ExpectShader(*found, id, CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D, "found shader should preserve id, asset id, and kind");
}

void TestMissingLookupReturnsFalseAndNull()
{
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		Shader("shader:canvas", CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D),
	});

	Expect(result.built, "lookup test setup should build");
	Expect(!result.catalog.contains(iggy::ResourceId { "shader:missing" }), "missing shader id should not be contained");
	Expect(result.catalog.find(iggy::ResourceId { "shader:missing" }) == nullptr, "missing shader id should return null from find");
}

void TestMultipleValidShadersPreserveInputOrder()
{
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		Shader("shader:canvas", CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D),
		Shader("shader:debug", DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown),
		Shader("shader:ui", iggy::ResourceId { "asset:ui_shader" }, iggy::render::ShaderResourceKind::Canvas2D),
	});

	Expect(result.built, "multiple valid shaders should build");
	Expect(result.catalog.resources().size() == 3, "multiple valid shaders should be preserved");
	if (result.catalog.resources().size() == 3) {
		Expect(result.catalog.resources()[0].id == iggy::ResourceId { "shader:canvas" }, "first shader should preserve input order");
		Expect(result.catalog.resources()[1].id == iggy::ResourceId { "shader:debug" }, "second shader should preserve input order");
		Expect(result.catalog.resources()[2].id == iggy::ResourceId { "shader:ui" }, "third shader should preserve input order");
	}
}

void TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog()
{
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		{ {}, CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D },
	});

	Expect(!result.built, "empty shader id should fail catalog build");
	Expect(result.catalog.resources().empty(), "empty shader id should return an empty catalog");
	Expect(result.issues.size() == 1, "empty shader id should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::render::ShaderResourceIssueCode::EmptyId, "empty shader id issue should use EmptyId code");
		Expect(result.issues[0].id.empty(), "empty shader id issue should preserve empty id");
		Expect(result.issues[0].kind == iggy::render::ShaderResourceKind::Canvas2D, "empty shader id issue should preserve kind");
	}
}

void TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog()
{
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		Shader("shader:canvas", CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D),
		Shader("shader:canvas", DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown),
	});

	Expect(!result.built, "duplicate shader ids should fail catalog build");
	Expect(result.catalog.resources().empty(), "duplicate shader ids should return an empty catalog");
	Expect(result.issues.size() == 1, "duplicate shader ids should produce one duplicate issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::render::ShaderResourceIssueCode::DuplicateId, "duplicate shader issue should use DuplicateId code");
		Expect(result.issues[0].id == iggy::ResourceId { "shader:canvas" }, "duplicate shader issue should preserve duplicate id");
		Expect(result.issues[0].kind == iggy::render::ShaderResourceKind::Unknown, "duplicate shader issue should preserve duplicate kind");
	}
}

void TestMultipleIssuesReportedInDeterministicOrder()
{
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		Shader("shader:canvas", CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D),
		{ {}, DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown },
		Shader("shader:canvas", DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown),
	});

	Expect(!result.built, "multiple shader issues should fail catalog build");
	Expect(result.catalog.resources().empty(), "multiple shader issues should return an empty catalog");
	Expect(result.issues.size() == 2, "multiple shader issues should all be reported");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::render::ShaderResourceIssueCode::EmptyId && result.issues[0].kind == iggy::render::ShaderResourceKind::Unknown, "empty id issue should be reported at its input position");
		Expect(result.issues[1].code == iggy::render::ShaderResourceIssueCode::DuplicateId && result.issues[1].id == iggy::ResourceId { "shader:canvas" }, "duplicate issue should appear when duplicate shader is encountered");
	}
}

void TestUnknownKindIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "shader:unknown" };
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		{ id, DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown },
	});

	Expect(result.built, "Unknown shader kind should be allowed");
	const iggy::render::ShaderResource *found = result.catalog.find(id);
	Expect(found != nullptr, "Unknown shader kind resource should be findable");
	if (found != nullptr)
		Expect(found->kind == iggy::render::ShaderResourceKind::Unknown, "Unknown shader kind should be preserved");
}

void TestEmptyAssetIdIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "shader:generated" };
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		{ id, {}, iggy::render::ShaderResourceKind::Canvas2D },
	});

	Expect(result.built, "empty shader asset id should be allowed");
	const iggy::render::ShaderResource *found = result.catalog.find(id);
	Expect(found != nullptr, "empty asset id shader should be findable");
	if (found != nullptr)
		Expect(found->assetId.empty(), "empty shader asset id should be preserved");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::ResourceId namespaced { "shader:canvas" };
	const iggy::ResourceId unqualified { "canvas" };
	const iggy::render::ShaderResourceBuildResult result = iggy::render::ShaderResourceCatalogBuilder {}.build({
		{ namespaced, CanvasShaderAsset, iggy::render::ShaderResourceKind::Canvas2D },
		{ unqualified, DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown },
	});

	Expect(result.built, "namespaced and unqualified shader ids should build as distinct resources");
	Expect(result.catalog.contains(namespaced), "shader catalog should contain namespaced id");
	Expect(result.catalog.contains(unqualified), "shader catalog should contain unqualified id");
	if (const iggy::render::ShaderResource *found = result.catalog.find(unqualified))
		ExpectShader(*found, unqualified, DebugShaderAsset, iggy::render::ShaderResourceKind::Unknown, "unqualified lookup should find exact unqualified shader resource");
	else
		Expect(false, "unqualified lookup should return a shader resource");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyCatalog();
	TestCanvas2DShaderBuildsAndFindsExactResource();
	TestMissingLookupReturnsFalseAndNull();
	TestMultipleValidShadersPreserveInputOrder();
	TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog();
	TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog();
	TestMultipleIssuesReportedInDeterministicOrder();
	TestUnknownKindIsAllowedAndPreserved();
	TestEmptyAssetIdIsAllowedAndPreserved();
	TestNamespacedAndUnqualifiedIdsAreDistinct();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
