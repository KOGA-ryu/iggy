#include <cstdlib>
#include <vector>

#include "core/resource/AssetCatalog.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

const iggy::ResourceId TextureKind { "asset_kind:texture" };
const iggy::ResourceId MaterialKind { "asset_kind:material" };

iggy::AssetRecord Record(const char *id, const iggy::ResourceId &kindId, const char *sourcePath)
{
	return { iggy::ResourceId { id }, kindId, sourcePath };
}

void ExpectRecord(const iggy::AssetRecord &record, const iggy::ResourceId &id, const iggy::ResourceId &kindId, std::string_view sourcePath, const char *message)
{
	Expect(record.id == id && record.kindId == kindId && record.sourcePath == sourcePath, message);
}

void TestEmptyInputBuildsValidEmptyCatalog()
{
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({});

	Expect(result.built, "empty input should build a valid catalog");
	Expect(result.issues.empty(), "empty input should produce no issues");
	Expect(result.catalog.records().empty(), "empty input should produce empty catalog records");
}

void TestSingleValidRecordBuildsAndFindsExactRecord()
{
	const iggy::ResourceId id { "texture:wall" };
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		{ id, TextureKind, "assets/wall.png" },
	});

	Expect(result.built, "single valid record should build");
	Expect(result.issues.empty(), "single valid record should produce no issues");
	Expect(result.catalog.contains(id), "catalog should contain valid record id");
	const iggy::AssetRecord *found = result.catalog.find(id);
	Expect(found != nullptr, "find should return valid record");
	if (found != nullptr)
		ExpectRecord(*found, id, TextureKind, "assets/wall.png", "found record should preserve id, kind, and source path");
}

void TestMissingLookupReturnsFalseAndNull()
{
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		Record("texture:wall", TextureKind, "assets/wall.png"),
	});

	Expect(result.built, "lookup test setup should build");
	Expect(!result.catalog.contains(iggy::ResourceId { "texture:floor" }), "missing id should not be contained");
	Expect(result.catalog.find(iggy::ResourceId { "texture:floor" }) == nullptr, "missing id should return null from find");
}

void TestMultipleValidRecordsPreserveInputOrder()
{
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		Record("texture:wall", TextureKind, "assets/wall.png"),
		Record("material:wall", MaterialKind, "assets/wall.material"),
		Record("texture:floor", TextureKind, "assets/floor.png"),
	});

	Expect(result.built, "multiple valid records should build");
	Expect(result.catalog.records().size() == 3, "multiple valid records should be preserved");
	if (result.catalog.records().size() == 3) {
		Expect(result.catalog.records()[0].id == iggy::ResourceId { "texture:wall" }, "first record should preserve input order");
		Expect(result.catalog.records()[1].id == iggy::ResourceId { "material:wall" }, "second record should preserve input order");
		Expect(result.catalog.records()[2].id == iggy::ResourceId { "texture:floor" }, "third record should preserve input order");
	}
}

void TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog()
{
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		Record("texture:wall", TextureKind, "assets/wall_a.png"),
		Record("texture:wall", TextureKind, "assets/wall_b.png"),
	});

	Expect(!result.built, "duplicate ids should fail catalog build");
	Expect(result.catalog.records().empty(), "duplicate ids should return an empty catalog");
	Expect(result.issues.size() == 1, "duplicate ids should produce one duplicate issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::AssetCatalogIssueCode::DuplicateId, "duplicate issue should use DuplicateId code");
		Expect(result.issues[0].id == iggy::ResourceId { "texture:wall" }, "duplicate issue should preserve duplicate id");
		Expect(result.issues[0].sourcePath == "assets/wall_b.png", "duplicate issue should preserve duplicate record source path");
	}
}

void TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog()
{
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		{ {}, TextureKind, "assets/missing.png" },
	});

	Expect(!result.built, "empty id should fail catalog build");
	Expect(result.catalog.records().empty(), "empty id should return an empty catalog");
	Expect(result.issues.size() == 1, "empty id should produce one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::AssetCatalogIssueCode::EmptyId, "empty id issue should use EmptyId code");
		Expect(result.issues[0].id.empty(), "empty id issue should preserve empty id");
		Expect(result.issues[0].sourcePath == "assets/missing.png", "empty id issue should preserve source path");
	}
}

void TestMultipleIssuesReportedInInputOrder()
{
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		Record("texture:wall", TextureKind, "assets/wall_a.png"),
		{ {}, TextureKind, "assets/missing.png" },
		Record("texture:wall", TextureKind, "assets/wall_b.png"),
	});

	Expect(!result.built, "multiple validation issues should fail catalog build");
	Expect(result.catalog.records().empty(), "multiple validation issues should return an empty catalog");
	Expect(result.issues.size() == 2, "multiple validation issues should be reported");
	if (result.issues.size() == 2) {
		Expect(result.issues[0].code == iggy::AssetCatalogIssueCode::EmptyId && result.issues[0].sourcePath == "assets/missing.png", "empty id issue should appear at its input position");
		Expect(result.issues[1].code == iggy::AssetCatalogIssueCode::DuplicateId && result.issues[1].sourcePath == "assets/wall_b.png", "duplicate issue should appear when duplicate record is encountered");
	}
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::ResourceId namespaced { "texture:wall" };
	const iggy::ResourceId unqualified { "wall" };
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		{ namespaced, TextureKind, "assets/texture_wall.png" },
		{ unqualified, TextureKind, "assets/unqualified_wall.png" },
	});

	Expect(result.built, "namespaced and unqualified ids should build as distinct records");
	Expect(result.catalog.contains(namespaced), "catalog should contain namespaced id");
	Expect(result.catalog.contains(unqualified), "catalog should contain unqualified id");
	if (const iggy::AssetRecord *found = result.catalog.find(unqualified))
		Expect(found->sourcePath == "assets/unqualified_wall.png", "unqualified lookup should find exact unqualified record");
	else
		Expect(false, "unqualified lookup should return a record");
}

void TestEmptySourcePathIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "texture:generated" };
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		{ id, TextureKind, "" },
	});

	Expect(result.built, "empty source path should be allowed");
	const iggy::AssetRecord *found = result.catalog.find(id);
	Expect(found != nullptr, "empty source path record should be findable");
	if (found != nullptr)
		Expect(found->sourcePath.empty(), "empty source path should be preserved");
}

void TestEmptyKindIdIsAllowedAndPreserved()
{
	const iggy::ResourceId id { "asset:unknown_kind" };
	const iggy::AssetCatalogBuildResult result = iggy::AssetCatalogBuilder {}.build({
		{ id, {}, "assets/unknown.asset" },
	});

	Expect(result.built, "empty kind id should be allowed");
	const iggy::AssetRecord *found = result.catalog.find(id);
	Expect(found != nullptr, "empty kind id record should be findable");
	if (found != nullptr)
		Expect(found->kindId.empty(), "empty kind id should be preserved");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyCatalog();
	TestSingleValidRecordBuildsAndFindsExactRecord();
	TestMissingLookupReturnsFalseAndNull();
	TestMultipleValidRecordsPreserveInputOrder();
	TestDuplicateIdsFailWithDuplicateIssueAndEmptyCatalog();
	TestEmptyIdFailsWithEmptyIdIssueAndEmptyCatalog();
	TestMultipleIssuesReportedInInputOrder();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestEmptySourcePathIsAllowedAndPreserved();
	TestEmptyKindIdIsAllowedAndPreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
