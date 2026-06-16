#include "runtime/RuntimeGameplayAsciiSourcePlanProfileScenarioConverter.hpp"

namespace iggy::runtime {
namespace {

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

bool PromoteActorsAndControls(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
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

RuntimeGameplayProfileScenarioDefinition BuildDefinition(
	const RuntimeGameplayAsciiSourcePlan &sourcePlan,
	const RuntimeGameplayAsciiSourcePlanProfileScenarioConversionConfig &config,
	const LevelTileMap &promotedMap,
	const NpcActorState2DRegistry &actors,
	const NpcActorControlState2DRegistry &controls)
{
	RuntimeGameplayProfileScenarioDefinition definition;
	definition.hasScenarioId = sourcePlan.hasSourceId;
	definition.scenarioId = sourcePlan.sourceId;
	definition.initialState.session.level.map = promotedMap;
	definition.initialState.npcActors = actors;
	definition.initialState.npcControls = controls;
	definition.profileTraits = config.profileTraits;
	RuntimeGameplayProfileScenarioFrameDefinition frame = config.defaultFrame;
	frame.movementMap = promotedMap;
	definition.frames.push_back(frame);
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

	if (!config.hasDefaultFrame) {
		AddIssue(result, MissingFrameDefaultsIssue());
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::MissingFrameDefaults;
		return result;
	}

	if (!PromoteTerrain(sourcePlan, config, result)) {
		result.issueCount = result.issues.size();
		result.status = RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::UnsupportedTerrainPromotion;
		return result;
	}

	if (!PromoteActorsAndControls(sourcePlan, config, result)) {
		result.issueCount = result.issues.size();
		result.status = !result.actorRegistry.built
			? RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ActorRegistryInvalid
			: RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::ControlRegistryInvalid;
		return result;
	}

	if (!PromoteRegionAiMap(sourcePlan, config, result)) {
		result.issueCount = result.issues.size();
		result.status =
			RuntimeGameplayAsciiSourcePlanProfileScenarioConversionStatus::AiMapPromotionInvalid;
		return result;
	}

	result.definition = BuildDefinition(
		sourcePlan,
		config,
		result.promotedMap,
		result.actorRegistry.registry,
		result.controlRegistry.registry);
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
