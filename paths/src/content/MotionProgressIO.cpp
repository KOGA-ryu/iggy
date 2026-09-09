#include "content/MotionProgressIO.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>

namespace paths {
namespace {
using Json=nlohmann::json;
constexpr std::size_t maximumBytes=512*1024;
void require(bool value,const char* reason){if(!value)throw std::runtime_error(reason);}
std::string read(const std::filesystem::path& path) {
  std::ifstream file(path,std::ios::binary);require(bool(file),"Cannot read motion save");
  std::string text(maximumBytes+1,'\0');file.read(text.data(),text.size());require(!file.bad(),"Cannot read motion save");
  text.resize(file.gcount());require(text.size()<=maximumBytes,"Motion save exceeds 512 KiB");return text;
}
MotionPlan plan(const Json& j) {
  require(j.is_array() && j.size()==2,"Invalid saved motion plan");
  return {{j.at(0).get<double>(),j.at(1).get<double>()}};
}
std::size_t integer(const Json& j,std::size_t maximum) {
  require(j.is_number_unsigned() || j.is_number_integer(),"Invalid motion save index");
  const auto n=j.get<std::int64_t>();require(n>=0 && static_cast<std::uint64_t>(n)<=maximum,"Motion save index out of range");return static_cast<std::size_t>(n);
}
}
void MotionProgressFile::load(MotionLesson& lesson) {
  if(path_.empty()){message_="Motion progress is kept for this session.";return;}
  try {
    require(!std::filesystem::is_symlink(path_),"Motion save is a symbolic link");
    if(!std::filesystem::exists(path_)){message_="Motion progress saves automatically.";return;}
    disk_=read(path_);const auto root=Json::parse(*disk_);
    require(root.at("format")=="paths_motion" && root.at("version")==1,"Unknown motion save format");
    MotionProgress p=MotionLesson{}.progress();const auto specs=motionChapters();const auto& rows=root.at("chapters");
    require(rows.is_array() && rows.size()<=specs.size(),"Invalid saved motion chapters");std::set<std::string> ids;
    for(const auto& row:rows) {
      const std::string id=row.at("id");require(ids.insert(id).second,"Repeated motion chapter");
      const auto spec=std::find_if(specs.begin(),specs.end(),[&](const auto& c){return c.id==id;});
      require(spec!=specs.end(),"A saved motion chapter has changed");
      auto& r=p.runs[spec-specs.begin()];r.plan=plan(row.at("plan"));
      r.started=row.at("started").get<bool>();r.solved=row.at("solved").get<bool>();
      const auto& attempts=row.at("attempts");const auto& undo=row.at("undo");
      require(attempts.is_array() && attempts.size()<=motionAttemptCapacity && undo.is_array() && undo.size()<=motionUndoCapacity,"Motion history exceeds its limit");
      for(const auto& value:attempts)r.attempts.push_back({plan(value),false});
      for(const auto& value:undo)r.undo.push_back(plan(value));
    }
    const auto selected=std::find_if(specs.begin(),specs.end(),[&](const auto& c){return c.id==root.at("selected").get<std::string>();});
    require(selected!=specs.end(),"Saved motion selection has changed");p.selected=selected-specs.begin();
    p.time=root.at("time");p.speed=root.at("speed");p.recording=root.at("recording").get<bool>();p.inspected.reset();
    if(!root.at("inspected").is_null())p.inspected=integer(root.at("inspected"),motionAttemptCapacity-1);
    require(lesson.restore(std::move(p)),"Saved motion working or evidence is invalid");
    savedRevision_=lesson.revision();message_="Motion progress restored; playback paused.";
  } catch(const std::exception& error){blocked_=true;message_=std::string(error.what())+". Original save retained.";}
}
void MotionProgressFile::save(const MotionLesson& lesson,bool closing) {
  if(path_.empty() || blocked_ || (!closing && savedRevision_==lesson.revision()))return;
  try {
    const auto& p=lesson.progress();const auto specs=motionChapters();
    Json root{{"format","paths_motion"},{"version",1},{"selected",specs[p.selected].id},{"time",p.time},{"speed",p.speed},
      {"inspected",p.inspected?Json(*p.inspected):Json(nullptr)},{"recording",p.recording},{"chapters",Json::array()}};
    for(std::size_t i=0;i<specs.size();++i) {
      const auto& r=p.runs[i];Json attempts=Json::array(),undo=Json::array();
      for(const auto& a:r.attempts)attempts.push_back(a.plan.values);
      for(const auto& v:r.undo)undo.push_back(v.values);
      root["chapters"].push_back({{"id",specs[i].id},{"plan",r.plan.values},{"started",r.started},{"solved",r.solved},{"attempts",attempts},{"undo",undo}});
    }
    const auto text=root.dump(2)+"\n";require(text.size()<=maximumBytes,"Motion save exceeds 512 KiB");
    if(disk_ && *disk_==text){savedRevision_=lesson.revision();return;}
    require(!std::filesystem::is_symlink(path_),"Motion save became a symbolic link");
    require(std::filesystem::exists(path_)?disk_ && read(path_)==*disk_:!disk_,"Motion save changed outside this window");
    if(!path_.parent_path().empty())std::filesystem::create_directories(path_.parent_path());
    auto temporary=path_;temporary+=".tmp";
    require(!std::filesystem::exists(temporary) && !std::filesystem::is_symlink(temporary),"Motion temporary save already exists");
    std::ofstream file(temporary,std::ios::binary);file<<text;file.close();require(bool(file),"Cannot write motion save");
    std::filesystem::rename(temporary,path_);disk_=text;savedRevision_=lesson.revision();message_="Motion progress saved.";
  } catch(const std::exception& error){blocked_=true;message_=std::string(error.what())+". Original save retained.";}
}
} // namespace paths
