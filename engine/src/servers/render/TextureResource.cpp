#include "servers/render/TextureResource.hpp"

#include <utility>

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::render::TextureResource> &resources, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (resources[index].id == resources[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy::render {

TextureResourceCatalog::TextureResourceCatalog(std::vector<TextureResource> resources)
    : resources_(std::move(resources))
{
}

bool TextureResourceCatalog::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

const TextureResource *TextureResourceCatalog::find(const ResourceId &id) const
{
	for (const TextureResource &resource : resources_) {
		if (resource.id == id)
			return &resource;
	}
	return nullptr;
}

const std::vector<TextureResource> &TextureResourceCatalog::resources() const
{
	return resources_;
}

TextureResourceBuildResult TextureResourceCatalogBuilder::build(std::vector<TextureResource> resources) const
{
	TextureResourceBuildResult result;
	for (std::size_t index = 0; index < resources.size(); ++index) {
		const TextureResource &resource = resources[index];
		if (resource.id.empty()) {
			result.issues.push_back({
				TextureResourceIssueCode::EmptyId,
				resource.id,
				resource.width,
				resource.height,
			});
		}
		if (HasEarlierMatchingId(resources, index)) {
			result.issues.push_back({
				TextureResourceIssueCode::DuplicateId,
				resource.id,
				resource.width,
				resource.height,
			});
		}
		if (resource.width <= 0 || resource.height <= 0) {
			result.issues.push_back({
				TextureResourceIssueCode::InvalidSize,
				resource.id,
				resource.width,
				resource.height,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.catalog = TextureResourceCatalog { std::move(resources) };
	return result;
}

} // namespace iggy::render
