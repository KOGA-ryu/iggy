#include "core/resource/AssetCatalog.hpp"

#include <utility>

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::AssetRecord> &records, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (records[index].id == records[currentIndex].id)
			return true;
	}
	return false;
}

} // namespace

namespace iggy {

AssetCatalog::AssetCatalog(std::vector<AssetRecord> records)
    : records_(std::move(records))
{
}

bool AssetCatalog::contains(const ResourceId &id) const
{
	return find(id) != nullptr;
}

const AssetRecord *AssetCatalog::find(const ResourceId &id) const
{
	for (const AssetRecord &record : records_) {
		if (record.id == id)
			return &record;
	}
	return nullptr;
}

const std::vector<AssetRecord> &AssetCatalog::records() const
{
	return records_;
}

AssetCatalogBuildResult AssetCatalogBuilder::build(std::vector<AssetRecord> records) const
{
	AssetCatalogBuildResult result;
	for (std::size_t index = 0; index < records.size(); ++index) {
		const AssetRecord &record = records[index];
		if (record.id.empty()) {
			result.issues.push_back({
				AssetCatalogIssueCode::EmptyId,
				record.id,
				record.sourcePath,
			});
		}
		if (HasEarlierMatchingId(records, index)) {
			result.issues.push_back({
				AssetCatalogIssueCode::DuplicateId,
				record.id,
				record.sourcePath,
			});
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.catalog = AssetCatalog { std::move(records) };
	return result;
}

} // namespace iggy
