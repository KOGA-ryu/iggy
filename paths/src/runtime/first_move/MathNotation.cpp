#include "runtime/first_move/MathNotation.hpp"

#include <algorithm>

namespace iggy3d::first_move {
namespace {
bool text(std::string_view value,std::size_t limit,bool multiline=true) noexcept {
  return !value.empty() && value.size()<=limit &&
      std::none_of(value.begin(),value.end(),[&](unsigned char c){return c<32 && !(multiline && c=='\n');});
}
bool identity(std::string_view value) noexcept {
  return text(value,64,false) && std::all_of(value.begin(),value.end(),[](unsigned char c) {
    return (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_';
  });
}
}
bool validNotationDefinition(const NotationDefinition& value) noexcept {
  return identity(value.id) && value.version && text(value.title,64,false) &&
      text(value.meaning,160) && text(value.definition,480) && text(value.example,240);
}
bool validNotationLesson(const NotationLesson& lesson) noexcept {
  if(!identity(lesson.id) || !lesson.version || !text(lesson.title,64,false) ||
      !text(lesson.context,160) || !text(lesson.reading,480) ||
      lesson.tokens.empty() || lesson.tokens.size()>kNotationTokenCapacity)return false;
  for(const auto& token:lesson.tokens)
    if(!text(token.text,32,false) || token.text.find("##")!=std::string::npos ||
        !text(token.role,240) || !validNotationDefinition(token.definition))return false;
  return !lesson.check || (lesson.check->answer<lesson.tokens.size() &&
      text(lesson.check->prompt,240) && text(lesson.check->correct,240) && text(lesson.check->retry,240));
}
bool validNotationLessons(std::span<const NotationLesson> lessons) noexcept {
  if(lessons.size()>kNotationLessonCapacity)return false;
  for(std::size_t i=0;i<lessons.size();++i) {
    if(!validNotationLesson(lessons[i]))return false;
    for(std::size_t j=0;j<i;++j)if(lessons[i].id==lessons[j].id)return false;
  }
  return true;
}
NotationVerdict checkNotation(const NotationLesson& lesson,std::size_t token) noexcept {
  if(!lesson.check || token>=lesson.tokens.size() || !validNotationLesson(lesson))return NotationVerdict::Unavailable;
  return token==lesson.check->answer?NotationVerdict::Correct:NotationVerdict::Retry;
}
} // namespace iggy3d::first_move
