#pragma once
#include "runtime/math_objects/MathObjects.hpp"
#include "scene/GalleryScene.hpp"
#include <bitset>

namespace paths {
enum class MathControlGroup : unsigned { Shape, ShapeA, ShapeB, Profile, Transform, Operation, Probe, Animation, Sampling, Display, Advanced, Count };
struct MathControlMetadata { MathControlGroup group=MathControlGroup::Shape; std::string_view label; };
struct MathControlRow { MathControlGroup group; std::string_view label; std::array<MathParameter,3> parameters{}; unsigned count=1; std::array<std::string_view,3> components{"X","Y","Z"}; };
struct MathControlRows { std::array<MathControlRow,static_cast<unsigned>(MathParameter::Count)> rows{};unsigned count=0; };
struct MathControlRange { double minimum=0,maximum=0; };
[[nodiscard]] MathControlMetadata mathControlMetadata(MathParameter);
[[nodiscard]] std::string_view mathControlGroupName(MathControlGroup);
[[nodiscard]] MathControlRows mathControlRows(const MathObjects&);
[[nodiscard]] MathControlRange mathControlRange(const MathObjects&,MathParameter);
[[nodiscard]] MathAction mathResetControlGroup(const MathObjects&,MathControlGroup);
[[nodiscard]] unsigned mathChangedControlCount(const MathObjects&,MathControlGroup);
[[nodiscard]] bool matchesMathObject(MathObjectKind,std::string_view query);

struct MathInspectorMemory {
  struct Object {
    bool initialized=false;
    std::array<bool,4> visited{};
    std::array<std::bitset<static_cast<unsigned>(MathControlGroup::Count)>,4> open;
    std::array<double,static_cast<unsigned>(MathParameter::Count)> exampleValues{};
    std::string_view exampleName="Defaults";
  };
  std::array<Object,static_cast<unsigned>(MathObjectKind::Count)> objects{};
  void visit(const MathObjects&);
  void rememberExample(const MathObjects&,std::string_view);
  [[nodiscard]] std::string_view exampleTitle(const MathObjects&) const;
  [[nodiscard]] bool groupOpen(const MathObjects&,MathControlGroup) const;
  void setGroupOpen(const MathObjects&,MathControlGroup,bool);
};
struct MathLabLayoutRequest {
  SceneViewport bounds{0,0,1440,900};float scale=1,inspectorWidth=340,drawerHeight=200;
  bool inspector=true,drawer=false;
};
struct MathLabLayout {
  SceneViewport toolbar{0,0,0,0},viewTools{0,0,0,0},metrics{0,0,0,0},viewport{0,0,0,0},inspector{0,0,0,0},drawer{0,0,0,0},inspectorDivider{0,0,0,0},drawerDivider{0,0,0,0};
  unsigned toolbarRows=1;bool overlayInspector=false;
};
// Pure rectangle planning. Coordinates can be relative to an embedded model pane.
[[nodiscard]] MathLabLayout planMathLabLayout(const MathLabLayoutRequest&);
} // namespace paths
