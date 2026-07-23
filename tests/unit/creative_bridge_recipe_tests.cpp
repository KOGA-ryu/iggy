#include "app/iggy3d/creative/recipes/BridgeRecipe.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

bool nearly(double lhs, double rhs) {
  return std::fabs(lhs - rhs) <= 1.0e-9;
}

cr::CreativeBridgeRecipeRequest defaultRequest() {
  cr::CreativeBridgeRecipeRequest request;
  request.instanceKey = "bridge.creek.1";
  request.name = "Creek Bridge";
  request.gridCellSizeMeters = 1.0;
  request.source.watercoursePathKey = "creek";
  request.source.crossingId = 7U;
  request.crossing.id = 7U;
  request.crossing.sourcePointId = 3U;
  request.crossing.centerMeters = {10.0, 13.0, 8.0};
  request.crossing.crossingAxis = {0.0, 0.0, 1.0};
  request.crossing.leftBankMeters = {10.0, 12.0, 12.0};
  request.crossing.rightBankMeters = {10.0, 12.0, 4.0};
  request.crossing.leftApproachMeters = {10.0, 12.0, 14.0};
  request.crossing.rightApproachMeters = {10.0, 12.0, 2.0};
  request.crossing.centerGrid = {10.0, 13.0, 8.0};
  request.crossing.leftBankGrid = {10.0, 12.0, 12.0};
  request.crossing.rightBankGrid = {10.0, 12.0, 4.0};
  request.crossing.leftApproachGrid = {10.0, 12.0, 14.0};
  request.crossing.rightApproachGrid = {10.0, 12.0, 2.0};
  request.crossing.channelBedGrid = {10.0, 6.0, 8.0};
  request.crossing.clearanceReferenceGrid = {10.0, 8.0, 8.0};
  request.crossing.channelBedMeters = {10.0, 6.0, 8.0};
  request.crossing.clearanceReferenceMeters = {10.0, 8.0, 8.0};
  request.crossing.bridgeTransform.position = request.crossing.centerMeters;
  request.crossing.spanMeters = 8.0;
  request.tags = {"world_layout:test"};
  return request;
}

bool bridgePlansStructureCollisionMembersAndApproaches() {
  const cr::CreativeBridgeRecipeRequest request = defaultRequest();
  const cr::CreativeBridgeRecipeResult first = cr::planCreativeBridge(request);
  const cr::CreativeBridgeRecipeResult repeated =
      cr::planCreativeBridge(request);
  const cr::CreativeRecipeMaterializeResult materialized =
      first.receipt.accepted
          ? cr::materializeCreativeRecipe(first.structure, 1U)
          : cr::CreativeRecipeMaterializeResult{};

  return expect(first.receipt.accepted &&
                    first.receipt.status == cr::CreativeBridgeRecipeStatus::Ready &&
                    first.structure.kind == cr::CreativeRecipeKind::Bridge &&
                    first.receipt.deckCount == 1U &&
                    first.receipt.supportStationCount == 1U &&
                    first.receipt.supportObjectCount == 2U &&
                    first.receipt.railCount == 2U &&
                    first.receipt.generatedObjectCount == 5U &&
                    first.approachGradeCount == 2U,
                "bridge emits one deck paired supports rails and approaches") &&
         expect(materialized.receipt.accepted &&
                    materialized.createRequests.size() == 5U &&
                    materialized.createRequests[0].kind ==
                        cr::CreativeObjectKind::Bridge &&
                    materialized.createRequests[1].kind ==
                        cr::CreativeObjectKind::Column &&
                    materialized.createRequests[3].kind ==
                        cr::CreativeObjectKind::Railing,
                "bridge members materialize through existing collision kinds") &&
         expect(first.approachGrades[0].start ==
                        cr::CreativeTerrainCoord2{10, 14} &&
                    first.approachGrades[0].end ==
                        cr::CreativeTerrainCoord2{10, 12} &&
                    first.approachGrades[0].startHeightCells == 12U &&
                    first.approachGrades[0].endHeightCells == 13U &&
                    first.approachGrades[1].start ==
                        cr::CreativeTerrainCoord2{10, 4} &&
                    first.approachGrades[1].end ==
                        cr::CreativeTerrainCoord2{10, 2} &&
                    nearly(first.receipt.maximumApproachGradePermille, 500.0),
                "approaches use canonical crossing cells and exact deck height") &&
         expect(first.receipt.definitionFingerprint != 0U &&
                    repeated.receipt.accepted &&
                    repeated.receipt.definitionFingerprint ==
                        first.receipt.definitionFingerprint &&
                    repeated.structure.objects.size() ==
                        first.structure.objects.size(),
                "bridge regeneration is deterministic");
}

bool bridgeSettingsControlOptionalStructure() {
  cr::CreativeBridgeRecipeRequest request = defaultRequest();
  request.source.settings.supportStyle = cr::CreativeBridgeSupportStyle::None;
  request.source.settings.rails = false;
  request.source.settings.materials.deck =
      cr::CreativeStructuralMaterial::Stone;
  const cr::CreativeBridgeRecipeResult result = cr::planCreativeBridge(request);

  return expect(result.receipt.accepted && result.receipt.deckCount == 1U &&
                    result.receipt.supportObjectCount == 0U &&
                    result.receipt.railCount == 0U &&
                    result.structure.objects.size() == 1U,
                "support and rail policy are explicit") &&
         expect(std::find(
                    result.structure.objects[0].createRequest.tags.begin(),
                    result.structure.objects[0].createRequest.tags.end(),
                    cr::creativeStructuralMaterialTag(
                        cr::CreativeStructuralMaterial::Stone)) !=
                    result.structure.objects[0].createRequest.tags.end(),
                "deck material kit reaches generated member tags");
}

