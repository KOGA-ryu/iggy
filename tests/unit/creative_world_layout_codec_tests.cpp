#include "app/iggy3d/creative/world/WorldLayoutCodec.hpp"
#include "app/iggy3d/creative/world/WorldLayoutRooms.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace cr = iggy3d::creative;

namespace {

bool expect(bool condition, const char* message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
  }
  return condition;
}

cr::CreativeWorldLayout richLayout() {
  cr::CreativeWorldLayout layout;
  layout.stableKey = "estate layout\nlevel=0%";
  layout.terrainOwnership = cr::CreativeWorldLayoutTerrainOwnership::ReplaceAll;

  cr::CreativeWorldLayoutBuilding building;
  building.stableKey = "building 1";
  building.name = "Main House\nNorth Wing";
  building.rootMode = cr::CreativeBuildingRootMode::CreateRoom;
  building.rootFootprint = {{-4, -3}, {8, 7}};
  building.rootBaseLayer = -1;
  building.rootHeightCells = 4U;
  building.tags = {"interior", "author=map maker"};
  layout.buildings.push_back(building);

  cr::CreativeWorldLayoutLevel level;
  level.buildingIndex = 0U;
  level.stableKey = "level.ground";
  level.name = "Ground Level";
  level.floorTopLayer = 1.25;
  level.wallHeightCells = 4U;
  level.floorThicknessLayers = 2U;
  level.ceilingThicknessLayers = 2U;
  level.roofThicknessLayers = 3U;
  level.roofStyle = cr::CreativeStructuralRoofStyle::Gable;
  level.roofRidgeAxis = cr::CreativeStructuralRoofRidgeAxis::Z;
  level.roofPitchDegrees = 37.5;
  level.roofOverhangCells = 0.75;
  layout.levels.push_back(level);

  cr::CreativeWorldLayoutLevel upperLevel = level;
  upperLevel.stableKey = "level.upper";
  upperLevel.name = "Upper Level";
  upperLevel.floorTopLayer = 5.25;
  layout.levels.push_back(upperLevel);

  cr::CreativeWorldLayoutRoom room;
  room.buildingIndex = 0U;
  room.levelIndex = 0U;
  room.stableKey = "room.study";
  room.name = "Study";
  room.footprint = {{0, 0}, {8, 6}};
  room.wallThicknessCells = 0.375;
  layout.rooms.push_back(room);

  cr::CreativeWorldLayoutRoom upperRoom = room;
  upperRoom.levelIndex = 1U;
  upperRoom.stableKey = "room.upper";
  upperRoom.name = "Upper Hall";
  layout.rooms.push_back(upperRoom);

  cr::CreativeWorldLayoutVerticalConnector stair;
  stair.buildingIndex = 0U;
  stair.lowerRoomIndex = 0U;
  stair.upperRoomIndex = 1U;
  stair.kind = cr::CreativeWorldLayoutVerticalConnectorKind::Stair;
  stair.direction = cr::CreativeWorldLayoutVerticalDirection::PositiveX;
  stair.stableKey = "stair.main";
  stair.name = "Main Stair";
  stair.footprint = {{1, 2}, {5, 4}};
  layout.verticalConnectors.push_back(stair);

  cr::CreativeWorldLayoutBox floor;
  floor.buildingIndex = 0U;
  floor.kind = cr::CreativeObjectKind::Floor;
  floor.stableKey = "floor.main";
  floor.name = "Ground Floor";
  floor.footprint = {{-4, -3}, {8, 7}};
  floor.anchorLayer = 1.75;
  floor.layerCount = 2U;
  layout.boxes.push_back(floor);

  cr::CreativeWorldLayoutWall wall;
  wall.buildingIndex = 0U;
  wall.stableKey = "wall.north";
  wall.name = "North Wall";
  wall.start = {-4, -3};
  wall.end = {8, -3};
  wall.heightCells = 4U;
  wall.thicknessCells = 0.375;
  layout.walls.push_back(wall);

  cr::CreativeWorldLayoutOpening opening;
  opening.wallIndex = 0U;
  opening.kind = cr::CreativeBuildingOpeningKind::Window;
  opening.stableKey = "window.north.1";
  opening.name = "Window = 1";
  opening.centerOffsetCells = 5.25;
  opening.widthCells = 1.5;
  opening.cutoutBottomCells = 1.0;
  opening.cutoutHeightCells = 1.25;
  opening.insertBottomCells = 1.0;
  opening.insertHeightCells = 1.25;
  opening.insertWidthCells = 1.5;
  opening.insertThicknessCells = 0.1;
  opening.insertAssetId = "homestead/modular/window_frame_1p5x1p2";
  opening.insertAssetSourceBoundsMeters =
      {{-0.75, 0.0, -0.05}, {0.75, 1.2, 0.05}};
  opening.hasInsertAssetSourceBounds = true;
  layout.openings.push_back(opening);

  cr::CreativeWorldLayoutOpening roomDoor;
  roomDoor.hostKind = cr::CreativeWorldLayoutOpeningHostKind::RoomEdge;
  roomDoor.roomIndex = 0U;
  roomDoor.roomEdge = cr::CreativeWorldLayoutRoomEdge::South;
  roomDoor.kind = cr::CreativeBuildingOpeningKind::Door;
  roomDoor.stableKey = "door.study";
  roomDoor.name = "Study Door";
  roomDoor.centerOffsetCells = 2.0;
  layout.openings.push_back(roomDoor);

  cr::CreativeWorldLayoutObject object;
  object.kind = cr::CreativeObjectKind::Rock;
  object.mode = cr::CreativeObjectLibraryPlacementMode::Bounds;
  object.stableKey = "rock.imported";
  object.name = "Imported Boulder";
  object.assetId = "boulder_01";
  object.boundsCells = {{2.25, 1.0, -4.5}, {4.75, 3.0, -2.0}};
  object.tags = {"prop", "source=blender"};
  layout.objects.push_back(object);

  cr::CreativeWorldLayoutObject posedAsset;
  posedAsset.kind = cr::CreativeObjectKind::Prop;
  posedAsset.mode = cr::CreativeObjectLibraryPlacementMode::Point;
  posedAsset.stableKey = "prop.posed";
  posedAsset.name = "Posed Catalog Asset";
  posedAsset.assetId = "homestead/interior/dresser_1p3";
  posedAsset.pointCells = {6.0, 0.5, -3.0};
  posedAsset.assetSourceBoundsMeters =
      {{-0.65, 0.0, -0.3}, {0.65, 1.1, 0.3}};
  posedAsset.hasAssetSourceBounds = true;
  posedAsset.yawRadians = 0.7853981633974483;
  posedAsset.scale = {1.25, 0.75, 1.5};
  posedAsset.tags = {"world_layout:catalog_asset"};
  layout.objects.push_back(posedAsset);

  cr::CreativeWorldLayoutTerrainProfile profile;
  profile.stableKey = "hill.west";
  profile.kind = cr::CreativeTerrainRecipeKind::Hill;
  profile.center = {-10, 2};
  profile.baseHeightCells = 2U;
  profile.radiusCells = 8U;
  profile.amplitudeCells = 5U;
  profile.spacingCells = 2U;
  profile.frequency = 2U;
  layout.terrainProfiles.push_back(profile);

  cr::CreativeWorldLayoutTerrainPath path;
  path.stableKey = "road.entry";
  path.kind = cr::CreativeTerrainRecipeKind::Road;
  path.firstPointIndex = 0U;
  path.pointCount = 3U;
  path.elevation = cr::CreativeTerrainPathElevation::Level;
  path.halfWidthCells = 2U;
  path.amplitudeCells = 1U;
  path.paintSurface = true;
  path.material = cr::CreativeTerrainMaterial::Dirt;
  layout.terrainPaths.push_back(path);
  layout.terrainPathPoints = {{{-6, 8}, 2U}, {{0, 8}, 2U}, {{0, 4}, 2U}};
  return layout;
}

