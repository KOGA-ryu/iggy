#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

void TestDefaultSourcePlanIsEmptyAndSafe()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;

	Expect(plan.formatId.empty(), "default source plan should not have format id");
	Expect(plan.version == 1, "default source plan should use version 1");
	Expect(!plan.hasSourceId, "default source plan should not have source id");
	Expect(!plan.hasSourceRef, "default source plan should not have source ref");
	Expect(!plan.hasRows(), "default source plan should not have rows");
	Expect(plan.rowCount() == 0, "default source plan should report zero rows");
	Expect(plan.legendCount() == 0, "default source plan should report zero legend entries");
	Expect(plan.annotatedCellCount() == 0, "default source plan should report zero annotated cells");
	Expect(plan.regionCount() == 0, "default source plan should report zero regions");
	Expect(plan.authoredProfileCount() == 0, "default source plan should report zero authored profiles");
	Expect(plan.authoredInteractionTargetCount() == 0, "default source plan should report zero authored interaction targets");
	Expect(plan.authoredItemDropCount() == 0, "default source plan should report zero authored item drops");
	Expect(!plan.hasExpectations(), "default source plan should not have expectations");
	Expect(plan.safeForAuthoring(), "default source plan should be safe authoring data");
}

void TestSourcePlanPreservesGridAndSourceFacts()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.version = 2;
	plan.hasSourceId = true;
	plan.sourceId = Id("source:yard");
	plan.hasSourceRef = true;
	plan.sourceRef = Id("ref:sketch-01");
	plan.grid.width = 5;
	plan.grid.height = 3;
	plan.grid.rows = {
		"#####",
		"#A..#",
		"#####",
	};
	plan.grid.backgroundGlyph = '.';

	Expect(plan.formatId == Id("iggy:ascii-source-plan"), "source plan should preserve format id");
	Expect(plan.version == 2, "source plan should preserve version");
	Expect(plan.hasSourceId && plan.sourceId == Id("source:yard"), "source plan should preserve source id");
	Expect(plan.hasSourceRef && plan.sourceRef == Id("ref:sketch-01"), "source plan should preserve source ref");
	Expect(plan.hasRows(), "source plan should report rows");
	Expect(plan.rowCount() == 3, "source plan should preserve row count");
	Expect(plan.grid.width == 5 && plan.grid.height == 3, "source plan should preserve declared grid dimensions");
	Expect(plan.grid.rows[1] == "#A..#", "source plan should preserve row text exactly");
	Expect(plan.grid.backgroundGlyph == '.', "source plan should preserve background glyph");
}

void TestLegendPreservesRolesAndScenarioMappings()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:hostile"), Id("tag:guard") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:guard"),
			Id("profile:guard"),
		},
		{
			'.',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Terrain,
			Id("role:floor"),
			{},
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Floor,
			{},
			{},
		},
	};

	Expect(plan.legendCount() == 2, "source plan should preserve legend count");
	Expect(plan.legend[0].glyph == 'A', "legend should preserve glyph");
	Expect(plan.legend[0].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor, "legend should preserve glyph kind");
	Expect(plan.legend[0].roleId == Id("role:npc"), "legend should preserve role id");
	Expect(plan.legend[0].roleTags.size() == 2, "legend should preserve role tags");
	Expect(plan.legend[0].roleTags[0] == Id("tag:hostile"), "legend should preserve exact namespaced role tag");
	Expect(plan.legend[0].roleTags[1] == Id("tag:guard"), "legend should preserve second role tag");
	Expect(plan.legend[0].mapsToScenarioMarker, "legend should preserve scenario mapping flag");
	Expect(plan.legend[0].scenarioMarkerKind == iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor, "legend should preserve scenario marker kind");
	Expect(plan.legend[0].targetMarkerId == Id("npc:guard"), "legend should preserve target marker id");
	Expect(plan.legend[0].targetProfileId == Id("profile:guard"), "legend should preserve target profile id");
}