bool bridgeOutputFingerprintIncludesApproachGrades() {
  const cr::CreativeBridgeRecipeRequest baseRequest = defaultRequest();
  cr::CreativeBridgeRecipeRequest widerFalloffRequest = baseRequest;
  ++widerFalloffRequest.source.settings.approachFalloffCells;

  const cr::CreativeBridgeRecipeResult base =
      cr::planCreativeBridge(baseRequest);
  const cr::CreativeBridgeRecipeResult widerFalloff =
      cr::planCreativeBridge(widerFalloffRequest);

  return expect(base.receipt.accepted && widerFalloff.receipt.accepted,
                "approach-only bridge variants are valid") &&
         expect(base.structure.definitionFingerprint ==
                    widerFalloff.structure.definitionFingerprint,
                "approach falloff does not alter generated structure") &&
         expect(base.receipt.definitionFingerprint !=
                    widerFalloff.receipt.definitionFingerprint,
                "bridge output fingerprint includes approach recipes") &&
         expect(widerFalloff.approachGrades[0].falloffCells ==
                    widerFalloffRequest.source.settings.approachFalloffCells &&
                    widerFalloff.approachGrades[1].falloffCells ==
                        widerFalloffRequest.source.settings.approachFalloffCells,
                "approach falloff reaches both generated grades");
}

bool bridgeRejectsUnsafeOrUnboundedSources() {
  cr::CreativeBridgeRecipeRequest span = defaultRequest();
  span.source.settings.maximumSpanMeters = 7.0;
  const cr::CreativeBridgeRecipeResult spanRejected =
      cr::planCreativeBridge(span);

  cr::CreativeBridgeRecipeRequest clearance = defaultRequest();
  clearance.source.settings.minimumClearanceMeters = 5.0;
  const cr::CreativeBridgeRecipeResult clearanceRejected =
      cr::planCreativeBridge(clearance);

  cr::CreativeBridgeRecipeRequest grade = defaultRequest();
  grade.source.settings.maximumApproachGradePermille = 499U;
  const cr::CreativeBridgeRecipeResult gradeRejected =
      cr::planCreativeBridge(grade);

  cr::CreativeBridgeRecipeRequest capacity = defaultRequest();
  capacity.crossing.spanMeters = 260.0;
  capacity.source.settings.maximumSpanMeters = 300.0;
  capacity.source.settings.supportSpacingMeters = 1.0;
  const cr::CreativeBridgeRecipeResult capacityRejected =
      cr::planCreativeBridge(capacity);

  cr::CreativeBridgeRecipeRequest overflow = defaultRequest();
  overflow.crossing.spanMeters = std::numeric_limits<double>::max();
  overflow.source.settings.maximumSpanMeters =
      std::numeric_limits<double>::max();
  overflow.source.settings.supportSpacingMeters =
      std::numeric_limits<double>::denorm_min();
  const cr::CreativeBridgeRecipeResult overflowRejected =
      cr::planCreativeBridge(overflow);

  cr::CreativeBridgeRecipeRequest coordinate = defaultRequest();
  coordinate.crossing.leftApproachGrid.x =
      std::numeric_limits<double>::max();
  const cr::CreativeBridgeRecipeResult coordinateRejected =
      cr::planCreativeBridge(coordinate);

  cr::CreativeBridgeRecipeRequest mismatch = defaultRequest();
  mismatch.crossing.id = 8U;
  const cr::CreativeBridgeRecipeResult attachmentRejected =
      cr::planCreativeBridge(mismatch);

  return expect(!spanRejected.receipt.accepted &&
                    spanRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::SpanExceeded,
                "maximum span is enforced") &&
         expect(!clearanceRejected.receipt.accepted &&
                    clearanceRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::ClearanceInsufficient,
                "under-deck clearance is enforced") &&
         expect(!gradeRejected.receipt.accepted &&
                    gradeRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::ApproachGradeExceeded,
                "maximum approach grade is enforced") &&
         expect(!capacityRejected.receipt.accepted &&
                    capacityRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::StructureCapacityExceeded,
                "support generation is bounded") &&
         expect(!overflowRejected.receipt.accepted &&
                    overflowRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::StructureCapacityExceeded,
                "support count overflow rejects before integer conversion") &&
         expect(!coordinateRejected.receipt.accepted &&
                    coordinateRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::ApproachRejected,
                "out-of-range grade coordinates reject before rounding") &&
         expect(!attachmentRejected.receipt.accepted &&
                    attachmentRejected.receipt.status ==
                        cr::CreativeBridgeRecipeStatus::InvalidAttachment,
                "stable crossing id mismatch fails closed");
}

}  // namespace

int main() {
  const bool ok = bridgePlansStructureCollisionMembersAndApproaches() &&
                  bridgeSettingsControlOptionalStructure() &&
                  bridgeOutputFingerprintIncludesApproachGrades() &&
                  bridgeRejectsUnsafeOrUnboundedSources();
  if (!ok) {
    return EXIT_FAILURE;
  }
  std::cout << "creative_bridge_recipe_tests: PASS\n";
  return EXIT_SUCCESS;
}
