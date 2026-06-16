#include <cstdlib>

#include "runtime/RuntimeGameplayAsciiScenarioPacketValidator.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanAdapter.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanValidator.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeGameplayAsciiSourcePlan SourcePlan()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan plan;
	plan.formatId = Id("iggy:ascii-source-plan");
	plan.version = 1;
	plan.hasSourceId = true;
	plan.sourceId = Id("source:toml-shaped-room");
	plan.hasSourceRef = true;
	plan.sourceRef = Id("ref:manual-authoring-packet");
	plan.grid.width = 7;
	plan.grid.height = 4;
	plan.grid.rows = {
		"#######",
		"#A...@#",
		"#.....#",
		"#######",
	};
	plan.grid.backgroundGlyph = '.';
	plan.legend = {
		{
			'A',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::Actor,
			Id("role:npc"),
			{ Id("tag:guard"), Id("tag:profiled") },
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::Actor,
			{},
			{},
		},
		{
			'@',
			iggy::runtime::RuntimeGameplayAsciiSourcePlanGlyphKind::PlayerStart,
			Id("role:player-start"),
			{},
			true,
			iggy::runtime::RuntimeGameplayAsciiScenarioMarkerKind::PlayerStart,
			{},
			{},
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
	plan.annotatedCells = {
		{
			true,
			Id("cell:guard"),
			1,
			1,
			'A',
			{ true, 1, 1 },
			{ true, 1.0, 1.0 },
			{ true, 1.0, 1.0, 2.0, 2.0 },
			{ Id("tag:guard") },
			Id("npc:guard"),
			Id("profile:guard"),
		},
		{
			true,
			Id("cell:player-start"),
			1,
			5,
			'@',
			{ true, 5, 1 },
			{ true, 5.0, 1.0 },
			{ true, 5.0, 1.0, 6.0, 2.0 },
			{ Id("tag:start") },
			{},
			{},
		},
	};
	plan.regions = {
		{ true, Id("region:room"), 0, 0, 3, 6, { Id("tag:room") } },
	};
	return plan;
}

void TestSourcePlanValidationAdapterAndPacketValidationChain()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan sourcePlan = SourcePlan();

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanValidator {}.validate(sourcePlan);
	Expect(sourceValidation.ok(), "source plan should validate before adaptation");
	Expect(sourceValidation.plan.sourceRef == Id("ref:manual-authoring-packet"), "source validation should preserve source ref");
	Expect(sourceValidation.annotatedCellCount == 2, "source validation should preserve annotated cell count");
	Expect(sourceValidation.regionCount == 1, "source validation should preserve region count");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult adapter =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanToPacketAdapter {}.convert(sourcePlan);
	Expect(adapter.converted(), "valid source plan should adapt to typed ASCII scenario packet");
	Expect(adapter.validation.ok(), "adapter should preserve nested source validation");
	Expect(adapter.packet.hasSourceId && adapter.packet.sourceId == Id("source:toml-shaped-room"), "adapter should preserve source id");
	Expect(adapter.packet.rows == sourcePlan.grid.rows, "adapter should preserve source rows");
	Expect(adapter.markerCount == 3, "adapter should expose representable floor/player/actor markers");

	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult packetValidation =
		iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidator {}.validate(adapter.packet);
	Expect(packetValidation.ok(), "adapted typed ASCII scenario packet should validate");
	Expect(packetValidation.rowCount == 4 && packetValidation.width == 7, "packet validation should preserve grid facts");
	Expect(packetValidation.markerCount == 3, "packet validation should preserve marker count");
	Expect(packetValidation.packet.markers[2].actorId == Id("npc:guard"), "packet validation should preserve exact actor id");
	Expect(packetValidation.packet.markers[2].profileId == Id("profile:guard"), "packet validation should preserve exact profile id");
}

void TestInvalidSourcePlanStopsBeforeTypedPacketValidation()
{
	iggy::runtime::RuntimeGameplayAsciiSourcePlan sourcePlan = SourcePlan();
	sourcePlan.noClaims.claimsGameplayExecution = true;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanValidator {}.validate(sourcePlan);
	Expect(!sourceValidation.ok(), "unsafe source plan should fail validation");

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult adapter =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanToPacketAdapter {}.convert(sourcePlan);
	Expect(!adapter.converted(), "unsafe source plan should not adapt to typed packet");
	Expect(adapter.status == iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterStatus::SourcePlanInvalid, "unsafe source plan should report source invalid");
	Expect(adapter.packet.rows.empty(), "adapter should not publish typed packet rows for invalid source plan");
}

void TestPipelineDoesNotMutateSourcePlan()
{
	const iggy::runtime::RuntimeGameplayAsciiSourcePlan sourcePlan = SourcePlan();
	iggy::runtime::RuntimeGameplayAsciiSourcePlan mutablePlan = sourcePlan;

	const iggy::runtime::RuntimeGameplayAsciiSourcePlanValidationResult sourceValidation =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanValidator {}.validate(mutablePlan);
	const iggy::runtime::RuntimeGameplayAsciiSourcePlanAdapterResult adapter =
		iggy::runtime::RuntimeGameplayAsciiSourcePlanToPacketAdapter {}.convert(mutablePlan);
	const iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidationResult packetValidation =
		iggy::runtime::RuntimeGameplayAsciiScenarioPacketValidator {}.validate(adapter.packet);

	Expect(sourceValidation.ok() && adapter.converted() && packetValidation.ok(), "pipeline should be valid before immutability checks");
	Expect(mutablePlan.grid.rows == sourcePlan.grid.rows, "pipeline should not mutate source rows");
	Expect(mutablePlan.legend[0].roleTags == sourcePlan.legend[0].roleTags, "pipeline should not mutate legend tags");
	Expect(mutablePlan.annotatedCells[0].markerId == sourcePlan.annotatedCells[0].markerId, "pipeline should not mutate annotated marker id");
	Expect(mutablePlan.regions[0].regionId == sourcePlan.regions[0].regionId, "pipeline should not mutate regions");
}

} // namespace

int main()
{
	TestSourcePlanValidationAdapterAndPacketValidationChain();
	TestInvalidSourcePlanStopsBeforeTypedPacketValidation();
	TestPipelineDoesNotMutateSourcePlan();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