bool deterministicRoundTripPreservesEveryTable() {
  const cr::CreativeWorldLayout source = richLayout();
  const cr::CreativeWorldLayoutEncodeResult first =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutEncodeResult second =
      cr::encodeCreativeWorldLayout(source);
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(first.encodedText);
  const cr::CreativeWorldLayoutEncodeResult reencoded =
      cr::encodeCreativeWorldLayout(decoded.layout);

  return expect(first.accepted && second.accepted && decoded.accepted &&
                    reencoded.accepted,
                "rich layout codec operations accepted") &&
         expect(first.encodedText == second.encodedText &&
                    first.encodedText == reencoded.encodedText,
                "layout codec is byte deterministic") &&
         expect(decoded.layout.stableKey == source.stableKey,
                "special layout key round trips") &&
         expect(
             decoded.layout.buildings.size() == 1U &&
                 decoded.layout.buildings[0].name == source.buildings[0].name &&
                 decoded.layout.buildings[0].tags == source.buildings[0].tags,
             "building strings and tags round trip") &&
         expect(decoded.layout.levels.size() == 2U &&
                    decoded.layout.levels[0].floorTopLayer == 1.25 &&
                    decoded.layout.levels[0].ceilingThicknessLayers == 2U &&
                    decoded.layout.levels[0].roofThicknessLayers == 3U &&
                    decoded.layout.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Gable &&
                    decoded.layout.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::Z &&
                    decoded.layout.levels[0].roofPitchDegrees == 37.5 &&
                    decoded.layout.levels[0].roofOverhangCells == 0.75 &&
                    decoded.layout.rooms.size() == 2U &&
                    decoded.layout.rooms[0].footprint.maximum ==
                        source.rooms[0].footprint.maximum &&
                    decoded.layout.rooms[0].levelIndex == 0U &&
                    decoded.layout.verticalConnectors.size() == 1U &&
                    decoded.layout.verticalConnectors[0].lowerRoomIndex == 0U &&
                    decoded.layout.verticalConnectors[0].upperRoomIndex == 1U &&
                    decoded.layout.verticalConnectors[0].direction ==
                        cr::CreativeWorldLayoutVerticalDirection::PositiveX &&
                    decoded.layout.verticalConnectors[0].footprint.minimum ==
                        source.verticalConnectors[0].footprint.minimum &&
                    decoded.layout.boxes.size() == 1U &&
                    decoded.layout.boxes[0].anchorLayer == 1.75 &&
                    decoded.layout.boxes[0].layerCount == 2U &&
                    decoded.layout.walls.size() == 1U &&
                    decoded.layout.openings.size() == 2U &&
                    decoded.layout.openings[0].insertAssetId ==
                        source.openings[0].insertAssetId &&
                    decoded.layout.openings[0]
                        .hasInsertAssetSourceBounds &&
                    decoded.layout.openings[0]
                            .insertAssetSourceBoundsMeters.max.y == 1.2 &&
                    decoded.layout.openings[1].hostKind ==
                        cr::CreativeWorldLayoutOpeningHostKind::RoomEdge,
                "building, room, connector, and opening tables round trip") &&
         expect(decoded.layout.objects.size() == 2U &&
                    decoded.layout.objects[0].assetId == "boulder_01" &&
                    decoded.layout.objects[0].tags == source.objects[0].tags &&
                    decoded.layout.objects[0].boundsCells.min.x == 2.25 &&
                    decoded.layout.objects[1].hasAssetSourceBounds &&
                    decoded.layout.objects[1].assetSourceBoundsMeters.min.x ==
                        -0.65 &&
                    decoded.layout.objects[1].yawRadians ==
                        source.objects[1].yawRadians &&
                    decoded.layout.objects[1].scale.z == 1.5,
                "object-library symbols and pose round trip") &&
         expect(decoded.layout.terrainProfiles.size() == 1U &&
                    decoded.layout.terrainPaths.size() == 1U &&
                    decoded.layout.terrainPathPoints ==
                        source.terrainPathPoints,
                "terrain symbol tables round trip");
}

