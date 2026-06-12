#include "servers/render/ShaderResource.hpp"

#include <utility>

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::render::ShaderResource> &resources, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (resources[index].id == resources[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy::render {

ShaderResourceCatalog::ShaderResourceCatalog(std::vector<ShaderResource> resources)
    : resources_(std::move(resources))
{
}

bool ShaderResourceCatalog::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

const ShaderResource *ShaderResourceCatalog::find(const ResourceId &id) const
{
	for (const ShaderResource &resource : resources_) {
		if (resource.id == id)
			return &resource;
	}
	return nullptr;
}

const std::vector<ShaderResource> &ShaderResourceCatalog::resources() const
{
	return resources_;
}

ShaderResourceBuildResult ShaderResourceCatalogBuilder::build(std::vector<ShaderResource> resources) const
{
	ShaderResourceBuildResult result;
	for (std::size_t index = 0; index < resources.size(); ++index) {
		const ShaderResource &resource = resources[index];
		if (resource.id.empty()) {
			result.issues.push_back({
				ShaderResourceIssueCode::EmptyId,
				resource.id,
				resource.kind,
			});
		}
		if (HasEarlierMatchingId(resources, index)) {
			result.issues.push_back({
				ShaderResourceIssueCode::DuplicateId,
				resource.id,
				resource.kind,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.catalog = ShaderResourceCatalog { std::move(resources) };
	return result;
}

} // namespace iggy::render
