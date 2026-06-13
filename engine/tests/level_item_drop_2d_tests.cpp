#include <cstdlib>
#include <vector>

#include "scene/inventory/LevelItemDrop2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId = "item:potion",
	std::uint32_t count = 1,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float pickupRadius = 0.0F,
	bool enabled = true)
{
	return {
		iggy::ResourceId { id },
		iggy::ResourceId { itemId },
		count,
		position,
		pickupRadius,
		enabled,
	};
}

void ExpectDrop(const iggy::LevelItemDrop2D &actual, const iggy::LevelItemDrop2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.itemId == expected.itemId, message);
	Expect(actual.count == expected.count, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.pickupRadius == expected.pickupRadius, message);
	Expect(actual.enabled == expected.enabled, message);
}

bool SameDrops(
	const std::vector<iggy::LevelItemDrop2D> &actual,
	const std::vector<iggy::LevelItemDrop2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].id != expected[index].id
			|| actual[index].itemId != expected[index].itemId
			|| actual[index].count != expected[index].count
			|| !NearVec(actual[index].position, expected[index].position)
			|| actual[index].pickupRadius != expected[index].pickupRadius
			|| actual[index].enabled != expected[index].enabled) {
			return false;
		}
	}
	return true;
}

void TestEmptyInputBuildsValidEmptyRegistry()
{
	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build({});

	Expect(result.built, "empty item drop input should build");
	Expect(result.issues.empty(), "empty item drop input should have no issues");
	Expect(result.registry.drops.empty(), "empty item drop input should produce empty registry");
	Expect(!result.registry.contains(iggy::ResourceId { "drop:missing" }), "empty item drop registry should not contain missing id");
	Expect(result.registry.find(iggy::ResourceId { "drop:missing" }) == nullptr, "empty item drop registry should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 3, { 1.0F, 2.0F }, 0.0F, true),
		Drop("drop:key", "item:key", 1, { -3.0F, 4.5F }, 1.25F, false),
		Drop("drop:gold", "gold", 25, { 0.0F, -1.0F }, 0.5F, true),
	};

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);

	Expect(result.built, "valid item drops should build");
	Expect(result.issues.empty(), "valid item drops should have no issues");
	Expect(result.registry.drops.size() == drops.size(), "valid item drop registry should preserve drop count");
	if (result.registry.drops.size() == drops.size()) {
		ExpectDrop(result.registry.drops[0], drops[0], "first item drop should preserve fields");
		ExpectDrop(result.registry.drops[1], drops[1], "second item drop should preserve fields");
		ExpectDrop(result.registry.drops[2], drops[2], "third item drop should preserve fields");
	}
}

void TestFindAndContainsUseExactDropIds()
{
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 4),
		Drop("drop:scroll", "item:scroll", 2),
	};
	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);

	Expect(result.built, "item drop lookup setup should build");
	Expect(result.registry.contains(iggy::ResourceId { "drop:potion" }), "item drop registry should contain exact drop id");
	Expect(!result.registry.contains(iggy::ResourceId { "drop:missing" }), "item drop registry should not contain missing drop id");
	const iggy::LevelItemDrop2D *drop = result.registry.find(iggy::ResourceId { "drop:scroll" });
	Expect(drop != nullptr, "item drop registry should find exact drop id");
	if (drop != nullptr)
		Expect(drop->itemId == iggy::ResourceId { "item:scroll" } && drop->count == 2, "item drop find should return matching payload");
	Expect(result.registry.find(iggy::ResourceId { "drop:missing" }) == nullptr, "item drop find should return null for missing id");
}

void TestEmptyDropIdFails()
{
	const iggy::LevelItemDrop2D drop = Drop("", "item:potion", 1);

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build({ drop });

	Expect(!result.built, "empty item drop id should fail build");
	Expect(result.registry.drops.empty(), "failed empty-drop-id build should not publish drops");
	Expect(result.issues.size() == 1, "empty item drop id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelItemDrop2DIssueCode::EmptyId, "empty drop id issue should use EmptyId code");
		Expect(result.issues[0].dropIndex == 0, "empty drop id issue should preserve drop index");
		ExpectDrop(result.issues[0].drop, drop, "empty drop id issue should preserve drop payload");
	}
}

void TestDuplicateDropIdFailsForLaterDrop()
{
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 1),
		Drop("drop:potion", "item:potion", 5, { 2.0F, 3.0F }, 1.0F),
	};

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);

	Expect(!result.built, "duplicate item drop id should fail build");
	Expect(result.registry.drops.empty(), "failed duplicate-drop-id build should not publish drops");
	Expect(result.issues.size() == 1, "duplicate item drop id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelItemDrop2DIssueCode::DuplicateId, "duplicate drop issue should use DuplicateId code");
		Expect(result.issues[0].dropIndex == 1, "duplicate drop issue should preserve later drop index");
		ExpectDrop(result.issues[0].drop, drops[1], "duplicate drop issue should preserve later drop payload");
	}
}

void TestEmptyItemIdFails()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:empty_item", "", 1);

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build({ drop });

	Expect(!result.built, "empty item id on drop should fail build");
	Expect(result.registry.drops.empty(), "failed empty-item-id drop build should not publish drops");
	Expect(result.issues.size() == 1, "empty item id on drop should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelItemDrop2DIssueCode::EmptyItemId, "empty item issue should use EmptyItemId code");
		Expect(result.issues[0].dropIndex == 0, "empty item issue should preserve drop index");
		ExpectDrop(result.issues[0].drop, drop, "empty item issue should preserve drop payload");
	}
}

