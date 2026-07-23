#include "app/iggy3d/creative/world/WorldLayoutOrthogonalRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"
#include "app/iggy3d/creative/world/WorldLayoutWallOperations.hpp"
#include "app/iggy3d/creative/adapters/RoomBake.hpp"
#include "render/vulkan/BufferImageResources.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {
namespace cr = iggy3d::creative;

bool expect(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayout explicitRoom() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "wall_operations";
  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building";
  building.name = "Building";
  building.rootFootprint = {{0, 0}, {10, 6}};
  layout.buildings.push_back(building);
  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "ground";
  level.name = "Ground";
  layout.levels.push_back(level);
  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room";
  room.name = "Room";
  room.footprint = {{0, 0}, {10, 6}};
  layout.rooms.push_back(room);
  const cr::CreativeWorldLayoutRoomGraphMaterializeResult materialized =
      cr::materializeCreativeWorldLayoutRoomGraph(layout);
  return materialized.accepted ? materialized.edited
                               : cr::CreativeWorldLayout{};
}

std::size_t findEdge(const cr::CreativeWorldLayout& layout,
                     cr::CreativeTerrainCoord2 start,
                     cr::CreativeTerrainCoord2 end) {
  for (std::size_t index = 0U; index < layout.topologyEdges.size(); ++index) {
    const cr::CreativeWorldLayoutTopologyEdge& edge =
        layout.topologyEdges[index];
    if (layout.topologyVertices[edge.startVertexIndex].position == start &&
        layout.topologyVertices[edge.endVertexIndex].position == end) {
      return index;
    }
  }
  return cr::kInvalidCreativeWorldLayoutIndex;
}

void appendOpening(cr::CreativeWorldLayout& layout,
                   std::size_t edgeIndex,
                   double center,
                   double width,
                   std::string_view key,
                   cr::CreativeWindowInsertKind treatment) {
  cr::CreativeWorldLayoutOpening opening;
  opening.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  opening.roomIndex = 0U;
  opening.roomEdge = cr::CreativeWorldLayoutRoomEdge::North;
  opening.roomTopologyEdgeIndex = edgeIndex;
  opening.kind = cr::CreativeBuildingOpeningKind::Window;
  opening.window.insertKind = treatment;
  opening.stableKey = key;
  opening.name = "Opening";
  opening.centerOffsetCells = center;
  opening.widthCells = width;
  opening.cutoutBottomCells = 1.0;
  opening.cutoutHeightCells = 1.0;
  opening.insertBottomCells = 1.0;
  opening.insertHeightCells = 1.0;
  layout.openings.push_back(opening);
}

