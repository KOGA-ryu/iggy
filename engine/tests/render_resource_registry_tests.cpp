#include <cstdlib>

#include "servers/render/RenderCommand2D.hpp"
#include "servers/render/RenderResourceRegistry.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::ResourceId TextureId { "texture:wall" };
const iggy::ResourceId ShaderId { "shader:canvas" };
const iggy::ResourceId MaterialId { "material:wall" };

iggy::render::TextureResourceCatalog Textures(std::vector<iggy::render::TextureResource> resources)
{
	return iggy::render::TextureResourceCatalogBuilder {}.build(resources).catalog;
}

iggy::render::ShaderResourceCatalog Shaders(std::vector<iggy::render::ShaderResource> resources)
{
	return iggy::render::ShaderResourceCatalogBuilder {}.build(resources).catalog;
}

iggy::render::MaterialResourceCatalog Materials(std::vector<iggy::render::MaterialResource> resources)
{
	return iggy::render::MaterialResourceCatalogBuilder {}.build(resources).catalog;
}

iggy::render::RenderResourceRegistry Registry(
	std::vector<iggy::render::TextureResource> textures,
	std::vector<iggy::render::ShaderResource> shaders,
	std::vector<iggy::render::MaterialResource> materials)
{
	return iggy::render::RenderResourceRegistry {
		{
			Textures(textures),
			Shaders(shaders),
			Materials(materials),
		},
	};
}

iggy::render::TextureResource Texture(const iggy::ResourceId &id = TextureId)
{
	return { id, iggy::ResourceId { "asset:texture_wall" }, 64, 64 };
}

iggy::render::ShaderResource Shader(const iggy::ResourceId &id = ShaderId)
{
	return { id, iggy::ResourceId { "asset:canvas_shader" }, iggy::render::ShaderResourceKind::Canvas2D };
}

iggy::render::MaterialResource Material(const iggy::ResourceId &id = MaterialId, const iggy::ResourceId &shaderId = ShaderId, const iggy::ResourceId &textureId = TextureId)
{
	return { id, shaderId, textureId };
}

void TestDefaultRegistryReturnsMissing()
{
	const iggy::render::RenderResourceRegistry registry;

	Expect(registry.findTexture(TextureId) == nullptr, "default registry should not find texture");
	Expect(registry.findShader(ShaderId) == nullptr, "default registry should not find shader");
	Expect(registry.findMaterial(MaterialId) == nullptr, "default registry should not find material");
	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);
	Expect(result.status == iggy::render::RenderMaterialResolveStatus::MissingMaterial, "default registry should resolve material as missing");
	Expect(result.material == nullptr && result.shader == nullptr && result.texture == nullptr, "missing material result should have no pointers");
}

void TestRegistryPreservesDirectLookups()
{
	const iggy::render::RenderResourceRegistry registry = Registry({ Texture() }, { Shader() }, { Material() });

	Expect(registry.findTexture(TextureId) != nullptr, "registry should find texture by exact id");
	Expect(registry.findShader(ShaderId) != nullptr, "registry should find shader by exact id");
	Expect(registry.findMaterial(MaterialId) != nullptr, "registry should find material by exact id");
	if (const iggy::render::TextureResource *texture = registry.findTexture(TextureId))
		Expect(texture->id == TextureId, "found texture should preserve id");
	if (const iggy::render::ShaderResource *shader = registry.findShader(ShaderId))
		Expect(shader->id == ShaderId, "found shader should preserve id");
	if (const iggy::render::MaterialResource *material = registry.findMaterial(MaterialId))
		Expect(material->id == MaterialId, "found material should preserve id");
}

void TestResolveMaterialWithExistingShaderAndTexture()
{
	const iggy::render::RenderResourceRegistry registry = Registry({ Texture() }, { Shader() }, { Material() });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::Resolved, "material with existing shader and texture should resolve");
	Expect(result.material != nullptr && result.shader != nullptr && result.texture != nullptr, "resolved material should include material, shader, and texture pointers");
	if (result.material != nullptr && result.shader != nullptr && result.texture != nullptr) {
		Expect(result.material->id == MaterialId, "resolved material pointer should match material id");
		Expect(result.shader->id == ShaderId, "resolved shader pointer should match shader id");
		Expect(result.texture->id == TextureId, "resolved texture pointer should match texture id");
	}
}

void TestEmptyShaderIdResolvesWithTextureOnly()
{
	const iggy::render::RenderResourceRegistry registry = Registry({ Texture() }, {}, { Material(MaterialId, {}, TextureId) });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::Resolved, "empty shader id should not be treated as missing");
	Expect(result.material != nullptr && result.shader == nullptr && result.texture != nullptr, "empty shader id should resolve with material and texture only");
}

void TestEmptyTextureIdResolvesWithShaderOnly()
{
	const iggy::render::RenderResourceRegistry registry = Registry({}, { Shader() }, { Material(MaterialId, ShaderId, {}) });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::Resolved, "empty texture id should not be treated as missing");
	Expect(result.material != nullptr && result.shader != nullptr && result.texture == nullptr, "empty texture id should resolve with material and shader only");
}

