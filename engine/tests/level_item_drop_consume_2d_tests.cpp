#include <cstdlib>
#include <vector>

#include "scene/inventory/LevelItemDropConsume2D.hpp"
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

iggy::LevelItemDrop2DRegistry Registry(std::vector<iggy::LevelItemDrop2D> drops)
{
	return { drops };
}

bool SameDrop(const iggy::LevelItemDrop2D &actual, const iggy::LevelItemDrop2D &expected)
{
	return actual.id == expected.id
		&& actual.itemId == expected.itemId
		&& actual.count == expected.count
		&& NearVec(actual.position, expected.position)
		&& actual.pickupRadius == expected.pickupRadius
		&& actual.enabled == expected.enabled;
}

bool SameDrops(
	const std::vector<iggy::LevelItemDrop2D> &actual,
	const std::vector<iggy::LevelItemDrop2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameDrop(actual[index], expected[index]))
			return false;
	}
	return true;
}

void TestEmptyDropId()
{
	const iggy::LevelItemDrop2DRegistry registry = Registry({ Drop("drop:potion") });

	const iggy::LevelItemDropConsume2DResult result = iggy::LevelItemDropConsume2D {}.consume(registry, iggy::ResourceId {});

	Expect(result.status == iggy::LevelItemDropConsume2DStatus::MissingDropId, "empty drop id should be missing");
	Expect(result.dropId.empty(), "empty drop id result should preserve requested id");
	Expect(result.mode == iggy::LevelItemDropConsume2DMode::Disable, "empty drop id result should preserve default mode");
	Expect(!result.changed, "empty drop id should not change registry");
	Expect(SameDrops(result.registry.drops, registry.drops), "empty drop id should return copied original registry");
}

void TestMissingDropId()
{
	const iggy::LevelItemDrop2DRegistry registry = Registry({ Drop("drop:potion") });

	const iggy::LevelItemDropConsume2DResult result = iggy::LevelItemDropConsume2D {}.consume(registry, iggy::ResourceId { "drop:missing" });

	Expect(result.status == iggy::LevelItemDropConsume2DStatus::DropNotFound, "missing drop id should be not found");
	Expect(result.dropId == iggy::ResourceId { "drop:missing" }, "missing drop id result should preserve requested id");
	Expect(!result.changed, "missing drop id should not change registry");
	Expect(SameDrops(result.registry.drops, registry.drops), "missing drop id should return copied original registry");
}

void TestAlreadyDisabledDrop()
{
	const iggy::LevelItemDrop2DRegistry registry = Registry({
		Drop("drop:potion", "item:potion", 1, { 1.0F, 2.0F }, 0.5F, false),
	});

	const iggy::LevelItemDropConsume2DResult result = iggy::LevelItemDropConsume2D {}.consume(
		registry,
		iggy::ResourceId { "drop:potion" },
		iggy::LevelItemDropConsume2DMode::Remove);

	Expect(result.status == iggy::LevelItemDropConsume2DStatus::AlreadyDisabled, "disabled drop should be already disabled");
	Expect(result.mode == iggy::LevelItemDropConsume2DMode::Remove, "disabled drop result should preserve requested mode");
	Expect(!result.changed, "disabled drop should not change registry");
	Expect(SameDrops(result.registry.drops, registry.drops), "disabled drop should return copied original registry");
}

void TestDisableModeConsumesDropAndPreservesFieldsAndOrder()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 3, { 1.0F, 2.0F }, 0.25F, true);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1, { -4.0F, 5.0F }, 1.0F, true);
	const iggy::LevelItemDrop2DRegistry registry = Registry({ first, second });

	const iggy::LevelItemDropConsume2DResult result = iggy::LevelItemDropConsume2D {}.consume(registry, first.id);

	Expect(result.status == iggy::LevelItemDropConsume2DStatus::Consumed, "disable mode should consume enabled drop");
	Expect(result.changed, "disable mode should change registry");
	Expect(result.registry.drops.size() == 2, "disable mode should preserve drop count");
	if (result.registry.drops.size() == 2) {
		iggy::LevelItemDrop2D expectedFirst = first;
		expectedFirst.enabled = false;
		Expect(SameDrop(result.registry.drops[0], expectedFirst), "disable mode should only disable matching drop");
		Expect(SameDrop(result.registry.drops[1], second), "disable mode should preserve neighboring drop");
	}
}

