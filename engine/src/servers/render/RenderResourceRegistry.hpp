#pragma once

#include "core/resource/ResourceId.hpp"
#include "servers/render/MaterialResource.hpp"
#include "servers/render/ShaderResource.hpp"
#include "servers/render/TextureResource.hpp"

namespace iggy::render {

enum class RenderMaterialResolveStatus {
	MissingMaterial,
	Resolved,
	MissingShader,
	MissingTexture,
	MissingShaderAndTexture,
};

struct RenderMaterialResolveResult {
	RenderMaterialResolveStatus status = RenderMaterialResolveStatus::MissingMaterial;
	const MaterialResource *material = nullptr;
	const ShaderResource *shader = nullptr;
	const TextureResource *texture = nullptr;
};

struct RenderResourceRegistryConfig {
	TextureResourceCatalog textures;
	ShaderResourceCatalog shaders;
	MaterialResourceCatalog materials;
};

class RenderResourceRegistry {
public:
	RenderResourceRegistry() = default;
	explicit RenderResourceRegistry(RenderResourceRegistryConfig config);

	[[nodiscard]] const TextureResource *findTexture(const ResourceId &id) const;
	[[nodiscard]] const ShaderResource *findShader(const ResourceId &id) const;
	[[nodiscard]] const MaterialResource *findMaterial(const ResourceId &id) const;
	[[nodiscard]] RenderMaterialResolveResult resolveMaterial(const ResourceId &materialId) const;

	[[nodiscard]] const TextureResourceCatalog &textures() const;
	[[nodiscard]] const ShaderResourceCatalog &shaders() const;
	[[nodiscard]] const MaterialResourceCatalog &materials() const;

private:
	TextureResourceCatalog textures_;
	ShaderResourceCatalog shaders_;
	MaterialResourceCatalog materials_;
};

} // namespace iggy::render