void TestAnnotatedCellsPreserveLocalFactsAndExactIds()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAnnotatedCell cell;
	cell.hasCellId = true;
	cell.cellId = Id("cell:npc-a");
	cell.row = 1;
	cell.column = 2;
	cell.glyph = 'A';
	cell.localTile = { true, 7, 9 };
	cell.localPosition = { true, 7.5, 9.25 };
	cell.cellBounds = { true, 7.0, 9.0, 8.0, 10.0 };
	cell.roleTags = { Id("actor"), Id("profile:guard") };
	cell.markerId = Id("npc:guard");
	cell.profileId = Id("profile:guard");
	plan.annotatedCells.push_back(cell);

	Expect(plan.annotatedCellCount() == 1, "source plan should preserve annotated cell count");
	Expect(plan.annotatedCells[0].hasCellId && plan.annotatedCells[0].cellId == Id("cell:npc-a"), "annotated cell should preserve cell id");
	Expect(plan.annotatedCells[0].row == 1 && plan.annotatedCells[0].column == 2, "annotated cell should preserve row and column");
	Expect(plan.annotatedCells[0].glyph == 'A', "annotated cell should preserve glyph");
	Expect(plan.annotatedCells[0].localTile.present && plan.annotatedCells[0].localTile.x == 7 && plan.annotatedCells[0].localTile.y == 9, "annotated cell should preserve local tile");
	Expect(plan.annotatedCells[0].localPosition.present && plan.annotatedCells[0].localPosition.x == 7.5 && plan.annotatedCells[0].localPosition.y == 9.25, "annotated cell should preserve local position");
	Expect(plan.annotatedCells[0].cellBounds.present && plan.annotatedCells[0].cellBounds.maxX == 8.0, "annotated cell should preserve cell bounds");
	Expect(plan.annotatedCells[0].roleTags[1] == Id("profile:guard"), "annotated cell should preserve exact role tag ids");
	Expect(plan.annotatedCells[0].markerId == Id("npc:guard"), "annotated cell should preserve marker id");
	Expect(plan.annotatedCells[0].profileId == Id("profile:guard"), "annotated cell should preserve profile id");
}

void TestRegionsAndNoClaimFlagsAreExplicit()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.regions = {
		{
			true,
			Id("region:room-a"),
			1,
			2,
			4,
			7,
			{ Id("tag:room"), Id("tag:encounter") },
		},
	};

	Expect(plan.regionCount() == 1, "source plan should preserve region count");
	Expect(plan.regions[0].hasRegionId && plan.regions[0].regionId == Id("region:room-a"), "region should preserve id");
	Expect(plan.regions[0].minRow == 1 && plan.regions[0].maxRow == 4, "region should preserve row bounds");
	Expect(plan.regions[0].minColumn == 2 && plan.regions[0].maxColumn == 7, "region should preserve column bounds");
	Expect(plan.regions[0].roleTags[0] == Id("tag:room"), "region should preserve role tags");
	Expect(plan.safeForAuthoring(), "safe no-claim defaults should report safe authoring data");

	plan.noClaims.claimsRuntimeTruth = true;
	Expect(!plan.safeForAuthoring(), "runtime truth claim should make source plan unsafe");
	plan.noClaims.claimsRuntimeTruth = false;
	plan.promotionPolicy.allowsProfileScenarioConversion = true;
	Expect(!plan.safeForAuthoring(), "profile scenario conversion permission should make source plan unsafe for this boundary");
}

void TestAuthoredControlsPreserveMovementFactsAndExactIds()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredControl control;
	control.hasFrameId = true;
	control.frameId = Id("frame:one");
	control.npcId = Id("npc:guard");
	control.behavior = iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking;
	control.moveMode = iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk;
	control.targetPosition = { true, 2.5, 1.5 };
	plan.authoredControls.push_back(control);

	Expect(plan.authoredControlCount() == 1, "source plan should preserve authored control count");
	Expect(plan.authoredControls[0].hasFrameId && plan.authoredControls[0].frameId == Id("frame:one"), "authored control should preserve optional frame id");
	Expect(plan.authoredControls[0].npcId == Id("npc:guard"), "authored control should preserve exact npc id");
	Expect(plan.authoredControls[0].behavior == iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking, "authored control should preserve behavior");
	Expect(plan.authoredControls[0].moveMode == iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, "authored control should preserve move mode");
	Expect(plan.authoredControls[0].targetPosition.present && plan.authoredControls[0].targetPosition.x == 2.5 && plan.authoredControls[0].targetPosition.y == 1.5, "authored control should preserve target position");
}