bool malformedAndNonFiniteInputsFailClosed() {
  cr::CreativeWorldLayout wrongEncodeSchema = richLayout();
  wrongEncodeSchema.schemaVersion = 99U;
  const cr::CreativeWorldLayoutEncodeResult invalidSchema =
      cr::encodeCreativeWorldLayout(wrongEncodeSchema);

  cr::CreativeWorldLayout layout = richLayout();
  layout.walls[0].thicknessCells = std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutEncodeResult nonFinite =
      cr::encodeCreativeWorldLayout(layout);
  cr::CreativeWorldLayout badObject = richLayout();
  badObject.objects[0].pointCells.y =
      std::numeric_limits<double>::infinity();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteObject =
      cr::encodeCreativeWorldLayout(badObject);
  cr::CreativeWorldLayout badBox = richLayout();
  badBox.boxes[0].anchorLayer =
      std::numeric_limits<double>::quiet_NaN();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteBox =
      cr::encodeCreativeWorldLayout(badBox);
  cr::CreativeWorldLayout badRoofStyle = richLayout();
  badRoofStyle.levels[0].roofStyle =
      cr::CreativeStructuralRoofStyle::Count;
  const cr::CreativeWorldLayoutEncodeResult invalidRoofStyle =
      cr::encodeCreativeWorldLayout(badRoofStyle);
  cr::CreativeWorldLayout badRoofPitch = richLayout();
  badRoofPitch.levels[0].roofPitchDegrees =
      std::numeric_limits<double>::infinity();
  const cr::CreativeWorldLayoutEncodeResult nonFiniteRoofPitch =
      cr::encodeCreativeWorldLayout(badRoofPitch);
  cr::CreativeWorldLayout badObjectScale = richLayout();
  badObjectScale.objects[1].scale.x = 0.0;
  const cr::CreativeWorldLayoutEncodeResult invalidObjectScale =
      cr::encodeCreativeWorldLayout(badObjectScale);
  cr::CreativeWorldLayout badObjectSourceBounds = richLayout();
  badObjectSourceBounds.objects[1].assetSourceBoundsMeters.max.x =
      badObjectSourceBounds.objects[1].assetSourceBoundsMeters.min.x;
  const cr::CreativeWorldLayoutEncodeResult invalidObjectSourceBounds =
      cr::encodeCreativeWorldLayout(badObjectSourceBounds);
  cr::CreativeWorldLayout badOpeningSourceBounds = richLayout();
  badOpeningSourceBounds.openings[0].insertAssetSourceBoundsMeters.max.x =
      badOpeningSourceBounds.openings[0].insertAssetSourceBoundsMeters.min.x;
  const cr::CreativeWorldLayoutEncodeResult invalidOpeningSourceBounds =
      cr::encodeCreativeWorldLayout(badOpeningSourceBounds);
  cr::CreativeWorldLayout mismatchedOpeningAsset = richLayout();
  mismatchedOpeningAsset.openings[0].hasInsertAssetSourceBounds = false;
  const cr::CreativeWorldLayoutEncodeResult invalidOpeningAssetContract =
      cr::encodeCreativeWorldLayout(mismatchedOpeningAsset);
  cr::CreativeWorldLayout cutoutOnlyAsset = richLayout();
  cutoutOnlyAsset.openings[0].includeInsert = false;
  const cr::CreativeWorldLayoutEncodeResult cutoutOnlyEncoded =
      cr::encodeCreativeWorldLayout(cutoutOnlyAsset);
  const cr::CreativeWorldLayoutDecodeResult cutoutOnlyDecoded =
      cr::decodeCreativeWorldLayout(cutoutOnlyEncoded.encodedText);

  const cr::CreativeWorldLayoutEncodeResult valid =
      cr::encodeCreativeWorldLayout(richLayout());
  const cr::CreativeWorldLayoutDecodeResult truncated =
      cr::decodeCreativeWorldLayout(
          valid.encodedText.substr(0U, valid.encodedText.find("END")));
  std::string unsupported = valid.encodedText;
  const std::string currentHeader =
      "IGGY3D_WORLD_LAYOUT " +
      std::to_string(cr::kCreativeWorldLayoutCodecVersion);
  unsupported.replace(unsupported.find(currentHeader), currentHeader.size(),
                      "IGGY3D_WORLD_LAYOUT 99");
  const cr::CreativeWorldLayoutDecodeResult wrongVersion =
      cr::decodeCreativeWorldLayout(unsupported);
  std::string wrongSchema = valid.encodedText;
  const std::string currentLayoutPrefix =
      "L " + std::to_string(cr::kCreativeWorldLayoutSchemaVersion) + " ";
  wrongSchema.replace(wrongSchema.find(currentLayoutPrefix),
                      currentLayoutPrefix.size(), "L 99 ");
  const cr::CreativeWorldLayoutDecodeResult schemaMismatch =
      cr::decodeCreativeWorldLayout(wrongSchema);
  const std::string overflowingCount =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 776f726c645f6c61796f7574 0 18446744073709551615 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult overflow =
      cr::decodeCreativeWorldLayout(overflowingCount);
  std::string invalidRoofEnumText = valid.encodedText;
  const std::size_t levelLine = invalidRoofEnumText.find("\nV ");
  const std::string validRoofFields = " 1 1 37.5 0.75\n";
  const std::size_t roofFields =
      invalidRoofEnumText.find(validRoofFields, levelLine);
  const bool locatedRoofFields =
      levelLine != std::string::npos && roofFields != std::string::npos;
  if (locatedRoofFields) {
    invalidRoofEnumText.replace(roofFields, validRoofFields.size(),
                                " 9 1 37.5 0.75\n");
  }
  const cr::CreativeWorldLayoutDecodeResult invalidRoofEnum =
      cr::decodeCreativeWorldLayout(invalidRoofEnumText);

  return expect(!invalidSchema.accepted &&
                    invalidSchema.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "obsolete in-memory schema is not encoded") &&
         expect(!nonFinite.accepted &&
                    nonFinite.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite source is not encoded") &&
         expect(!nonFiniteObject.accepted &&
                    nonFiniteObject.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite object symbol is not encoded") &&
         expect(!nonFiniteBox.accepted &&
                    nonFiniteBox.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite box anchor is not encoded") &&
         expect(!invalidRoofStyle.accepted &&
                    invalidRoofStyle.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "invalid roof style is not encoded") &&
         expect(!nonFiniteRoofPitch.accepted &&
                    nonFiniteRoofPitch.status ==
                        cr::CreativeWorldLayoutCodecStatus::NonFiniteValue,
                "non-finite roof pitch is not encoded") &&
         expect(!invalidObjectScale.accepted &&
                    invalidObjectScale.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "non-positive catalog scale is not encoded") &&
         expect(!invalidObjectSourceBounds.accepted &&
                    invalidObjectSourceBounds.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "degenerate catalog source bounds are not encoded") &&
         expect(!invalidOpeningSourceBounds.accepted &&
                    invalidOpeningSourceBounds.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "degenerate opening asset bounds are not encoded") &&
         expect(!invalidOpeningAssetContract.accepted &&
                    invalidOpeningAssetContract.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "partial opening asset metadata is not encoded") &&
         expect(cutoutOnlyEncoded.accepted && cutoutOnlyDecoded.accepted &&
                    !cutoutOnlyDecoded.layout.openings[0].includeInsert &&
                    cutoutOnlyDecoded.layout.openings[0].insertAssetId ==
                        cutoutOnlyAsset.openings[0].insertAssetId,
                "cutout-only opening retains its dormant asset identity") &&
         expect(!truncated.accepted, "truncated source is rejected") &&
         expect(!wrongVersion.accepted &&
                    wrongVersion.status ==
                        cr::CreativeWorldLayoutCodecStatus::UnsupportedVersion,
                "unsupported codec version is rejected") &&
         expect(!schemaMismatch.accepted &&
                    schemaMismatch.status ==
                        cr::CreativeWorldLayoutCodecStatus::InvalidRecord,
                "codec and source schema mismatch is rejected") &&
         expect(!overflow.accepted &&
                    overflow.status ==
                        cr::CreativeWorldLayoutCodecStatus::CapacityExceeded,
                "overflowing declared record counts fail before allocation") &&
         expect(locatedRoofFields && !invalidRoofEnum.accepted,
                "invalid serialized roof enum fails closed");
}

bool versionOneSourceMigratesToCurrentSchema() {
  const std::string versionOne =
      "IGGY3D_WORLD_LAYOUT 1\n"
      "L 1 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionOne);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(decoded.layout);
  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.rooms.empty(),
                "version-one source migrates without fabricated rooms") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "migrated source writes the current codec version");
}

bool versionTwoSourceMigratesWithoutFabricatedObjects() {
  const std::string versionTwo =
      "IGGY3D_WORLD_LAYOUT 2\n"
      "L 2 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionTwo);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      cr::encodeCreativeWorldLayout(decoded.layout);
  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.objects.empty(),
                "version-two source migrates without fabricated objects") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-two migration writes the current codec version");
}

