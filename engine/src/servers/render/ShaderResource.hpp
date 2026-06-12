#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::render {

enum class ShaderResourceKind {
	Unknown,
	Canvas2D,
};

enum class ShaderResourceIssueCode {
	EmptyId,
	DuplicateId,
};

struct ShaderResource {
	ResourceId id;
	ResourceId assetId;
	ShaderResourceKind kind = ShaderResourceKind::Unknown;
};

struct ShaderResourceIssue {
	ShaderResourceIssueCode code = ShaderResourceIssueCode::EmptyId;
	ResourceId id;
	ShaderResourceKind kind = ShaderResourceKind::Unknown;
};

class ShaderResourceCatalog {
public:
	ShaderResourceCatalog() = default;
	explicit ShaderResourceCatalog(std::vector<ShaderResource> resources);

	[[nodiscard]] bool contains(const ResourceId &id) const;
	[[nodiscard]] const ShaderResource *find(const ResourceId &id) const;
	[[nodiscard]] const std::vector<ShaderResource> &resources() const;

private:
	std::vector<ShaderResource> resources_;
};

struct ShaderResourceBuildResult {
	bool built = false;
	ShaderResourceCatalog catalog;
	std::vector<ShaderResourceIssue> issues;
};

class ShaderResourceCatalogBuilder {
public:
	[[nodiscard]] ShaderResourceBuildResult build(std::vector<ShaderResource> resources) const;
};

} // namespace iggy::render
