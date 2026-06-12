#include "servers/render/RenderResourceRegistry.hpp"

#include <utility>

namespace iggy::render {

RenderResourceRegistry::RenderResourceRegistry(RenderResourceRegistryConfig config)
    : textures_(std::move(config.textures))
    , shaders_(std::move(config.shaders))
    , materials_(std::move(config.materials))
{
}

const TextureResource *RenderResourceRegistry::findTexture(const ResourceId &id) const
{
	return textures_.find(id);
}

const ShaderResource *RenderResourceRegistry::findShader(const ResourceId &id) const
{
	return shaders_.find(id);
}

const MaterialResource *RenderResourceRegistry::findMaterial(const ResourceId &id) const
{
	return materials_.find(id);
}

RenderMaterialResolveResult RenderResourceRegistry::resolveMaterial(const ResourceId &materialId) const
{
	RenderMaterialResolveResult result;
	result.material = findMaterial(materialId);
	if (result.material == nullptr)
		return result;

	bool missingShader = false;
	bool missingTexture = false;

	if (!result.material->shaderId.empty()) {
		result.shader = findShader(result.material->shaderId);
		missingShader = result.shader == nullptr;
	}

	if (!result.material->textureId.empty()) {
		result.texture = findTexture(result.material->textureId);
		missingTexture = result.texture == nullptr;
	}

	if (missingShader && missingTexture)
		result.status = RenderMaterialResolveStatus::MissingShaderAndTexture;
	else if (missingShader)
		result.status = RenderMaterialResolveStatus::MissingShader;
	else if (missingTexture)
		result.status = RenderMaterialResolveStatus::MissingTexture;
	else
		result.status = RenderMaterialResolveStatus::Resolved;

	return result;
}

const TextureResourceCatalog &RenderResourceRegistry::textures() const
{
	return textures_;
}

const ShaderResourceCatalog &RenderResourceRegistry::shaders() const
{
	return shaders_;
}

const MaterialResourceCatalog &RenderResourceRegistry::materials() const
{
	return materials_;
}

} // namespace iggy::render