bool versionThreeRoomPreservesWallPlaneDuringMigration() {
  const std::string versionThree =
      "IGGY3D_WORLD_LAYOUT 3\n"
      "L 3 6c65676163795f726f6f6d 0 1 1 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "R 0 726f6f6d 526f6f6d 0 0 4 4 2 3 0.25 2\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionThree);
  const cr::CreativeWorldLayoutRoomCompileResult expanded =
      decoded.accepted ? cr::expandCreativeWorldLayoutRooms(decoded.layout)
                       : cr::CreativeWorldLayoutRoomCompileResult{};
  return expect(decoded.accepted && decoded.layout.rooms.size() == 1U &&
                    decoded.layout.levels.size() == 1U &&
                    decoded.layout.rooms[0].levelIndex == 0U &&
                    decoded.layout.levels[0].floorTopLayer == 3.0 &&
                    decoded.layout.levels[0].floorThicknessLayers == 2U,
                "version-three room migrates base plus half thickness") &&
         expect(expanded.accepted && !expanded.expanded.walls.empty() &&
                    expanded.expanded.walls[0].baseLayer == 3.0,
                "version-three migration preserves the generated wall plane");
}

bool versionFourBoxesMigrateToExplicitAnchorPlanes() {
  const std::string versionFour =
      "IGGY3D_WORLD_LAYOUT 4\n"
      "L 4 6c65676163795f7375726661636573 0 1 0 2 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "X 0 " +
      std::to_string(static_cast<unsigned>(cr::CreativeObjectKind::Floor)) +
      " 666c6f6f72 466c6f6f72 0 0 4 4 2 2\n"
      "X 0 " +
      std::to_string(static_cast<unsigned>(cr::CreativeObjectKind::Roof)) +
      " 726f6f66 526f6f66 0 0 4 4 7 1\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionFour);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.boxes.size() == 2U &&
                    decoded.layout.boxes[0].anchorLayer == 2.0 &&
                    decoded.layout.boxes[0].layerCount == 2U &&
                    decoded.layout.boxes[1].anchorLayer == 7.0 &&
                    decoded.layout.boxes[1].layerCount == 1U,
                "version-four box layers migrate to explicit anchor planes") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "migrated box source writes current schema");
}