bool splitAndMergePreserveWallSemanticsAndOpenings() {
  cr::CreativeWorldLayout source = explicitRoom();
  const std::size_t north = findEdge(source, {0, 0}, {10, 0});
  if (north >= source.topologyEdges.size()) {
    return expect(false, "north topology edge exists");
  }
  cr::CreativeWorldLayoutTopologyEdge& wall = source.topologyEdges[north];
  wall.wallThicknessCells = 0.4;
  wall.wallHeightCells = 12U;
  wall.profile = cr::CreativeWorldLayoutWallProfile::Exterior;
  wall.material = cr::CreativeStructuralMaterial::Brick;
  wall.joinStyle = cr::CreativeWorldLayoutWallJoinStyle::Square;
  appendOpening(source, north, 2.0, 1.0, "west_window",
                cr::CreativeWindowInsertKind::Glazing);
  appendOpening(source, north, 8.0, 1.0, "east_window",
                cr::CreativeWindowInsertKind::PairedShutters);

  const std::string retainedKey = wall.stableKey;
  const cr::CreativeWorldLayoutWallOperationResult split =
      cr::splitCreativeWorldLayoutWall(
          source, {north, 5U, "north_midpoint", "north_east"});
  if (!expect(split.accepted && split.changed,
              "valid wall split is accepted atomically")) {
    return false;
  }
  const cr::CreativeWorldLayoutTopologyEdge& first =
      split.edited.topologyEdges[split.topologyEdgeIndex];
  const cr::CreativeWorldLayoutTopologyEdge& second =
      split.edited.topologyEdges[split.newTopologyEdgeIndex];
  const bool attributesPreserved =
      first.wallThicknessCells == 0.4 && second.wallThicknessCells == 0.4 &&
      first.wallHeightCells == 12U && second.wallHeightCells == 12U &&
      first.profile == cr::CreativeWorldLayoutWallProfile::Exterior &&
      second.profile == cr::CreativeWorldLayoutWallProfile::Exterior &&
      first.material == cr::CreativeStructuralMaterial::Brick &&
      second.material == cr::CreativeStructuralMaterial::Brick &&
      first.joinStyle == cr::CreativeWorldLayoutWallJoinStyle::Square &&
      second.joinStyle == cr::CreativeWorldLayoutWallJoinStyle::Square;
  const bool openingsPreserved =
      split.edited.openings[0].roomTopologyEdgeIndex ==
          split.topologyEdgeIndex &&
      split.edited.openings[0].centerOffsetCells == 2.0 &&
      split.edited.openings[0].window.insertKind ==
          cr::CreativeWindowInsertKind::Glazing &&
      split.edited.openings[1].roomTopologyEdgeIndex ==
          split.newTopologyEdgeIndex &&
      split.edited.openings[1].centerOffsetCells == 3.0 &&
      split.edited.openings[1].window.insertKind ==
          cr::CreativeWindowInsertKind::PairedShutters;

  const cr::CreativeWorldLayoutWallOperationResult merged =
      cr::mergeCreativeWorldLayoutWalls(
          split.edited,
          {split.topologyEdgeIndex, split.newTopologyEdgeIndex});
  const cr::CreativeWorldLayoutRoomGraph mergedGraph =
      merged.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(merged.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  return expect(attributesPreserved,
                "both split spans inherit the complete wall contract") &&
         expect(openingsPreserved,
                "openings retain physical placement across split hosts") &&
         expect(merged.accepted && merged.changed && mergedGraph.accepted,
                "compatible spans merge back to valid topology") &&
         expect(merged.edited.topologyEdges.size() ==
                        source.topologyEdges.size() &&
                    merged.edited.topologyVertices.size() ==
                        source.topologyVertices.size() &&
                    merged.edited.topologyEdges[merged.topologyEdgeIndex]
                            .stableKey == retainedKey,
                "merge removes only the split junction and retains primary identity") &&
         expect(merged.edited.openings[0].roomTopologyEdgeIndex ==
                        merged.topologyEdgeIndex &&
                    merged.edited.openings[0].centerOffsetCells == 2.0 &&
                    merged.edited.openings[1].roomTopologyEdgeIndex ==
                        merged.topologyEdgeIndex &&
                    merged.edited.openings[1].centerOffsetCells == 8.0 &&
                    merged.edited.openings[1].window.insertKind ==
                        cr::CreativeWindowInsertKind::PairedShutters,
                "merge restores window offsets and treatment semantics");
}

bool reverseBoundarySplitRemainsClosed() {
  cr::CreativeWorldLayout source = explicitRoom();
  const std::size_t south = findEdge(source, {0, 6}, {10, 6});
  const cr::CreativeWorldLayoutWallOperationResult split =
      cr::splitCreativeWorldLayoutWall(
          source, {south, 4U, "south_midpoint", "south_east"});
  const cr::CreativeWorldLayoutRoomGraph graph =
      split.accepted
          ? cr::buildCreativeWorldLayoutRoomGraph(split.edited)
          : cr::CreativeWorldLayoutRoomGraph{};
  return expect(split.accepted && graph.accepted,
                "reverse-oriented room boundary splits without opening loop") &&
         expect(graph.rooms[0].boundaryCount == 5U,
                "reverse boundary gains exactly one relational segment");
}

bool splitOpeningConflictAndMergeAttributeMismatchReject() {
  cr::CreativeWorldLayout source = explicitRoom();
  const std::size_t north = findEdge(source, {0, 0}, {10, 0});
  appendOpening(source, north, 5.0, 1.0, "crossing_window",
                cr::CreativeWindowInsertKind::Glazing);
  const cr::CreativeWorldLayoutWallOperationResult conflict =
      cr::splitCreativeWorldLayoutWall(
          source, {north, 5U, "midpoint", "east_half"});

  source.openings.clear();
  const cr::CreativeWorldLayoutWallOperationResult split =
      cr::splitCreativeWorldLayoutWall(
          source, {north, 5U, "midpoint", "east_half"});
  cr::CreativeWorldLayout mismatch = split.edited;
  mismatch.topologyEdges[split.newTopologyEdgeIndex].material =
      cr::CreativeStructuralMaterial::Stone;
  const cr::CreativeWorldLayoutWallOperationResult merge =
      cr::mergeCreativeWorldLayoutWalls(
          mismatch, {split.topologyEdgeIndex, split.newTopologyEdgeIndex});

  return expect(!conflict.accepted && !conflict.changed &&
                    conflict.status ==
                        cr::CreativeWorldLayoutWallOperationStatus::OpeningConflict &&
                    conflict.failedOpeningIndex == 0U &&
                    conflict.edited.topologyEdges.empty(),
                "split crossing an opening rejects with no partial layout") &&
         expect(split.accepted && !merge.accepted && !merge.changed &&
                    merge.status ==
                        cr::CreativeWorldLayoutWallOperationStatus::AttributesDiffer,
                "merge never discards differing wall semantics");
}

bool wallContractReachesBakeAndRenderWithoutChangingGeometry() {
  cr::CreativeWorldLayout source = explicitRoom();
  const std::size_t north = findEdge(source, {0, 0}, {10, 0});
  cr::CreativeWorldLayoutTopologyEdge& edge = source.topologyEdges[north];
  edge.wallThicknessCells = 0.4;
  edge.wallHeightCells = 5U;
  edge.profile = cr::CreativeWorldLayoutWallProfile::Exterior;
  edge.material = cr::CreativeStructuralMaterial::Brick;

  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      cr::expandCreativeWorldLayoutRooms(source);
  std::size_t expandedWallIndex = cr::kInvalidCreativeWorldLayoutIndex;
  for (std::size_t wallIndex = 0U;
       wallIndex < expanded.wallProvenance.size(); ++wallIndex) {
    const auto& contributors = expanded.wallProvenance[wallIndex].contributors;
    if (std::any_of(contributors.begin(), contributors.end(),
                    [north](const auto& contributor) {
                      return contributor.topologyEdgeIndex == north;
                    })) {
      expandedWallIndex = wallIndex;
      break;
    }
  }

  cr::CreativeDocument document = cr::CreativeDocument::create("Walls");
  static_cast<void>(document.assignId(14001U));
  const cr::CreativeWorldLayoutCompileResult plan =
      cr::buildCreativeWorldLayoutPlan(document, source);
  const cr::CreativeWorldLayoutPreviewResult preview =
      cr::previewCreativeWorldLayoutPlan(document, plan.plan);
  const cr::CreativeObject* brickObject = nullptr;
  for (const cr::CreativeObject& object : preview.document.objects()) {
    cr::CreativeStructuralMaterial material =
        cr::CreativeStructuralMaterial::Count;
    if (object.kind == cr::CreativeObjectKind::Wall &&
        cr::parseCreativeStructuralMaterialTag(object.tags, material) &&
        material == cr::CreativeStructuralMaterial::Brick) {
      brickObject = &object;
      break;
    }
  }
  cr::CreativeRoomBakeRequest bakeRequest;
  bakeRequest.document = &preview.document;
  bakeRequest.validateReachability = false;
  const cr::CreativeRoomBakeResult baked =
      cr::buildRoomAssetFromCreativeDocument(bakeRequest);
  const iggy3d::RoomStaticMeshAsset* brickMesh = nullptr;
  if (brickObject != nullptr) {
    const std::string id =
        "creative_object_" + std::to_string(brickObject->id);
    const auto found = std::find_if(
        baked.room.staticMeshes.begin(), baked.room.staticMeshes.end(),
        [&id](const iggy3d::RoomStaticMeshAsset& mesh) {
          return mesh.id == id;
        });
    if (found != baked.room.staticMeshes.end()) {
      brickMesh = &*found;
    }
  }

  iggy3d::SceneRoomProjection projected;
  if (brickMesh != nullptr) {
    iggy3d::SceneRoomMeshItem mesh;
    mesh.id = brickMesh->id;
    mesh.meshId = brickMesh->meshId;
    mesh.role = brickMesh->role;
    mesh.materialId = brickMesh->materialId;
    mesh.position = brickMesh->positionMeters;
    mesh.size = brickMesh->sizeMeters;
    mesh.rotationEulerRadians = brickMesh->rotationEulerRadians;
    mesh.hasWallSegment = brickMesh->hasWallSegment;
    mesh.wallStartMeters = brickMesh->wallStartMeters;
    mesh.wallEndMeters = brickMesh->wallEndMeters;
    mesh.wallBottomY = brickMesh->wallBottomY;
    mesh.wallHeightMeters = brickMesh->wallHeightMeters;
    mesh.wallThicknessMeters = brickMesh->wallThicknessMeters;
    projected.meshes.push_back(std::move(mesh));
  }
  const iggy3d::vulkan::RoomMeshCpuGeometry geometry =
      iggy3d::vulkan::buildRoomMeshCpuGeometry(projected);

  const cr::CreativeWorldLayoutWall* expandedWall =
      expandedWallIndex < expanded.expanded.walls.size()
          ? &expanded.expanded.walls[expandedWallIndex]
          : nullptr;
  return expect(expanded.accepted && expandedWall != nullptr &&
                    expandedWall->heightCells == 5U &&
                    expandedWall->thicknessCells == 0.4 &&
                    expandedWall->profile ==
                        cr::CreativeWorldLayoutWallProfile::Exterior &&
                    expandedWall->material ==
                        cr::CreativeStructuralMaterial::Brick,
                "graph expansion preserves authored wall semantics") &&
         expect(plan.receipt.accepted && preview.accepted &&
                    brickObject != nullptr && baked.receipt.accepted &&
                    brickMesh != nullptr &&
                    brickMesh->materialId == "creative_wall_brick" &&
                    brickMesh->sizeMeters.y == 5.0F &&
                    std::min(brickMesh->sizeMeters.x,
                             brickMesh->sizeMeters.z) == 0.4F,
                "document and room bake retain material and exact wall bounds") &&
         expect(geometry.ready && geometry.roomWallDrawCount == 1U &&
                    !geometry.vertices.empty() &&
                    geometry.vertices.front().color[0] == 0.55F &&
                    geometry.vertices.front().color[1] == 0.25F &&
                    geometry.vertices.front().color[2] == 0.18F,
                "optimized procedural render selects the brick palette color");
}

}  // namespace

int main() {
  return splitAndMergePreserveWallSemanticsAndOpenings() &&
                 reverseBoundarySplitRemainsClosed() &&
                 splitOpeningConflictAndMergeAttributeMismatchReject() &&
                 wallContractReachesBakeAndRenderWithoutChangingGeometry()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
