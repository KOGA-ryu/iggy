#include "ObjectLesson.hpp"
#include <algorithm>
#include <stdexcept>

namespace paths {
ObjectLesson::ObjectLesson(const ObjectLessonSpec& spec):spec_(spec){
  if(spec_.examples.empty()||spec_.controls.empty())throw std::invalid_argument("Object lesson needs controls and examples");
  for(auto& model:models_){const auto result=initialize(model,0);if(!result.accepted)throw std::invalid_argument(std::string(result.reason));}
  for(std::size_t i=0;i<spec_.controls.size();++i){
    const auto p=spec_.controls[i].parameter;
    if(!models_[0].parameterAvailable(p))throw std::invalid_argument("Object lesson control is unavailable");
    for(std::size_t j=0;j<i;++j)if(p==spec_.controls[j].parameter)throw std::invalid_argument("Duplicate object lesson control");
  }
  const auto& state=models_[0].snapshot();
  for(const auto label:spec_.metrics)if(std::none_of(state.metrics.begin(),state.metrics.begin()+state.metricCount,[&](const auto& m){return m.label==label;}))throw std::invalid_argument("Unknown object lesson metric");
  for(const auto name:spec_.matrices)if(std::none_of(state.matrices.begin(),state.matrices.begin()+state.matrixCount,[&](const auto& m){return m.name==name;}))throw std::invalid_argument("Unknown object lesson matrix");
  for(unsigned i=1;i<spec_.examples.size();++i){auto probe=models_[0];const auto result=initialize(probe,i);if(!result.accepted)throw std::invalid_argument(std::string(result.reason));}
}
MathActionResult ObjectLesson::initialize(MathObjects& model,unsigned example)const{
  if(example>=spec_.examples.size())return {false,"Unknown teaching example."};
  // Work on a copy so an invalid authored preset cannot partly change live state.
  auto next=model;
  for(const auto action:{MathAction{MathActionKind::Select,spec_.object},MathAction{MathActionKind::Reset},MathAction{MathActionKind::SetLevel,spec_.object,MathParameter::X,static_cast<double>(spec_.level)}}){
    const auto result=next.dispatch(action);if(!result.accepted)return result;
  }
  for(const auto value:spec_.examples[example].values){
    const auto result=next.dispatch({MathActionKind::SetParameter,spec_.object,value.parameter,value.value});if(!result.accepted)return result;
  }
  model=std::move(next);return {true,{}};
}
MathActionResult ObjectLesson::dispatch(ObjectLessonAction action){
  auto& model=models_[action.practice];
  switch(action.kind){
    case ObjectLessonActionKind::SelectExample:{
      if(action.practice)return {false,"Teaching presets do not change the practice example."};
      const auto result=initialize(model,action.example);
      if(result.accepted){example_=action.example;customized_=false;}return result;
    }
    case ObjectLessonActionKind::SetParameter:{
      if(std::none_of(spec_.controls.begin(),spec_.controls.end(),[&](auto c){return c.parameter==action.parameter;}))return {false,"This parameter is outside the lesson."};
      const auto result=model.dispatch({MathActionKind::SetParameter,spec_.object,action.parameter,action.value});
      if(result.accepted&&!action.practice)customized_=true;return result;
    }
    case ObjectLessonActionKind::Reset:{
      const auto result=initialize(model,0);if(result.accepted&&!action.practice){example_=0;customized_=false;}return result;
    }
    case ObjectLessonActionKind::Check:
      if(!action.practice)return {false,"Open Exercise to check your work."};
      return model.dispatch({MathActionKind::Check});
  }
  return {false,"Unknown object lesson action."};
}
} // namespace paths