bool versionFiveRoomsMigrateToSharedLevels() {
  const std::string versionFive =
      "IGGY3D_WORLD_LAYOUT 5\n"
      "L 5 6c65676163795f6c61796f7574 0 1 1 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "R 0 726f6f6d 526f6f6d 0 0 4 4 2.5 4 0.25 2\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionFive);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.rooms.size() == 1U &&
                    decoded.layout.levels.size() == 1U &&
                    decoded.layout.rooms[0].levelIndex == 0U &&
                    decoded.layout.levels[0].buildingIndex == 0U &&
                    decoded.layout.levels[0].floorTopLayer == 2.5 &&
                    decoded.layout.levels[0].wallHeightCells == 4U &&
                    decoded.layout.levels[0].floorThicknessLayers == 2U &&
                    decoded.layout.levels[0].ceilingThicknessLayers == 1U &&
                    decoded.layout.levels[0].roofThicknessLayers == 1U,
                "version-five room geometry migrates into one shared level") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-five migration writes the current codec version");
}

bool versionSixSourceMigratesWithoutFabricatedVerticalConnectors() {
  const std::string versionSix =
      "IGGY3D_WORLD_LAYOUT 6\n"
      "L 6 6c65676163795f6c61796f7574 0 0 0 0 0 0 0 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionSix);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.verticalConnectors.empty(),
                "version-six source migrates with an empty connector table") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-six migration writes the current codec version");
}

