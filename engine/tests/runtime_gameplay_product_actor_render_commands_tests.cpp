#include "runtime/RuntimeGameplayProductActorRenderCommands.hpp"

#include <cstdlib>
#include <vector>

#include "support/GeometryAssertions.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameBounds;

const iggy::ResourceId PlayerMaterial { "material:test_player" };
const iggy::ResourceId NpcMaterial { "material:test_npc" };

iggy::NpcActorState2D Actor(
	const char *id,
	iggy::Vec2 position,
	bool present = true)
{
	iggy::NpcActorState2D actor;
	actor.npcId = iggy::ResourceId { id };
	actor.aiProfileId = iggy::ResourceId { "profile:test" };
	actor.position = position;
	actor.present = present;
	return actor;
}

iggy::runtime::RuntimeGameplayState State(
	bool hasPlayer = false,
	iggy::Vec2 playerPosition = {},
	std::vector<iggy::NpcActorState2D> actors = {})
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.hasPlayer = hasPlayer;
	state.session.player.position = playerPosition;
	state.npcActors.actors = actors;
	return state;
}

iggy::runtime::RuntimeGameplayProductActorRenderCommandConfig Config()
{
	iggy::runtime::RuntimeGameplayProductActorRenderCommandConfig config;
	config.playerMaterialId = PlayerMaterial;
	config.npcActorMaterialId = NpcMaterial;
	config.playerLayer = 4;
	config.npcActorLayer = 5;
	return config;
}

void ExpectCommand(
	const iggy::render::RenderCommand2D &command,
	iggy::Aabb2 bounds,
	const iggy::ResourceId &materialId,
	int layer,
	std::size_t order,
	const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad &&
			SameBounds(command.worldBounds, bounds) &&
			command.materialId == materialId &&
			command.layer == layer &&
			command.order == order &&
			command.texture.textureId.empty() &&
			!command.texture.hasSourceRect,
		message);
}

bool SameState(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected)
{
	if (actual.session.hasPlayer != expected.session.hasPlayer ||
			!NearVec(actual.session.player.position, expected.session.player.position) ||
			actual.npcActors.actors.size() != expected.npcActors.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.npcActors.actors.size(); ++index) {
		const iggy::NpcActorState2D &actualActor = actual.npcActors.actors[index];
		const iggy::NpcActorState2D &expectedActor = expected.npcActors.actors[index];
		if (actualActor.npcId != expectedActor.npcId ||
				actualActor.present != expectedActor.present ||
				!NearVec(actualActor.position, expectedActor.position))
			return false;
	}
	return true;
}

void TestNoPlayerEmitsNoPlayerCommand()
{
	const iggy::runtime::RuntimeGameplayState state = State(false);

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult result =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			Config());

	Expect(!result.emittedPlayer, "no-player state should not emit player command");
	Expect(result.commandCount == 0 && result.commands.commands.empty(),
		"no-player state without NPC actors should emit no commands");
}

void TestPlayerEmitsConfiguredQuad()
{
	iggy::runtime::RuntimeGameplayProductActorRenderCommandConfig config =
		Config();
	config.playerSize = { 2.0F, 4.0F };
	config.playerAnchor = { 0.25F, 0.75F };
	config.playerLayer = 9;
	const iggy::runtime::RuntimeGameplayState state =
		State(true, { 10.0F, 20.0F });

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult result =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			config);

	Expect(result.emittedPlayer && result.commandCount == 1,
		"player state should emit one player command");
	if (result.commands.commands.size() == 1)
		ExpectCommand(
			result.commands.commands[0],
			{ { 9.5F, 17.0F }, { 11.5F, 21.0F } },
			PlayerMaterial,
			9,
			0,
			"player command should use configured bounds/material/layer");
}

void TestAbsentNpcActorsAreSkipped()
{
	const iggy::runtime::RuntimeGameplayState state = State(
		false,
		{},
		{
			Actor("npc:absent", { 1.0F, 1.0F }, false),
			Actor("npc:present", { 3.0F, 1.0F }, true),
		});

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult result =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			Config());

	Expect(result.emittedNpcActorCount == 1 && result.commandCount == 1,
		"only present NPC actors should emit commands");
	if (result.commands.commands.size() == 1)
		ExpectCommand(
			result.commands.commands[0],
			{ { 2.5F, 0.5F }, { 3.5F, 1.5F } },
			NpcMaterial,
			5,
			0,
			"present NPC actor should use centered default bounds");
}

