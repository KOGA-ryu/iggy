#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include "servers/navigation/NavigationPathFollower.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool Near(float actual, float expected, float tolerance = 0.0001F)
{
	return std::fabs(actual - expected) <= tolerance;
}

bool NearVec(iggy::Vec2 actual, iggy::Vec2 expected)
{
	return Near(actual.x, expected.x) && Near(actual.y, expected.y);
}

iggy::navigation::NavigationPath FoundPath()
{
	iggy::navigation::NavigationPath path;
	path.status = iggy::navigation::NavigationPathStatus::Found;
	path.waypoints = {
		{ 0.5F, 0.5F },
		{ 1.5F, 0.5F },
		{ 2.5F, 0.5F },
		{ 2.5F, 1.5F },
	};
	return path;
}

void TestEmptyOrNonFoundPathProducesNoMovement()
{
	const iggy::Vec2 current { 4.0F, 2.0F };
	iggy::navigation::NavigationPath path;
	iggy::navigation::NavigationPathFollowResult result = iggy::navigation::NavigationPathFollower {}.step(path, {}, current, 1.0F);

	Expect(result.completed, "empty non-found path should complete immediately");
	Expect(!result.moved, "empty non-found path should not move");
	Expect(result.position == current, "empty non-found path should keep current position");

	path.status = iggy::navigation::NavigationPathStatus::NoPath;
	path.waypoints.push_back({ 5.0F, 2.0F });
	result = iggy::navigation::NavigationPathFollower {}.step(path, {}, current, 1.0F);
	Expect(result.completed, "non-found path with waypoints should complete immediately");
	Expect(result.position == current, "non-found path should keep current position");
}

void TestAlreadyAtDestinationCompletes()
{
	iggy::navigation::NavigationPath path;
	path.status = iggy::navigation::NavigationPathStatus::Found;
	path.waypoints.push_back({ 2.5F, 1.5F });
	const iggy::navigation::NavigationPathFollowResult result = iggy::navigation::NavigationPathFollower {}.step(path, {}, { 2.5F, 1.5F }, 1.0F);

	Expect(result.completed, "already at final waypoint should complete");
	Expect(!result.moved, "already at final waypoint should not move");
	Expect(result.waypointIndex == 0, "already completed path should keep final waypoint index");
	Expect(result.position == iggy::Vec2 { 2.5F, 1.5F }, "already completed path should preserve position");
}

void TestPartialStepTowardNextWaypoint()
{
	const iggy::navigation::NavigationPath path = FoundPath();
	const iggy::navigation::NavigationPathFollowState state { 1, false };
	const iggy::navigation::NavigationPathFollowResult result = iggy::navigation::NavigationPathFollower {}.step(path, state, { 0.5F, 0.5F }, 0.25F);

	Expect(result.moved, "partial step should move");
	Expect(!result.completed, "partial step should not complete");
	Expect(result.waypointIndex == 1, "partial step should keep target waypoint index");
	Expect(NearVec(result.position, { 0.75F, 0.5F }), "partial step should advance along segment");
}

void TestExactStepAdvancesWaypoint()
{
	const iggy::navigation::NavigationPath path = FoundPath();
	const iggy::navigation::NavigationPathFollowState state { 1, false };
	const iggy::navigation::NavigationPathFollowResult result = iggy::navigation::NavigationPathFollower {}.step(path, state, { 0.5F, 0.5F }, 1.0F);

	Expect(result.moved, "exact step should move");
	Expect(!result.completed, "exact step to intermediate waypoint should not complete");
	Expect(result.waypointIndex == 2, "exact step should advance to next waypoint");
	Expect(result.position == iggy::Vec2 { 1.5F, 0.5F }, "exact step should land on waypoint");
}

void TestLargeStepCrossesMultipleWaypoints()
{
	const iggy::navigation::NavigationPath path = FoundPath();
	const iggy::navigation::NavigationPathFollowState state { 1, false };
	const iggy::navigation::NavigationPathFollowResult result = iggy::navigation::NavigationPathFollower {}.step(path, state, { 0.5F, 0.5F }, 2.25F);

	Expect(result.moved, "large step should move");
	Expect(!result.completed, "large step that does not reach final waypoint should not complete");
	Expect(result.waypointIndex == 3, "large step should advance across multiple waypoints");
	Expect(NearVec(result.position, { 2.5F, 0.75F }), "large step should spend remaining distance on next segment");
}

void TestFinalResultCompletesAtDestination()
{
	const iggy::navigation::NavigationPath path = FoundPath();
	const iggy::navigation::NavigationPathFollowState state { 1, false };
	const iggy::navigation::NavigationPathFollowResult result = iggy::navigation::NavigationPathFollower {}.step(path, state, { 0.5F, 0.5F }, 4.0F);

	Expect(result.moved, "final step should move");
	Expect(result.completed, "final step should complete at destination");
	Expect(result.waypointIndex == 3, "completed path should report final waypoint index");
	Expect(result.position == iggy::Vec2 { 2.5F, 1.5F }, "completed path should land on destination");
}

} // namespace

int main()
{
	TestEmptyOrNonFoundPathProducesNoMovement();
	TestAlreadyAtDestinationCompletes();
	TestPartialStepTowardNextWaypoint();
	TestExactStepAdvancesWaypoint();
	TestLargeStepCrossesMultipleWaypoints();
	TestFinalResultCompletesAtDestination();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
