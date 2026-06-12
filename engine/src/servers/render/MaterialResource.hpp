#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::render {

enum class MaterialResourceIssueCode {
	EmptyId,
	DuplicateId,
};

struct MaterialResource {
	ResourceId id;
	ResourceId shaderId;
	ResourceId textureId;
};

struct MaterialResourceIssue {
	MaterialResourceIssueCode code = MaterialResourceIssueCode::EmptyId;
	ResourceId id;
	ResourceId shaderId;
	ResourceId textureId;
};

class MaterialResourceCatalog {
public:
	MaterialResourceCatalog() = default;
	explicit MaterialResourceCatalog(std::vector<MaterialResource> resources);

	[[nodiscard]] bool contains(const ResourceId &id) const;
	[[nodiscard]] const MaterialResource *find(const ResourceId &id) const;
	[[nodiscard]] const std::vector<MaterialResource> &resources() const;

private:
	std::vector<MaterialResource> resources_;
};

struct MaterialResourceBuildResult {
	bool built = false;
	MaterialResourceCatalog catalog;
	std::vector<MaterialResourceIssue> issues;
};

class MaterialResourceCatalogBuilder {
public:
	[[nodiscard]] MaterialResourceBuildResult build(std::vector<MaterialResource> resources) const;
};

} // namespace iggy::render