void TestAuthoredProfilesPreserveTraitsAndExactIds()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredProfile profile;
	profile.profileId = Id("plain-profile");
	profile.traits.strength = 12;
	profile.traits.dexterity = 11;
	profile.traits.constitution = 10;
	profile.traits.intelligence = 9;
	profile.traits.wisdom = 8;
	profile.traits.charisma = 7;
	plan.authoredProfiles.push_back(profile);

	Expect(plan.authoredProfileCount() == 1, "source plan should preserve authored profile count");
	Expect(plan.authoredProfiles[0].profileId == Id("plain-profile"), "authored profile should preserve exact profile id");
	Expect(plan.authoredProfiles[0].traits.strength == 12, "authored profile should preserve strength");
	Expect(plan.authoredProfiles[0].traits.dexterity == 11, "authored profile should preserve dexterity");
	Expect(plan.authoredProfiles[0].traits.constitution == 10, "authored profile should preserve constitution");
	Expect(plan.authoredProfiles[0].traits.intelligence == 9, "authored profile should preserve intelligence");
	Expect(plan.authoredProfiles[0].traits.wisdom == 8, "authored profile should preserve wisdom");
	Expect(plan.authoredProfiles[0].traits.charisma == 7, "authored profile should preserve charisma");
}

void TestAuthoredInteractionTargetsPreserveFactsAndExactIds()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredInteractionTarget target;
	target.targetId = Id("plain-target");
	target.kind = iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Door;
	target.localTile = { true, 2, 1 };
	target.localPosition = { true, 2.5, 1.5 };
	target.radius = 1.25;
	target.enabled = false;
	target.effect = iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionEffectKind::ToggleTarget;
	target.effectTargetId = Id("target:door");
	target.requiredItemId = Id("item:key");
	target.enabledValue = true;
	plan.authoredInteractionTargets.push_back(target);

	Expect(plan.authoredInteractionTargetCount() == 1, "source plan should preserve authored interaction target count");
	Expect(plan.authoredInteractionTargets[0].targetId == Id("plain-target"), "authored interaction target should preserve exact target id");
	Expect(plan.authoredInteractionTargets[0].kind == iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Door, "authored interaction target should preserve kind");
	Expect(plan.authoredInteractionTargets[0].localTile.present && plan.authoredInteractionTargets[0].localTile.x == 2 && plan.authoredInteractionTargets[0].localTile.y == 1, "authored interaction target should preserve tile position");
	Expect(plan.authoredInteractionTargets[0].localPosition.present && plan.authoredInteractionTargets[0].localPosition.x == 2.5 && plan.authoredInteractionTargets[0].localPosition.y == 1.5, "authored interaction target should preserve point position");
	Expect(plan.authoredInteractionTargets[0].radius == 1.25, "authored interaction target should preserve radius");
	Expect(!plan.authoredInteractionTargets[0].enabled, "authored interaction target should preserve enabled flag");
	Expect(plan.authoredInteractionTargets[0].effect == iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionEffectKind::ToggleTarget, "authored interaction target should preserve effect kind");
	Expect(plan.authoredInteractionTargets[0].effectTargetId == Id("target:door"), "authored interaction target should preserve effect target id");
	Expect(plan.authoredInteractionTargets[0].requiredItemId == Id("item:key"), "authored interaction target should preserve required item id");
}

void TestAuthoredItemDropsPreserveFactsAndExactIds()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanAuthoredItemDrop drop;
	drop.dropId = Id("plain-drop");
	drop.itemId = Id("item:key");
	drop.count = 2;
	drop.localTile = { true, 3, 1 };
	drop.localPosition = { true, 3.5, 1.5 };
	drop.pickupRadius = 0.75;
	drop.enabled = false;
	drop.glyph = 'k';
	plan.authoredItemDrops.push_back(drop);

	Expect(plan.authoredItemDropCount() == 1, "source plan should preserve authored item drop count");
	Expect(plan.authoredItemDrops[0].dropId == Id("plain-drop"), "authored item drop should preserve exact drop id");
	Expect(plan.authoredItemDrops[0].itemId == Id("item:key"), "authored item drop should preserve exact item id");
	Expect(plan.authoredItemDrops[0].count == 2, "authored item drop should preserve count");
	Expect(plan.authoredItemDrops[0].localTile.present && plan.authoredItemDrops[0].localTile.x == 3 && plan.authoredItemDrops[0].localTile.y == 1, "authored item drop should preserve tile position");
	Expect(plan.authoredItemDrops[0].localPosition.present && plan.authoredItemDrops[0].localPosition.x == 3.5 && plan.authoredItemDrops[0].localPosition.y == 1.5, "authored item drop should preserve point position");
	Expect(plan.authoredItemDrops[0].pickupRadius == 0.75, "authored item drop should preserve pickup radius");
	Expect(!plan.authoredItemDrops[0].enabled, "authored item drop should preserve enabled flag");
	Expect(plan.authoredItemDrops[0].glyph == 'k', "authored item drop should preserve optional glyph");
}