void TestPresentNpcActorsPreserveRegistryOrder()
{
	const iggy::runtime::RuntimeGameplayState state = State(
		false,
		{},
		{
			Actor("npc:first", { 0.0F, 0.0F }),
			Actor("npc:second", { 4.0F, 0.0F }),
			Actor("npc:third", { 8.0F, 0.0F }),
		});

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult result =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			Config());

	Expect(result.emittedNpcActorCount == 3 && result.commandCount == 3,
		"three present NPC actors should emit three commands");
	if (result.commands.commands.size() == 3) {
		ExpectCommand(result.commands.commands[0], { { -0.5F, -0.5F }, { 0.5F, 0.5F } }, NpcMaterial, 5, 0, "first NPC actor should emit first command");
		ExpectCommand(result.commands.commands[1], { { 3.5F, -0.5F }, { 4.5F, 0.5F } }, NpcMaterial, 5, 1, "second NPC actor should emit second command");
		ExpectCommand(result.commands.commands[2], { { 7.5F, -0.5F }, { 8.5F, 0.5F } }, NpcMaterial, 5, 2, "third NPC actor should emit third command");
	}
}

void TestPlayerThenNpcCommandOrderIsStable()
{
	const iggy::runtime::RuntimeGameplayState state = State(
		true,
		{ 1.0F, 1.0F },
		{ Actor("npc:one", { 3.0F, 1.0F }) });

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult result =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			Config());

	Expect(result.emittedPlayer && result.emittedNpcActorCount == 1 &&
			result.commandCount == 2,
		"player plus NPC actor should emit two commands");
	if (result.commands.commands.size() == 2) {
		ExpectCommand(result.commands.commands[0], { { 0.5F, 0.5F }, { 1.5F, 1.5F } }, PlayerMaterial, 4, 0, "player command should precede NPC actor commands");
		ExpectCommand(result.commands.commands[1], { { 2.5F, 0.5F }, { 3.5F, 1.5F } }, NpcMaterial, 5, 1, "NPC actor command should follow player command");
	}
}

void TestIncludeFlagsDisableProjection()
{
	iggy::runtime::RuntimeGameplayProductActorRenderCommandConfig config =
		Config();
	config.includePlayer = false;
	const iggy::runtime::RuntimeGameplayState state = State(
		true,
		{ 1.0F, 1.0F },
		{ Actor("npc:one", { 3.0F, 1.0F }) });

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult noPlayer =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			config);
	config.includeNpcActors = false;
	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult none =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			config);

	Expect(!noPlayer.emittedPlayer && noPlayer.emittedNpcActorCount == 1 &&
			noPlayer.commandCount == 1,
		"includePlayer=false should leave NPC actor projection enabled");
	Expect(none.commandCount == 0 && none.commands.commands.empty(),
		"disabling player and NPC actor projection should emit no commands");
}

void TestNegativeSizeIsNormalizedAndEmptyMaterialPreserved()
{
	iggy::runtime::RuntimeGameplayProductActorRenderCommandConfig config =
		Config();
	config.playerMaterialId = {};
	config.playerSize = { -2.0F, -4.0F };
	config.playerAnchor = { 0.5F, 0.5F };
	const iggy::runtime::RuntimeGameplayState state =
		State(true, { 0.0F, 0.0F });

	const iggy::runtime::RuntimeGameplayProductActorRenderCommandResult result =
		iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
			state,
			config);

	Expect(result.commandCount == 1,
		"negative-size player should still emit one command");
	if (result.commands.commands.size() == 1) {
		ExpectCommand(
			result.commands.commands[0],
			{ { -1.0F, -2.0F }, { 1.0F, 2.0F } },
			{},
			4,
			0,
			"negative size should be normalized and empty material preserved");
		Expect(result.commands.commands[0].materialId.empty(),
			"empty material id should be preserved");
	}
}

void TestInputStateIsNotMutated()
{
	const iggy::runtime::RuntimeGameplayState state = State(
		true,
		{ 1.0F, 2.0F },
		{
			Actor("npc:one", { 3.0F, 4.0F }),
			Actor("npc:absent", { 5.0F, 6.0F }, false),
		});
	const iggy::runtime::RuntimeGameplayState before = state;

	(void) iggy::runtime::RuntimeGameplayProductActorRenderCommands {}.build(
		state,
		Config());

	Expect(SameState(state, before),
		"actor render projection should not mutate runtime gameplay state");
}

} // namespace

int main()
{
	TestNoPlayerEmitsNoPlayerCommand();
	TestPlayerEmitsConfiguredQuad();
	TestAbsentNpcActorsAreSkipped();
	TestPresentNpcActorsPreserveRegistryOrder();
	TestPlayerThenNpcCommandOrderIsStable();
	TestIncludeFlagsDisableProjection();
	TestNegativeSizeIsNormalizedAndEmptyMaterialPreserved();
	TestInputStateIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
