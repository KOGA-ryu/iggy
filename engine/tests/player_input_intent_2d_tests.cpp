#include <cstdlib>

#include "runtime/GameplayCommand2D.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId TargetId { "target:door" };

void TestDefaultIntentIsNoneWithDefaultPayloads()
{
	const iggy::PlayerInputIntent2D intent;

	Expect(intent.type == iggy::PlayerInputIntent2DType::None, "default player input intent should be None");
	Expect(NearVec(intent.worldPoint, { 0.0F, 0.0F }), "default player input intent should have default world point");
	Expect(intent.tile == iggy::TileCoord { -1, -1 }, "default player input intent should have default tile");
	Expect(intent.targetId.empty(), "default player input intent should have empty target id");
	Expect(iggy::validate(intent) == iggy::PlayerInputIntent2DStatus::Valid, "default player input intent should validate");
	Expect(iggy::valid(intent), "default player input intent should be valid");
}

void TestMoveToPointIntentPreservesWorldPoint()
{
	const iggy::Vec2 point { -3.5F, 9.25F };
	const iggy::PlayerInputIntent2D intent = iggy::playerMoveToPointIntent(point);

	Expect(intent.type == iggy::PlayerInputIntent2DType::MoveToPoint, "move-to-point intent should have MoveToPoint type");
	Expect(NearVec(intent.worldPoint, point), "move-to-point intent should preserve world point");
	Expect(intent.tile == iggy::TileCoord { -1, -1 }, "move-to-point intent should leave tile defaulted");
	Expect(intent.targetId.empty(), "move-to-point intent should leave target id empty");
	Expect(iggy::valid(intent), "move-to-point intent should validate");
}

void TestMoveToTileIntentPreservesTileIncludingNegativeCoords()
{
	const iggy::TileCoord tile { -4, 7 };
	const iggy::PlayerInputIntent2D intent = iggy::playerMoveToTileIntent(tile);

	Expect(intent.type == iggy::PlayerInputIntent2DType::MoveToTile, "move-to-tile intent should have MoveToTile type");
	Expect(intent.tile == tile, "move-to-tile intent should preserve tile including negative coordinates");
	Expect(NearVec(intent.worldPoint, { 0.0F, 0.0F }), "move-to-tile intent should leave world point defaulted");
	Expect(intent.targetId.empty(), "move-to-tile intent should leave target id empty");
	Expect(iggy::valid(intent), "move-to-tile intent should validate");
}

void TestInteractIntentRequiresTargetId()
{
	const iggy::PlayerInputIntent2D validIntent = iggy::playerInteractIntent(TargetId);
	const iggy::PlayerInputIntent2D invalidIntent = iggy::playerInteractIntent({});

	Expect(validIntent.type == iggy::PlayerInputIntent2DType::Interact, "interact intent should have Interact type");
	Expect(validIntent.targetId == TargetId, "interact intent should preserve target id");
	Expect(iggy::validate(validIntent) == iggy::PlayerInputIntent2DStatus::Valid, "interact intent with target should validate");
	Expect(iggy::valid(validIntent), "interact intent with target should be valid");
	Expect(iggy::validate(invalidIntent) == iggy::PlayerInputIntent2DStatus::MissingTarget, "interact intent without target should report MissingTarget");
	Expect(!iggy::valid(invalidIntent), "interact intent without target should not be valid");
}

void TestInspectIntentRequiresTargetId()
{
	const iggy::PlayerInputIntent2D validIntent = iggy::playerInspectIntent(TargetId);
	const iggy::PlayerInputIntent2D invalidIntent = iggy::playerInspectIntent({});

	Expect(validIntent.type == iggy::PlayerInputIntent2DType::Inspect, "inspect intent should have Inspect type");
	Expect(validIntent.targetId == TargetId, "inspect intent should preserve target id");
	Expect(iggy::validate(validIntent) == iggy::PlayerInputIntent2DStatus::Valid, "inspect intent with target should validate");
	Expect(iggy::valid(validIntent), "inspect intent with target should be valid");
	Expect(iggy::validate(invalidIntent) == iggy::PlayerInputIntent2DStatus::MissingTarget, "inspect intent without target should report MissingTarget");
	Expect(!iggy::valid(invalidIntent), "inspect intent without target should not be valid");
}

void TestWaitAndCancelIntentAreValidWithoutPayload()
{
	const iggy::PlayerInputIntent2D wait = iggy::playerWaitIntent();
	const iggy::PlayerInputIntent2D cancel = iggy::playerCancelIntent();

	Expect(wait.type == iggy::PlayerInputIntent2DType::Wait, "wait intent should have Wait type");
	Expect(wait.targetId.empty() && wait.tile == iggy::TileCoord { -1, -1 }, "wait intent should carry no required payload");
	Expect(iggy::valid(wait), "wait intent should validate");
	Expect(cancel.type == iggy::PlayerInputIntent2DType::Cancel, "cancel intent should have Cancel type");
	Expect(cancel.targetId.empty() && cancel.tile == iggy::TileCoord { -1, -1 }, "cancel intent should carry no required payload");
	Expect(iggy::valid(cancel), "cancel intent should validate");
}

void TestIntentConstructionDoesNotMutatePlayerOrRuntimeCommandState()
{
	iggy::PlayerAgentState player = PlayerAgent(
		iggy::ResourceId("player:intent"),
		{ 1.0F, 2.0F },
		{ 1, 2 },
		iggy::PlayerMovementStatus::Moving,
		iggy::PlayerFacing2D::South);
	const iggy::PlayerAgentState playerBefore = player;
	iggy::runtime::GameplayCommandFrame2D frame;
	frame.commands.push_back(iggy::runtime::GameplayCommand2DFactory {}.wait(iggy::ResourceId("player:intent")));
	const iggy::runtime::GameplayCommandFrame2D frameBefore = frame;

	const iggy::PlayerInputIntent2D intent = iggy::playerInteractIntent(TargetId);

	Expect(iggy::valid(intent), "intent immutability setup should create valid intent");
	ExpectPlayerAgent(player, playerBefore, "player input intent construction");
	Expect(frame.commands.size() == frameBefore.commands.size(), "player input intent construction should not mutate runtime command frame count");
	if (frame.commands.size() == frameBefore.commands.size()) {
		Expect(frame.commands[0].type == frameBefore.commands[0].type, "player input intent construction should not mutate runtime command type");
		Expect(frame.commands[0].actorId == frameBefore.commands[0].actorId, "player input intent construction should not mutate runtime command actor");
	}
}

} // namespace

int main()
{
	TestDefaultIntentIsNoneWithDefaultPayloads();
	TestMoveToPointIntentPreservesWorldPoint();
	TestMoveToTileIntentPreservesTileIncludingNegativeCoords();
	TestInteractIntentRequiresTargetId();
	TestInspectIntentRequiresTargetId();
	TestWaitAndCancelIntentAreValidWithoutPayload();
	TestIntentConstructionDoesNotMutatePlayerOrRuntimeCommandState();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
