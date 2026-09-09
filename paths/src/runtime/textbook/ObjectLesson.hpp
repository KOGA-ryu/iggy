#pragma once
#include "runtime/math_objects/MathObjects.hpp"
#include <array>
#include <vector>

namespace paths {
struct ObjectLessonControl { MathParameter parameter; const char* label; };
struct ObjectLessonValue { MathParameter parameter; double value; };
struct ObjectLessonExample {
  const char* title;
  const char* explanation;
  std::vector<ObjectLessonValue> values;
};
// Static lesson content chooses an existing model and a bounded set of inputs.
// MathObjects remains the only mathematical and challenge-checking authority.
struct ObjectLessonSpec {
  MathObjectKind object;
  unsigned level;
  const char* relationship;
  const char* convention;
  const char* challenge;
  std::vector<ObjectLessonControl> controls;
  std::vector<ObjectLessonExample> examples;
  // Empty selectors show every readout. These filter presentation, never values.
  std::vector<std::string_view> metrics;
  std::vector<std::string_view> matrices;
};
enum class ObjectLessonActionKind { SelectExample, SetParameter, Reset, Check };
struct ObjectLessonAction {
  ObjectLessonActionKind kind;
  unsigned example=0;
  MathParameter parameter=MathParameter::X;
  double value=0;
  bool practice=false;
};
class ObjectLesson {
public:
  explicit ObjectLesson(const ObjectLessonSpec&);
  MathActionResult dispatch(ObjectLessonAction);
  const ObjectLessonSpec& spec()const{return spec_;}
  const MathObjectSnapshot& snapshot(bool practice=false)const{return models_[practice].snapshot();}
  double parameter(MathParameter p,bool practice=false)const{return models_[practice].parameter(p);}
  unsigned example()const{return example_;}
  bool customized()const{return customized_;}
private:
  MathActionResult initialize(MathObjects&,unsigned)const;
  const ObjectLessonSpec& spec_;
  std::array<MathObjects,2> models_;
  unsigned example_=0;
  bool customized_=false;
};
const ObjectLessonSpec& determinantVolumeSpec();
} // namespace paths
