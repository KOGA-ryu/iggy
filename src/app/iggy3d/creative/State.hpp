#pragma once

#include "app/iggy3d/creative/Core.hpp"

namespace iggy3d::creative {

struct State {
  Flags flags;
  Tool tool = Tool::Select;
  FrameRef frame;
  TargetRef hovered;
  TargetRef selected;
};

}  // namespace iggy3d::creative