bool versionSevenLevelsMigrateToFlatRoofDefaults() {
  const std::string versionSeven =
      "IGGY3D_WORLD_LAYOUT 7\n"
      "L 7 6c65676163795f726f6f66 0 1 1 1 0 0 0 0 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 4 4 0 3 1 0\n"
      "V 0 6c6576656c 4c6576656c 0 3 1 1 1\n"
      "R 0 0 726f6f6d 526f6f6d 0 0 4 4 0.25\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionSeven);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};
  return expect(decoded.accepted && decoded.layout.levels.size() == 1U &&
                    decoded.layout.levels[0].roofStyle ==
                        cr::CreativeStructuralRoofStyle::Flat &&
                    decoded.layout.levels[0].roofRidgeAxis ==
                        cr::CreativeStructuralRoofRidgeAxis::X &&
                    decoded.layout.levels[0].roofPitchDegrees ==
                        cr::kDefaultCreativeStructuralRoofPitchDegrees &&
                    decoded.layout.levels[0].roofOverhangCells == 0.0,
                "version-seven level migrates to stable flat roof defaults") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-seven roof migration writes current schema");
}

bool versionEightObjectsMigrateToIdentityCatalogPose() {
  const std::string versionEight =
      "IGGY3D_WORLD_LAYOUT 8\n"
      "L 8 6c6567616379 0 0 0 0 0 0 0 0 1 0 0 0\n"
      "Y " +
      std::to_string(static_cast<unsigned>(cr::CreativeObjectKind::Rock)) +
      " 1 726f636b 526f636b 626f756c6465725f3031 1 "
      "0 0 0 1 1 1 4 2 6 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionEight);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.objects.size() == 1U &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    !decoded.layout.objects[0].hasAssetSourceBounds &&
                    decoded.layout.objects[0].yawRadians == 0.0 &&
                    decoded.layout.objects[0].scale.x == 1.0 &&
                    decoded.layout.objects[0].scale.y == 1.0 &&
                    decoded.layout.objects[0].scale.z == 1.0,
                "version-eight objects migrate to identity catalog pose") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-eight migration writes current object fields");
}

