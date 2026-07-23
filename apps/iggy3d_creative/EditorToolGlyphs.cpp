#include "EditorToolGlyphs.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "imgui.h"

namespace iggy3d_creative_app {

namespace {

using GlyphPoint = CreativeEditorToolGlyphPoint;
using GlyphOp = CreativeEditorToolGlyphOp;
using OpKind = CreativeEditorToolGlyphOpKind;

constexpr float kParentFillAlpha = 0.85F;
constexpr float kSoftFillAlpha = 0.25F;
constexpr float kGhostWidthCells = 1.0F;
constexpr float kGhostAlpha = 0.5F;

constexpr GlyphOp strokeOp(std::span<const GlyphPoint> points,
                           bool closed = false) {
  GlyphOp op;
  op.kind = OpKind::Stroke;
  op.closed = closed;
  op.points = points;
  return op;
}

constexpr GlyphOp dashedOp(std::span<const GlyphPoint> points, float dashCells,
                           float gapCells, bool closed = false,
                           float widthCells = kCreativeEditorToolGlyphStrokeCells,
                           float alpha = 1.0F) {
  GlyphOp op;
  op.kind = OpKind::Stroke;
  op.closed = closed;
  op.widthCells = widthCells;
  op.alpha = alpha;
  op.dashCells = dashCells;
  op.gapCells = gapCells;
  op.points = points;
  return op;
}

constexpr GlyphOp fillOp(std::span<const GlyphPoint> points, float alpha) {
  GlyphOp op;
  op.kind = OpKind::Fill;
  op.closed = true;
  op.alpha = alpha;
  op.points = points;
  return op;
}

constexpr GlyphOp ellipseOp(std::span<const GlyphPoint> centerAndRadii,
                            bool filled, float alpha = 1.0F,
                            float dashCells = 0.0F, float gapCells = 0.0F) {
  GlyphOp op;
  op.kind = filled ? OpKind::EllipseFilled : OpKind::Ellipse;
  op.alpha = alpha;
  op.dashCells = dashCells;
  op.gapCells = gapCells;
  op.points = centerAndRadii;
  return op;
}

constexpr GlyphOp pathOp(std::span<const GlyphPoint> chain,
                         float dashCells = 0.0F, float gapCells = 0.0F) {
  GlyphOp op;
  op.kind = OpKind::Path;
  op.dashCells = dashCells;
  op.gapCells = gapCells;
  op.points = chain;
  return op;
}

constexpr GlyphPoint kCursorOutline[] = {{8.0F, 3.0F},    {8.0F, 19.0F},
                                         {11.8F, 15.4F},  {13.8F, 20.2F},
                                         {16.4F, 19.0F},  {14.4F, 14.4F},
                                         {19.4F, 14.0F}};
constexpr GlyphOp kParentSelectOps[] = {fillOp(kCursorOutline, kParentFillAlpha),
                                        strokeOp(kCursorOutline, true)};

constexpr GlyphPoint kHouseSilhouette[] = {{3.0F, 12.0F},  {12.0F, 4.0F},
                                           {21.0F, 12.0F}, {18.0F, 12.0F},
                                           {18.0F, 20.5F}, {6.0F, 20.5F},
                                           {6.0F, 12.0F}};
constexpr GlyphOp kParentStructureOps[] = {
    fillOp(kHouseSilhouette, kParentFillAlpha),
    strokeOp(kHouseSilhouette, true)};

constexpr GlyphPoint kMountainSilhouette[] = {{2.0F, 20.0F},
                                              {8.5F, 7.0F},
                                              {12.5F, 13.5F},
                                              {16.0F, 6.0F},
                                              {22.0F, 20.0F}};
constexpr GlyphOp kParentTerrainOps[] = {
    fillOp(kMountainSilhouette, kParentFillAlpha),
    strokeOp(kMountainSilhouette, true)};

constexpr GlyphPoint kCubeHex[] = {{12.0F, 3.0F},  {20.0F, 7.5F},
                                   {20.0F, 16.5F}, {12.0F, 21.0F},
                                   {4.0F, 16.5F},  {4.0F, 7.5F}};
constexpr GlyphPoint kCubeTop[] = {{12.0F, 3.0F},
                                   {20.0F, 7.5F},
                                   {12.0F, 12.0F},
                                   {4.0F, 7.5F}};
constexpr GlyphPoint kCubeSeamAcross[] = {{4.0F, 7.5F},
                                          {12.0F, 12.0F},
                                          {20.0F, 7.5F}};
constexpr GlyphPoint kCubeSeamDown[] = {{12.0F, 12.0F}, {12.0F, 21.0F}};
constexpr GlyphOp kParentObjectOps[] = {fillOp(kCubeHex, kSoftFillAlpha),
                                        fillOp(kCubeTop, kParentFillAlpha),
                                        strokeOp(kCubeHex, true),
                                        strokeOp(kCubeSeamAcross),
                                        strokeOp(kCubeSeamDown)};

constexpr GlyphPoint kFlagPole[] = {{7.0F, 3.0F}, {7.0F, 21.0F}};
constexpr GlyphPoint kFlagPennant[] = {{7.0F, 4.0F},
                                       {19.0F, 7.5F},
                                       {7.0F, 11.0F}};
constexpr GlyphOp kParentGameplayOps[] = {
    strokeOp(kFlagPole), fillOp(kFlagPennant, kParentFillAlpha),
    strokeOp(kFlagPennant, true)};

constexpr GlyphPoint kEstateOutline[] = {{2.5F, 10.0F},  {10.0F, 3.0F},
                                         {17.5F, 10.0F}, {15.5F, 10.0F},
                                         {15.5F, 18.0F}, {4.5F, 18.0F},
                                         {4.5F, 10.0F}};
constexpr GlyphPoint kEstateStar[] = {{19.5F, 16.0F}, {20.4F, 18.6F},
                                      {23.0F, 19.5F}, {20.4F, 20.4F},
                                      {19.5F, 23.0F}, {18.6F, 20.4F},
                                      {16.0F, 19.5F}, {18.6F, 18.6F}};
constexpr GlyphOp kEstateHouseOps[] = {strokeOp(kEstateOutline, true),
                                       fillOp(kEstateStar, 1.0F)};

constexpr GlyphPoint kShellRoof[] = {{4.0F, 11.5F},
                                     {12.0F, 4.0F},
                                     {20.0F, 11.5F}};
constexpr GlyphPoint kShellWallLeft[] = {{6.0F, 11.0F}, {6.0F, 20.0F}};
constexpr GlyphPoint kShellWallRight[] = {{18.0F, 11.0F}, {18.0F, 20.0F}};
constexpr GlyphPoint kShellBase[] = {{6.0F, 20.0F}, {18.0F, 20.0F}};
constexpr GlyphOp kBuildingShellOps[] = {strokeOp(kShellRoof),
                                         strokeOp(kShellWallLeft),
                                         strokeOp(kShellWallRight),
                                         dashedOp(kShellBase, 2.0F, 2.0F)};

constexpr GlyphPoint kRoomRect[] = {{3.0F, 5.0F},
                                    {15.0F, 5.0F},
                                    {15.0F, 17.0F},
                                    {3.0F, 17.0F}};
constexpr GlyphPoint kRoomPlusVertical[] = {{19.5F, 16.0F}, {19.5F, 23.0F}};
constexpr GlyphPoint kRoomPlusHorizontal[] = {{16.0F, 19.5F}, {23.0F, 19.5F}};
constexpr GlyphOp kAddRoomOps[] = {strokeOp(kRoomRect, true),
                                   strokeOp(kRoomPlusVertical),
                                   strokeOp(kRoomPlusHorizontal)};

constexpr GlyphPoint kFloorDiamond[] = {{12.0F, 8.0F},
                                        {21.0F, 13.0F},
                                        {12.0F, 18.0F},
                                        {3.0F, 13.0F}};
constexpr GlyphPoint kFloorSeamA[] = {{7.5F, 10.5F}, {16.5F, 15.5F}};
constexpr GlyphPoint kFloorSeamB[] = {{16.5F, 10.5F}, {7.5F, 15.5F}};
constexpr GlyphOp kFloorOps[] = {strokeOp(kFloorDiamond, true),
                                 strokeOp(kFloorSeamA), strokeOp(kFloorSeamB)};

constexpr GlyphPoint kWallRect[] = {{4.0F, 6.0F},
                                    {20.0F, 6.0F},
                                    {20.0F, 20.0F},
                                    {4.0F, 20.0F}};
constexpr GlyphPoint kWallCourseTop[] = {{4.0F, 10.7F}, {20.0F, 10.7F}};
constexpr GlyphPoint kWallCourseBottom[] = {{4.0F, 15.3F}, {20.0F, 15.3F}};
constexpr GlyphPoint kWallJointTop[] = {{12.0F, 6.0F}, {12.0F, 10.7F}};
constexpr GlyphPoint kWallJointLeft[] = {{8.0F, 10.7F}, {8.0F, 15.3F}};
constexpr GlyphPoint kWallJointRight[] = {{16.0F, 10.7F}, {16.0F, 15.3F}};
constexpr GlyphPoint kWallJointBottom[] = {{12.0F, 15.3F}, {12.0F, 20.0F}};
constexpr GlyphOp kPartitionOps[] = {
    strokeOp(kWallRect, true),      strokeOp(kWallCourseTop),
    strokeOp(kWallCourseBottom),    strokeOp(kWallJointTop),
    strokeOp(kWallJointLeft),       strokeOp(kWallJointRight),
    strokeOp(kWallJointBottom)};

constexpr GlyphPoint kDoorRect[] = {{5.0F, 3.0F},
                                    {16.0F, 3.0F},
                                    {16.0F, 21.0F},
                                    {5.0F, 21.0F}};
constexpr GlyphPoint kDoorKnob[] = {{13.0F, 12.5F}, {1.0F, 1.0F}};
constexpr GlyphPoint kDoorSwing[] = {{16.0F, 21.0F},
                                     {19.59F, 21.0F},
                                     {22.5F, 18.09F},
                                     {22.5F, 14.5F}};
constexpr GlyphOp kDoorOps[] = {strokeOp(kDoorRect, true),
                                ellipseOp(kDoorKnob, true),
                                pathOp(kDoorSwing, 1.6F, 2.0F)};

constexpr GlyphPoint kWindowRect[] = {{4.5F, 4.5F},
                                      {19.5F, 4.5F},
                                      {19.5F, 19.5F},
                                      {4.5F, 19.5F}};
constexpr GlyphPoint kWindowMullionVertical[] = {{12.0F, 4.5F}, {12.0F, 19.5F}};
constexpr GlyphPoint kWindowMullionHorizontal[] = {{4.5F, 12.0F},
                                                   {19.5F, 12.0F}};
constexpr GlyphOp kWindowOps[] = {strokeOp(kWindowRect, true),
                                  strokeOp(kWindowMullionVertical),
                                  strokeOp(kWindowMullionHorizontal)};

constexpr GlyphPoint kStairOutline[] = {{3.0F, 21.0F}, {3.0F, 16.0F},
                                        {8.0F, 16.0F}, {8.0F, 11.0F},
                                        {13.0F, 11.0F}, {13.0F, 6.0F},
                                        {21.0F, 6.0F},  {21.0F, 21.0F}};
constexpr GlyphOp kStairOps[] = {fillOp(kStairOutline, kSoftFillAlpha),
                                 strokeOp(kStairOutline, true)};

constexpr GlyphPoint kRampOutline[] = {{3.0F, 20.0F},
                                       {21.0F, 20.0F},
                                       {21.0F, 6.0F}};
constexpr GlyphOp kRampOps[] = {fillOp(kRampOutline, kSoftFillAlpha),
                                strokeOp(kRampOutline, true)};

constexpr GlyphPoint kPlateauMesa[] = {{7.0F, 20.0F},
                                       {10.0F, 9.0F},
                                       {16.0F, 9.0F},
                                       {19.0F, 20.0F}};
constexpr GlyphPoint kPlateauGround[] = {{2.0F, 20.0F}, {22.0F, 20.0F}};
constexpr GlyphOp kPlateauOps[] = {fillOp(kPlateauMesa, kSoftFillAlpha),
                                   strokeOp(kPlateauGround),
                                   strokeOp(kPlateauMesa)};

constexpr GlyphPoint kRoadEdgeLeft[] = {{8.0F, 21.0F}, {11.0F, 3.0F}};
constexpr GlyphPoint kRoadEdgeRight[] = {{16.0F, 21.0F}, {13.0F, 3.0F}};
constexpr GlyphPoint kRoadCenter[] = {{12.0F, 19.0F}, {12.0F, 5.0F}};
constexpr GlyphOp kRoadOps[] = {strokeOp(kRoadEdgeLeft),
                                strokeOp(kRoadEdgeRight),
                                dashedOp(kRoadCenter, 2.0F, 2.5F)};

constexpr GlyphPoint kDitchProfile[] = {{2.0F, 9.0F},  {8.0F, 9.0F},
                                        {8.0F, 17.0F}, {16.0F, 17.0F},
                                        {16.0F, 9.0F}, {22.0F, 9.0F}};
constexpr GlyphOp kDitchOps[] = {strokeOp(kDitchProfile)};

constexpr GlyphPoint kRegionGhostRect[] = {{3.5F, 3.5F},
                                           {20.5F, 3.5F},
                                           {20.5F, 20.5F},
                                           {3.5F, 20.5F}};
constexpr GlyphOp regionGhostOp() {
  return dashedOp(kRegionGhostRect, 2.0F, 2.0F, true, kGhostWidthCells,
                  kGhostAlpha);
}

constexpr GlyphPoint kFlattenLine[] = {{6.5F, 13.0F}, {17.5F, 13.0F}};
constexpr GlyphPoint kFlattenChevronTop[] = {{10.0F, 8.0F},
                                             {12.0F, 10.0F},
                                             {14.0F, 8.0F}};
constexpr GlyphPoint kFlattenChevronBottom[] = {{10.0F, 18.0F},
                                                {12.0F, 16.0F},
                                                {14.0F, 18.0F}};
constexpr GlyphOp kRegionFlattenOps[] = {regionGhostOp(),
                                         strokeOp(kFlattenLine),
                                         strokeOp(kFlattenChevronTop),
                                         strokeOp(kFlattenChevronBottom)};

constexpr GlyphPoint kRaiseBase[] = {{6.5F, 17.0F}, {17.5F, 17.0F}};
constexpr GlyphPoint kRaiseChevron[] = {{8.5F, 12.5F},
                                        {12.0F, 8.5F},
                                        {15.5F, 12.5F}};
constexpr GlyphOp kRegionRaiseOps[] = {regionGhostOp(), strokeOp(kRaiseBase),
                                       strokeOp(kRaiseChevron)};

constexpr GlyphPoint kLowerTop[] = {{6.5F, 7.0F}, {17.5F, 7.0F}};
constexpr GlyphPoint kLowerChevron[] = {{8.5F, 11.5F},
                                        {12.0F, 15.5F},
                                        {15.5F, 11.5F}};
constexpr GlyphOp kRegionLowerOps[] = {regionGhostOp(), strokeOp(kLowerTop),
                                       strokeOp(kLowerChevron)};

constexpr GlyphPoint kSmoothCurve[] = {{6.0F, 14.5F},
                                       {9.0F, 8.5F},
                                       {15.0F, 17.5F},
                                       {18.0F, 11.5F}};
constexpr GlyphOp kRegionSmoothOps[] = {regionGhostOp(), pathOp(kSmoothCurve)};

constexpr GlyphPoint kNoiseZigzag[] = {{6.0F, 14.0F},  {8.5F, 9.5F},
                                       {11.0F, 15.5F}, {13.5F, 8.5F},
                                       {16.0F, 13.5F}, {18.0F, 10.5F}};
constexpr GlyphOp kRegionNoiseOps[] = {regionGhostOp(), strokeOp(kNoiseZigzag)};

constexpr GlyphPoint kMaskRect[] = {{4.5F, 7.0F},
                                    {19.5F, 7.0F},
                                    {19.5F, 18.0F},
                                    {4.5F, 18.0F}};
constexpr GlyphOp kMaskRectangleOps[] = {dashedOp(kMaskRect, 2.5F, 2.0F, true)};

constexpr GlyphPoint kMaskEllipse[] = {{12.0F, 12.0F}, {8.5F, 6.0F}};
constexpr GlyphOp kMaskEllipseOps[] = {
    ellipseOp(kMaskEllipse, false, 1.0F, 2.5F, 2.0F)};

constexpr GlyphPoint kBridgeDeck[] = {{2.0F, 9.0F}, {22.0F, 9.0F}};
constexpr GlyphPoint kBridgePierLeft[] = {{4.5F, 9.0F}, {4.5F, 17.5F}};
constexpr GlyphPoint kBridgePierRight[] = {{19.5F, 9.0F}, {19.5F, 17.5F}};
constexpr GlyphPoint kBridgeArch[] = {{4.5F, 17.5F},
                                      {9.5F, 10.833F},
                                      {14.5F, 10.833F},
                                      {19.5F, 17.5F}};
constexpr GlyphOp kBridgeOps[] = {strokeOp(kBridgeDeck),
                                  strokeOp(kBridgePierLeft),
                                  strokeOp(kBridgePierRight),
                                  pathOp(kBridgeArch)};

constexpr GlyphPoint kBoxBody[] = {{5.0F, 10.5F},
                                   {19.0F, 10.5F},
                                   {19.0F, 20.0F},
                                   {5.0F, 20.0F}};
constexpr GlyphPoint kBoxFlapLeft[] = {{5.0F, 10.5F}, {2.5F, 5.5F}};
constexpr GlyphPoint kBoxFlapRight[] = {{19.0F, 10.5F}, {21.5F, 5.5F}};
constexpr GlyphPoint kBoxFlapInner[] = {{9.5F, 10.5F},
                                        {12.0F, 5.5F},
                                        {14.5F, 10.5F}};
constexpr GlyphOp kAssetBoxOps[] = {strokeOp(kBoxBody, true),
                                    strokeOp(kBoxFlapLeft),
                                    strokeOp(kBoxFlapRight),
                                    strokeOp(kBoxFlapInner)};

constexpr GlyphPoint kColumnCap[] = {{6.5F, 4.0F}, {17.5F, 4.0F}};
constexpr GlyphPoint kColumnAbacus[] = {{8.0F, 7.0F}, {16.0F, 7.0F}};
constexpr GlyphPoint kColumnBase[] = {{8.0F, 17.0F}, {16.0F, 17.0F}};
constexpr GlyphPoint kColumnPlinth[] = {{6.5F, 20.0F}, {17.5F, 20.0F}};
constexpr GlyphPoint kColumnFluteLeft[] = {{9.5F, 7.0F}, {9.5F, 17.0F}};
constexpr GlyphPoint kColumnFluteMiddle[] = {{12.0F, 7.0F}, {12.0F, 17.0F}};
constexpr GlyphPoint kColumnFluteRight[] = {{14.5F, 7.0F}, {14.5F, 17.0F}};
constexpr GlyphOp kAssetArchitectureOps[] = {
    strokeOp(kColumnCap),        strokeOp(kColumnAbacus),
    strokeOp(kColumnBase),       strokeOp(kColumnPlinth),
    strokeOp(kColumnFluteLeft),  strokeOp(kColumnFluteMiddle),
    strokeOp(kColumnFluteRight)};

constexpr GlyphPoint kPineOutline[] = {{12.0F, 3.0F},  {17.5F, 10.0F},
                                       {14.5F, 10.0F}, {19.0F, 16.5F},
                                       {5.0F, 16.5F},  {9.5F, 10.0F},
                                       {6.5F, 10.0F}};
constexpr GlyphPoint kPineTrunk[] = {{12.0F, 16.5F}, {12.0F, 21.0F}};
constexpr GlyphOp kAssetNatureOps[] = {fillOp(kPineOutline, kSoftFillAlpha),
                                       strokeOp(kPineOutline, true),
                                       strokeOp(kPineTrunk)};

constexpr GlyphPoint kCoverWall[] = {{3.0F, 12.0F},
                                     {15.0F, 12.0F},
                                     {15.0F, 20.0F},
                                     {3.0F, 20.0F}};
constexpr GlyphPoint kCoverHead[] = {{18.5F, 8.5F}, {2.5F, 2.5F}};
constexpr GlyphPoint kCoverShoulder[] = {{15.5F, 13.5F},
                                         {17.5F, 11.833F},
                                         {19.5F, 11.833F},
                                         {21.5F, 13.5F}};
constexpr GlyphOp kAssetCoverOps[] = {strokeOp(kCoverWall, true),
                                      ellipseOp(kCoverHead, false),
                                      pathOp(kCoverShoulder)};

constexpr GlyphPoint kCrateRect[] = {{5.0F, 5.0F},
                                     {19.0F, 5.0F},
                                     {19.0F, 19.0F},
                                     {5.0F, 19.0F}};
constexpr GlyphPoint kCrateDiagonalDown[] = {{5.0F, 5.0F}, {19.0F, 19.0F}};
constexpr GlyphPoint kCrateDiagonalUp[] = {{19.0F, 5.0F}, {5.0F, 19.0F}};
constexpr GlyphOp kAssetPropsOps[] = {strokeOp(kCrateRect, true),
                                      strokeOp(kCrateDiagonalDown),
                                      strokeOp(kCrateDiagonalUp)};

constexpr GlyphPoint kPinOutline[] = {
    {12.0F, 21.0F}, {12.0F, 21.0F}, {5.5F, 13.5F},  {5.5F, 9.5F},
    {5.5F, 5.91F},  {8.41F, 3.0F},  {12.0F, 3.0F},  {15.59F, 3.0F},
    {18.5F, 5.91F}, {18.5F, 9.5F},  {18.5F, 13.5F}, {12.0F, 21.0F},
    {12.0F, 21.0F}};
constexpr GlyphPoint kPinDot[] = {{12.0F, 9.5F}, {2.5F, 2.5F}};
constexpr GlyphOp kPlayerSpawnOps[] = {pathOp(kPinOutline),
                                       ellipseOp(kPinDot, true)};

constexpr GlyphPoint kNpcDiamond[] = {{12.0F, 6.5F},
                                      {15.0F, 9.5F},
                                      {12.0F, 12.5F},
                                      {9.0F, 9.5F}};
constexpr GlyphOp kNpcSpawnOps[] = {pathOp(kPinOutline),
                                    strokeOp(kNpcDiamond, true)};

constexpr GlyphPoint kEyeOutline[] = {{2.5F, 12.0F},  {6.5F, 6.0F},
                                      {17.5F, 6.0F},  {21.5F, 12.0F},
                                      {17.5F, 18.0F}, {6.5F, 18.0F},
                                      {2.5F, 12.0F}};
constexpr GlyphPoint kEyePupil[] = {{12.0F, 12.0F}, {2.6F, 2.6F}};
constexpr GlyphOp kActionPreviewOps[] = {pathOp(kEyeOutline),
                                         ellipseOp(kEyePupil, true)};

constexpr GlyphPoint kCheckMark[] = {{4.5F, 13.0F},
                                     {10.0F, 18.5F},
                                     {19.5F, 6.0F}};
constexpr GlyphOp kActionApplyOps[] = {strokeOp(kCheckMark)};

constexpr GlyphPoint kCancelDown[] = {{6.0F, 6.0F}, {18.0F, 18.0F}};
constexpr GlyphPoint kCancelUp[] = {{18.0F, 6.0F}, {6.0F, 18.0F}};
constexpr GlyphOp kActionCancelOps[] = {strokeOp(kCancelDown),
                                        strokeOp(kCancelUp)};

constexpr GlyphPoint kDieRect[] = {{4.0F, 4.0F},
                                   {20.0F, 4.0F},
                                   {20.0F, 20.0F},
                                   {4.0F, 20.0F}};
constexpr GlyphPoint kDiePipTop[] = {{8.5F, 8.5F}, {1.4F, 1.4F}};
constexpr GlyphPoint kDiePipMiddle[] = {{12.0F, 12.0F}, {1.4F, 1.4F}};
constexpr GlyphPoint kDiePipBottom[] = {{15.5F, 15.5F}, {1.4F, 1.4F}};
constexpr GlyphOp kActionNewSeedOps[] = {strokeOp(kDieRect, true),
                                         ellipseOp(kDiePipTop, true),
                                         ellipseOp(kDiePipMiddle, true),
                                         ellipseOp(kDiePipBottom, true)};

constexpr GlyphPoint kPlayTriangle[] = {{8.5F, 4.5F},
                                        {19.5F, 12.0F},
                                        {8.5F, 19.5F}};
constexpr GlyphOp kActionPlayOps[] = {fillOp(kPlayTriangle, kSoftFillAlpha),
                                      strokeOp(kPlayTriangle, true)};

constexpr GlyphPoint kBadgePlusVertical[] = {{12.0F, 6.0F}, {12.0F, 18.0F}};
constexpr GlyphPoint kBadgePlusHorizontal[] = {{6.0F, 12.0F}, {18.0F, 12.0F}};
constexpr GlyphOp kBadgeAddOps[] = {strokeOp(kBadgePlusVertical),
                                    strokeOp(kBadgePlusHorizontal)};

constexpr GlyphOp kBadgeRemoveOps[] = {strokeOp(kBadgePlusHorizontal)};

constexpr GlyphPoint kBadgeChevronUp[] = {{6.0F, 14.5F},
                                          {12.0F, 8.5F},
                                          {18.0F, 14.5F}};
constexpr GlyphOp kBadgeRaiseOps[] = {strokeOp(kBadgeChevronUp)};

constexpr GlyphPoint kBadgeChevronDown[] = {{6.0F, 9.5F},
                                            {12.0F, 15.5F},
                                            {18.0F, 9.5F}};
constexpr GlyphOp kBadgeLowerOps[] = {strokeOp(kBadgeChevronDown)};

constexpr GlyphPoint kBadgeWave[] = {{4.5F, 12.0F},  {7.0F, 7.5F},
                                     {9.5F, 7.5F},   {12.0F, 12.0F},
                                     {14.5F, 16.5F}, {17.0F, 16.5F},
                                     {19.5F, 12.0F}};
constexpr GlyphOp kBadgeNoiseOps[] = {pathOp(kBadgeWave)};

constexpr GlyphPoint kBadgeStar[] = {{12.0F, 4.5F},  {13.9F, 10.1F},
                                     {19.5F, 12.0F}, {13.9F, 13.9F},
                                     {12.0F, 19.5F}, {10.1F, 13.9F},
                                     {4.5F, 12.0F},  {10.1F, 10.1F}};
constexpr GlyphOp kBadgeTemplateOps[] = {fillOp(kBadgeStar, 1.0F)};

constexpr GlyphPoint kViewPlanFrame[] = {{3.0F, 3.0F},
                                         {21.0F, 3.0F},
                                         {21.0F, 21.0F},
                                         {3.0F, 21.0F}};
constexpr GlyphPoint kViewPlanVertical[] = {{13.0F, 3.0F}, {13.0F, 12.0F}};
constexpr GlyphPoint kViewPlanHorizontal[] = {{3.0F, 12.0F}, {21.0F, 12.0F}};
constexpr GlyphOp kViewPlanOps[] = {strokeOp(kViewPlanFrame, true),
                                    strokeOp(kViewPlanVertical),
                                    strokeOp(kViewPlanHorizontal)};

constexpr GlyphPoint kElevationGround[] = {{2.0F, 20.0F}, {22.0F, 20.0F}};
constexpr GlyphPoint kElevationFacade[] = {{5.0F, 9.0F},
                                           {19.0F, 9.0F},
                                           {19.0F, 20.0F},
                                           {5.0F, 20.0F}};
constexpr GlyphPoint kElevationRoof[] = {{5.0F, 9.0F},
                                         {12.0F, 4.0F},
                                         {19.0F, 9.0F}};
constexpr GlyphOp kViewElevationOps[] = {strokeOp(kElevationGround),
                                         strokeOp(kElevationFacade, true),
                                         strokeOp(kElevationRoof)};

constexpr GlyphOp kView3dOps[] = {strokeOp(kCubeHex, true),
                                  strokeOp(kCubeSeamAcross),
                                  strokeOp(kCubeSeamDown)};

constexpr GlyphPoint kLevelChevronUp[] = {{9.0F, 6.5F},
                                          {12.0F, 3.5F},
                                          {15.0F, 6.5F}};
constexpr GlyphPoint kLevelUpperPlate[] = {{12.0F, 9.0F},
                                           {19.0F, 12.5F},
                                           {12.0F, 16.0F},
                                           {5.0F, 12.5F}};
constexpr GlyphPoint kLevelLowerEdge[] = {{5.0F, 16.0F},
                                          {12.0F, 19.5F},
                                          {19.0F, 16.0F}};
constexpr GlyphOp kLevelUpOps[] = {strokeOp(kLevelChevronUp),
                                   strokeOp(kLevelUpperPlate, true),
                                   strokeOp(kLevelLowerEdge)};

constexpr GlyphPoint kLevelDownPlate[] = {{12.0F, 8.0F},
                                          {19.0F, 11.5F},
                                          {12.0F, 15.0F},
                                          {5.0F, 11.5F}};
constexpr GlyphPoint kLevelDownEdge[] = {{5.0F, 15.0F},
                                         {12.0F, 18.5F},
                                         {19.0F, 15.0F}};
constexpr GlyphPoint kLevelChevronDown[] = {{9.0F, 17.5F},
                                            {12.0F, 20.5F},
                                            {15.0F, 17.5F}};
constexpr GlyphOp kLevelDownOps[] = {strokeOp(kLevelDownPlate, true),
                                     strokeOp(kLevelDownEdge),
                                     strokeOp(kLevelChevronDown)};

constexpr GlyphPoint kFitBracketTopLeft[] = {{3.0F, 7.0F},
                                             {3.0F, 3.0F},
                                             {7.0F, 3.0F}};
constexpr GlyphPoint kFitBracketTopRight[] = {{17.0F, 3.0F},
                                              {21.0F, 3.0F},
                                              {21.0F, 7.0F}};
constexpr GlyphPoint kFitBracketBottomRight[] = {{21.0F, 17.0F},
                                                 {21.0F, 21.0F},
                                                 {17.0F, 21.0F}};
constexpr GlyphPoint kFitBracketBottomLeft[] = {{7.0F, 21.0F},
                                                {3.0F, 21.0F},
                                                {3.0F, 17.0F}};
constexpr GlyphPoint kFitInnerRect[] = {{8.0F, 8.0F},
                                        {16.0F, 8.0F},
                                        {16.0F, 16.0F},
                                        {8.0F, 16.0F}};
constexpr GlyphOp kFitAllOps[] = {strokeOp(kFitBracketTopLeft),
                                  strokeOp(kFitBracketTopRight),
                                  strokeOp(kFitBracketBottomRight),
                                  strokeOp(kFitBracketBottomLeft),
                                  strokeOp(kFitInnerRect, true)};
constexpr GlyphOp kFitSelectionOps[] = {
    strokeOp(kFitBracketTopLeft), strokeOp(kFitBracketTopRight),
    strokeOp(kFitBracketBottomRight), strokeOp(kFitBracketBottomLeft),
    dashedOp(kFitInnerRect, 1.6F, 1.4F, true)};

constexpr GlyphPoint kRoofGable[] = {{3.0F, 14.0F},
                                     {12.0F, 5.0F},
                                     {21.0F, 14.0F}};
constexpr GlyphPoint kRoofRidgeTick[] = {{12.0F, 5.0F}, {12.0F, 9.0F}};
constexpr GlyphPoint kRoofEyeOutline[] = {{12.0F, 17.5F}, {4.5F, 2.6F}};
constexpr GlyphPoint kRoofEyePupil[] = {{12.0F, 17.5F}, {1.2F, 1.2F}};
constexpr GlyphOp kRoofVisibilityOps[] = {strokeOp(kRoofGable),
                                          strokeOp(kRoofRidgeTick),
                                          ellipseOp(kRoofEyeOutline, false),
                                          ellipseOp(kRoofEyePupil, true)};

constexpr GlyphPoint kContextUpperPlate[] = {{12.0F, 4.5F},
                                             {19.5F, 8.5F},
                                             {12.0F, 12.5F},
                                             {4.5F, 8.5F}};
constexpr GlyphPoint kContextLowerPlate[] = {{12.0F, 11.5F},
                                             {19.5F, 15.5F},
                                             {12.0F, 19.5F},
                                             {4.5F, 15.5F}};
constexpr GlyphOp kLowerLevelContextOps[] = {
    strokeOp(kContextUpperPlate, true),
    dashedOp(kContextLowerPlate, 1.6F, 1.4F, true)};
constexpr GlyphOp kUpperLevelContextOps[] = {
    dashedOp(kContextUpperPlate, 1.6F, 1.4F, true),
    strokeOp(kContextLowerPlate, true)};

constexpr GlyphPoint kContourOuter[] = {{12.0F, 12.0F}, {9.0F, 6.5F}};
constexpr GlyphPoint kContourMiddle[] = {{12.0F, 12.0F}, {6.0F, 4.2F}};
constexpr GlyphPoint kContourInner[] = {{12.0F, 12.0F}, {3.0F, 1.9F}};
constexpr GlyphOp kContoursOps[] = {ellipseOp(kContourOuter, false),
                                    ellipseOp(kContourMiddle, false),
                                    ellipseOp(kContourInner, false)};

constexpr GlyphPoint kDimensionLeftTick[] = {{5.0F, 6.0F}, {5.0F, 18.0F}};
constexpr GlyphPoint kDimensionRightTick[] = {{19.0F, 6.0F}, {19.0F, 18.0F}};
constexpr GlyphPoint kDimensionLine[] = {{5.0F, 12.0F}, {19.0F, 12.0F}};
constexpr GlyphPoint kDimensionLeftArrow[] = {{8.0F, 10.5F},
                                              {5.0F, 12.0F},
                                              {8.0F, 13.5F}};
constexpr GlyphPoint kDimensionRightArrow[] = {{16.0F, 10.5F},
                                               {19.0F, 12.0F},
                                               {16.0F, 13.5F}};
constexpr GlyphOp kDimensionsOps[] = {strokeOp(kDimensionLeftTick),
                                      strokeOp(kDimensionRightTick),
                                      strokeOp(kDimensionLine),
                                      strokeOp(kDimensionLeftArrow),
                                      strokeOp(kDimensionRightArrow)};

constexpr GlyphPoint kSnapLeftLeg[] = {{8.0F, 21.0F}, {8.0F, 11.0F}};
constexpr GlyphPoint kSnapRightLeg[] = {{16.0F, 21.0F}, {16.0F, 11.0F}};
constexpr GlyphPoint kSnapArch[] = {{8.0F, 11.0F},
                                    {8.0F, 5.7F},
                                    {16.0F, 5.7F},
                                    {16.0F, 11.0F}};
constexpr GlyphPoint kSnapLeftCap[] = {{6.8F, 17.0F}, {9.2F, 17.0F}};
constexpr GlyphPoint kSnapRightCap[] = {{14.8F, 17.0F}, {17.2F, 17.0F}};
constexpr GlyphOp kSnapOps[] = {strokeOp(kSnapLeftLeg),
                                strokeOp(kSnapRightLeg), pathOp(kSnapArch),
                                strokeOp(kSnapLeftCap),
                                strokeOp(kSnapRightCap)};

constexpr std::size_t kGlyphCount =
    static_cast<std::size_t>(CreativeEditorToolGlyph::Count);

constexpr std::array<std::span<const GlyphOp>, kGlyphCount> kGlyphOpsTable = {{
    kParentSelectOps,     kParentStructureOps,  kParentTerrainOps,
    kParentObjectOps,     kParentGameplayOps,   kEstateHouseOps,
    kBuildingShellOps,    kAddRoomOps,          kFloorOps,
    kPartitionOps,        kDoorOps,             kWindowOps,
    kStairOps,            kRampOps,             kPlateauOps,
    kRoadOps,             kDitchOps,            kRegionFlattenOps,
    kRegionRaiseOps,      kRegionLowerOps,      kRegionSmoothOps,
    kRegionNoiseOps,      kMaskRectangleOps,    kMaskEllipseOps,
    kBridgeOps,           kAssetBoxOps,         kAssetArchitectureOps,
    kAssetNatureOps,      kAssetCoverOps,       kAssetPropsOps,
    kPlayerSpawnOps,      kNpcSpawnOps,         kActionPreviewOps,
    kActionApplyOps,      kActionCancelOps,     kActionNewSeedOps,
    kActionPlayOps,       kBadgeAddOps,         kBadgeRemoveOps,
    kBadgeRaiseOps,       kBadgeLowerOps,       kBadgeNoiseOps,
    kBadgeTemplateOps,    kViewPlanOps,         kViewElevationOps,
    kView3dOps,           kLevelUpOps,          kLevelDownOps,
    kFitAllOps,           kFitSelectionOps,     kRoofVisibilityOps,
    kLowerLevelContextOps, kUpperLevelContextOps, kContoursOps,
    kDimensionsOps,       kSnapOps,
}};

constexpr std::array<std::string_view, kGlyphCount> kGlyphNames = {{
    "select",          "structure",       "terrain",
    "object",          "gameplay",        "estate house",
    "building shell",  "add room",        "floor",
    "partition",       "door",            "window",
    "stair",           "ramp",            "plateau",
    "road",            "ditch",           "region flatten",
    "region raise",    "region lower",    "region smooth",
    "region noise",    "mask rectangle",  "mask ellipse",
    "bridge",          "asset box",       "asset architecture",
    "asset nature",    "asset cover",     "asset props",
    "player spawn",    "npc spawn",       "preview",
    "apply",           "cancel",          "new seed",
    "play",            "badge add",       "badge remove",
    "badge raise",     "badge lower",     "badge noise",
    "badge template",  "plan view",       "elevation view",
    "3d view",         "level up",        "level down",
    "fit all",         "fit selection",   "roof visibility",
    "lower level context",                "upper level context",
    "contours",        "dimensions",      "snap",
}};

constexpr int kPathSegmentsPerCubic = 12;
constexpr int kEllipseSegments = 48;

GlyphPoint cubicPointAt(GlyphPoint start, GlyphPoint controlA,
                        GlyphPoint controlB, GlyphPoint end,
                        float t) noexcept {
  const float u = 1.0F - t;
  const float uu = u * u;
  const float tt = t * t;
  const float weightStart = uu * u;
  const float weightControlA = 3.0F * uu * t;
  const float weightControlB = 3.0F * u * tt;
  const float weightEnd = tt * t;
  return {weightStart * start.x + weightControlA * controlA.x +
              weightControlB * controlB.x + weightEnd * end.x,
          weightStart * start.y + weightControlA * controlA.y +
              weightControlB * controlB.y + weightEnd * end.y};
}

float distanceBetween(GlyphPoint a, GlyphPoint b) noexcept {
  const float deltaX = b.x - a.x;
  const float deltaY = b.y - a.y;
  return std::sqrt(deltaX * deltaX + deltaY * deltaY);
}

GlyphPoint lerpBetween(GlyphPoint a, GlyphPoint b, float t) noexcept {
  return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

bool pathChainIsWellFormed(std::span<const GlyphPoint> chain) noexcept {
  return chain.size() >= 4U && (chain.size() - 1U) % 3U == 0U;
}

ImU32 tintWithOpAlpha(std::uint32_t tint, float alpha) noexcept {
  const std::uint32_t alphaByte = (tint >> IM_COL32_A_SHIFT) & 0xFFU;
  const float clamped = std::clamp(alpha, 0.0F, 1.0F);
  const auto scaled = static_cast<std::uint32_t>(
      static_cast<float>(alphaByte) * clamped + 0.5F);
  const std::uint32_t alphaMask = 0xFFU << IM_COL32_A_SHIFT;
  return static_cast<ImU32>((tint & ~alphaMask) |
                            ((scaled & 0xFFU) << IM_COL32_A_SHIFT));
}

}  // namespace

std::span<const CreativeEditorToolGlyphOp> creativeEditorToolGlyphOps(
    CreativeEditorToolGlyph glyph) noexcept {
  const auto index = static_cast<std::size_t>(glyph);
  if (index >= kGlyphCount) {
    return {};
  }
  return kGlyphOpsTable[index];
}

std::string_view toString(CreativeEditorToolGlyph glyph) noexcept {
  const auto index = static_cast<std::size_t>(glyph);
  if (index >= kGlyphCount) {
    return "unknown";
  }
  return kGlyphNames[index];
}

CreativeEditorToolGlyph creativeEditorToolGlyphForWorldLayoutTool(
    CreativeEditorWorldLayoutTool tool) noexcept {
  switch (tool) {
    case CreativeEditorWorldLayoutTool::Select:
      return CreativeEditorToolGlyph::ParentSelect;
    case CreativeEditorWorldLayoutTool::Room:
      return CreativeEditorToolGlyph::AddRoom;
    case CreativeEditorWorldLayoutTool::Floor:
      return CreativeEditorToolGlyph::Floor;
    case CreativeEditorWorldLayoutTool::Wall:
      return CreativeEditorToolGlyph::Partition;
    case CreativeEditorWorldLayoutTool::Door:
      return CreativeEditorToolGlyph::Door;
    case CreativeEditorWorldLayoutTool::Window:
      return CreativeEditorToolGlyph::Window;
    case CreativeEditorWorldLayoutTool::Stair:
      return CreativeEditorToolGlyph::Stair;
    case CreativeEditorWorldLayoutTool::Ramp:
      return CreativeEditorToolGlyph::Ramp;
    case CreativeEditorWorldLayoutTool::Plateau:
      return CreativeEditorToolGlyph::Plateau;
    case CreativeEditorWorldLayoutTool::Road:
      return CreativeEditorToolGlyph::Road;
    case CreativeEditorWorldLayoutTool::Ditch:
      return CreativeEditorToolGlyph::Ditch;
    case CreativeEditorWorldLayoutTool::Bridge:
      return CreativeEditorToolGlyph::Bridge;
    case CreativeEditorWorldLayoutTool::CatalogAsset:
      return CreativeEditorToolGlyph::AssetBox;
    case CreativeEditorWorldLayoutTool::PlayerSpawn:
      return CreativeEditorToolGlyph::PlayerSpawn;
    case CreativeEditorWorldLayoutTool::NpcSpawn:
      return CreativeEditorToolGlyph::NpcSpawn;
    case CreativeEditorWorldLayoutTool::BuildingShell:
      return CreativeEditorToolGlyph::BuildingShell;
    case CreativeEditorWorldLayoutTool::Count:
      break;
  }
  return CreativeEditorToolGlyph::ParentSelect;
}

CreativeEditorToolGlyph creativeEditorToolGlyphForTerrainRegionOperation(
    cr::CreativeTerrainRegionMode operation) noexcept {
  switch (operation) {
    case cr::CreativeTerrainRegionMode::Flatten:
      return CreativeEditorToolGlyph::RegionFlatten;
    case cr::CreativeTerrainRegionMode::Raise:
      return CreativeEditorToolGlyph::RegionRaise;
    case cr::CreativeTerrainRegionMode::Lower:
      return CreativeEditorToolGlyph::RegionLower;
    case cr::CreativeTerrainRegionMode::Smooth:
      return CreativeEditorToolGlyph::RegionSmooth;
    case cr::CreativeTerrainRegionMode::Noise:
      return CreativeEditorToolGlyph::RegionNoise;
    case cr::CreativeTerrainRegionMode::Erase:
      return CreativeEditorToolGlyph::BadgeRemove;
    case cr::CreativeTerrainRegionMode::Count:
      break;
  }
  return CreativeEditorToolGlyph::RegionFlatten;
}

CreativeEditorToolGlyph creativeEditorToolGlyphForTerrainMask(
    cr::CreativeTerrainCompositionMask mask) noexcept {
  switch (mask) {
    case cr::CreativeTerrainCompositionMask::Rectangle:
      return CreativeEditorToolGlyph::MaskRectangle;
    case cr::CreativeTerrainCompositionMask::Ellipse:
      return CreativeEditorToolGlyph::MaskEllipse;
    case cr::CreativeTerrainCompositionMask::Count:
      break;
  }
  return CreativeEditorToolGlyph::MaskRectangle;
}

CreativeEditorToolGlyph creativeEditorToolGlyphForPaletteCategory(
    CreativeEditorWorldLayoutPaletteCategory category) noexcept {
  switch (category) {
    case CreativeEditorWorldLayoutPaletteCategory::Structure:
      return CreativeEditorToolGlyph::ParentStructure;
    case CreativeEditorWorldLayoutPaletteCategory::Terrain:
      return CreativeEditorToolGlyph::ParentTerrain;
    case CreativeEditorWorldLayoutPaletteCategory::Object:
      return CreativeEditorToolGlyph::ParentObject;
    case CreativeEditorWorldLayoutPaletteCategory::Gameplay:
      return CreativeEditorToolGlyph::ParentGameplay;
    case CreativeEditorWorldLayoutPaletteCategory::Count:
      break;
  }
  return CreativeEditorToolGlyph::ParentStructure;
}

CreativeEditorToolGlyph creativeEditorToolGlyphForAssetCategory(
    CreativeEditorWorldLayoutAssetCategory category) noexcept {
  switch (category) {
    case CreativeEditorWorldLayoutAssetCategory::Architecture:
      return CreativeEditorToolGlyph::AssetArchitecture;
    case CreativeEditorWorldLayoutAssetCategory::Nature:
      return CreativeEditorToolGlyph::AssetNature;
    case CreativeEditorWorldLayoutAssetCategory::Cover:
      return CreativeEditorToolGlyph::AssetCover;
    case CreativeEditorWorldLayoutAssetCategory::Props:
      return CreativeEditorToolGlyph::AssetProps;
    case CreativeEditorWorldLayoutAssetCategory::Gameplay:
      return CreativeEditorToolGlyph::ParentGameplay;
    case CreativeEditorWorldLayoutAssetCategory::Count:
      break;
  }
  return CreativeEditorToolGlyph::AssetBox;
}

CreativeEditorToolGlyphBounds measureCreativeEditorToolGlyphBounds(
    CreativeEditorToolGlyph glyph) noexcept {
  CreativeEditorToolGlyphBounds bounds;
  for (const CreativeEditorToolGlyphOp& op : creativeEditorToolGlyphOps(glyph)) {
    const bool isEllipse = op.kind == OpKind::Ellipse ||
                           op.kind == OpKind::EllipseFilled;
    if (isEllipse) {
      if (op.points.size() != 2U) {
        continue;
      }
      const GlyphPoint center = op.points[0];
      const GlyphPoint radii = op.points[1];
      const GlyphPoint corners[] = {{center.x - radii.x, center.y - radii.y},
                                    {center.x + radii.x, center.y + radii.y}};
      for (const GlyphPoint point : corners) {
        if (!bounds.present) {
          bounds.present = true;
          bounds.minimumX = bounds.maximumX = point.x;
          bounds.minimumY = bounds.maximumY = point.y;
          continue;
        }
        bounds.minimumX = std::min(bounds.minimumX, point.x);
        bounds.maximumX = std::max(bounds.maximumX, point.x);
        bounds.minimumY = std::min(bounds.minimumY, point.y);
        bounds.maximumY = std::max(bounds.maximumY, point.y);
      }
      continue;
    }
    for (const GlyphPoint point : op.points) {
      if (!bounds.present) {
        bounds.present = true;
        bounds.minimumX = bounds.maximumX = point.x;
        bounds.minimumY = bounds.maximumY = point.y;
        continue;
      }
      bounds.minimumX = std::min(bounds.minimumX, point.x);
      bounds.maximumX = std::max(bounds.maximumX, point.x);
      bounds.minimumY = std::min(bounds.minimumY, point.y);
      bounds.maximumY = std::max(bounds.maximumY, point.y);
    }
  }
  return bounds;
}

float creativeEditorToolGlyphStrokeThicknessPixels(
    float widthCells, float sizePixels) noexcept {
  const float scaled =
      widthCells * sizePixels / kCreativeEditorToolGlyphGridCells;
  return std::max(1.0F, scaled);
}

void flattenCreativeEditorToolGlyphPath(
    std::span<const CreativeEditorToolGlyphPoint> chain,
    std::vector<CreativeEditorToolGlyphPoint>& out) {
  if (!pathChainIsWellFormed(chain)) {
    out.assign(chain.begin(), chain.end());
    return;
  }
  out.clear();
  out.push_back(chain[0]);
  for (std::size_t segment = 0U; segment + 3U < chain.size(); segment += 3U) {
    const GlyphPoint start = chain[segment];
    const GlyphPoint controlA = chain[segment + 1U];
    const GlyphPoint controlB = chain[segment + 2U];
    const GlyphPoint end = chain[segment + 3U];
    for (int step = 1; step <= kPathSegmentsPerCubic; ++step) {
      const float t = static_cast<float>(step) /
                      static_cast<float>(kPathSegmentsPerCubic);
      out.push_back(cubicPointAt(start, controlA, controlB, end, t));
    }
  }
}

void flattenCreativeEditorToolGlyphEllipse(
    CreativeEditorToolGlyphPoint center, CreativeEditorToolGlyphPoint radii,
    std::vector<CreativeEditorToolGlyphPoint>& out) {
  out.clear();
  out.reserve(static_cast<std::size_t>(kEllipseSegments));
  for (int step = 0; step < kEllipseSegments; ++step) {
    const float angle = 2.0F * 3.14159265358979F * static_cast<float>(step) /
                        static_cast<float>(kEllipseSegments);
    out.push_back({center.x + radii.x * std::cos(angle),
                   center.y + radii.y * std::sin(angle)});
  }
}

void appendCreativeEditorToolGlyphDashes(
    std::span<const CreativeEditorToolGlyphPoint> points, bool closed,
    float dashCells, float gapCells,
    std::vector<std::pair<CreativeEditorToolGlyphPoint,
                          CreativeEditorToolGlyphPoint>>& out) {
  if (points.size() < 2U || dashCells <= 0.0F || gapCells <= 0.0F) {
    return;
  }
  const std::size_t edgeCount = closed ? points.size() : points.size() - 1U;
  bool drawing = true;
  float remaining = dashCells;
  for (std::size_t edge = 0U; edge < edgeCount; ++edge) {
    const GlyphPoint from = points[edge];
    const GlyphPoint to = points[(edge + 1U) % points.size()];
    const float length = distanceBetween(from, to);
    if (length <= 1.0e-6F) {
      continue;
    }
    float traveled = 0.0F;
    while (traveled < length - 1.0e-6F) {
      const float step = std::min(remaining, length - traveled);
      const GlyphPoint segmentStart =
          lerpBetween(from, to, traveled / length);
      const GlyphPoint segmentEnd =
          lerpBetween(from, to, (traveled + step) / length);
      if (drawing) {
        out.emplace_back(segmentStart, segmentEnd);
      }
      traveled += step;
      remaining -= step;
      if (remaining <= 1.0e-6F) {
        drawing = !drawing;
        remaining = drawing ? dashCells : gapCells;
      }
    }
  }
}

void drawCreativeEditorToolGlyph(ImDrawList& drawList,
                                 CreativeEditorToolGlyph glyph,
                                 float leftPixels, float topPixels,
                                 float sizePixels, std::uint32_t tint) {
  const auto ops = creativeEditorToolGlyphOps(glyph);
  if (ops.empty() || sizePixels <= 0.0F) {
    return;
  }
  const float scale = sizePixels / kCreativeEditorToolGlyphGridCells;
  const auto toPixel = [leftPixels, topPixels, scale](GlyphPoint point) {
    return ImVec2(leftPixels + point.x * scale, topPixels + point.y * scale);
  };
  std::vector<ImVec2> pixelPoints;
  std::vector<GlyphPoint> flattened;
  std::vector<std::pair<GlyphPoint, GlyphPoint>> dashes;
  const auto drawDashed = [&](std::span<const GlyphPoint> points, bool closed,
                              const GlyphOp& op, ImU32 color,
                              float thickness) {
    dashes.clear();
    appendCreativeEditorToolGlyphDashes(points, closed, op.dashCells,
                                        op.gapCells, dashes);
    for (const auto& segment : dashes) {
      drawList.AddLine(toPixel(segment.first), toPixel(segment.second), color,
                       thickness);
    }
  };
  for (const CreativeEditorToolGlyphOp& op : ops) {
    if (op.points.empty()) {
      continue;
    }
    const ImU32 color = tintWithOpAlpha(tint, op.alpha);
    const float thickness =
        creativeEditorToolGlyphStrokeThicknessPixels(op.widthCells, sizePixels);
    const bool dashed = op.dashCells > 0.0F && op.gapCells > 0.0F;
    switch (op.kind) {
      case OpKind::Stroke: {
        if (dashed) {
          drawDashed(op.points, op.closed, op, color, thickness);
          break;
        }
        pixelPoints.clear();
        for (const GlyphPoint point : op.points) {
          pixelPoints.push_back(toPixel(point));
        }
        drawList.AddPolyline(pixelPoints.data(),
                             static_cast<int>(pixelPoints.size()), color,
                             op.closed ? ImDrawFlags_Closed : ImDrawFlags_None,
                             thickness);
        break;
      }
      case OpKind::Fill: {
        pixelPoints.clear();
        for (const GlyphPoint point : op.points) {
          pixelPoints.push_back(toPixel(point));
        }
        drawList.AddConcavePolyFilled(pixelPoints.data(),
                                      static_cast<int>(pixelPoints.size()),
                                      color);
        break;
      }
      case OpKind::Ellipse: {
        if (op.points.size() != 2U) {
          break;
        }
        if (dashed) {
          flattened.clear();
          flattenCreativeEditorToolGlyphEllipse(op.points[0], op.points[1],
                                                flattened);
          drawDashed(flattened, true, op, color, thickness);
          break;
        }
        drawList.AddEllipse(toPixel(op.points[0]),
                            ImVec2(op.points[1].x * scale,
                                   op.points[1].y * scale),
                            color, 0.0F, 0, thickness);
        break;
      }
      case OpKind::EllipseFilled: {
        if (op.points.size() != 2U) {
          break;
        }
        drawList.AddEllipseFilled(toPixel(op.points[0]),
                                  ImVec2(op.points[1].x * scale,
                                         op.points[1].y * scale),
                                  color);
        break;
      }
      case OpKind::Path: {
        if (dashed || !pathChainIsWellFormed(op.points)) {
          flattened.clear();
          flattenCreativeEditorToolGlyphPath(op.points, flattened);
          if (dashed) {
            drawDashed(flattened, false, op, color, thickness);
            break;
          }
          pixelPoints.clear();
          for (const GlyphPoint point : flattened) {
            pixelPoints.push_back(toPixel(point));
          }
          drawList.AddPolyline(pixelPoints.data(),
                               static_cast<int>(pixelPoints.size()), color,
                               ImDrawFlags_None, thickness);
          break;
        }
        drawList.PathLineTo(toPixel(op.points[0]));
        for (std::size_t segment = 0U; segment + 3U < op.points.size();
             segment += 3U) {
          drawList.PathBezierCubicCurveTo(toPixel(op.points[segment + 1U]),
                                          toPixel(op.points[segment + 2U]),
                                          toPixel(op.points[segment + 3U]), 0);
        }
        drawList.PathStroke(color, ImDrawFlags_None, thickness);
        break;
      }
      case OpKind::Count:
        break;
    }
  }
}

bool drawCreativeEditorToolGlyphButton(
    const char* id, CreativeEditorToolGlyph glyph, float tileSize,
    bool active, std::string_view tooltip) {
  constexpr float kPadding = 2.0F;
  constexpr float kRounding = 2.0F;
  const ImVec2 position = ImGui::GetCursorScreenPos();
  const bool pressed = ImGui::InvisibleButton(id, {tileSize, tileSize});
  const bool hovered = ImGui::IsItemHovered();
  ImDrawList* drawList = ImGui::GetWindowDrawList();
  const ImVec2 end{position.x + tileSize, position.y + tileSize};
  if (active) {
    drawList->AddRectFilled(
        position, end,
        ImGui::GetColorU32(ImVec4{0.16F, 0.47F, 0.25F, 1.0F}), kRounding);
  } else if (hovered) {
    drawList->AddRectFilled(position, end,
                            ImGui::GetColorU32(ImGuiCol_ButtonHovered),
                            kRounding);
  }
  drawCreativeEditorToolGlyph(*drawList, glyph, position.x + kPadding,
                              position.y + kPadding,
                              tileSize - (2.0F * kPadding),
                              ImGui::GetColorU32(ImGuiCol_Text));
  if (!tooltip.empty() &&
      ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
    ImGui::SetTooltip("%.*s", static_cast<int>(tooltip.size()),
                      tooltip.data());
  }
  return pressed;
}

}  // namespace iggy3d_creative_app
