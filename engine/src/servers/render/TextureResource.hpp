#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy::render {

enum class TextureResourceIssueCode {
	EmptyId,
	DuplicateId,
	InvalidSize,
};

struct TextureResource {
	ResourceId id;
	ResourceId assetId;
	int width = 0;
	int height = 0;
};

struct TextureResourceIssue {
	TextureResourceIssueCode code = TextureResourceIssueCode::EmptyId;
	ResourceId id;
	int width = 0;
	int height = 0;
};

class TextureResourceCatalog {
public:
	TextureResourceCatalog() = default;
	explicit TextureResourceCatalog(std::vector<TextureResource> resources);

	[[nodiscard]] bool contains(const ResourceId &id) const;
	[[nodiscard]] const TextureResource *find(const ResourceId &id) const;
	[[nodiscard]] const std::vector<TextureResource> &resources() const;

private:
	std::vector<TextureResource> resources_;
};

struct TextureResourceBuildResult {
	bool built = false;
	TextureResourceCatalog catalog;
	std::vector<TextureResourceIssue> issues;
};

class TextureResourceCatalogBuilder {
public:
	[[nodiscard]] TextureResourceBuildResult build(std::vector<TextureResource> resources) const;
};

} // namespace iggy::render
