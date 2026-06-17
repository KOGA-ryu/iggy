#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"

#include "runtime/RuntimeInventoryState.hpp"
#include "scene/player/PlayerAgentState.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"

namespace iggy::runtime {
namespace {

struct AuthoredFrameGroup {
	ResourceId frameId;
	bool hasDeclarationIndex = false;
	std::size_t declarationIndex = 0;
	std::vector<NpcActorControlState2D> controls;
	std::vector<PlayerInputIntent2D> playerIntents;
};

void AddIssue(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid:
		++result.sourcePlanIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnsupportedTerrainPromotion:
		++result.unsupportedTerrainPromotionCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::MissingFrameDefaults:
		++result.missingFrameDefaultsCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ActorRegistryInvalid:
		++result.actorRegistryIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ControlRegistryInvalid:
		++result.controlRegistryIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnknownAuthoredControlActor:
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicateAuthoredControlActor:
		++result.authoredControlIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		AmbiguousAuthoredPlayerCommandFrame:
		++result.authoredPlayerCommandIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicatePlayerStart:
		++result.playerStartIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		ProfileTraitCatalogInvalid:
		++result.profileTraitCatalogIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		InteractionTargetRegistryInvalid:
		++result.interactionTargetRegistryIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		InteractionEffectCatalogInvalid:
		++result.interactionEffectCatalogIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		ItemDropRegistryInvalid:
		++result.itemDropRegistryIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::AiMapPromotionInvalid:
		++result.aiMapPromotionIssueCount;
		break;
	case RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid:
		++result.profileScenarioIssueCount;
		break;
	}
	result.issues.push_back(issue);
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue SourceIssue(
	const RuntimeGameplayAsciiSourcePlanIssue &sourceIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::SourcePlanInvalid;
	issue.sourceIssue = sourceIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue UnsupportedTerrainIssue(
	char glyph,
	std::size_t row,
	std::size_t column)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnsupportedTerrainPromotion;
	issue.glyph = glyph;
	issue.row = row;
	issue.column = column;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue MissingFrameDefaultsIssue()
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::MissingFrameDefaults;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue ActorIssue(
	const NpcActorState2DIssue &actorIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ActorRegistryInvalid;
	issue.actorIssue = actorIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue ControlIssue(
	const NpcActorControlState2DIssue &controlIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ControlRegistryInvalid;
	issue.controlIssue = controlIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue AuthoredControlIssue(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code,
	const RuntimeGameplayAsciiSourcePlanAuthoredControl &control)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = code;
	issue.authoredControl = control;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue AuthoredPlayerCommandIssue(
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode code,
	const RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand &command)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = code;
	issue.authoredPlayerCommand = command;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue ProfileIssue(
	const RuntimeGameplayProfileScenarioIssue &profileIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::ProfileScenarioInvalid;
	issue.profileIssue = profileIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue AiMapPromotionIssue(
	const RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue &aiMapPromotionIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::AiMapPromotionInvalid;
	issue.aiMapPromotionIssue = aiMapPromotionIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue DuplicatePlayerStartIssue(
	std::size_t row,
	std::size_t column,
	char glyph)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code =
		RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicatePlayerStart;
	issue.row = row;
	issue.column = column;
	issue.glyph = glyph;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue ProfileTraitIssue(
	const NpcAiProfileTraitCatalogIssue &profileTraitIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		ProfileTraitCatalogInvalid;
	issue.profileTraitIssue = profileTraitIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue InteractionTargetIssue(
	const InteractionTarget2DRegistryIssue &targetIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		InteractionTargetRegistryInvalid;
	issue.interactionTargetIssue = targetIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue InteractionEffectIssue(
	const InteractionEffectCatalog2DIssue &effectIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		InteractionEffectCatalogInvalid;
	issue.interactionEffectIssue = effectIssue;
	return issue;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue ItemDropIssue(
	const LevelItemDrop2DIssue &dropIssue)
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssue issue;
	issue.code = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
		ItemDropRegistryInvalid;
	issue.itemDropIssue = dropIssue;
	return issue;
}

const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *FindLegend(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	char glyph)
{
	for (const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &entry : sourcePlan.legend) {
		if (entry.glyph == glyph)
			return &entry;
	}
	return nullptr;
}

const RuntimeGameplayAsciiSourcePlanTerrainPromotion *FindTerrainPromotion(
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	char glyph)
{
	for (const RuntimeGameplayAsciiSourcePlanTerrainPromotion &entry : config.terrain) {
		if (entry.glyph == glyph)
			return &entry;
	}
	return nullptr;
}

bool IsBuiltInFloorGlyph(const RuntimeGameplayAsciiSourcePlan &sourcePlan, char glyph)
{
	return glyph == '.' || glyph == ' ' || glyph == sourcePlan.grid.backgroundGlyph;
}

bool PromoteTile(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	char glyph,
	bool &walkable)
{
	if (glyph == '#') {
		walkable = false;
		return true;
	}
	if (IsBuiltInFloorGlyph(sourcePlan, glyph)) {
		walkable = true;
		return true;
	}

	const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend = FindLegend(sourcePlan, glyph);
	if (legend == nullptr) {
		return false;
	}

	if (legend->kind != RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain) {
		walkable = true;
		return true;
	}

	const RuntimeGameplayAsciiSourcePlanTerrainPromotion *promotion =
		FindTerrainPromotion(config, glyph);
	if (promotion == nullptr) {
		return false;
	}

	walkable = promotion->walkable;
	return true;
}

bool PromoteTerrain(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result)
{
	LevelTileMap map;
	map.id = sourcePlan.sourceId;
	map.width = static_cast<int>(sourcePlan.grid.width);
	map.height = static_cast<int>(sourcePlan.grid.height);
	map.tiles.reserve(sourcePlan.grid.width * sourcePlan.grid.height);

	for (std::size_t row = 0; row < sourcePlan.grid.rows.size(); ++row) {
		const std::string &line = sourcePlan.grid.rows[row];
		for (std::size_t column = 0; column < line.size(); ++column) {
			bool walkable = true;
			if (!PromoteTile(sourcePlan, config, line[column], walkable)) {
				AddIssue(result, UnsupportedTerrainIssue(line[column], row, column));
				continue;
			}
			map.tiles.push_back({ walkable });
		}
	}

	result.promotedMap = map;
	return result.unsupportedTerrainPromotionCount == 0;
}

bool IsActorCell(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell)
{
	const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend = FindLegend(sourcePlan, cell.glyph);
	if (legend == nullptr)
		return false;
	return legend->kind == RuntimeGameplayAsciiSourcePlanGlyphKind::Actor
		|| legend->scenarioMarkerKind == RuntimeGameplayAsciiScenarioMarkerKind::Actor;
}

bool IsPlayerStartLegend(const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry &legend)
{
	return legend.kind == RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart
		|| legend.scenarioMarkerKind == RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart;
}

bool IsPlayerStartGlyph(const RuntimeGameplayAsciiSourcePlan &sourcePlan, char glyph)
{
	const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend = FindLegend(sourcePlan, glyph);
	return legend != nullptr && IsPlayerStartLegend(*legend);
}

Vec2 ActorPosition(const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell)
{
	if (cell.localPosition.present) {
		return {
			static_cast<float>(cell.localPosition.x),
			static_cast<float>(cell.localPosition.y),
		};
	}
	return {
		static_cast<float>(cell.column) + 0.5F,
		static_cast<float>(cell.row) + 0.5F,
	};
}

NpcActorState2D ActorFromCell(
	const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config)
{
	NpcActorState2D actor;
	actor.npcId = cell.markerId;
	actor.aiProfileId = cell.profileId;
	actor.factionId = config.defaultFactionId;
	actor.position = ActorPosition(cell);
	actor.currentGoalId = config.defaultGoalId;
	actor.present = true;
	return actor;
}

TileCoord PlayerStartTile(const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell)
{
	if (cell.localTile.present) {
		return { cell.localTile.x, cell.localTile.y };
	}
	return {
		static_cast<int>(cell.column),
		static_cast<int>(cell.row),
	};
}

ResourceId PlayerIdFromLegendOrCell(
	const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend,
	const RuntimeGameplayAsciiSourcePlanAnnotatedCell *cell)
{
	if (cell != nullptr && !cell->markerId.empty()) {
		return cell->markerId;
	}
	if (legend != nullptr && !legend->targetMarkerId.empty()) {
		return legend->targetMarkerId;
	}
	return ResourceId("player:source-plan");
}

PlayerAgentState PlayerFromStart(
	const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend,
	const RuntimeGameplayAsciiSourcePlanAnnotatedCell *cell,
	TileCoord tile)
{
	PlayerAgentState player;
	player.id = PlayerIdFromLegendOrCell(legend, cell);
	player.position = tileCenter(tile);
	player.spawnTile = tile;
	player.movementStatus = PlayerMovementStatus::Idle;
	player.facing = PlayerFacing2D::South;
	return player;
}

NpcActorControlState2D DefaultControlForActor(
	const ResourceId &npcId,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config)
{
	NpcActorControlState2D control;
	if (config.hasDefaultControl) {
		control = config.defaultControl;
	} else {
		control.objective = waitNpcObjective();
		control.behavior = waitingNpcBehaviorState();
		control.moveMode = NpcMoveMode::Still;
	}
	control.npcId = npcId;
	return control;
}

bool ActorExists(const std::vector<NpcActorState2D> &actors, const ResourceId &npcId)
{
	for (const NpcActorState2D &actor : actors) {
		if (actor.npcId == npcId)
			return true;
	}
	return false;
}

NpcMoveMode ConvertMoveMode(RuntimeGameplayAsciiSourcePlanControlMoveMode mode)
{
	switch (mode) {
	case RuntimeGameplayAsciiSourcePlanControlMoveMode::Still:
		return NpcMoveMode::Still;
	case RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk:
		return NpcMoveMode::Walk;
	case RuntimeGameplayAsciiSourcePlanControlMoveMode::Jog:
		return NpcMoveMode::Jog;
	case RuntimeGameplayAsciiSourcePlanControlMoveMode::Run:
		return NpcMoveMode::Run;
	case RuntimeGameplayAsciiSourcePlanControlMoveMode::Sprint:
		return NpcMoveMode::Sprint;
	case RuntimeGameplayAsciiSourcePlanControlMoveMode::Unknown:
		return NpcMoveMode::None;
	}
	return NpcMoveMode::None;
}

Vec2 TargetPosition(const RuntimeGameplayAsciiSourcePlanAuthoredControl &control)
{
	return {
		static_cast<float>(control.targetPosition.x),
		static_cast<float>(control.targetPosition.y),
	};
}

NpcActorControlState2D ControlFromAuthoredControl(
	const RuntimeGameplayAsciiSourcePlanAuthoredControl &authored)
{
	NpcActorControlState2D control;
	control.npcId = authored.npcId;
	control.moveMode = ConvertMoveMode(authored.moveMode);
	switch (authored.behavior) {
	case RuntimeGameplayAsciiSourcePlanControlBehavior::Waiting:
		control.objective = waitNpcObjective();
		control.behavior = waitingNpcBehaviorState();
		return control;
	case RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking:
		control.objective = moveToNpcObjective(TargetPosition(authored));
		control.behavior = seekingNpcBehaviorState(TargetPosition(authored));
		return control;
	case RuntimeGameplayAsciiSourcePlanControlBehavior::Unknown:
		control.objective = noneNpcObjective();
		control.behavior = noneNpcBehaviorState();
		return control;
	}
	control.objective = noneNpcObjective();
	control.behavior = noneNpcBehaviorState();
	return control;
}

bool ReplaceControl(
	std::vector<NpcActorControlState2D> &controls,
	const NpcActorControlState2D &replacement)
{
	for (NpcActorControlState2D &control : controls) {
		if (control.npcId == replacement.npcId) {
			control = replacement;
			return true;
		}
	}
	return false;
}

AuthoredFrameGroup &FindOrCreateFrameGroup(
	std::vector<AuthoredFrameGroup> &frameGroups,
	const ResourceId &frameId,
	bool hasDeclarationIndex,
	std::size_t declarationIndex)
{
	for (AuthoredFrameGroup &candidate : frameGroups) {
		if (candidate.frameId == frameId) {
			return candidate;
		}
	}

	AuthoredFrameGroup group;
	group.frameId = frameId;
	group.hasDeclarationIndex = hasDeclarationIndex;
	group.declarationIndex = declarationIndex;

	if (!hasDeclarationIndex) {
		frameGroups.push_back(group);
		return frameGroups.back();
	}

	for (auto iterator = frameGroups.begin(); iterator != frameGroups.end();
		++iterator) {
		if (iterator->hasDeclarationIndex &&
			declarationIndex < iterator->declarationIndex) {
			return *frameGroups.insert(iterator, group);
		}
	}

	frameGroups.push_back(group);
	return frameGroups.back();
}

bool ApplyAuthoredControls(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const std::vector<NpcActorState2D> &actors,
	std::vector<NpcActorControlState2D> &controls,
	std::vector<AuthoredFrameGroup> &frameGroups,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result)
{
	result.authoredControlCount = sourcePlan.authoredControls.size();
	std::vector<ResourceId> controlledActors;
	for (const RuntimeGameplayAsciiSourcePlanAuthoredControl &authored : sourcePlan.authoredControls) {
		if (!ActorExists(actors, authored.npcId)) {
			AddIssue(
				result,
				AuthoredControlIssue(
					RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::UnknownAuthoredControlActor,
					authored));
			continue;
		}

		if (authored.hasFrameId) {
			AuthoredFrameGroup &group = FindOrCreateFrameGroup(
				frameGroups,
				authored.frameId,
				authored.hasDeclarationIndex,
				authored.declarationIndex);

			bool duplicate = false;
			for (const NpcActorControlState2D &control : group.controls) {
				if (control.npcId == authored.npcId) {
					duplicate = true;
					break;
				}
			}
			if (duplicate) {
				AddIssue(
					result,
					AuthoredControlIssue(
						RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicateAuthoredControlActor,
						authored));
					continue;
			}

			group.controls.push_back(ControlFromAuthoredControl(authored));
			continue;
		}

		bool duplicate = false;
		for (const ResourceId &controlledActor : controlledActors) {
			if (controlledActor == authored.npcId) {
				duplicate = true;
				break;
			}
		}
		if (duplicate) {
			AddIssue(
				result,
				AuthoredControlIssue(
					RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::DuplicateAuthoredControlActor,
					authored));
			continue;
		}

		controlledActors.push_back(authored.npcId);
		ReplaceControl(controls, ControlFromAuthoredControl(authored));
	}

	return result.authoredControlIssueCount == 0;
}

PlayerInputIntent2D PlayerIntentFromAuthoredCommand(
	const RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand &command)
{
	switch (command.command) {
	case RuntimeGameplayAsciiSourcePlanPlayerCommandKind::MoveToTile:
		return playerMoveToTileIntent(command.targetTile);
	case RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Interact:
	case RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Pickup:
		return playerInteractIntent(command.targetId);
	case RuntimeGameplayAsciiSourcePlanPlayerCommandKind::Unknown:
		return {};
	}
	return {};
}

bool SourceHasFrameIdAuthoredFrames(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan)
{
	for (const RuntimeGameplayAsciiSourcePlanAuthoredControl &control :
		sourcePlan.authoredControls) {
		if (control.hasFrameId) {
			return true;
		}
	}
	for (const RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand &command :
		sourcePlan.authoredPlayerCommands) {
		if (command.hasFrameId) {
			return true;
		}
	}
	return false;
}

bool ApplyAuthoredPlayerCommands(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	std::vector<PlayerInputIntent2D> &defaultPlayerIntents,
	std::vector<AuthoredFrameGroup> &frameGroups,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result)
{
	result.authoredPlayerCommandCount =
		sourcePlan.authoredPlayerCommands.size();
	const bool hasFrameIdAuthoredFrames = SourceHasFrameIdAuthoredFrames(sourcePlan);
	for (const RuntimeGameplayAsciiSourcePlanAuthoredPlayerCommand &command :
		sourcePlan.authoredPlayerCommands) {
		if (command.hasFrameId) {
			AuthoredFrameGroup &group = FindOrCreateFrameGroup(
				frameGroups,
				command.frameId,
				command.hasDeclarationIndex,
				command.declarationIndex);
			group.playerIntents.push_back(PlayerIntentFromAuthoredCommand(command));
			continue;
		}

		if (hasFrameIdAuthoredFrames) {
			AddIssue(
				result,
				AuthoredPlayerCommandIssue(
					RuntimeGameplayAsciiSourcePlanProfileScenarioConversionIssueCode::
						AmbiguousAuthoredPlayerCommandFrame,
					command));
			continue;
		}

		defaultPlayerIntents.push_back(PlayerIntentFromAuthoredCommand(command));
	}

	return result.authoredPlayerCommandIssueCount == 0;
}

bool PromoteActorsAndControls(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	std::vector<AuthoredFrameGroup> &frameGroups,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result)
{
	std::vector<NpcActorState2D> actors;
	std::vector<NpcActorControlState2D> controls;
	for (const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell : sourcePlan.annotatedCells) {
		if (!IsActorCell(sourcePlan, cell))
			continue;
		const NpcActorState2D actor = ActorFromCell(cell, config);
		actors.push_back(actor);
		controls.push_back(DefaultControlForActor(actor.npcId, config));
	}

	if (!ApplyAuthoredControls(sourcePlan, actors, controls, frameGroups, result))
		return false;

	result.actorRegistry = NpcActorState2DRegistryBuilder {}.build(actors);
	if (!result.actorRegistry.built) {
		for (const NpcActorState2DIssue &issue : result.actorRegistry.issues) {
			AddIssue(result, ActorIssue(issue));
		}
	}

	result.controlRegistry = NpcActorControlState2DRegistryBuilder {}.build(controls);
	if (!result.controlRegistry.built) {
		for (const NpcActorControlState2DIssue &issue : result.controlRegistry.issues) {
			AddIssue(result, ControlIssue(issue));
		}
	}

	if (!result.actorRegistry.built || !result.controlRegistry.built)
		return false;

	result.promotedActorCount = result.actorRegistry.registry.actors.size();
	result.defaultControlCount = result.controlRegistry.registry.entries.size();
	return true;
}

InteractionTarget2DKind ConvertInteractionTargetKind(
	RuntimeGameplayAsciiSourcePlanInteractionTargetKind kind)
{
	switch (kind) {
	case RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Inspectable:
		return InteractionTarget2DKind::Inspectable;
	case RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Usable:
		return InteractionTarget2DKind::Usable;
	case RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Pickup:
		return InteractionTarget2DKind::Pickup;
	case RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Talk:
		return InteractionTarget2DKind::Talk;
	case RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Door:
		return InteractionTarget2DKind::Door;
	case RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Unknown:
		return InteractionTarget2DKind::Unknown;
	}
	return InteractionTarget2DKind::Unknown;
}

Vec2 InteractionTargetPosition(
	const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &authored)
{
	if (authored.localPosition.present) {
		return {
			static_cast<float>(authored.localPosition.x),
			static_cast<float>(authored.localPosition.y),
		};
	}
	return tileCenter({ authored.localTile.x, authored.localTile.y });
}

InteractionTarget2D InteractionTargetFromAuthoredTarget(
	const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &authored)
{
	return {
		authored.targetId,
		ConvertInteractionTargetKind(authored.kind),
		InteractionTargetPosition(authored),
		static_cast<float>(authored.radius),
		authored.enabled,
	};
}

ResourceId EffectTargetId(
	const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &authored)
{
	return authored.effectTargetId.empty()
		? authored.targetId
		: authored.effectTargetId;
}

bool EffectFromAuthoredTarget(
	const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &authored,
	InteractionEffect2D &effect)
{
	switch (authored.effect) {
	case RuntimeGameplayAsciiSourcePlanInteractionEffectKind::None:
		return false;
	case RuntimeGameplayAsciiSourcePlanInteractionEffectKind::InspectText:
		effect = inspectTextInteractionEffect(EffectTargetId(authored), authored.text);
		return true;
	case RuntimeGameplayAsciiSourcePlanInteractionEffectKind::ToggleTarget:
		effect = toggleTargetInteractionEffect(
			EffectTargetId(authored),
			authored.enabledValue);
		return true;
	case RuntimeGameplayAsciiSourcePlanInteractionEffectKind::EmitEvent:
		effect = emitInteractionEventEffect(EffectTargetId(authored), authored.eventId);
		return true;
	case RuntimeGameplayAsciiSourcePlanInteractionEffectKind::PickupItem:
		effect = pickupItemInteractionEffect(EffectTargetId(authored), authored.dropId);
		return true;
	case RuntimeGameplayAsciiSourcePlanInteractionEffectKind::Unknown:
		return false;
	}
	return false;
}

bool PromoteInteractionState(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	RuntimeInteractionState &interaction)
{
	std::vector<InteractionTarget2D> targets;
	std::vector<InteractionEffectEntry2D> effectEntries;
	targets.reserve(sourcePlan.authoredInteractionTargets.size());
	effectEntries.reserve(sourcePlan.authoredInteractionTargets.size());

	for (const RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget &authored :
		sourcePlan.authoredInteractionTargets) {
		targets.push_back(InteractionTargetFromAuthoredTarget(authored));

		InteractionEffect2D effect;
		if (EffectFromAuthoredTarget(authored, effect)) {
			effectEntries.push_back({ authored.targetId, { effect } });
		}
	}

	result.interactionTargetRegistry =
		InteractionTarget2DRegistryBuilder {}.build(targets);
	if (!result.interactionTargetRegistry.built) {
		for (const InteractionTarget2DRegistryIssue &issue :
			result.interactionTargetRegistry.issues) {
			AddIssue(result, InteractionTargetIssue(issue));
		}
		return false;
	}

	interaction.targets = result.interactionTargetRegistry.registry;

	if (!effectEntries.empty()) {
		result.interactionEffectCatalog =
			InteractionEffectCatalog2DBuilder {}.build(effectEntries);
		if (!result.interactionEffectCatalog.built) {
			for (const InteractionEffectCatalog2DIssue &issue :
				result.interactionEffectCatalog.issues) {
				AddIssue(result, InteractionEffectIssue(issue));
			}
			return false;
		}
		interaction.effects = result.interactionEffectCatalog.catalog;
	}

	result.promotedInteractionTargetCount = interaction.targets.targets().size();
	result.promotedInteractionEffectEntryCount = interaction.effects.entries().size();
	return true;
}

Vec2 ItemDropPosition(
	const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &authored)
{
	if (authored.localPosition.present) {
		return {
			static_cast<float>(authored.localPosition.x),
			static_cast<float>(authored.localPosition.y),
		};
	}
	return tileCenter({ authored.localTile.x, authored.localTile.y });
}

LevelItemDrop2D ItemDropFromAuthoredDrop(
	const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &authored)
{
	return {
		authored.dropId,
		authored.itemId,
		authored.count,
		ItemDropPosition(authored),
		static_cast<float>(authored.pickupRadius),
		authored.enabled,
	};
}

bool PromoteInventoryState(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	RuntimeInventoryState &inventory)
{
	std::vector<LevelItemDrop2D> drops;
	drops.reserve(sourcePlan.authoredItemDrops.size());
	for (const RuntimeGameplayAsciiSourcePlanAuthoredItemDrop &authored :
		sourcePlan.authoredItemDrops) {
		drops.push_back(ItemDropFromAuthoredDrop(authored));
	}

	result.itemDropRegistry = LevelItemDrop2DRegistryBuilder {}.build(drops);
	if (!result.itemDropRegistry.built) {
		for (const LevelItemDrop2DIssue &issue : result.itemDropRegistry.issues) {
			AddIssue(result, ItemDropIssue(issue));
		}
		return false;
	}

	inventory.drops = result.itemDropRegistry.registry;
	result.promotedItemDropCount = inventory.drops.drops.size();
	return true;
}

bool PromoteRegionAiMap(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result)
{
	if (!config.promoteRegionAiMap) {
		return true;
	}

	result.regionAiMapPromotion =
		RuntimeGameplayAsciiSourcePlanRegionAiMapPromoter {}.promote(
			sourcePlan,
			config.regionAiMap);
	result.promotedAiMapRegionCount = result.regionAiMapPromotion.mappedRegionCount;
	result.unmappedAiMapRegionCount = result.regionAiMapPromotion.unmappedRegionCount;
	if (result.regionAiMapPromotion.ok()) {
		return true;
	}

	for (const RuntimeGameplayAsciiSourcePlanRegionAiMapPromotionIssue &issue :
		result.regionAiMapPromotion.issues) {
		AddIssue(result, AiMapPromotionIssue(issue));
	}
	return false;
}

bool PromoteAnnotatedPlayerStart(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	PlayerAgentState &player)
{
	bool found = false;
	for (const RuntimeGameplayAsciiSourcePlanAnnotatedCell &cell : sourcePlan.annotatedCells) {
		const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend =
			FindLegend(sourcePlan, cell.glyph);
		if (legend == nullptr || !IsPlayerStartLegend(*legend)) {
			continue;
		}

		if (found) {
			AddIssue(result, DuplicatePlayerStartIssue(cell.row, cell.column, cell.glyph));
			continue;
		}

		const TileCoord tile = PlayerStartTile(cell);
		player = PlayerFromStart(legend, &cell, tile);
		found = true;
	}

	return found;
}

bool PromoteGridPlayerStart(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	PlayerAgentState &player)
{
	bool found = false;
	for (std::size_t row = 0; row < sourcePlan.grid.rows.size(); ++row) {
		const std::string &line = sourcePlan.grid.rows[row];
		for (std::size_t column = 0; column < line.size(); ++column) {
			const char glyph = line[column];
			if (!IsPlayerStartGlyph(sourcePlan, glyph)) {
				continue;
			}

			if (found) {
				AddIssue(result, DuplicatePlayerStartIssue(row, column, glyph));
				continue;
			}

			const RuntimeGameplayAsciiSourcePlanGlyphLegendEntry *legend =
				FindLegend(sourcePlan, glyph);
			const TileCoord tile {
				static_cast<int>(column),
				static_cast<int>(row),
			};
			player = PlayerFromStart(legend, nullptr, tile);
			found = true;
		}
	}
	return found;
}

bool PromotePlayerStart(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result,
	PlayerAgentState &player,
	bool &hasPlayer)
{
	hasPlayer = PromoteAnnotatedPlayerStart(sourcePlan, result, player);
	if (!hasPlayer) {
		hasPlayer = PromoteGridPlayerStart(sourcePlan, result, player);
	}
	if (result.playerStartIssueCount > 0) {
		return false;
	}
	result.promotedPlayerCount = hasPlayer ? 1 : 0;
	return true;
}

bool PromoteProfileTraits(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult &result)
{
	std::vector<NpcAiProfileTraitEntry> entries;
	entries.reserve(sourcePlan.authoredProfiles.size() + config.profileTraits.entries.size());
	for (const RuntimeGameplayAsciiSourcePlanAuthoredProfile &profile :
		sourcePlan.authoredProfiles) {
		entries.push_back({ profile.profileId, profile.traits });
	}
	for (const NpcAiProfileTraitEntry &entry : config.profileTraits.entries) {
		entries.push_back(entry);
	}

	result.profileTraitCatalog = NpcAiProfileTraitCatalogBuilder {}.build(entries);
	if (result.profileTraitCatalog.built) {
		return true;
	}

	for (const NpcAiProfileTraitCatalogIssue &issue :
		result.profileTraitCatalog.issues) {
		AddIssue(result, ProfileTraitIssue(issue));
	}
	return false;
}

RuntimeGameplayProfileScenarioDefinition BuildDefinition(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const NpcAiProfileTraitCatalog &profileTraits,
	const RuntimeGameplayProfileScenarioFrameDefinition &frameTemplate,
	const LevelTileMap &promotedMap,
	const NpcActorState2DRegistry &actors,
	const NpcActorControlState2DRegistry &controls,
	const PlayerAgentState *player,
	const RuntimeInteractionState &interaction,
	const RuntimeInventoryState &inventory,
	const std::vector<AuthoredFrameGroup> &frameGroups,
	const std::vector<PlayerInputIntent2D> &defaultPlayerIntents,
	const AiMap2D *promotedAiMap)
{
	RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = sourcePlan.hasSourceId;
	definition.scenarioId = sourcePlan.sourceId;
	definition.initialState.session.level.map = promotedMap;
	if (player != nullptr) {
		definition.initialState.session.hasPlayer = true;
		definition.initialState.session.player = *player;
		definition.initialState.session.level.map.playerStart = {
			player->spawnTile.x,
			player->spawnTile.y,
		};
	}
	definition.initialState.npcActors = actors;
	definition.initialState.npcControls = controls;
	definition.initialState.interaction = interaction;
	definition.initialState.inventory = inventory;
	definition.profileTraits = profileTraits;
	const auto appendFrame = [&](RuntimeGameplayProfileScenarioFrameDefinition frame) {
		frame.movementMap = promotedMap;
		frame.interactionTargets = interaction.targets;
		if (promotedAiMap != nullptr) {
			frame.aiMap = *promotedAiMap;
			frame.refreshAiMap = *promotedAiMap;
		}
		definition.frames.push_back(frame);
	};

	if (frameGroups.empty()) {
		RuntimeGameplayProfileScenarioFrameDefinition frame = frameTemplate;
		frame.playerFrame.playerIntents.insert(
			frame.playerFrame.playerIntents.end(),
			defaultPlayerIntents.begin(),
			defaultPlayerIntents.end());
		appendFrame(frame);
	} else {
		for (const AuthoredFrameGroup &group : frameGroups) {
			RuntimeGameplayProfileScenarioFrameDefinition frame = frameTemplate;
			frame.hasFrameId = true;
			frame.frameId = group.frameId;
			frame.controlOverrides = group.controls;
			frame.playerFrame.playerIntents.insert(
				frame.playerFrame.playerIntents.end(),
				group.playerIntents.begin(),
				group.playerIntents.end());
			appendFrame(frame);
		}
	}
	return definition;
}

} // namespace

bool RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult::ok() const
{
	return status == RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::Converted;
}

RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult
RuntimeGameplayAsciiSourcePlanProfileScenarioConverter::convert(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config) const
{
	RuntimeGameplayAsciiSourcePlanProfileScenarioConversionResult result;
	result.sourcePlan = sourcePlan;
	result.config = config;
	result.sourceValidation = RuntimeGameplayAsciiSourcePlanValidator {}.validate(sourcePlan);

	if (!result.sourceValidation.ok()) {
		for (const RuntimeGameplayAsciiSourcePlanIssue &issue : result.sourceValidation.issues) {
			AddIssue(result, SourceIssue(issue));
		}
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::SourcePlanInvalid;
		return result;
	}

	if (!PromoteTerrain(sourcePlan, config, result)) {
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::UnsupportedTerrainPromotion;
		return result;
	}

	RuntimeGameplayProfileScenarioFrameDefinition frameTemplate =
		config.hasDefaultFrame ? config.defaultFrame : RuntimeGameplayProfileScenarioFrameDefinition {};
	frameTemplate.movementMap = result.promotedMap;

	std::vector<AuthoredFrameGroup> frameGroups;
	if (!PromoteActorsAndControls(sourcePlan, config, frameGroups, result)) {
		result.issueCount = result.issues.size();
		result.status = result.authoredControlIssueCount > 0
			? RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AuthoredControlInvalid
			: !result.actorRegistry.built
			? RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ActorRegistryInvalid
			: RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ControlRegistryInvalid;
		return result;
	}

	PlayerAgentState player;
	bool hasPlayer = false;
	if (!PromotePlayerStart(sourcePlan, result, player, hasPlayer)) {
		result.issueCount = result.issues.size();
		result.status =
			RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::PlayerStartInvalid;
		return result;
	}

	std::vector<PlayerInputIntent2D> defaultPlayerIntents;
	if (!ApplyAuthoredPlayerCommands(
			sourcePlan,
			defaultPlayerIntents,
			frameGroups,
			result)) {
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::
			AuthoredPlayerCommandInvalid;
		return result;
	}

	RuntimeInteractionState interaction;
	if (!PromoteInteractionState(sourcePlan, result, interaction)) {
		result.issueCount = result.issues.size();
		result.status = result.interactionTargetRegistryIssueCount > 0
			? RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::
				InteractionTargetRegistryInvalid
			: RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::
				InteractionEffectCatalogInvalid;
		return result;
	}

	RuntimeInventoryState inventory;
	if (!PromoteInventoryState(sourcePlan, result, inventory)) {
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::
			ItemDropRegistryInvalid;
		return result;
	}

	if (!PromoteRegionAiMap(sourcePlan, config, result)) {
		result.issueCount = result.issues.size();
		result.status =
			RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AiMapPromotionInvalid;
		return result;
	}

	if (!PromoteProfileTraits(sourcePlan, config, result)) {
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::
			ProfileTraitCatalogInvalid;
		return result;
	}

	result.definition = BuildDefinition(
		sourcePlan,
		result.profileTraitCatalog.catalog,
		frameTemplate,
		result.promotedMap,
		result.actorRegistry.registry,
		result.controlRegistry.registry,
		hasPlayer ? &player : nullptr,
		interaction,
		inventory,
		frameGroups,
		defaultPlayerIntents,
		config.promoteRegionAiMap ? &result.regionAiMapPromotion.aiMap : nullptr);
	result.profileValidation = RuntimeGameplayProfileScenarioValidator {}.validate(result.definition);
	if (!result.profileValidation.ok()) {
		for (const RuntimeGameplayProfileScenarioIssue &issue : result.profileValidation.issues) {
			AddIssue(result, ProfileIssue(issue));
		}
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ProfileScenarioInvalid;
		return result;
	}

	result.converted = true;
	result.issueCount = result.issues.size();
	result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::Converted;
	return result;
}

} // namespace iggy::runtime
