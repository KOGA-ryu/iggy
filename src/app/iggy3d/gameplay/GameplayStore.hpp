#pragma once

#include <string>

namespace iggy3d {

struct GameplayStore {
  bool runtimeSessionCreated = false;
  bool gameplayActive = false;
  bool gameplayInputUsed = false;
  std::string gameplayInputSource = "none";
  bool gameplayTickAdvanced = false;
  bool playerPositionChanged = false;
};

}  // namespace iggy3d