void TestExpectationsPreserveFinalRowsAndSummaryCounts()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.expectations.hasFinalRows = true;
	plan.expectations.finalRows = { "###", "#@#", "###" };
	plan.expectations.hasFrameCount = true;
	plan.expectations.frameCount = 2;
	plan.expectations.hasAcceptedCommandCount = true;
	plan.expectations.acceptedCommandCount = 1;
	plan.expectations.hasPickedUpCount = true;
	plan.expectations.pickedUpCount = 1;
	plan.expectations.hasInteractionChanged = true;
	plan.expectations.interactionChanged = true;
	plan.expectations.hasNpcMovedCount = true;
	plan.expectations.npcMovedCount = 3;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedTraceFrame frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:one");
	frame.hasRows = true;
	frame.rows = { "###", "#@#", "###" };
	frame.hasAcceptedCommandCount = true;
	frame.acceptedCommandCount = 1;
	frame.hasPickedUpCount = true;
	frame.pickedUpCount = 0;
	frame.hasInteractionChanged = true;
	frame.interactionChanged = false;
	frame.hasNpcMovedCount = true;
	frame.npcMovedCount = 2;
	plan.expectations.traceFrames.push_back(frame);
	plan.expectations.inventoryStacks.push_back({ Id("item:key"), 1 });
	plan.expectations.interactionTargets.push_back({ Id("target:door"), false });

	Expect(plan.hasExpectations(), "source plan should report authored expectations");
	Expect(plan.expectations.hasFinalRows, "expectations should preserve final rows presence");
	Expect(plan.expectations.finalRows[1] == "#@#", "expectations should preserve final rows exactly");
	Expect(plan.expectations.hasFrameCount && plan.expectations.frameCount == 2, "expectations should preserve frame count");
	Expect(plan.expectations.hasAcceptedCommandCount && plan.expectations.acceptedCommandCount == 1, "expectations should preserve accepted command count");
	Expect(plan.expectations.hasPickedUpCount && plan.expectations.pickedUpCount == 1, "expectations should preserve picked up count");
	Expect(plan.expectations.hasInteractionChanged && plan.expectations.interactionChanged, "expectations should preserve interaction changed flag");
	Expect(plan.expectations.hasNpcMovedCount && plan.expectations.npcMovedCount == 3, "expectations should preserve NPC moved count");
	Expect(plan.expectations.traceFrames.size() == 1, "expectations should preserve expected trace frames");
	Expect(plan.expectations.traceFrames[0].frameId == Id("frame:one"), "trace expectation should preserve frame id");
	Expect(plan.expectations.traceFrames[0].rows[1] == "#@#", "trace expectation should preserve rows");
	Expect(plan.expectations.traceFrames[0].hasAcceptedCommandCount && plan.expectations.traceFrames[0].acceptedCommandCount == 1, "trace expectation should preserve accepted command count");
	Expect(plan.expectations.inventoryStacks.size() == 1, "expectations should preserve inventory stacks");
	Expect(plan.expectations.inventoryStacks[0].itemId == Id("item:key") && plan.expectations.inventoryStacks[0].count == 1, "inventory expectation should preserve item id and count");
	Expect(plan.expectations.interactionTargets.size() == 1, "expectations should preserve interaction target states");
	Expect(plan.expectations.interactionTargets[0].targetId == Id("target:door") && !plan.expectations.interactionTargets[0].enabled, "interaction expectation should preserve target id and enabled state");
}

