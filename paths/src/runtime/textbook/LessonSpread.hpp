#pragma once
#include "scene/GalleryScene.hpp"

namespace paths {
enum class LessonPresentation { Together, Reading, Figure };
struct LessonSpreadRequest {
  float width=1440,height=900;
  bool contents=true,hasFigure=false;
  LessonPresentation presentation=LessonPresentation::Together;
  float readingFraction=.46f;
};
struct LessonSpread {
  SceneViewport header,contents,reading,figure,footer,divider;
  bool showContents=false,showReading=true,showFigure=false,split=false,compact=false;
};
// The same shell serves every topic. Small windows use an explicit single pane.
LessonSpread planLessonSpread(const LessonSpreadRequest&);
struct LessonFigureRegions { SceneViewport title,viewport,controls; };
LessonFigureRegions planLessonFigureRegions(SceneViewport panel);
} // namespace paths
