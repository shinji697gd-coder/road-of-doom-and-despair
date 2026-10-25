#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <cmath>
using namespace geode::prelude;
namespace training {
struct Preset { std::string name; double percent=0; };
struct Progress {
 int best=0; bool favorite=false; int status=0;
 std::string notes; std::vector<std::pair<double,double>> runs;
 std::vector<Preset> presets;
};
struct Store {
 std::map<int,Progress> levels; int goal=0;
 std::string encode() const {
  std::ostringstream s; s << "RDOD 1 " << goal << ' ' << levels.size() << '\n';
  for(auto const& [id,p]:levels) {
   s<<id<<' '<<p.best<<' '<<p.favorite<<' '<<p.status<<' '<<std::quoted(p.notes)<<' '<<p.runs.size()<<' '<<p.presets.size()<<'\n';
   for(auto [a,b]:p.runs) s<<a<<' '<<b<<'\n';
   for(auto const& q:p.presets) s<<std::quoted(q.name)<<' '<<q.percent<<'\n';
  } return s.str();
 }
 bool decode(std::string const& data) {
  if(data.size()>2000000) return false;
  std::istringstream s(data); Store next; std::string magic; int schema,n;
  if(!(s>>magic>>schema>>next.goal>>n)||magic!="RDOD"||schema!=1||n<0||n>75) return false;
  for(int i=0;i<n;i++) {
   int id,r,q; Progress p;
   if(!(s>>id>>p.best>>p.favorite>>p.status>>std::quoted(p.notes)>>r>>q)||id<=0||p.best<0||p.best>100||p.status<0||p.status>3||r<0||r>1000||q<0||q>1000||p.notes.size()>4096||next.levels.contains(id)) return false;
   for(int j=0;j<r;j++){double a,b;if(!(s>>a>>b)||!std::isfinite(a)||!std::isfinite(b)||a<0||a>=b||b>100)return false;p.runs.emplace_back(a,b);}
   for(int j=0;j<q;j++){Preset v;if(!(s>>std::quoted(v.name)>>v.percent)||v.name.size()>80||!std::isfinite(v.percent)||v.percent<0||v.percent>=100)return false;p.presets.push_back(v);}
   next.levels[id]=p;
  }
  s>>std::ws; if(!s.eof()) return false; *this=std::move(next); return true;
 }
 void save(){Mod::get()->setSavedValue("training-v1",encode());}
 static Store& get(){static Store s; static bool loaded=false;if(!loaded){loaded=true;auto raw=Mod::get()->getSavedValue<std::string>("training-v1","");if(!raw.empty()&&!s.decode(raw))log::error("[RDoD] Invalid training save; original retained");}return s;}
};
class Detail final : public Popup {
 int id; TextInput *notes,*label,*from,*to; CCLabelBMFont* summary;
 void refresh(){auto& s=Store::get();auto& p=s.levels[id];summary->setString(fmt::format("{}% | {} | {}\n{} runs | {} presets (PARTIAL)",p.best,p.favorite?"FAVORITE":"-",s.goal==id?"GOAL":"-",p.runs.size(),p.presets.size()).c_str());}
 void save(CCObject*){Store::get().levels[id].notes=notes->getString();Store::get().save();refresh();}
 void favorite(CCObject*){auto& p=Store::get().levels[id];p.favorite=!p.favorite;Store::get().save();refresh();}
 void goal(CCObject*){auto& s=Store::get();s.goal=s.goal==id?0:id;s.save();refresh();}
 bool number(TextInput* input,double& n){auto str=input->getString();std::istringstream s(str);s>>n;s>>std::ws;return !s.fail()&&s.eof()&&std::isfinite(n);}
 void run(CCObject*){double a,b;if(!number(from,a)||!number(to,b)||a<0||a>=b||b>100){FLAlertLayer::create("Run","Enter 0 <= FROM < TO <= 100.","OK")->show();return;}auto& p=Store::get().levels[id];if(p.runs.size()<1000)p.runs.emplace_back(a,b);Store::get().save();refresh();}
 void preset(CCObject*){double a;if(!number(from,a)||a<0||a>=100||label->getString().empty()){FLAlertLayer::create("Startpos","Enter a name and FROM percentage (0 to 99.99).","OK")->show();return;}auto& p=Store::get().levels[id];if(p.presets.size()<1000)p.presets.push_back({label->getString(),a});Store::get().save();refresh();}
 void list(CCObject*){auto& p=Store::get().levels[id];std::string text="Saved training records:\n";for(auto [a,b]:p.runs)text+=fmt::format("Run {}-{}%\n",a,b);for(auto const& q:p.presets)text+=fmt::format("{}: {}% (PARTIAL)\n",q.name,q.percent);FLAlertLayer::create("Records",text.c_str(),"OK")->show();}
 void remove(CCObject*){auto& p=Store::get().levels[id];if(!p.presets.empty())p.presets.pop_back();Store::get().save();refresh();}
 void apply(CCObject*){FLAlertLayer::create("Startpos TEST","APPLY is locked: these are percentage markers, not validated native start positions. In-game isolation testing is required.","OK")->show();}
 void button(const char* text,float x,float y,SEL_MenuHandler cb){auto l=CCLabelBMFont::create(text,"goldFont.fnt");l->setScale(.45f);auto b=CCMenuItemSpriteExtra::create(l,this,cb);b->setPosition({x,y});m_buttonMenu->addChild(b);}
 TextInput* input(float w,const char* hint,float x,float y){auto v=TextInput::create(w,hint);v->setPosition({x,y});m_mainLayer->addChild(v);return v;}
 bool setup(int level,std::string const& name){if(!Popup::init(400,280))return false;id=level;setTitle(name);auto& p=Store::get().levels[id];summary=CCLabelBMFont::create("","bigFont.fnt");summary->setScale(.4f);summary->setPosition({200,222});m_mainLayer->addChild(summary);
 notes=input(340,"Notes",200,180);notes->setString(p.notes);notes->setMaxCharCount(4096);
 label=input(180,"Preset name",115,137);label->setMaxCharCount(80);from=input(65,"From %",250,137);to=input(65,"To %",330,137);
 button("SAVE NOTES",75,101,menu_selector(Detail::save));button("FAVORITE",205,101,menu_selector(Detail::favorite));button("GOAL",325,101,menu_selector(Detail::goal));
 button("ADD RUN",75,72,menu_selector(Detail::run));button("ADD PRESET",210,72,menu_selector(Detail::preset));button("RECORDS",330,72,menu_selector(Detail::list));
 button("DELETE LAST PRESET",125,40,menu_selector(Detail::remove));button("APPLY (LOCKED)",300,40,menu_selector(Detail::apply));refresh();return true;}
public: static Detail* create(int id,std::string const& name){auto p=new Detail;if(p->setup(id,name)){p->autorelease();return p;}delete p;return nullptr;}
};
}
