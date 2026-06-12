#include "servers/render/MaterialResource.hpp"

#include <utility>

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::render::MaterialResource> &resources, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (resources[index].id == resources[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy::render {

MaterialResourceCatalog::MaterialResourceCatalog(std::vector<MaterialResource> resources)
    : resources_(std::move(resources))
{
}

bool MaterialResourceCatalog::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

const MaterialResource *MaterialResourceCatalog::find(const ResourceId &id) const
{
	for (const MaterialResource &resource : resources_) {
		if (resource.id == id)
			return &resource;
	}
	return nullptr;
}

const std::vector<MaterialResource> &MaterialResourceCatalog::resources() const
{
	return resources_;
}

MaterialResourceBuildResult MaterialResourceCatalogBuilder::build(std::vector<MaterialResource> resources) const
{
	MaterialResourceBuildResult result;
	for (std::size_t index = 0; index < resources.size(); ++index) {
		const MaterialResource &resource = resources[index];
		if (resource.id.empty()) {
			result.issues.push_back({
				MaterialResourceIssueCode::EmptyId,
				resource.id,
				resource.shaderId,
				resource.textureId,
			});
		}
		if (HasEarlierMatchingId(resources, index)) {
			result.issues.push_back({
				MaterialResourceIssueCode::DuplicateId,
				resource.id,
				resource.shaderId,
				resource.textureId,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.catalog = MaterialResourceCatalog { std::move(resources) };
	return result;
}

} // namespace iggy::render