void TestBothEmptyRefsResolveWithMaterialOnly()
{
	const iggy::render::RenderResourceRegistry registry = Registry({}, {}, { Material(MaterialId, {}, {}) });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::Resolved, "both empty refs should resolve");
	Expect(result.material != nullptr && result.shader == nullptr && result.texture == nullptr, "both empty refs should resolve with only material pointer");
}

void TestMissingShaderPreservesResolvedTexture()
{
	const iggy::render::RenderResourceRegistry registry = Registry({ Texture() }, {}, { Material() });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::MissingShader, "missing non-empty shader id should report MissingShader");
	Expect(result.material != nullptr && result.shader == nullptr && result.texture != nullptr, "missing shader result should preserve material and resolved texture");
}

void TestMissingTexturePreservesResolvedShader()
{
	const iggy::render::RenderResourceRegistry registry = Registry({}, { Shader() }, { Material() });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::MissingTexture, "missing non-empty texture id should report MissingTexture");
	Expect(result.material != nullptr && result.shader != nullptr && result.texture == nullptr, "missing texture result should preserve material and resolved shader");
}

void TestMissingShaderAndTexture()
{
	const iggy::render::RenderResourceRegistry registry = Registry({}, {}, { Material() });

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(MaterialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::MissingShaderAndTexture, "missing shader and texture should report combined status");
	Expect(result.material != nullptr && result.shader == nullptr && result.texture == nullptr, "combined missing result should preserve material only");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::ResourceId namespacedMaterial { "material:wall" };
	const iggy::ResourceId unqualifiedMaterial { "wall" };
	const iggy::render::RenderResourceRegistry registry = Registry(
		{ Texture() },
		{ Shader() },
		{
			Material(namespacedMaterial, ShaderId, TextureId),
			Material(unqualifiedMaterial, {}, {}),
		});

	Expect(registry.findMaterial(namespacedMaterial) != nullptr, "registry should find namespaced material id");
	Expect(registry.findMaterial(unqualifiedMaterial) != nullptr, "registry should find unqualified material id");
	const iggy::render::RenderMaterialResolveResult namespaced = registry.resolveMaterial(namespacedMaterial);
	const iggy::render::RenderMaterialResolveResult unqualified = registry.resolveMaterial(unqualifiedMaterial);
	Expect(namespaced.status == iggy::render::RenderMaterialResolveStatus::Resolved && namespaced.shader != nullptr && namespaced.texture != nullptr, "namespaced material should resolve its refs");
	Expect(unqualified.status == iggy::render::RenderMaterialResolveStatus::Resolved && unqualified.shader == nullptr && unqualified.texture == nullptr, "unqualified material should resolve independently");
}

void TestRenderCommandMaterialIdResolvesThroughRegistry()
{
	const iggy::render::RenderResourceRegistry registry = Registry({ Texture() }, { Shader() }, { Material() });
	const iggy::render::RenderCommand2D command {
		iggy::render::RenderCommand2DType::Quad,
		{ { 0.0F, 0.0F }, { 1.0F, 1.0F } },
		MaterialId,
		0,
		0,
	};

	const iggy::render::RenderMaterialResolveResult result = registry.resolveMaterial(command.materialId);

	Expect(result.status == iggy::render::RenderMaterialResolveStatus::Resolved, "render command material id should resolve through registry in test-only composition");
	Expect(result.material != nullptr && result.material->id == MaterialId, "render command material id should point to matching material descriptor");
}

void TestAccessorsReturnStoredCatalogs()
{
	const iggy::render::RenderResourceRegistry registry = Registry(
		{ Texture(TextureId), Texture(iggy::ResourceId { "texture:floor" }) },
		{ Shader(ShaderId), Shader(iggy::ResourceId { "shader:debug" }) },
		{ Material(MaterialId), Material(iggy::ResourceId { "material:floor" }, ShaderId, TextureId) });

	Expect(registry.textures().resources().size() == 2, "textures accessor should return stored texture catalog");
	Expect(registry.shaders().resources().size() == 2, "shaders accessor should return stored shader catalog");
	Expect(registry.materials().resources().size() == 2, "materials accessor should return stored material catalog");
	if (registry.textures().resources().size() == 2 && registry.shaders().resources().size() == 2 && registry.materials().resources().size() == 2) {
		Expect(registry.textures().resources()[0].id == TextureId, "textures accessor should preserve resource order");
		Expect(registry.shaders().resources()[0].id == ShaderId, "shaders accessor should preserve resource order");
		Expect(registry.materials().resources()[0].id == MaterialId, "materials accessor should preserve resource order");
	}
}

} // namespace

int main()
{
	TestDefaultRegistryReturnsMissing();
	TestRegistryPreservesDirectLookups();
	TestResolveMaterialWithExistingShaderAndTexture();
	TestEmptyShaderIdResolvesWithTextureOnly();
	TestEmptyTextureIdResolvesWithShaderOnly();
	TestBothEmptyRefsResolveWithMaterialOnly();
	TestMissingShaderPreservesResolvedTexture();
	TestMissingTexturePreservesResolvedShader();
	TestMissingShaderAndTexture();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestRenderCommandMaterialIdResolvesThroughRegistry();
	TestAccessorsReturnStoredCatalogs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