void TestSourcePlanCopiesAreIndependent()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.grid.rows = { "A." };
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:original") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			Id("npc:copy"),
			Id("profile:copy"),
		},
	};
	plan.annotatedCells = {
		{ true, Id("cell:copy"), 0, 0, 'A', {}, {}, {}, { Id("tag:cell") }, Id("npc:copy"), Id("profile:copy") },
	};
	plan.authoredControls = {
		{ true, Id("frame:copy"), Id("npc:copy"), iggy::runtime::RuntimeGameplayAsciiSourcePlanControlBehavior::Seeking, iggy::runtime::RuntimeGameplayAsciiSourcePlanControlMoveMode::Walk, { true, 2.5, 1.5 } },
	};
	plan.authoredProfiles = {
		{ Id("profile:copy"), {} },
	};
	plan.authoredInteractionTargets = {
		{ Id("target:copy"), iggy::runtime::RuntimeGameplayAsciiSourcePlanInteractionTargetKind::Usable, { true, 0, 0 } },
	};
	plan.authoredItemDrops = {
		{ Id("drop:copy"), Id("item:copy"), 1, { true, 0, 0 }, {}, 0.0, true, 'k' },
	};
	plan.expectations.hasFinalRows = true;
	plan.expectations.finalRows = { "A." };
	plan.expectations.hasFrameCount = true;
	plan.expectations.frameCount = 1;
	iggy::runtime::RuntimeGameplayAsciiSourcePlanExpectedTraceFrame frame;
	frame.hasFrameId = true;
	frame.frameId = Id("frame:copy");
	frame.hasRows = true;
	frame.rows = { "A." };
	plan.expectations.traceFrames.push_back(frame);
	plan.expectations.inventoryStacks.push_back({ Id("item:copy"), 1 });
	plan.expectations.interactionTargets.push_back({ Id("target:copy"), false });

	iggy::runtime::RuntimeGameplayAsciiSourcePlan copy = plan;
	copy.grid.rows[0] = "..";
	copy.legend[0].targetMarkerId = Id("npc:changed");
	copy.annotatedCells[0].profileId = Id("profile:changed");
	copy.authoredControls[0].npcId = Id("npc:changed");
	copy.authoredProfiles[0].profileId = Id("profile:changed");
	copy.authoredInteractionTargets[0].targetId = Id("target:changed");
	copy.authoredItemDrops[0].dropId = Id("drop:changed");
	copy.expectations.finalRows[0] = "..";
	copy.expectations.frameCount = 2;
	copy.expectations.traceFrames[0].rows[0] = "..";
	copy.expectations.inventoryStacks[0].itemId = Id("item:changed");
	copy.expectations.interactionTargets[0].targetId = Id("target:changed");

	Expect(plan.grid.rows[0] == "A.", "source plan copy should not mutate source rows");
	Expect(plan.legend[0].targetMarkerId == Id("npc:copy"), "source plan copy should not mutate legend target id");
	Expect(plan.annotatedCells[0].profileId == Id("profile:copy"), "source plan copy should not mutate annotated cell profile id");
	Expect(plan.authoredControls[0].npcId == Id("npc:copy"), "source plan copy should not mutate authored controls");
	Expect(plan.authoredProfiles[0].profileId == Id("profile:copy"), "source plan copy should not mutate authored profiles");
	Expect(plan.authoredInteractionTargets[0].targetId == Id("target:copy"), "source plan copy should not mutate authored interaction targets");
	Expect(plan.authoredItemDrops[0].dropId == Id("drop:copy"), "source plan copy should not mutate authored item drops");
	Expect(plan.expectations.finalRows[0] == "A.", "source plan copy should not mutate expectation rows");
	Expect(plan.expectations.frameCount == 1, "source plan copy should not mutate expectation counts");
	Expect(plan.expectations.traceFrames[0].rows[0] == "A.", "source plan copy should not mutate trace expectation rows");
	Expect(plan.expectations.inventoryStacks[0].itemId == Id("item:copy"), "source plan copy should not mutate inventory expectations");
	Expect(plan.expectations.interactionTargets[0].targetId == Id("target:copy"), "source plan copy should not mutate interaction expectations");
}

} // namespace

int main()
{
	TestDefaultSourcePlanIsEmptyAndSafe();
	TestSourcePlanPreservesGridAndSourceFacts();
	TestLegendPreservesRolesAndScenarioMappings();
	TestAnnotatedCellsPreserveLocalFactsAndExactIds();
	TestRegionsAndNoClaimFlagsAreExplicit();
	TestAuthoredControlsPreserveMovementFactsAndExactIds();
	TestAuthoredProfilesPreserveTraitsAndExactIds();
	TestAuthoredInteractionTargetsPreserveFactsAndExactIds();
	TestAuthoredItemDropsPreserveFactsAndExactIds();
	TestExpectationsPreserveFinalRowsAndSummaryCounts();
	TestSourcePlanCopiesAreIndependent();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