void TestRemoveModeConsumesDropAndPreservesRemainingOrder()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 3);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1);
	const iggy::LevelItemDrop2D third = Drop("drop:scroll", "item:scroll", 2);
	const iggy::LevelItemDrop2DRegistry registry = Registry({ first, second, third });

	const iggy::LevelItemDropConsume2DResult result = iggy::LevelItemDropConsume2D {}.consume(
		registry,
		second.id,
		iggy::LevelItemDropConsume2DMode::Remove);

	Expect(result.status == iggy::LevelItemDropConsume2DStatus::Consumed, "remove mode should consume enabled drop");
	Expect(result.changed, "remove mode should change registry");
	Expect(result.registry.drops.size() == 2, "remove mode should remove one drop");
	if (result.registry.drops.size() == 2) {
		Expect(SameDrop(result.registry.drops[0], first), "remove mode should preserve preceding drop");
		Expect(SameDrop(result.registry.drops[1], third), "remove mode should preserve following drop order");
	}
}

void TestDuplicateIdsConsumeFirstOnly()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 1, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::LevelItemDrop2D duplicate = Drop("drop:potion", "item:rare_potion", 2, { 4.0F, 5.0F }, 1.0F, true);
	const iggy::LevelItemDrop2DRegistry registry = Registry({ first, duplicate });

	const iggy::LevelItemDropConsume2DResult disableResult = iggy::LevelItemDropConsume2D {}.consume(registry, first.id);
	const iggy::LevelItemDropConsume2DResult removeResult = iggy::LevelItemDropConsume2D {}.consume(
		registry,
		first.id,
		iggy::LevelItemDropConsume2DMode::Remove);

	Expect(disableResult.status == iggy::LevelItemDropConsume2DStatus::Consumed, "duplicate disable should consume first match");
	if (disableResult.registry.drops.size() == 2) {
		iggy::LevelItemDrop2D disabledFirst = first;
		disabledFirst.enabled = false;
		Expect(SameDrop(disableResult.registry.drops[0], disabledFirst), "duplicate disable should affect first match");
		Expect(SameDrop(disableResult.registry.drops[1], duplicate), "duplicate disable should preserve later duplicate");
	}
	Expect(removeResult.status == iggy::LevelItemDropConsume2DStatus::Consumed, "duplicate remove should consume first match");
	Expect(removeResult.registry.drops.size() == 1, "duplicate remove should remove one matching drop");
	if (removeResult.registry.drops.size() == 1)
		Expect(SameDrop(removeResult.registry.drops[0], duplicate), "duplicate remove should preserve later duplicate");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const iggy::LevelItemDrop2D unqualified = Drop("potion", "item:potion", 1);
	const iggy::LevelItemDrop2D namespaced = Drop("drop:potion", "item:potion", 1);
	const iggy::LevelItemDrop2DRegistry registry = Registry({ unqualified, namespaced });

	const iggy::LevelItemDropConsume2DResult result = iggy::LevelItemDropConsume2D {}.consume(
		registry,
		iggy::ResourceId { "potion" },
		iggy::LevelItemDropConsume2DMode::Remove);

	Expect(result.status == iggy::LevelItemDropConsume2DStatus::Consumed, "unqualified id should match exact unqualified drop");
	Expect(result.registry.drops.size() == 1, "consuming unqualified id should remove only one drop");
	if (result.registry.drops.size() == 1)
		Expect(SameDrop(result.registry.drops[0], namespaced), "namespaced drop should remain distinct");
}

void TestOriginalRegistryIsNotMutated()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 3);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1);
	iggy::LevelItemDrop2DRegistry registry = Registry({ first, second });
	const iggy::LevelItemDrop2DRegistry before = registry;

	const iggy::LevelItemDropConsume2DResult disableResult = iggy::LevelItemDropConsume2D {}.consume(registry, first.id);
	const iggy::LevelItemDropConsume2DResult removeResult = iggy::LevelItemDropConsume2D {}.consume(
		registry,
		second.id,
		iggy::LevelItemDropConsume2DMode::Remove);

	Expect(disableResult.changed, "immutability disable setup should change result");
	Expect(removeResult.changed, "immutability remove setup should change result");
	Expect(SameDrops(registry.drops, before.drops), "consume should not mutate original registry");
}

} // namespace

int main()
{
	TestEmptyDropId();
	TestMissingDropId();
	TestAlreadyDisabledDrop();
	TestDisableModeConsumesDropAndPreservesFieldsAndOrder();
	TestRemoveModeConsumesDropAndPreservesRemainingOrder();
	TestDuplicateIdsConsumeFirstOnly();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestOriginalRegistryIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
