#include <cstdlib>
#include <vector>

#include "scene/interaction/InteractionTargetToggle2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Inspectable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "interaction target toggle registry setup should build");
	return result.registry;
}

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void ExpectRegistryTargets(
	const iggy::InteractionTarget2DRegistry &registry,
	const std::vector<iggy::InteractionTarget2D> &expected,
	const char *message)
{
	Expect(registry.targets().size() == expected.size(), message);
	if (registry.targets().size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		ExpectTarget(registry.targets()[index], expected[index], message);
}

void TestEmptyTargetIdReturnsMissingTargetId()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:one", iggy::InteractionTarget2DKind::Usable, { 1.0F, 2.0F }, 0.5F, true),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(registry, iggy::ResourceId {}, false);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::MissingTargetId, "empty target id should return MissingTargetId");
	Expect(result.targetId.empty(), "empty target id should be preserved");
	Expect(!result.requestedEnabled, "empty target id should preserve requested enabled value");
	Expect(!result.changed, "empty target id should not change registry");
	ExpectRegistryTargets(result.registry, targets, "empty target id should return copied original registry");
}

void TestMissingTargetIdReturnsTargetNotFound()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:one", iggy::InteractionTarget2DKind::Usable, { 1.0F, 2.0F }, 0.5F, true),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(registry, iggy::ResourceId { "target:missing" }, false);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::TargetNotFound, "missing target id should return TargetNotFound");
	Expect(result.targetId == iggy::ResourceId { "target:missing" }, "missing target id should be preserved");
	Expect(!result.requestedEnabled, "missing target id should preserve requested enabled value");
	Expect(!result.changed, "missing target id should not change registry");
	ExpectRegistryTargets(result.registry, targets, "missing target id should return copied original registry");
}

void TestToggleFalseToTrue()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:first", iggy::InteractionTarget2DKind::Inspectable, { -1.0F, 0.0F }, 0.25F, true),
		Target("target:toggle", iggy::InteractionTarget2DKind::Door, { 2.0F, 3.0F }, 1.5F, false),
		Target("target:last", iggy::InteractionTarget2DKind::Pickup, { 4.0F, 5.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[1].enabled = true;

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(Registry(targets), iggy::ResourceId { "target:toggle" }, true);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::Toggled, "false-to-true target toggle should return Toggled");
	Expect(result.changed, "false-to-true target toggle should mark changed");
	Expect(result.requestedEnabled, "false-to-true target toggle should preserve requested enabled value");
	ExpectRegistryTargets(result.registry, expected, "false-to-true target toggle should update only enabled state");
}

void TestToggleTrueToFalse()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:toggle", iggy::InteractionTarget2DKind::Usable, { 2.0F, 3.0F }, 1.0F, true),
		Target("target:other", iggy::InteractionTarget2DKind::Talk, { -2.0F, -3.0F }, 2.0F, false),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(Registry(targets), iggy::ResourceId { "target:toggle" }, false);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::Toggled, "true-to-false target toggle should return Toggled");
	Expect(result.changed, "true-to-false target toggle should mark changed");
	Expect(!result.requestedEnabled, "true-to-false target toggle should preserve requested enabled value");
	ExpectRegistryTargets(result.registry, expected, "true-to-false target toggle should update only enabled state");
}

void TestNoChangeWhenAlreadyRequestedValue()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:toggle", iggy::InteractionTarget2DKind::Usable, { 2.0F, 3.0F }, 1.0F, false),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(registry, iggy::ResourceId { "target:toggle" }, false);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::NoChange, "already matching target should return NoChange");
	Expect(!result.changed, "already matching target should not mark changed");
	Expect(!result.requestedEnabled, "already matching target should preserve requested enabled value");
	ExpectRegistryTargets(result.registry, targets, "already matching target should return copied original registry");
}

void TestPreservesOrderAndNonEnabledFields()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:a", iggy::InteractionTarget2DKind::Unknown, { -3.0F, 4.0F }, 0.0F, true),
		Target("target:b", iggy::InteractionTarget2DKind::Door, { 9.0F, -8.0F }, 3.25F, true),
		Target("target:c", iggy::InteractionTarget2DKind::Talk, { 0.5F, 0.25F }, 2.0F, false),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[1].enabled = false;

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(Registry(targets), iggy::ResourceId { "target:b" }, false);

	Expect(result.changed, "field-preserving target toggle should change setup target");
	ExpectRegistryTargets(result.registry, expected, "target toggle should preserve order and non-enabled fields");
}

void TestDuplicateTargetIdsToggleFirstOnly()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:duplicate", iggy::InteractionTarget2DKind::Door, { 1.0F, 0.0F }, 1.0F, true),
		Target("target:other", iggy::InteractionTarget2DKind::Talk, { 2.0F, 0.0F }, 2.0F, true),
		Target("target:duplicate", iggy::InteractionTarget2DKind::Pickup, { 3.0F, 0.0F }, 3.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[0].enabled = false;
	const iggy::InteractionTarget2DRegistry registry { targets };

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(registry, iggy::ResourceId { "target:duplicate" }, false);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::Toggled, "duplicate target id should toggle first matching target");
	Expect(result.changed, "duplicate target id should mark changed");
	ExpectRegistryTargets(result.registry, expected, "duplicate target id should preserve later duplicate unchanged");
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 1.0F, true),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 1.0F, 0.0F }, 1.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expected = targets;
	expected[1].enabled = false;

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(Registry(targets), iggy::ResourceId { "target:door" }, false);

	Expect(result.status == iggy::InteractionTargetToggle2DStatus::Toggled, "namespaced id should toggle exact target");
	ExpectRegistryTargets(result.registry, expected, "namespaced target toggle should not affect unqualified id");
}

void TestOriginalRegistryIsNotMutated()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:toggle", iggy::InteractionTarget2DKind::Usable, { 2.0F, 3.0F }, 1.0F, true),
		Target("target:other", iggy::InteractionTarget2DKind::Talk, { -2.0F, -3.0F }, 2.0F, false),
	};
	const iggy::InteractionTarget2DRegistry registry = Registry(targets);

	const iggy::InteractionTargetToggle2DResult result =
		iggy::InteractionTargetToggle2D {}.apply(registry, iggy::ResourceId { "target:toggle" }, false);

	Expect(result.changed, "immutability setup should toggle target");
	ExpectRegistryTargets(registry, targets, "target toggle should not mutate original registry");
}

} // namespace

int main()
{
	TestEmptyTargetIdReturnsMissingTargetId();
	TestMissingTargetIdReturnsTargetNotFound();
	TestToggleFalseToTrue();
	TestToggleTrueToFalse();
	TestNoChangeWhenAlreadyRequestedValue();
	TestPreservesOrderAndNonEnabledFields();
	TestDuplicateTargetIdsToggleFirstOnly();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestOriginalRegistryIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
