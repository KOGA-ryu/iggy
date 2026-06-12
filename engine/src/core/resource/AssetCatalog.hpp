#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"

namespace iggy {

enum class AssetCatalogIssueCode {
	EmptyId,
	DuplicateId,
};

struct AssetRecord {
	ResourceId id;
	ResourceId kindId;
	std::string sourcePath;
};

struct AssetCatalogIssue {
	AssetCatalogIssueCode code = AssetCatalogIssueCode::EmptyId;
	ResourceId id;
	std::string sourcePath;
};

class AssetCatalog {
public:
	AssetCatalog() = default;
	explicit AssetCatalog(std::vector<AssetRecord> records);

	[[nodiscard]] bool contains(const ResourceId &id) const;
	[[nodiscard]] const AssetRecord *find(const ResourceId &id) const;
	[[nodiscard]] const std::vector<AssetRecord> &records() const;

private:
	std::vector<AssetRecord> records_;
};

struct AssetCatalogBuildResult {
	bool built = false;
	AssetCatalog catalog;
	std::vector<AssetCatalogIssue> issues;
};

class AssetCatalogBuilder {
public:
	[[nodiscard]] AssetCatalogBuildResult build(std::vector<AssetRecord> records) const;
};

} // namespace iggy
