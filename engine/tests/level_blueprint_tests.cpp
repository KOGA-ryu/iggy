#include <cstdlib>
#include <iostream>
#include <string_view>

#include "modules/blueprint/LevelBlueprintValidator.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool HasIssue(const iggy::LevelBlueprintValidationReport &report, iggy::LevelBlueprintIssueCode code)
{
	for (const iggy::LevelBlueprintIssue &issue : report.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

iggy::LevelBlueprint ValidBlueprint()
{
	iggy::LevelBlueprint blueprint;
	blueprint.bounds = { 3, 2 };
	blueprint.tiles = {
		{ true },
		{ true },
		{ false },
		{ true },
		{ true },
		{ true },
	};
	blueprint.playerStarts.push_back({ 1, 1 });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 2, 1 });
	return blueprint;
}

void TestValidBlueprint()
{
	const iggy::LevelBlueprint blueprint = ValidBlueprint();
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(report.valid, "valid blueprint should pass validation");
	Expect(report.issues.empty(), "valid blueprint should not report issues");
}

void TestMissingPlayerStart()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.playerStarts.clear();
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "missing player start should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::MissingPlayerStart), "missing player start should report issue code");
}

void TestDuplicatePlayerStart()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.playerStarts.push_back({ 0, 0 });
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "duplicate player starts should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::DuplicatePlayerStart), "duplicate player starts should report issue code");
}

void TestOutOfBoundsPlayerStart()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.playerStarts[0] = { 3, 0 };
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "out-of-bounds player start should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::PlayerStartOutOfBounds), "out-of-bounds player start should report issue code");
}

void TestOutOfBoundsEntitySpawn()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.entitySpawns[0].y = -1;
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "out-of-bounds entity spawn should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::EntitySpawnOutOfBounds), "out-of-bounds entity spawn should report issue code");
}

void TestEmptyEntityType()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.entitySpawns[0].type = iggy::ResourceId {};
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "empty entity ResourceId should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::EmptyEntityType), "empty entity ResourceId should report issue code");
}

void TestDuplicateEntityId()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.entitySpawns[0].id = iggy::ResourceId { "spawn:guard_01" };
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "item:potion" }, 0, 0, iggy::ResourceId { "spawn:guard_01" } });
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "duplicate non-empty entity ids should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::DuplicateEntityId), "duplicate entity ids should report issue code");
}

void TestInvalidTileCount()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.tiles.pop_back();
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "wrong explicit tile count should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::InvalidTileCount), "wrong explicit tile count should report issue code");
}

void TestInvalidBounds()
{
	iggy::LevelBlueprint blueprint = ValidBlueprint();
	blueprint.bounds = { 0, 2 };
	const iggy::LevelBlueprintValidationReport report = iggy::LevelBlueprintValidator {}.validate(blueprint);

	Expect(!report.valid, "non-positive bounds should fail validation");
	Expect(HasIssue(report, iggy::LevelBlueprintIssueCode::InvalidBounds), "non-positive bounds should report issue code");
}

} // namespace

int main()
{
	TestValidBlueprint();
	TestMissingPlayerStart();
	TestDuplicatePlayerStart();
	TestOutOfBoundsPlayerStart();
	TestOutOfBoundsEntitySpawn();
	TestEmptyEntityType();
	TestDuplicateEntityId();
	TestInvalidTileCount();
	TestInvalidBounds();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
