#include <cstdlib>
#include <vector>

#include "scene/ai/NpcAiProfileTraitCatalog.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::NpcTraitSet Traits(int strength = 10)
{
	iggy::NpcTraitSet traits;
	traits.strength = strength;
	return traits;
}

iggy::NpcAiProfileTraitEntry Entry(const char *profileId, iggy::NpcTraitSet traits = {})
{
	return { Id(profileId), traits };
}

void TestEmptyCatalogBuildsValid()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build({});

	Expect(result.built, "empty profile trait catalog should build");
	Expect(result.catalog.entries.empty(), "empty profile trait catalog should publish empty entries");
	Expect(result.issues.empty(), "empty profile trait catalog should have no issues");
}

void TestCatalogPreservesEntriesAndFindsExactIds()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build({
			Entry("profile:guard", Traits(12)),
			Entry("guard", Traits(15)),
		});

	Expect(result.built, "profile trait catalog should build valid entries");
	Expect(result.catalog.entries.size() == 2, "profile trait catalog should preserve entries");
	Expect(result.catalog.contains(Id("profile:guard")), "profile trait catalog should contain namespaced id");
	Expect(result.catalog.contains(Id("guard")), "profile trait catalog should contain unqualified id distinctly");
	const iggy::NpcAiProfileTraitEntry *entry = result.catalog.find(Id("profile:guard"));
	Expect(entry != nullptr && entry->traits.strength == 12, "profile trait catalog should find exact profile traits");
}

void TestEmptyProfileIdFails()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build({ { {}, Traits() } });

	Expect(!result.built, "empty profile id should fail catalog build");
	Expect(result.catalog.entries.empty(), "failed empty profile id build should publish empty catalog");
	Expect(result.issues.size() == 1, "empty profile id should produce one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::NpcAiProfileTraitCatalogIssueCode::EmptyProfileId, "empty profile issue should use EmptyProfileId");
		Expect(result.issues[0].entryIndex == 0, "empty profile issue should preserve entry index");
	}
}

void TestDuplicateProfileIdFails()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build({
			Entry("profile:dup", Traits(10)),
			Entry("profile:dup", Traits(11)),
		});

	Expect(!result.built, "duplicate profile id should fail catalog build");
	Expect(result.catalog.entries.empty(), "failed duplicate profile id build should publish empty catalog");
	Expect(result.issues.size() == 1, "duplicate profile id should produce one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::NpcAiProfileTraitCatalogIssueCode::DuplicateProfileId, "duplicate profile issue should use DuplicateProfileId");
		Expect(result.issues[0].entryIndex == 1 && result.issues[0].firstEntryIndex == 0, "duplicate profile issue should preserve first/later indexes");
		Expect(result.issues[0].entry.profileId == Id("profile:dup"), "duplicate profile issue should preserve entry");
	}
}

void TestInvalidTraitSetFails()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build({ Entry("profile:invalid", Traits(99)) });

	Expect(!result.built, "invalid trait set should fail catalog build");
	Expect(result.issues.size() == 1, "invalid trait set should produce one issue");
	if (!result.issues.empty()) {
		Expect(result.issues[0].code == iggy::NpcAiProfileTraitCatalogIssueCode::InvalidTraitSet, "invalid trait issue should use InvalidTraitSet");
		Expect(!result.issues[0].traitValidation.ok(), "invalid trait issue should preserve trait validation");
	}
}

void TestMultipleIssuesPreserveOrder()
{
	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build({
			{ {}, Traits() },
			Entry("profile:dup", Traits()),
			Entry("profile:dup", Traits(99)),
		});

	Expect(!result.built, "multi-issue catalog should fail");
	Expect(result.issues.size() == 3, "multi-issue catalog should preserve all issues");
	if (result.issues.size() == 3) {
		Expect(result.issues[0].code == iggy::NpcAiProfileTraitCatalogIssueCode::EmptyProfileId, "empty id should be first issue");
		Expect(result.issues[1].code == iggy::NpcAiProfileTraitCatalogIssueCode::DuplicateProfileId, "duplicate id should be second issue");
		Expect(result.issues[2].code == iggy::NpcAiProfileTraitCatalogIssueCode::InvalidTraitSet, "invalid traits should be third issue");
	}
}

void TestBuilderDoesNotMutateInputs()
{
	std::vector<iggy::NpcAiProfileTraitEntry> entries {
		Entry("profile:immutable", Traits(13)),
	};
	const std::vector<iggy::NpcAiProfileTraitEntry> before = entries;

	const iggy::NpcAiProfileTraitCatalogBuildResult result =
		iggy::NpcAiProfileTraitCatalogBuilder {}.build(entries);

	Expect(result.built, "immutability setup should build");
	Expect(entries.size() == before.size(), "catalog builder should not mutate input count");
	Expect(entries[0].profileId == before[0].profileId && entries[0].traits.strength == before[0].traits.strength, "catalog builder should not mutate input entry");
}

} // namespace

int main()
{
	TestEmptyCatalogBuildsValid();
	TestCatalogPreservesEntriesAndFindsExactIds();
	TestEmptyProfileIdFails();
	TestDuplicateProfileIdFails();
	TestInvalidTraitSetFails();
	TestMultipleIssuesPreserveOrder();
	TestBuilderDoesNotMutateInputs();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