bool versionNineOpeningsMigrateToProceduralInserts() {
  const std::string versionNine =
      "IGGY3D_WORLD_LAYOUT 9\n"
      "L 9 6c6567616379 0 1 0 0 0 0 1 1 0 0 0 0\n"
      "B 686f757365 486f757365 0 0 0 8 4 0 3 1 0\n"
      "W 0 77616c6c 57616c6c 0 0 8 0 0 3 0.25\n"
      "O 0 0 0 0 0 0 646f6f72 446f6f72 4 1 0 2.1 1 0 0 0 0\n"
      "END\n";
  const cr::CreativeWorldLayoutDecodeResult decoded =
      cr::decodeCreativeWorldLayout(versionNine);
  const cr::CreativeWorldLayoutEncodeResult encoded =
      decoded.accepted ? cr::encodeCreativeWorldLayout(decoded.layout)
                       : cr::CreativeWorldLayoutEncodeResult{};

  return expect(decoded.accepted && decoded.layout.openings.size() == 1U &&
                    decoded.layout.schemaVersion ==
                        cr::kCreativeWorldLayoutSchemaVersion &&
                    decoded.layout.openings[0].insertAssetId.empty() &&
                    !decoded.layout.openings[0]
                         .hasInsertAssetSourceBounds,
                "version-nine openings retain procedural insert semantics") &&
         expect(encoded.accepted &&
                    encoded.encodedText.starts_with(
                        "IGGY3D_WORLD_LAYOUT " +
                        std::to_string(cr::kCreativeWorldLayoutCodecVersion) +
                        "\n"),
                "version-nine migration writes current opening fields");
}

}  // namespace

int main() {
  const bool ok = deterministicRoundTripPreservesEveryTable() &&
                  malformedAndNonFiniteInputsFailClosed() &&
                  versionOneSourceMigratesToCurrentSchema() &&
                  versionTwoSourceMigratesWithoutFabricatedObjects() &&
                  versionThreeRoomPreservesWallPlaneDuringMigration() &&
                  versionFourBoxesMigrateToExplicitAnchorPlanes() &&
                  versionFiveRoomsMigrateToSharedLevels() &&
                  versionSixSourceMigratesWithoutFabricatedVerticalConnectors() &&
                  versionSevenLevelsMigrateToFlatRoofDefaults() &&
                  versionEightObjectsMigrateToIdentityCatalogPose() &&
                  versionNineOpeningsMigrateToProceduralInserts();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