void TestZeroCountFails()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:empty_count", "item:potion", 0);

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build({ drop });

	Expect(!result.built, "zero item drop count should fail build");
	Expect(result.registry.drops.empty(), "failed zero-count drop build should not publish drops");
	Expect(result.issues.size() == 1, "zero item drop count should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelItemDrop2DIssueCode::ZeroCount, "zero count issue should use ZeroCount code");
		Expect(result.issues[0].dropIndex == 0, "zero count issue should preserve drop index");
		ExpectDrop(result.issues[0].drop, drop, "zero count issue should preserve drop payload");
	}
}

void TestNegativePickupRadiusFails()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:bad_radius", "item:potion", 1, { 1.0F, 1.0F }, -0.01F);

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build({ drop });

	Expect(!result.built, "negative item drop pickup radius should fail build");
	Expect(result.registry.drops.empty(), "failed negative-radius drop build should not publish drops");
	Expect(result.issues.size() == 1, "negative item drop pickup radius should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::LevelItemDrop2DIssueCode::NegativePickupRadius, "negative radius issue should use NegativePickupRadius code");
		Expect(result.issues[0].dropIndex == 0, "negative radius issue should preserve drop index");
		ExpectDrop(result.issues[0].drop, drop, "negative radius issue should preserve drop payload");
	}
}

void TestDisabledDropsAndRadiusZeroAreValid()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:disabled", "item:key", 1, { -2.0F, 8.0F }, 0.0F, false);

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build({ drop });

	Expect(result.built, "disabled item drop with zero pickup radius should be valid data");
	Expect(result.issues.empty(), "disabled item drop with zero radius should not report issues");
	Expect(result.registry.drops.size() == 1, "disabled item drop should be preserved");
	if (result.registry.drops.size() == 1)
		ExpectDrop(result.registry.drops[0], drop, "disabled item drop should preserve payload");
}

void TestMultipleIssuesAreReportedInDeterministicInputOrder()
{
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 2),
		Drop("", "", 0, { 1.0F, 1.0F }, -1.0F),
		Drop("drop:potion", "item:potion", 0, { 2.0F, 2.0F }, -2.0F),
		Drop("drop:potion", "item:potion", 7),
	};

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);

	Expect(!result.built, "multiple item drop issues should fail build");
	Expect(result.registry.drops.empty(), "failed multi-issue drop build should not publish drops");
	Expect(result.issues.size() == 8, "multiple item drop issues should all be reported");
	if (result.issues.size() == 8) {
		Expect(result.issues[0].code == iggy::LevelItemDrop2DIssueCode::EmptyId && result.issues[0].dropIndex == 1, "empty drop id should be first issue for second drop");
		Expect(result.issues[1].code == iggy::LevelItemDrop2DIssueCode::EmptyItemId && result.issues[1].dropIndex == 1, "empty item id should follow empty drop id for second drop");
		Expect(result.issues[2].code == iggy::LevelItemDrop2DIssueCode::ZeroCount && result.issues[2].dropIndex == 1, "zero count should follow empty item id for second drop");
		Expect(result.issues[3].code == iggy::LevelItemDrop2DIssueCode::NegativePickupRadius && result.issues[3].dropIndex == 1, "negative radius should follow zero count for second drop");
		Expect(result.issues[4].code == iggy::LevelItemDrop2DIssueCode::DuplicateId && result.issues[4].dropIndex == 2, "duplicate id should be first issue for third drop");
		Expect(result.issues[5].code == iggy::LevelItemDrop2DIssueCode::ZeroCount && result.issues[5].dropIndex == 2, "zero count should follow duplicate id for third drop");
		Expect(result.issues[6].code == iggy::LevelItemDrop2DIssueCode::NegativePickupRadius && result.issues[6].dropIndex == 2, "negative radius should follow zero count for third drop");
		Expect(result.issues[7].code == iggy::LevelItemDrop2DIssueCode::DuplicateId && result.issues[7].dropIndex == 3, "duplicate id should be reported for fourth drop");
	}
}

void TestNamespacedAndUnqualifiedDropIdsAreDistinct()
{
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("potion", "item:potion", 1),
		Drop("drop:potion", "item:potion", 1),
	};

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);

	Expect(result.built, "namespaced and unqualified item drop ids should build as distinct ids");
	Expect(result.issues.empty(), "namespaced and unqualified item drop ids should not produce duplicate issues");
	Expect(result.registry.contains(iggy::ResourceId { "potion" }), "drop registry should contain unqualified drop id");
	Expect(result.registry.contains(iggy::ResourceId { "drop:potion" }), "drop registry should contain namespaced drop id");
	const iggy::LevelItemDrop2D *unqualified = result.registry.find(iggy::ResourceId { "potion" });
	const iggy::LevelItemDrop2D *namespaced = result.registry.find(iggy::ResourceId { "drop:potion" });
	Expect(unqualified != nullptr && namespaced != nullptr && unqualified != namespaced, "distinct item drop ids should resolve to distinct drops");
}

void TestInputVectorIsNotMutated()
{
	std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 3),
		Drop("drop:key", "item:key", 1),
	};
	const std::vector<iggy::LevelItemDrop2D> before = drops;

	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);

	Expect(result.built, "item drop immutability setup should build");
	Expect(SameDrops(drops, before), "item drop builder should not mutate input drops");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyRegistry();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactDropIds();
	TestEmptyDropIdFails();
	TestDuplicateDropIdFailsForLaterDrop();
	TestEmptyItemIdFails();
	TestZeroCountFails();
	TestNegativePickupRadiusFails();
	TestDisabledDropsAndRadiusZeroAreValid();
	TestMultipleIssuesAreReportedInDeterministicInputOrder();
	TestNamespacedAndUnqualifiedDropIdsAreDistinct();
	TestInputVectorIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
