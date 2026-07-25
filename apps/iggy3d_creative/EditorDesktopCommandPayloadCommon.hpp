#pragma once

#include <string>

#include "app/iggy3d/creative/tools/MeasurementAnnotation.hpp"

namespace iggy3d_creative_app {

struct CreativeDesktopSaveAsPayload {
  std::string saveId;
};

struct CreativeDesktopMapTemplatePayload {
  std::string templateId;
};

struct CreativeDesktopMeasurementAnnotationPayload {
  iggy3d::creative::CreativeMeasurementAnnotationId annotationId =
      iggy3d::creative::kInvalidCreativeMeasurementAnnotationId;
  std::string name;
};

}  // namespace iggy3d_creative_app
